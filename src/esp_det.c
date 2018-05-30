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
#define ESP_DET_EV_GOT_IP "espDetIp"
// Access point disconnection event name.
#define ESP_DET_EV_DISC "espDetDisc"
// User callback event name.
#define ESP_DET_EV_USER "espDetUser"

// Supported commands.
#define ESP_DET_CMD_SET_AP "cfg"

// Delay in ms for calling mine function.
#define ESP_DET_FAST_CALL 10
#define ESP_DET_SLOW_CALL 500

// The ESP detection stages.
typedef enum {
  ESP_DET_ST_DM = 1, // Detect Me stage.
  ESP_DET_ST_CN,     // Connecting to access point.
  ESP_DET_ST_OP      // Operational stage.
} esp_det_st;

// The ESP detection flash stored configuration.
typedef struct STORE_ATTR {
  uint8_t magic;      // The magic number indicating the config version.
  uint32_t mqtt_ip;   // The MQTT broker IP.
  uint16_t mqtt_port; // The MQTT broker port.
  esp_det_st stage;   // The current detection stage.
  char mqtt_user[ESP_DET_MQTT_USER_MAX]; // The MQTT broker username.
  char mqtt_pass[ESP_DET_MQTT_PASS_MAX]; // The MQTT broker password.
  char ap_name[ESP_DET_AP_NAME_MAX];     // The access name.
  char ap_pass[ESP_DET_AP_PASS_MAX];     // The access point password.
} flash_cfg;

// The ESP detection global state.
typedef struct {
  bool connected;     // Set to true if we are connected to access point.
  bool dm_run;        // Was detect me stage running.
  bool restarting;    // Restarting.
  char ap_prefix[ESP_DET_AP_NAME_PREFIX_MAX]; // The access point name prefix.
  char ap_pass[ESP_DET_AP_PASS_MAX];          // The password for access point.
  uint8_t ap_cn;      // The channel to use for access point.
  uint8_t dm_err_cnt; // Unsuccessful switches to ESP_DET_ST_DM.
  uint8_t cn_err_cnt; // Unsuccessful switches to ESP_DET_ST_CN.
  esp_det_st stage;   // The current detection stage.
  esp_det_done_cb *done_cb;    // The user program callback.
  esp_det_disconnect *disc_cb; // The wifi disconnection callback.
  esp_det_enc_dec *encrypt_cb; // Encryption callback.
  esp_det_enc_dec *decrypt_cb; // Decryption callback.
  os_timer_t *ip_to;           // The maximum time for acquiring IP.
} det_state;

// The configuration loaded from flash.
static flash_cfg *g_cfg;

// The detection state.
static det_state *g_sta;

///////////////////////////////////////////////////////////////////////////////
// Declarations                                                              //
///////////////////////////////////////////////////////////////////////////////

static esp_det_err ICACHE_FLASH_ATTR ip_to_start(uint32 ms);

static void ICACHE_FLASH_ATTR ip_to_stop();

static void ICACHE_FLASH_ATTR ip_to_cb();

static void ICACHE_FLASH_ATTR ip_got_cb(const char *event, void *arg);

static void ICACHE_FLASH_ATTR main_e_cb(const char *event, void *arg);

static void ICACHE_FLASH_ATTR wifi_event_cb(System_Event_t *event);

static esp_det_err ICACHE_FLASH_ATTR cfg_load();

static esp_det_err ICACHE_FLASH_ATTR cfg_reset();

static esp_det_err ICACHE_FLASH_ATTR cfg_set_stage(esp_det_st stage);

static uint32_t ICACHE_FLASH_ATTR flash_real_size(void);

static bool ICACHE_FLASH_ATTR ap_config_equal(struct softap_config *c1,
                                              struct softap_config *c2);

static void ICACHE_FLASH_ATTR disc_e_cb(const char *event, void *arg);

static void ICACHE_FLASH_ATTR call_user_cb(const char *event, void *arg);

static uint16 ICACHE_FLASH_ATTR cmd_handle_cb(uint8_t *res,
                                              uint16 res_len,
                                              const uint8_t *req,
                                              uint16_t req_len);

static uint16 ICACHE_FLASH_ATTR encrypt_noop(uint8_t *dst,
                                             const uint8_t *src,
                                             uint16 src_len);

static uint16 ICACHE_FLASH_ATTR decrypt_noop(uint8_t *dst,
                                             const uint8_t *src,
                                             uint16 src_len);

///////////////////////////////////////////////////////////////////////////////
// External interface.                                                       //
///////////////////////////////////////////////////////////////////////////////

esp_det_err ICACHE_FLASH_ATTR
esp_det_start(char *ap_prefix,
              char *ap_pass,
              uint8_t ap_cn,
              esp_det_done_cb *done_cb,
              esp_det_disconnect *disc_cb,
              esp_det_enc_dec *encrypt,
              esp_det_enc_dec *decrypt)
{
  // Guards against multiple calls to esp_det_start.
  if (g_sta != NULL) return ESP_DET_ERR_INITIALIZED;

  // Allocate memory for global structures.
  g_sta = os_zalloc(sizeof(det_state));
  if (g_sta == NULL) {
    return ESP_DET_ERR_MEM;
  }

  g_cfg = os_zalloc(sizeof(flash_cfg));
  if (g_cfg == NULL) {
    os_free(g_sta);
    return ESP_DET_ERR_MEM;
  }

  // Load configuration from flash or reset config to default values on error.
  if ((cfg_load()) != ESP_DET_OK) {
    if (cfg_reset() != ESP_DET_OK) {
      return ESP_DET_ERR_CFG_LOAD;
    }
  }

  // Set detection state struct.
  g_sta->done_cb = done_cb;
  g_sta->disc_cb = disc_cb;

  g_sta->encrypt_cb = encrypt == NULL ? encrypt_noop : encrypt;
  g_sta->decrypt_cb = decrypt == NULL ? decrypt_noop : decrypt;
  g_sta->ap_cn = ap_cn;
  g_sta->stage = g_cfg->stage;
  g_sta->connected = false;
  strlcpy(g_sta->ap_prefix, ap_prefix, ESP_DET_AP_NAME_PREFIX_MAX);
  strlcpy(g_sta->ap_pass, ap_pass, ESP_DET_AP_PASS_MAX);

  // Attach event handlers.
  wifi_set_event_handler_cb(wifi_event_cb);
  esp_eb_attach(ESP_DET_EV_MAIN, main_e_cb);
  esp_eb_attach(ESP_DET_EV_GOT_IP, ip_got_cb);
  esp_eb_attach(ESP_DET_EV_DISC, disc_e_cb);
  esp_eb_attach(ESP_DET_EV_USER, call_user_cb);

  // Kick off the detection process.
  esp_eb_trigger(ESP_DET_EV_MAIN, NULL);

  return ESP_DET_OK;
}

void ICACHE_FLASH_ATTR
esp_det_stop()
{
  ip_to_stop();
  wifi_set_event_handler_cb(NULL);

  esp_eb_detach(ESP_DET_EV_MAIN, main_e_cb);
  esp_eb_detach(ESP_DET_EV_GOT_IP, ip_got_cb);
  esp_eb_detach(ESP_DET_EV_DISC, disc_e_cb);
  esp_eb_detach(ESP_DET_EV_USER, call_user_cb);

  os_free(g_sta);
}

void ICACHE_FLASH_ATTR
esp_det_get_mqtt(esp_det_mqtt *srv)
{
  srv->ip = g_cfg->mqtt_ip;
  srv->port = g_cfg->mqtt_port;
  strlcpy(srv->user, g_cfg->mqtt_user, ESP_DET_MQTT_USER_MAX);
  strlcpy(srv->pass, g_cfg->mqtt_pass, ESP_DET_MQTT_PASS_MAX);
}

///////////////////////////////////////////////////////////////////////////////
// IP related functions                                                      //
///////////////////////////////////////////////////////////////////////////////

/**
 * Start timeout countdown for getting the IP address.
 *
 * Starts timer (if not already started) which calls callback after
 * given number of milliseconds.
 *
 * @param ms The timeout for getting IP address.
 *
 * @returns The error code.
 */
static esp_det_err ICACHE_FLASH_ATTR
ip_to_start(uint32 ms)
{
  // Already started.
  if (g_sta->ip_to != NULL) return ESP_DET_OK;

  g_sta->ip_to = os_zalloc(sizeof(os_timer_t));
  if (g_sta->ip_to == NULL) return ESP_DET_ERR_MEM;

  os_timer_setfn(g_sta->ip_to, (os_timer_func_t *) ip_to_cb, NULL);
  os_timer_arm(g_sta->ip_to, ms, false);

  return ESP_DET_OK;
}

/**
 * Stop timeout counter for getting the IP address.
 *
 * It releases memory pointed by g_sta->ip_to.
 */
static void ICACHE_FLASH_ATTR
ip_to_stop()
{
  if (g_sta->ip_to == NULL) return;

  os_timer_disarm(g_sta->ip_to);
  os_free(g_sta->ip_to);
  g_sta->ip_to = NULL;
}

/**
 * Trigger IP not received callback.
 *
 * Called when no IP is assigned by the access point within
 * timeout passed to ip_to_start.
 */
static void ICACHE_FLASH_ATTR
ip_to_cb()
{
  ESP_DET_DEBUG("did not receive IP\n");

  cfg_reset();
  esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_FAST_CALL, NULL);
}

/**
 * Callback function when IP address was assigned by access point.
 *
 * @param event The event name.
 * @param arg   The event argument.
 */
static void ICACHE_FLASH_ATTR
ip_got_cb(const char *event, void *arg)
{
  UNUSED(event);
  UNUSED(arg);

  ip_to_stop();
  g_sta->connected = true;

  if (g_sta->stage == ESP_DET_ST_CN) {
    cfg_set_stage(ESP_DET_ST_OP);
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_FAST_CALL, NULL);

  } else if (g_sta->stage == ESP_DET_ST_OP) {
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_FAST_CALL, NULL);

  } else {
    ESP_DET_ERROR("run ip_got_cb in unexpected stage %d\n", g_sta->stage);
    cfg_reset();
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_SLOW_CALL, NULL);
  }
}

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
call_user_cb(const char *event, void *arg)
{
  UNUSED(event);
  esp_det_done_cb *cb = g_sta->done_cb;
  esp_det_stop();
  cb((esp_det_err) ((uint32_t) arg));
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
  os_sprintf(ap_name, "%s_%02X%02X%02X%02X%02X%02X", g_sta->ap_prefix, MAC2STR(mac_address));

  ESP_DET_DEBUG("creating access point '%s/%s'\n", ap_name, g_sta->ap_pass);

  // Make sure we are in correct opmode.
  if (wifi_get_opmode() != STATIONAP_MODE) {
    if (wifi_set_opmode_current(STATIONAP_MODE) == false) {
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
  ap_conf.max_connection = 1; // How many stations can connect to access point.

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
 * Load ESP detect configuration from flash.
 *
 * @return The error code.
 */
static esp_det_err ICACHE_FLASH_ATTR
cfg_load()
{
  if (esp_cfg_init(ESP_DET_CFG_IDX, g_cfg, sizeof(flash_cfg)) != ESP_CFG_OK) {
    return ESP_DET_ERR_CFG_INIT;
  }

  // Check if we get what we expected.
  if (esp_cfg_read(ESP_DET_CFG_IDX) != ESP_CFG_OK || g_cfg->magic != ESP_DET_CFG_MAGIC) {
    return ESP_DET_ERR_CFG_LOAD;
  }

  return ESP_DET_OK;
}

/**
 * Set MQTT broker and access point configuration and write it to flash.
 *
 * @param ap_name The access point name.
 * @param ap_pass The access point password.
 * @param mqtt_ip   The MQTT broker IP address.
 * @param mqtt_port The MQTT broker port.
 * @param mqtt_user The MQTT broker user.
 * @param mqtt_pass The MQTT broker password.
 * @param stage The one of ESP_DET_ST_*.
 *
 * @return The error code.
 */
static esp_det_err ICACHE_FLASH_ATTR
cfg_set(
    char *ap_name,
    char *ap_pass,
    uint32_t mqtt_ip,
    uint16_t mqtt_port,
    char *mqtt_user,
    char *mqtt_pass,
    esp_det_st stage)
{
  struct station_config station_config;

  strlcpy(g_cfg->ap_name, ap_name, ESP_DET_AP_NAME_MAX);
  strlcpy(g_cfg->ap_pass, ap_pass, ESP_DET_AP_NAME_MAX);
  g_cfg->mqtt_ip = mqtt_ip;
  g_cfg->mqtt_port = mqtt_port;
  strlcpy(g_cfg->mqtt_user, mqtt_user, ESP_DET_MQTT_USER_MAX);
  strlcpy(g_cfg->mqtt_pass, mqtt_pass, ESP_DET_MQTT_PASS_MAX);
  g_cfg->stage = stage;

  os_memset(&station_config, 0, sizeof(struct station_config));
  strlcpy((char *) station_config.ssid, ap_name, 32);
  strlcpy((char *) station_config.password, ap_pass, 64);

  ESP_DET_DEBUG("setting access point config: '%s/%s'\n", station_config.ssid, station_config.password);

  ETS_UART_INTR_DISABLE();
  bool success = wifi_station_set_config(&station_config);
  ETS_UART_INTR_ENABLE();
  if (success == false) return ESP_DET_ERR_AP_CFG;

  if (esp_cfg_write(ESP_DET_CFG_IDX) != ESP_CFG_OK) return ESP_DET_ERR_CFG_WRITE;

  return ESP_DET_OK;
}

/**
 * Set current detection stage and write it to flash.
 *
 * @param stage The one of ESP_DET_ST_*.
 *
 * @return The error code.
 */
static esp_det_err ICACHE_FLASH_ATTR
cfg_set_stage(esp_det_st stage)
{
  ESP_DET_DEBUG("Setting stage to %d.\n", stage);

  ip_to_stop();

  g_cfg->stage = stage;
  g_sta->stage = stage;

  // When changing detection stage we reset the error counters.
  g_sta->dm_err_cnt = 0;
  g_sta->cn_err_cnt = 0;

  if (esp_cfg_write(ESP_DET_CFG_IDX) != ESP_CFG_OK) {
    return ESP_DET_ERR_CFG_WRITE;
  }

  return ESP_DET_OK;
}

/**
 * Reset ESP configuration and write it to flash.
 *
 * @return The error code.
 */
static esp_det_err ICACHE_FLASH_ATTR
cfg_reset()
{
  ip_to_stop();

  // Reset flash config.
  os_memset(g_cfg, 0, sizeof(flash_cfg));
  g_cfg->magic = ESP_DET_CFG_MAGIC;
  g_cfg->stage = ESP_DET_ST_DM;

  // Reset detection state.
  g_sta->connected = false;
  g_sta->restarting = false;
  g_sta->dm_err_cnt = 0;
  g_sta->cn_err_cnt = 0;
  g_sta->stage = ESP_DET_ST_DM;

  if (esp_cfg_write(ESP_DET_CFG_IDX) != ESP_CFG_OK) {
    return ESP_DET_ERR_CFG_WRITE;
  }

  return ESP_DET_OK;
}

/** Disconnect event callback. */
static void ICACHE_FLASH_ATTR
disc_e_cb(const char *event, void *arg)
{
  UNUSED(event);
  uint32_t reason = (uint32_t) arg;

  g_sta->connected = false;
  ESP_DET_DEBUG("running disc_e_cb in stage %d reason %d\n", g_sta->stage, reason);

  if (g_sta->stage == ESP_DET_ST_DM) return;

  if (g_sta->stage == ESP_DET_ST_CN) {
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_FAST_CALL, NULL);
    return;
  }

  if (g_sta->disc_cb && g_sta->stage == ESP_DET_ST_OP) g_sta->disc_cb();

  cfg_set_stage(ESP_DET_ST_CN);
  esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_FAST_CALL, NULL);
}

/** Go into detect me stage */
static void ICACHE_FLASH_ATTR
stage_detect_me()
{
  // Check back off.
  g_sta->dm_err_cnt += 1;
  if (g_sta->dm_err_cnt >= 10) {
    cfg_reset();
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_SLOW_CALL, NULL);
    return;
  }

  g_sta->dm_run = true;

  bool success;
  success = wifi_station_set_reconnect_policy(false);
  ESP_DET_DEBUG("wifi_station_set_reconnect_policy: %d\n", success);
  success = wifi_station_set_auto_connect(false);
  ESP_DET_DEBUG("wifi_station_set_auto_connect: %d\n", success);
  success = wifi_station_dhcpc_start();
  ESP_DET_DEBUG("wifi_station_dhcpc_start: %d\n", success);
  success = wifi_softap_dhcps_stop();
  ESP_DET_DEBUG("wifi_softap_dhcps_stop: %d\n", success);

  // Create access point.
  if (create_ap() != ESP_DET_OK) {
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_SLOW_CALL, NULL);
    return;
  }

  // Start command server.
  sint8 cmd_err = esp_cmd_start(ESP_DET_CMD_PORT, ESP_DET_CMD_MAX, &cmd_handle_cb);
  if (cmd_err != ESPCONN_OK && cmd_err != ESP_CMD_ERR_ALREADY_STARTED) {
    ESP_DET_ERROR("starting command server failed with error code %d\n", cmd_err);
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_SLOW_CALL, NULL);
    return;
  }
}

/** Go into wifi connect stage. */
static void ICACHE_FLASH_ATTR
stage_connect()
{
  ESP_DET_DEBUG("running stage_connect (%d)\n", g_sta->stage);

  g_sta->cn_err_cnt += 1;
  if (g_sta->cn_err_cnt >= 10) {
    cfg_reset();
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_SLOW_CALL, NULL);
    return;
  }

  if (wifi_set_opmode(STATION_MODE) != true) {
    ESP_DET_ERROR("failed to change opmode in stage_connect\n");
    cfg_reset();
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_SLOW_CALL, NULL);
  }

  // Connect to access point and wait for IP for 15 seconds.
  if (wifi_station_connect() == false) {
    ESP_DET_ERROR("calling wifi_station_connect failed\n");
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_SLOW_CALL, NULL);
    return;
  }

  if (ip_to_start(15000) != ESP_DET_OK) {
    ESP_DET_ERROR("error starting IP timeout counter\n");
    cfg_reset();
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_SLOW_CALL, NULL);
  }
}

/** Go into operational stage. */
static void ICACHE_FLASH_ATTR
stage_operational()
{
  ESP_DET_DEBUG("running stage_operational (%d)\n", g_sta->stage);

  if (g_sta->dm_run) {
    ESP_DET_DEBUG("restarting...\n");
    g_sta->restarting = true;
    system_restart();
    return;
  }

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
  UNUSED(event);
  UNUSED(arg);
  ESP_DET_DEBUG("running main_e_cb in stage %d\n", g_sta->stage);

  if (g_sta->stage == ESP_DET_ST_DM) {
    stage_detect_me();
  } else if (g_sta->stage == ESP_DET_ST_CN) {
    stage_connect();
  } else if (g_sta->stage == ESP_DET_ST_OP) {
    if (g_sta->connected) {
      stage_operational();
    } else {
      stage_connect();
    }
  } else {
    ESP_DET_ERROR("unexpected stage %d - resetting config\n", g_sta->stage);
    cfg_reset();
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, ESP_DET_SLOW_CALL, NULL);
  }
}

/**
 * The WiFi events handler.
 *
 * @param event The WiFi event.
 */
static void ICACHE_FLASH_ATTR
wifi_event_cb(System_Event_t *event)
{
  if (g_sta->restarting == true) {
    return;
  }

  switch (event->event) {
    case EVENT_STAMODE_CONNECTED:
      ESP_DET_DEBUG("wifi event: EVENT_STAMODE_CONNECTED\n");
      break;

    case EVENT_STAMODE_DISCONNECTED:
      ESP_DET_DEBUG("wifi event: EVENT_STAMODE_DISCONNECTED reason %d\n",
                    event->event_info.disconnected.reason);

      esp_eb_trigger(ESP_DET_EV_DISC,
                     (void *) ((uint32_t) event->event_info.disconnected.reason));
      break;

    case EVENT_STAMODE_AUTHMODE_CHANGE:
      ESP_DET_DEBUG("wifi event: EVENT_STAMODE_AUTHMODE_CHANGE %d -> %d\n",
                    event->event_info.auth_change.old_mode,
                    event->event_info.auth_change.new_mode);
      break;

    case EVENT_STAMODE_GOT_IP:
      ESP_DET_DEBUG("got IP %d.%d.%d.%d / %d.%d.%d.%d\n",
                    IP2STR(&(event->event_info.got_ip.ip)),
                    IP2STR(&(event->event_info.got_ip.mask)));

      esp_eb_trigger(ESP_DET_EV_GOT_IP, NULL);
      break;

    case EVENT_STAMODE_DHCP_TIMEOUT:
      ESP_DET_DEBUG("wifi event: EVENT_STAMODE_DHCP_TIMEOUT\n");
      break;

    case EVENT_SOFTAPMODE_STACONNECTED:
      ESP_DET_DEBUG("wifi event: EVENT_SOFTAPMODE_STACONNECTED\n");
      break;

    case EVENT_SOFTAPMODE_STADISCONNECTED:
      ESP_DET_DEBUG("wifi event: EVENT_SOFTAPMODE_STADISCONNECTED\n");
      break;

    case EVENT_OPMODE_CHANGED:
      ESP_DET_DEBUG("wifi event: EVENT_OPMODE_CHANGED %d\n", wifi_get_opmode());
      break;

    case EVENT_SOFTAPMODE_PROBEREQRECVED:
      // Ignoring this one.
      break;

    default:
      ESP_DET_DEBUG("unexpected wifi event: %d\n", event->event);
      break;
  }
}

static uint16 ICACHE_FLASH_ATTR
encrypt_noop(uint8_t *dst, const uint8_t *src, uint16 src_len)
{
  os_memmove(dst, src, src_len);
  return src_len;
}

static uint16 ICACHE_FLASH_ATTR
decrypt_noop(uint8_t *dst, const uint8_t *src, uint16 src_len)
{
  os_memmove(dst, src, src_len);
  return src_len;
}

///////////////////////////////////////////////////////////////////////////////
// Command handling                                                          //
///////////////////////////////////////////////////////////////////////////////

/**
 * Build error JSON response.
 *
 * @param msg  The response message.
 * @param code The error code if success is false, 0 otherwise.
 *
 * @return  Returns cJSON pointer or NULL on error.
 */
static cJSON *ICACHE_FLASH_ATTR
cmd_valid_err_resp(char *msg, uint16 code)
{
  cJSON *json = cJSON_CreateObject();
  if (json == NULL) return NULL;

  cJSON_AddItemToObject(json, "success", cJSON_CreateBool(false));
  cJSON_AddItemToObject(json, "code", cJSON_CreateNumber((double) code));
  cJSON_AddItemToObject(json, "msg", cJSON_CreateString(msg));

  return json;
}

/**
 * Build success JSON response.
 *
 * @return  Returns cJSON pointer or NULL on error.
 */
static cJSON *ICACHE_FLASH_ATTR
cmd_success_resp()
{
  cJSON *json = cJSON_CreateObject();
  if (json == NULL) return NULL;

  cJSON_AddItemToObject(json, "success", cJSON_CreateBool(true));
  cJSON_AddItemToObject(json, "ic", cJSON_CreateString("ESP2866"));
  cJSON_AddItemToObject(json, "memory", cJSON_CreateNumber(flash_real_size()));

  return json;
}

/**
 * Send response.
 *
 * @param dst     The pointer to response string.
 * @param dst_len The maximum response length in bytes.
 * @param resp    The command to send as a response.
 *
 * @return Returns response length.
 */
static uint16 ICACHE_FLASH_ATTR
cmd_resp(uint8_t *dst, uint16 dst_len, cJSON *resp)
{
  UNUSED(dst_len);
  uint16 resp_len = 0;

  char *resp_str = cJSON_PrintUnformatted(resp);
  if (resp_str != NULL) {
    os_printf("sending: %s -> %d\n", resp_str, strlen(resp_str));
    resp_len = g_sta->encrypt_cb(dst, (const uint8_t *) resp_str, (uint16) strlen(resp_str));
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
    return cmd_valid_err_resp("missing ap_name key", ESP_DET_ERR_VALIDATION);
  }

  cJSON *ap_pass = cJSON_GetObjectItem(cmd, "ap_pass");
  if (ap_pass == NULL) {
    return cmd_valid_err_resp("missing ap_pass key", ESP_DET_ERR_VALIDATION);
  }

  cJSON *ip = cJSON_GetObjectItem(cmd, "mqtt_ip");
  if (ip == NULL) {
    return cmd_valid_err_resp("missing mqtt_ip key", ESP_DET_ERR_VALIDATION);
  }

  cJSON *mqtt_port = cJSON_GetObjectItem(cmd, "mqtt_port");
  if (mqtt_port == NULL) {
    return cmd_valid_err_resp("missing mqtt_port key", ESP_DET_ERR_VALIDATION);
  }

  cJSON *mqtt_user = cJSON_GetObjectItem(cmd, "mqtt_user");
  if (mqtt_user == NULL) {
    return cmd_valid_err_resp("missing mqtt_user key", ESP_DET_ERR_VALIDATION);
  }

  cJSON *mqtt_pass = cJSON_GetObjectItem(cmd, "mqtt_pass");
  if (mqtt_pass == NULL) {
    return cmd_valid_err_resp("missing mqtt_pass key", ESP_DET_ERR_VALIDATION);
  }

  // Check valid stages this command can be run.

  if (g_sta->stage != ESP_DET_ST_DM) {
    return cmd_valid_err_resp("unexpected stage", ESP_DET_ERR_VALIDATION);
  }

  // Make changes.

  uint32_t mqtt_ip = ipaddr_addr(ip->valuestring);
  esp_det_err err = cfg_set(
      ap_name->valuestring,
      ap_pass->valuestring,
      mqtt_ip,
      (uint16_t) mqtt_port->valueint,
      mqtt_user->valuestring,
      mqtt_pass->valuestring,
      ESP_DET_ST_CN);

  if (err != ESP_DET_OK) {
    return cmd_valid_err_resp("failed setting configuration", err);
  }

  // Success.

  return cmd_success_resp();
}

/**
 * Handle command callback.
 *
 * @param res      Pointer to response buffer.
 * @param res_len  The response buffer length.
 * @param req      The client command.
 * @param req_len  The client command length.
 *
 * @return Returns response length.
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
  g_sta->decrypt_cb(buff, req, req_len);
  ESP_DET_DEBUG("handling cmd: %s\n", buff);

  cmd_json = cJSON_Parse((const char *) buff);
  if (cmd_json == NULL) {
    json_resp = cmd_valid_err_resp("could not decode json", ESP_DET_ERR_CMD_BAD_JSON);
    os_free(buff);
    return cmd_resp(res, res_len, json_resp);
  }

  cJSON *det_cmd = cJSON_GetObjectItem(cmd_json, "cmd");
  if (det_cmd == NULL) {
    json_resp = cmd_valid_err_resp("bad command format", ESP_DET_ERR_CMD_MISSING);
    cJSON_Delete(cmd_json);
    os_free(buff);
    return cmd_resp(res, res_len, json_resp);
  }

  if (strcmp(det_cmd->valuestring, ESP_DET_CMD_SET_AP) == 0) {
    json_resp = cmd_set_ap(cmd_json);
  } else {
    json_resp = cmd_valid_err_resp("unknown command", ESP_DET_ERR_UNKNOWN_CMD);
  }

  resp_len = cmd_resp(res, res_len, json_resp);
  if (cJSON_GetObjectItem(json_resp, "success")->type == cJSON_True) {
    esp_eb_trigger_delayed(ESP_DET_EV_MAIN, 250, NULL);
  }

  cJSON_Delete(cmd_json);
  cJSON_Delete(json_resp);
  os_free(buff);

  return resp_len;
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions                                                          //
///////////////////////////////////////////////////////////////////////////////

/**
 * Returns true if access point configuration is the same.
 *
 * @param c1 The access point config.
 * @param c2 The access point config.
 * @return True if configs are equal.
 */
static bool ICACHE_FLASH_ATTR
ap_config_equal(struct softap_config *c1, struct softap_config *c2)
{
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

/** Return flash size in bytes */
static uint32_t ICACHE_FLASH_ATTR
flash_real_size(void)
{
  return (uint32_t) (1 << ((spi_flash_get_id() >> 16) & 0xFF));
}
