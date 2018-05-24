/*
 * Copyright 2017 Rafal Zajac <rzajac@gmail.com>.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */


#include <esp_det.h>
#include <esp_eb.h>
#include <esp_cmd.h>
#include <esp_json.h>
#include <mem.h>

// Trigger user callback event name.
#define ESP_DET_EV_MAIN "espDetMain"
// Received IP from access point event name.
#define ESP_DET_EV_GOT_IP "espDetGotIp"
// Access point disconnection event name.
#define ESP_DET_EV_DISC "espDetDisc"
// User callback event name.
#define ESP_DET_EV_USER "espDetUser"

// Supported commands.
#define ESP_DET_CMD_SET_AP "cfg"
#define ESP_DET_CMD_DISCOVERY "iotDiscovery"

#define ESP_DET_FAST_CALL 10
#define ESP_DET_SLOW_CALL 500

// The ESP detection stages.
typedef enum {
  ESP_DET_ST_DM = 1, // Creates AP and waits for detection and configuration.
  ESP_DET_ST_CN,     // Connecting to access point.
  ESP_DET_ST_OP      // All needed configuration is present. Relinquish control to user program.
} esp_det_st;

// The ESP detection flash stored configuration.
typedef struct STORE_ATTR {
  uint8_t magic;      // The magic number indicating the config version. Used to validate loaded data.
  uint32_t load_cnt;  // The number of times config was loaded from flash.
  uint32_t mqtt_ip;   // The MQTT broker IP.
  uint16_t mqtt_port; // The MQTT broker port.
  esp_det_st stage;   // The current detection stage.
  char mqtt_user[ESP_DET_MQTT_USER_MAX]; // The MQTT broker username.
  char mqtt_pass[ESP_DET_MQTT_PASS_MAX]; // The MQTT broker password.
  char ap_name[ESP_DET_AP_NAME_MAX];   // The access name.
  char ap_pass[ESP_DET_AP_PASS_MAX];   // The access point password.
} flash_cfg;

// The ESP detection global state.
typedef struct {
  bool connected;     // Set to true if we are connected to access point.
  bool dm_run;        // Was detect me stage running.
  char *ap_pass;      // The password for access point created in ESP_DET_ST_DM.
  uint8_t ap_cn;      // The channel to use for access point created in ESP_DET_ST_DM.
  uint8_t dm_err_cnt; // Unsuccessful switches to ESP_DET_ST_DM.
  uint8_t cn_err_cnt; // Unsuccessful switches to ESP_DET_ST_CN.
  uint8_t sr_err_cnt; // Unsuccessful switches to ESP_DET_ST_DS.
  uint32 brd_addr;    // Broadcast address. Set every time we get an IP address.
  esp_det_st stage;   // The current detection stage.
  esp_det_done_cb *done_cb;    // The user program callback.
  esp_det_disconnect *disc_cb; // The wifi disconnection callback.
  esp_det_enc_dec *encrypt_cb; // Encryption callback.
  esp_det_enc_dec *decrypt_cb; // Decryption callback.
  os_timer_t *ip_to;           // The maximum time for acquiring IP.
  struct espconn udp_conn;     // The UDP broadcast connection.
} det_state;

// The configuration loaded from flash.
static flash_cfg *g_cfg;

// The detection state.
static det_state *g_sta;

///////////////////////////////////////////////////////////////////////////////
// Declarations                                                              //
///////////////////////////////////////////////////////////////////////////////

static esp_cfg_err ICACHE_FLASH_ATTR load_config();

static esp_cfg_err ICACHE_FLASH_ATTR cfg_reset();

static esp_cfg_err ICACHE_FLASH_ATTR cfg_set_stage(esp_det_st stage);

static unsigned short ICACHE_FLASH_ATTR cmd_handle_cb(uint8_t *res,
                                                      uint16 res_len,
                                                      const uint8_t *req,
                                                      uint16_t req_len);

///////////////////////////////////////////////////////////////////////////////
// ESP detection                                                             //
///////////////////////////////////////////////////////////////////////////////

/**
 * Call user provided callback.
 *
 * @param event The event name.
 * @param arg   The data passed to the event.
 */
static void ICACHE_FLASH_ATTR
call_user_e_cb(const char *event, void *arg)
{
  g_sta->done_cb((esp_det_err) ((uint32_t) arg));
}

/**
 * Returns true if access point configuration is the same.
 *
 * @param c1 The access point config.
 * @param c2 The access point config.
 * @return True if configs are equal.
 */
static bool ICACHE_FLASH_ATTR
ap_config_equal(struct softap_config *c1, struct softap_config *c2) {
  if (os_strncmp((const char *) c1->ssid, (const char *) c2->ssid, 32) != 0) {
    return false;
  }

  if (os_strncmp((const char *) c1->password, (const char *) c2->password, 64) != 0) {
    return false;
  }

  if (c1->channel != c2->channel) {
    return false;
  }

  if (c1->max_connection != c2->max_connection) {
    return false;
  }

  return true;
}

/**
 * Create access point.
 *
 * @return The error code.
 */
static esp_det_err ICACHE_FLASH_ATTR
create_ap()
{
  struct softap_config ap_conf;
  char ap_name[ESP_DET_AP_NAME_MAX];
  uint8_t mac_address[6];

  // Build access point name.
  wifi_get_macaddr(STATION_IF, mac_address);
  os_memset(ap_name, 0, ESP_DET_AP_NAME_MAX);
  // TODO: this needs to be configurable.
  os_sprintf(ap_name, "IOT_%02X%02X%02X%02X%02X%02X", MAC2STR(mac_address));

  ESP_DET_DEBUG("Creating access point %s / %s\n", ap_name, g_sta->ap_pass);

  // Make sure we are in correct opmode.
  if (wifi_get_opmode() != STATIONAP_MODE) {
    if (!wifi_set_opmode_current(STATIONAP_MODE)) {
      return ESP_DET_ERR_OPMODE;
    }
  }

  struct softap_config ap_conf_curr;
  if (!wifi_softap_get_config(&ap_conf_curr)) {
    return ESP_DET_ERR_WIFI_CFG_GET;
  }

  // Build our Access Point configuration details.
  os_memset(ap_conf.ssid, 0, 32);
  os_memset(ap_conf.password, 0, 64);
  os_memcpy(ap_conf.ssid, ap_name, (unsigned int) os_strlen(ap_name));
  os_memcpy(ap_conf.password, g_sta->ap_pass, (unsigned int) os_strlen(g_sta->ap_pass));
  ap_conf.ssid_len = (uint8) os_strlen(ap_name);
  ap_conf.authmode = AUTH_WPA_PSK;
  ap_conf.channel = g_sta->ap_cn;
  ap_conf.max_connection = 1; // How many stations can connect to ESP8266 softAP at most.

  if (ap_config_equal(&ap_conf_curr, &ap_conf) == false) {
    ETS_UART_INTR_DISABLE();
    if (!wifi_softap_set_config(&ap_conf)) {
      ETS_UART_INTR_ENABLE();
      return ESP_DET_ERR_WIFI_CFG_SET;
    }
    ETS_UART_INTR_ENABLE();
  }

  struct ip_info ip_info;
  if (!wifi_get_ip_info(SOFTAP_IF, &ip_info)) {
    return ESP_DET_ERR_IP_INFO_GET;
  }

  IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
  IP4_ADDR(&ip_info.ip, 192, 168, 42, 1);
  IP4_ADDR(&ip_info.gw, 192, 168, 42, 1);

  if (!wifi_set_ip_info(SOFTAP_IF, &ip_info)) {
    return ESP_DET_ERR_IP_INFO_SET;
  }

  return ESP_DET_OK;
}

/**
 * Set access point connection details and write it to flash.
 *
 * @param ap_name The access point name.
 * @param ap_pass The access point password.
 *
 * @return Error code.
 */
static esp_det_err ICACHE_FLASH_ATTR
cfg_set_ap(char *ap_name, char *ap_pass)
{
  struct station_config station_config;

  strlcpy(g_cfg->ap_name, ap_name, ESP_DET_AP_NAME_MAX);
  strlcpy(g_cfg->ap_pass, ap_pass, ESP_DET_AP_NAME_MAX);

  ESP_DET_DEBUG("Setting access point config: %s/%s\n", ap_name, ap_pass);

  os_memset(&station_config, 0, sizeof(struct station_config));
  strlcpy((char *) station_config.ssid, ap_name, 32);
  strlcpy((char *) station_config.password, ap_pass, 64);

  ETS_UART_INTR_DISABLE();
  bool success = wifi_station_set_config(&station_config);
  ETS_UART_INTR_ENABLE();
  if (success == false) return ESP_DET_ERR_AP;

  esp_cfg_err err = esp_cfg_write(ESP_DET_CFG_IDX);
  if (err != ESP_CFG_OK) return ESP_DET_ERR_CFG;

  return ESP_DET_OK;
}

/**
 * Save main server connection details to flash.
 *
 * @param ip   The main server IP address.
 * @param port The main server port.
 * @param user The main server user.
 * @param pass The main server password.
 *
 * @return The status of flash operation.
 */
static esp_cfg_err ICACHE_FLASH_ATTR
cfg_set_srv(uint32_t ip, uint16_t port, char *user, char *pass)
{
  g_cfg->mqtt_ip = ip;
  g_cfg->mqtt_port = port;
  strlcpy(g_cfg->mqtt_user, user, ESP_DET_MQTT_USER_MAX);
  strlcpy(g_cfg->mqtt_pass, pass, ESP_DET_MQTT_PASS_MAX);

  return esp_cfg_write(ESP_DET_CFG_IDX);
}

/**
 * Trigger main event handler.
 *
 * @param reset_cfg Set to true to reset flash stored configuration.
 * @param delay     The delay in milliseconds.
 */
static void ICACHE_FLASH_ATTR
trigger_main(bool reset_cfg, uint32_t delay)
{
  ESP_DET_DEBUG("Triggering main with delay %d.\n", delay);

  if (reset_cfg) cfg_reset();
  esp_eb_trigger_delayed(ESP_DET_EV_MAIN, delay, NULL);
}

/** Stop get IP time out timer. */
void ICACHE_FLASH_ATTR
stop_ip_to()
{
  if (g_sta->ip_to == NULL) return;

  os_timer_disarm(g_sta->ip_to);
  os_free(g_sta->ip_to);
  g_sta->ip_to = NULL;
}

/** Getting IP address timeout callback. */
static void ICACHE_FLASH_ATTR
get_ip_to_cb()
{
  ESP_DET_DEBUG("Running get_ip_to_cb in stage %d\n", g_sta->stage);

  stop_ip_to();
  cfg_reset();
  trigger_main(true, ESP_DET_FAST_CALL);
}

/**
 * Got IP address after connection go access point callback.
 *
 * @param event The event name.
 * @param arg   The event argument.
 */
static void ICACHE_FLASH_ATTR
got_ip_e_cb(const char *event, void *arg)
{
  ESP_DET_DEBUG("Running got_ip_e_cb in stage %d.\n", g_sta->stage);

  stop_ip_to();
  g_sta->connected = true;

  if (g_sta->stage == ESP_DET_ST_CN) {
    cfg_set_stage(ESP_DET_ST_OP);
    trigger_main(false, ESP_DET_FAST_CALL);
    return;
  }

  if (g_sta->stage == ESP_DET_ST_OP) {
    trigger_main(false, ESP_DET_FAST_CALL);
    return;
  }

  ESP_DET_ERROR("Run got_ip_e_cb in unexpected stage %d.\n", g_sta->stage);
}

static void ICACHE_FLASH_ATTR
disc_e_cb(const char *event, void *arg)
{
  uint32_t reason = (uint32_t) arg;

  ESP_DET_DEBUG("Running disc_e_cb in stage %d reason %d.\n", g_sta->stage, reason);
  g_sta->connected = false;

  if (g_sta->stage == ESP_DET_ST_DM) return;

  if (g_sta->stage == ESP_DET_ST_CN) {
    trigger_main(false, ESP_DET_FAST_CALL);
    return;
  }

  if (g_sta->disc_cb && g_sta->stage == ESP_DET_ST_OP) g_sta->disc_cb();
  cfg_set_stage(ESP_DET_ST_CN);
  trigger_main(false, ESP_DET_FAST_CALL);
}

/** Go into detect me stage */
static void ICACHE_FLASH_ATTR
stage_detect_me()
{
  esp_det_err err;

  ESP_DET_DEBUG("Running stage_detect_me in stage %d.\n", g_sta->stage);

  // Check back off.
  g_sta->dm_err_cnt += 1;
  if (g_sta->dm_err_cnt >= 10) {
    cfg_reset();
    trigger_main(true, ESP_DET_SLOW_CALL);
    return;
  }

  g_sta->dm_run = true;

  // Setup
  err = create_ap();
  if (err != ESP_DET_OK) {
    trigger_main(false, ESP_DET_SLOW_CALL);
    return;
  }

  sint8 cmd_err = esp_cmd_start(ESP_DET_CMD_PORT, ESP_DET_CMD_MAX, &cmd_handle_cb);
  if (cmd_err != ESPCONN_OK && cmd_err != ESP_CMD_ERR_ALREADY_STARTED) {
    ESP_DET_ERROR("Starting command server failed with error code %d.\n", cmd_err);
    trigger_main(false, ESP_DET_SLOW_CALL);
    return;
  }

  // End
}

/** Go into wifi connect stage. */
static void ICACHE_FLASH_ATTR
stage_connect()
{
  ESP_DET_DEBUG("Running stage_connect in stage %d.\n", g_sta->stage);

  g_sta->cn_err_cnt += 1;
  if (g_sta->cn_err_cnt >= 10) {
    cfg_reset();
    trigger_main(true, ESP_DET_SLOW_CALL);
    return;
  }

  ETS_UART_INTR_DISABLE();
  if (wifi_station_connect() == false) {
    ESP_DET_ERROR("Calling wifi_station_connect failed.\n");
    trigger_main(false, ESP_DET_SLOW_CALL);
    return;
  }
  ETS_UART_INTR_ENABLE();

  if (g_sta->ip_to == NULL) {
    g_sta->ip_to = os_zalloc(sizeof(os_timer_t));
    if (g_sta->ip_to == NULL) {
      ESP_DET_ERROR("Out of memory allocating os_timer_t\n");
      return;
    }
    os_timer_setfn(g_sta->ip_to, (os_timer_func_t *) get_ip_to_cb, NULL);
    // Wait 15 seconds for IP.
    os_timer_arm(g_sta->ip_to, 15000, false);
  }
}

/** Go into operational stage. */
static void ICACHE_FLASH_ATTR
stage_operational()
{
  ESP_DET_DEBUG("Running stage_operational in stage %d.\n", g_sta->stage);

  if (g_sta->dm_run) {
    ESP_DET_DEBUG("Will restart...\n");
    system_restart();
    return;
  }

  wifi_set_opmode(STATION_MODE);
  esp_eb_trigger(ESP_DET_EV_USER, NULL);
}

/**
 * Main ESP detect event handler.
 *
 * If the current detection stage is unknown it will reset the config and
 * start the detection process again.
 *
 * @param event The event name.
 * @param arg   The event argument.
 */
static void ICACHE_FLASH_ATTR
main_e_cb(const char *event, void *arg)
{
  ESP_DET_DEBUG("Running main_e_cb in stage %d.\n", g_sta->stage);

  if (g_sta->stage == ESP_DET_ST_DM) {
    stage_detect_me();
    return;
  } else if (g_sta->stage == ESP_DET_ST_CN) {
    stage_connect();
    return;
  } else if (g_sta->stage == ESP_DET_ST_OP) {
    if (g_sta->connected) {
      stage_operational();
    } else {
      stage_connect();
    }
    return;
  }

  ESP_DET_ERROR("Unexpected stage %d. Resetting config.\n", g_sta->stage);
  cfg_reset();
  esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_SLOW_CALL, NULL);
}

/**
 * The WiFi events handler.
 *
 * @param event The WiFi event.
 */
static void ICACHE_FLASH_ATTR
wifi_event_cb(System_Event_t *event)
{
  switch (event->event) {
    case EVENT_STAMODE_CONNECTED:
      ESP_DET_DEBUG("Wifi event: EVENT_STAMODE_CONNECTED\n");
      break;

    case EVENT_STAMODE_DISCONNECTED:
      ESP_DET_DEBUG("Wifi event: EVENT_STAMODE_DISCONNECTED reason %d\n",
                    event->event_info.disconnected.reason);

      esp_eb_trigger(ESP_DET_EV_DISC, (void *) ((uint32_t) event->event_info.disconnected.reason));
      break;

    case EVENT_STAMODE_AUTHMODE_CHANGE:
      ESP_DET_DEBUG("Wifi event: EVENT_STAMODE_AUTHMODE_CHANGE %d -> %d\n",
                    event->event_info.auth_change.old_mode,
                    event->event_info.auth_change.new_mode);
      break;

    case EVENT_STAMODE_GOT_IP:
      ESP_DET_DEBUG("Got IP: %d.%d.%d.%d / %d.%d.%d.%d\n",
                    IP2STR(&(event->event_info.got_ip.ip)),
                    IP2STR(&(event->event_info.got_ip.mask)));

      g_sta->brd_addr = event->event_info.got_ip.ip.addr | (~event->event_info.got_ip.mask.addr);
      esp_eb_trigger(ESP_DET_EV_GOT_IP, NULL);
      break;

    case EVENT_STAMODE_DHCP_TIMEOUT:
      ESP_DET_DEBUG("Wifi event: EVENT_STAMODE_DHCP_TIMEOUT\n");
      break;

    case EVENT_SOFTAPMODE_STACONNECTED:
      ESP_DET_DEBUG("Wifi event: EVENT_SOFTAPMODE_STACONNECTED\n");
      break;

    case EVENT_SOFTAPMODE_STADISCONNECTED:
      ESP_DET_DEBUG("Wifi event: EVENT_SOFTAPMODE_STADISCONNECTED\n");
      break;

    case EVENT_OPMODE_CHANGED:
      ESP_DET_DEBUG("Wifi event: EVENT_OPMODE_CHANGED %d\n", wifi_get_opmode());
      break;

    case EVENT_SOFTAPMODE_PROBEREQRECVED:
      // Ignoring this one.
      break;

    default:
      ESP_DET_DEBUG("Unexpected wifi event: %d\n", event->event);
      break;
  }
}

/**
 * Initialize ESP8266 to the known state.
 *
 * - Set current mode to station + AP.
 * - Turn off reconnect policy.
 * - Turn off auto connect policy.
 * - Turn off station DHCP server.
 * - Turn off AP DHCP policy.
 */
static void ICACHE_FLASH_ATTR
init()
{
  bool success;

  // We must always start with known state.
  success = wifi_set_opmode_current(STATIONAP_MODE);
  ESP_DET_DEBUG("wifi_set_opmode_current: %d\n", success);
  success = wifi_station_set_reconnect_policy(false);
  ESP_DET_DEBUG("wifi_station_set_reconnect_policy: %d\n", success);
  success = wifi_station_set_auto_connect(false);
  ESP_DET_DEBUG("wifi_station_set_auto_connect: %d\n", success);
  success = wifi_station_dhcpc_start();
  ESP_DET_DEBUG("wifi_station_dhcpc_start: %d\n", success);
  success = wifi_softap_dhcps_stop();
  ESP_DET_DEBUG("wifi_softap_dhcps_stop: %d\n", success);
}

esp_det_err ICACHE_FLASH_ATTR
esp_det_start(char *ap_pass,
              uint8_t ap_cn,
              esp_det_done_cb *done_cb,
              esp_det_disconnect *disc_cb,
              esp_det_enc_dec *encrypt,
              esp_det_enc_dec *decrypt)
{
  esp_cfg_err err;

  // Guards against multiple calls to esp_det_start.
  if (g_cfg != NULL) return ESP_DET_ERR_INITIALIZED;

  g_cfg = os_zalloc(sizeof(flash_cfg));
  if (g_cfg == NULL) return ESP_DET_ERR_MEM;

  g_sta = os_zalloc(sizeof(det_state));
  if (g_sta == NULL) {
    os_free(g_cfg);
    return ESP_DET_ERR_MEM;
  }

  g_sta->ap_pass = os_zalloc(ESP_DET_AP_PASS_MAX);
  if (g_sta->ap_pass == NULL) {
    os_free(g_cfg);
    os_free(g_sta);
    return ESP_DET_ERR_MEM;
  }

  err = load_config();
  if (err != ESP_CFG_OK) {
    ESP_DET_ERROR("Error %d loading configuration. Resetting config.\n", err);
    err = cfg_reset();
    if (err != ESP_CFG_OK) {
      ESP_DET_ERROR("Error %d resetting config.\n", err);
      return ESP_DET_ERR_CFG;
    }
  }

  g_sta->done_cb = done_cb;
  g_sta->disc_cb = disc_cb;
  g_sta->encrypt_cb = encrypt;
  g_sta->decrypt_cb = decrypt;
  g_sta->ap_cn = ap_cn;
  g_sta->stage = g_cfg->stage;
  g_sta->connected = false;
  strlcpy(g_sta->ap_pass, ap_pass, ESP_DET_AP_PASS_MAX);

  init();
  wifi_set_event_handler_cb(wifi_event_cb);

  // Attach event handlers.
  esp_eb_attach(ESP_DET_EV_MAIN, main_e_cb);
  esp_eb_attach(ESP_DET_EV_GOT_IP, got_ip_e_cb);
  esp_eb_attach(ESP_DET_EV_DISC, disc_e_cb);
  esp_eb_attach(ESP_DET_EV_USER, call_user_e_cb);

  // Kick off the detection process.
  esp_eb_trigger(ESP_DET_EV_MAIN, NULL);

  return ESP_DET_OK;
}

void ICACHE_FLASH_ATTR
esp_det_reset()
{
  init();
  trigger_main(true, ESP_DET_FAST_CALL);
}

void ICACHE_FLASH_ATTR
esp_det_get_mqtt(esp_det_mqtt *srv)
{
  srv->ip = g_cfg->mqtt_ip;
  srv->port = g_cfg->mqtt_port;
  strlcpy(srv->user, g_cfg->mqtt_user, ESP_DET_MQTT_USER_MAX);
  strlcpy(srv->pass, g_cfg->mqtt_pass, ESP_DET_MQTT_PASS_MAX);
}

uint32_t ICACHE_FLASH_ATTR
get_start_cnt()
{
  return g_cfg->load_cnt;
}

/**
 * Load ESP detect configuration from flash.
 *
 * @return The error code.
 */
static esp_cfg_err ICACHE_FLASH_ATTR
load_config()
{
  esp_cfg_err err;

  err = esp_cfg_init(ESP_DET_CFG_IDX, g_cfg, sizeof(flash_cfg));
  if (err != ESP_CFG_OK) return err;

  err = esp_cfg_read(ESP_DET_CFG_IDX);
  if (err != ESP_CFG_OK) return err;

  // Check if we get what we expected.
  if (g_cfg->magic != ESP_DET_CFG_MAGIC) {
    ESP_DET_ERROR("Error validating flash loaded config. Resetting config.\n");
    return cfg_reset();
  }

  // Bump load counter and save.
  g_cfg->load_cnt += 1;

  return esp_cfg_write(ESP_DET_CFG_IDX);
}

/**
 * Set current detection stage and write it to flash.
 *
 * @param stage The one of ESP_DET_ST_*.
 *
 * @return Error code.
 */
static esp_cfg_err ICACHE_FLASH_ATTR
cfg_set_stage(esp_det_st stage)
{
  ESP_DET_DEBUG("Setting stage to %d.\n", stage);

  g_cfg->stage = stage;
  g_sta->stage = stage;

  // When changing detection stage we reset the error counters.
  g_sta->dm_err_cnt = 0;
  g_sta->cn_err_cnt = 0;
  g_sta->sr_err_cnt = 0;
  stop_ip_to();

  return esp_cfg_write(ESP_DET_CFG_IDX);
}

/**
 * Reset ESP configuration and write it to flash.
 *
 * @return The flash operation error code.
 */
static esp_cfg_err ICACHE_FLASH_ATTR
cfg_reset()
{
  g_cfg->magic = ESP_DET_CFG_MAGIC;
  g_cfg->load_cnt = 0;
  g_cfg->mqtt_ip = 0;
  g_cfg->mqtt_port = 0;
  g_cfg->stage = ESP_DET_ST_DM;
  os_memset(g_cfg->mqtt_user, 0, ESP_DET_MQTT_USER_MAX);
  os_memset(g_cfg->mqtt_pass, 0, ESP_DET_MQTT_PASS_MAX);
  os_memset(g_cfg->ap_name, 0, ESP_DET_AP_NAME_MAX);
  os_memset(g_cfg->ap_pass, 0, ESP_DET_AP_PASS_MAX);

  // Reset detection state.
  g_sta->dm_err_cnt = 0;
  g_sta->cn_err_cnt = 0;
  g_sta->sr_err_cnt = 0;
  g_sta->brd_addr = 0;
  g_sta->stage = g_cfg->stage;
  g_sta->connected = false;
  stop_ip_to();

  return esp_cfg_write(ESP_DET_CFG_IDX);
}

static uint32_t ICACHE_FLASH_ATTR
flash_real_size(void)
{
  return (uint32_t) (1 << ((spi_flash_get_id() >> 16) & 0xFF));
}

static uint16 ICACHE_FLASH_ATTR
encrypt(uint8_t *dst, const uint8_t *src, uint16 src_len)
{
  if (g_sta->encrypt_cb == NULL) {
    os_memmove(dst, src, src_len);
    return src_len;
  } else {
    return g_sta->encrypt_cb(dst, src, src_len);
  }
}

static uint16 ICACHE_FLASH_ATTR
decrypt(uint8_t *dst, const uint8_t *src, uint16 src_len)
{
  if (g_sta->decrypt_cb == NULL) {
    os_memmove(dst, src, src_len);
    return src_len;
  } else {
    return g_sta->decrypt_cb(dst, src, src_len);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Command handling                                                          //
///////////////////////////////////////////////////////////////////////////////

/**
 * Build JSON response object template.
 *
 * @param success Is response a success or failure.
 * @param msg     The response message.
 * @param code    The error code if success is false, 0 otherwise.
 *
 * @return  Returns cJSON pointer or NULL on error.
 */
static cJSON *ICACHE_FLASH_ATTR
cmd_resp_tpl(bool success, char *msg, uint16 code)
{
  cJSON *json = cJSON_CreateObject();
  if (json == NULL) return NULL;

  cJSON_AddItemToObject(json, "success", cJSON_CreateBool(success));
  cJSON_AddItemToObject(json, "code", cJSON_CreateNumber((double) code));
  cJSON_AddItemToObject(json, "msg", cJSON_CreateString(msg));

  return json;
}

/**
 * Send response.
 *
 * @param dst     The pointer to response string.
 * @param dst_len The maximum response length in bytes.
 * @param resp    The command to send as a response.
 *
 * @return Returns true if response set, false otherwise.
 */
static uint16 ICACHE_FLASH_ATTR
cmd_resp(uint8_t *dst, uint16 dst_len, cJSON *resp)
{
  uint16 resp_len = 0;

  char *resp_str = cJSON_PrintUnformatted(resp);
  if (resp_str != NULL) {
    os_printf("sending: %s -> %d\n", resp_str, strlen(resp_str));
    resp_len = encrypt(dst, (const uint8_t *) resp_str, (uint16) strlen(resp_str));
    os_free(resp_str);
  }

  return resp_len;
}

static cJSON *ICACHE_FLASH_ATTR
cmd_set_ap(cJSON *cmd)
{
  // Validate JSON.

  cJSON *ap_name = cJSON_GetObjectItem(cmd, "ap_name");
  if (ap_name == NULL) {
    return cmd_resp_tpl(false, "missing ap_name key", ESP_DET_ERR_CMD);
  }

  cJSON *ap_pass = cJSON_GetObjectItem(cmd, "ap_pass");
  if (ap_pass == NULL) {
    return cmd_resp_tpl(false, "missing ap_pass key", ESP_DET_ERR_CMD);
  }

  cJSON *srvIp = cJSON_GetObjectItem(cmd, "mqtt_ip");
  if (srvIp == NULL) {
    return cmd_resp_tpl(false, "missing mqtt_ key", ESP_DET_ERR_AP);
  }

  cJSON *srvPort = cJSON_GetObjectItem(cmd, "mqtt_port");
  if (srvPort == NULL) {
    return cmd_resp_tpl(false, "missing mqtt_port key", ESP_DET_ERR_AP);
  }

  cJSON *srvUser = cJSON_GetObjectItem(cmd, "mqtt_user");
  if (srvUser == NULL) {
    return cmd_resp_tpl(false, "missing mqtt_user key", ESP_DET_ERR_AP);
  }

  cJSON *srvPass = cJSON_GetObjectItem(cmd, "mqtt_pass");
  if (srvPass == NULL) {
    return cmd_resp_tpl(false, "missing mqtt_pass key", ESP_DET_ERR_AP);
  }

  // Check valid stages this command can be run.

  if (g_sta->stage != ESP_DET_ST_DM) {
    return cmd_resp_tpl(false, "unexpected stage", ESP_DET_ERR_CMD);
  }

  // Make changes.

  esp_det_err err = cfg_set_ap(ap_name->valuestring, ap_pass->valuestring);
  if (err != ESP_DET_OK) {
    return cmd_resp_tpl(false, "failed setting access point", err);
  }

  uint32_t ip = ipaddr_addr(srvIp->valuestring);
  esp_cfg_err err2 = cfg_set_srv(ip, (uint16_t) srvPort->valueint, srvUser->valuestring, srvPass->valuestring);
  if (err2 != ESP_CFG_OK) {
    return cmd_resp_tpl(false, "failed setting main server", err2);
  }

  // Update detection stage.

  if (cfg_set_stage(ESP_DET_ST_CN) != ESP_CFG_OK) {
    return cmd_resp_tpl(false, "failed setting config stage", ESP_DET_ERR_CFG);
  }

  // Success.

  return cmd_resp_tpl(true, "access point set", 0);
}

// TODO: post it to MQTT
/** Build UDP discovery broadcast payload. */
static char *ICACHE_FLASH_ATTR
cmd_discovery()
{
  uint8 mac[6];
  char mac_str[ESP_DET_AP_NAME_MAX];

  os_memset(mac, 0, 6);
  os_memset(mac_str, 0, ESP_DET_AP_NAME_MAX);
  wifi_get_macaddr(STATION_IF, mac);
  os_sprintf(mac_str, "%02X%02X%02X%02X%02X%02X", MAC2STR(mac));

  cJSON *resp = cJSON_CreateObject();
  cJSON_AddItemToObject(resp, "cmd", cJSON_CreateString(ESP_DET_CMD_DISCOVERY));
  cJSON_AddItemToObject(resp, "mac", cJSON_CreateString(mac_str));
  cJSON_AddItemToObject(resp, "memory", cJSON_CreateNumber(flash_real_size()));

  char *json = cJSON_PrintUnformatted(resp);
  cJSON_Delete(resp);

  return json;
}

/**
 * Handle command callback.
 *
 * @param res      Pointer to response buffer.
 * @param res_len  The response buffer length.
 * @param req      The client command.
 * @param req_len  The client command length.
 */
static uint16 ICACHE_FLASH_ATTR
cmd_handle_cb(uint8_t *res, uint16 res_len, const uint8_t *req, uint16_t req_len)
{
  uint16 resp_len = 0;
  cJSON *cmd_json = NULL;
  cJSON *json_resp = NULL;

  uint8_t *buff = os_zalloc(req_len + 1);
  if (buff == NULL) return 0; // No more memory.

  // We cast because AES can decode in place.
  decrypt(buff, req, req_len);
  os_printf("Handling cmd: %s %d -> %d\n", buff, req_len, strlen((const char *) buff));

  cmd_json = cJSON_Parse((const char *) buff);
  if (cmd_json == NULL) {
    json_resp = cmd_resp_tpl(false, "could not decode json", ESP_DET_ERR_CMD_BAD_JSON);
    os_free(buff);
    return cmd_resp(res, res_len, json_resp);
  }

  cJSON *det_cmd = cJSON_GetObjectItem(cmd_json, "cmd");
  if (det_cmd == NULL) {
    json_resp = cmd_resp_tpl(false, "bad command format", ESP_DET_ERR_CMD_BAD_FORMAT);
    cJSON_Delete(cmd_json);
    os_free(buff);
    return cmd_resp(res, res_len, json_resp);
  }

  if (strcmp(det_cmd->valuestring, ESP_DET_CMD_SET_AP) == 0) {
    json_resp = cmd_set_ap(cmd_json);
  } else {
    json_resp = cmd_resp_tpl(false, "unknown command", ESP_DET_ERR_CMD);
  }

  resp_len = cmd_resp(res, res_len, json_resp);
  if (cJSON_GetObjectItem(json_resp, "success")->type == cJSON_True) trigger_main(false, 250);

  cJSON_Delete(cmd_json);
  cJSON_Delete(json_resp);
  os_free(buff);

  return resp_len;
}
