/*
 * Copyright 2018 Rafal Zajac <rzajac@gmail.com>.
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


#ifndef ESP_DET_H
#define ESP_DET_H

#include <esp_cfg.h>
#include <c_types.h>
#include <osapi.h>
#include <user_interface.h>

#ifndef ESP_DET_DEBUG_ON
#define ESP_DET_DEBUG_ON 1
#endif

#ifndef DEBUG_ON
#define DEBUG_ON 0
#endif

#if ESP_DET_DEBUG_ON || DEBUG_ON
#define ESP_DET_DEBUG(format, ...) os_printf("DET DBG: " format, ## __VA_ARGS__ )
#else
#define ESP_DET_DEBUG(format, ...) do {} while(0)
#endif

#define ESP_DET_ERROR(format, ...) os_printf("DET ERR: " format, ## __VA_ARGS__ )

// Used for unused arguments.
#ifndef UNUSED
  #define UNUSED(x) ( (void)(x) )
#endif

// This must be changed every time flash_cfg structure changes.
#define ESP_DET_CFG_MAGIC 17
// The esp_cfg configuration index to use.
#define ESP_DET_CFG_IDX 0
// The maximum detection access point name length.
#define ESP_DET_AP_NAME_MAX 20
// The maximum detection access point name prefix.
#define ESP_DET_AP_NAME_PREFIX_MAX 6
// The maximum detection access point password length.
#define ESP_DET_AP_PASS_MAX 20
// The MQTT broker maximum username length.
#define ESP_DET_MQTT_USER_MAX 10
// The MQTT broker maximum password length.
#define ESP_DET_MQTT_PASS_MAX 10
// The port to start command server on.
#define ESP_DET_CMD_PORT 7802
// Maximum number of TCP connections to allow for command server.
#define ESP_DET_CMD_MAX 2

// Supported TCP commands.
#define ESP_DET_CMD_SET_AP "cfg"

// The ESP detect error codes.
typedef enum {
  ESP_DET_OK,                //  (0) No error (success).
  ESP_DET_ERR_MEM,           //  (1) Out of memory.
  ESP_DET_ERR_INITIALIZED,   //  (2) Already initialized.
  ESP_DET_ERR_OPMODE,        //  (3) Error setting opmode.
  ESP_DET_ERR_WIFI_CFG_GET,  //  (4) Error getting WiFi configuration.
  ESP_DET_ERR_WIFI_CFG_SET,  //  (5) Error setting WiFi configuration.
  ESP_DET_ERR_DHCP_STOP,     //  (6) Error stopping DHCP server.
  ESP_DET_ERR_DHCP_START,    //  (7)
  ESP_DET_ERR_DHCP_OPT,      //  (8)
  ESP_DET_ERR_IP_INFO_GET,   //  (9) Error getting IP configuration.
  ESP_DET_ERR_IP_INFO_SET,   // (10) Error setting IP configuration.
  ESP_DET_ERR_CMD_BAD_JSON,  // (11) Bad JSON format (can't parse).
  ESP_DET_ERR_CMD_MISSING,   // (12) Missing cmd JSON key.
  ESP_DET_ERR_UNKNOWN_CMD,   // (13) Unknown JSON command.
  ESP_DET_ERR_VALIDATION,    // (14) JSON validation error.
  ESP_DET_ERR_AP_CFG,        // (15) Error configuring access point.
  ESP_DET_ERR_CFG_WRITE,     // (16) Error writing config.
  ESP_DET_ERR_CFG_INIT,      // (17) Error initializing flash configuration.
  ESP_DET_ERR_CFG_LOAD,      // (18) Error loading flash configuration.
} esp_det_err;

// Structure describing MQTT broker connection.
typedef struct {
  uint32_t ip;                      // The MQTT broker IP.
  uint16_t port;                    // The MQTT broker port.
  char user[ESP_DET_MQTT_USER_MAX]; // The MQTT broker username.
  char pass[ESP_DET_MQTT_PASS_MAX]; // The MQTT broker password.
} esp_det_mqtt;

/**
 * Detection done.
 *
 * Function prototype called when device is successfully configured
 * and is connected to WiFi network.
 *
 * @param err Unrecoverable error code or ESP_DET_OK on success.
 */
typedef void (esp_det_done_cb)(esp_det_err err);

/**
 * Function prototype called when device disconnects from WiFi.
 *
 * User program after receives this callback it should stop all
 * network operations and wait for esp_det_done_cb callback to be called.
 */
typedef void (esp_det_disconnect)();

/**
 * Function prototype for encrypting and decrypting array of bytes.
 *
 * @param dst     The destination buffer.
 * @param src     The source buffer (to be encrypted or decrypted).
 * @param src_len The length of the source data.
 *
 * @return The number of bytes written to dst.
 */
typedef uint16 (esp_det_enc_dec)(uint8_t *dst,
                                 const uint8_t *src,
                                 uint16 src_len);

/**
 * Start the detection procedure.
 *
 * The callback function will be called when detection is done. Meaning:
 *
 * - We know the access point and its credentials the ESP8266 should connect to.
 * - The access point connection (from previous point) was successful.
 *
 * This function must not be called multiple times.
 *
 * @param ap_prefix The access point name prefix.
 * @param ap_pass   The password for detection access point.
 * @param ap_cn     The channel to use for detection access point.
 *
 * @param done_cb   The user program callback. ESP8266 configured and
 *                  connected to WiFi network.
 *
 * @param disc_cb   Notify user program that we are no longer connected to wifi.
 *                  When connection is restored done_cb will ba called again.
 *
 * @param encrypt   The encryption callback. May be set to NULL if no
 *                  encryption is not needed.
 *
 * @param decrypt   The decryption callback. May be set to NULL if no
 *                  decryption is not needed.
 *
 * @return The error code.
 */
esp_det_err ICACHE_FLASH_ATTR
esp_det_start(char *ap_prefix,
              char *ap_pass,
              uint8_t ap_cn,
              esp_det_done_cb *done_cb,
              esp_det_disconnect *disc_cb,
              esp_det_enc_dec *encrypt,
              esp_det_enc_dec *decrypt);

/**
 * Stop ESP detection process.
 *
 * It releases memory pointed by g_sta and detaches event buss callbacks.
 */
void ICACHE_FLASH_ATTR
esp_det_stop();

/**
 * Get MQTT broker connection details.
 *
 * @param srv The structure to set values on.
 */
void ICACHE_FLASH_ATTR
esp_det_get_mqtt(esp_det_mqtt *srv);

#endif // ESP_DET_H
