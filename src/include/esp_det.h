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

// This must be changed every time flash_cfg structure changes.
#define ESP_DET_CFG_MAGIC 16
// The esp_cfg configuration index to use.
#define ESP_DET_CFG_IDX 0
// The maximum detection access point name length.
#define ESP_DET_AP_NAME_MAX 18
// The maximum detection access point password length.
#define ESP_DET_AP_PASS_MAX 18
// The maximum main server username length.
#define ESP_DET_SRV_USER_MAX 10
// The maximum main server password length.
#define ESP_DET_SRV_PASS_MAX 10
// The port to start command server on.
// Also used as a port for UDP broadcast messages in ESP_DET_ST_DS detection stage.
#define ESP_DET_CMD_PORT 7802
// Maximum number of TCP connections to allow for command server.
#define ESP_DET_CMD_MAX 2

// The ESP detect error codes.
typedef enum {
  ESP_DET_OK,
  ESP_DET_ERR_MEM,
  ESP_DET_ERR_INITIALIZED,
  ESP_DET_ERR_OPMODE,
  ESP_DET_ERR_WIFI_CFG_GET,
  ESP_DET_ERR_WIFI_CFG_SET,
  ESP_DET_ERR_DHCP_STOP,
  ESP_DET_ERR_DHCP_START,
  ESP_DET_ERR_DHCP_OPT,
  ESP_DET_ERR_IP_INFO_GET,
  ESP_DET_ERR_IP_INFO_SET,
  ESP_DET_ERR_CMD_BAD_JSON,
  ESP_DET_ERR_CMD_BAD_FORMAT,
  ESP_DET_ERR_CMD,
  ESP_DET_ERR_AP,
  ESP_DET_ERR_CFG,
} esp_det_err;

// Structure describing main server connection.
typedef struct {
  uint32_t ip;   // The main server IP.
  uint16_t port; // The main server port.
  char user[ESP_DET_SRV_USER_MAX]; // The main server username.
  char pass[ESP_DET_SRV_PASS_MAX]; // The main server password.
} esp_det_srv;

/**
 * Function prototype called when device is successfully configured and is connected to WiFi network.
 *
 * @param err When set to something other then 0 it means esp-det got unrecoverable error.
 */
typedef void (esp_det_done_cb)(esp_det_err err);

/**
 * Function prototype called when device disconnects from WiFi.
 *
 * User program after receives this callback it should stop all network operations and
 * wait for esp_det_done_cb callback to be called.
 */
typedef void (esp_det_disconnect)();

/**
 * Function prototype for encrypting and decrypting array of bytes.
 *
 * @param dst     The destination buffer.
 * @param src     The source buffer (to be encrypted or decrypted).
 * @param src_len The length of the source data.
 */
typedef uint16 (esp_det_enc_dec)(uint8_t *dst, const uint8_t *src, uint16 src_len);

/**
 * Start the detection procedure.
 *
 * The callback function will be called when detection is done. Meaning:
 *
 * - We know the access point and its credentials the ESP8266 should connect to.
 * - The access point connection (from previous point) was successful.
 * - We know the Main Server address and its credentials.
 *
 * @param ap_pass The password for detection access point.
 * @param ap_cn   The channel to use for detection access point.
 * @param done_cb The user program callback. ESP8266 configured and connected to WiFi network.
 * @param disc_cb Notify user program that we are no longer connected to wifi. When connection is restored
 *                done_cb will ba called again.
 * @param encrypt The encryption callback. May be set to NULL if no encryption is not needed.
 * @param decrypt The decryption callback. May be set to NULL if no decryption is not needed.
 * @param det_srv Set to true to detect main server.
 *
 * @return Error code.
 */
esp_det_err ICACHE_FLASH_ATTR
esp_det_start(char *ap_pass,
              uint8_t ap_cn,
              esp_det_done_cb *done_cb,
              esp_det_disconnect *disc_cb,
              esp_det_enc_dec *encrypt,
              esp_det_enc_dec *decrypt,
              bool det_srv);

/** Reset ESP detect library and start over. */
void ICACHE_FLASH_ATTR
esp_det_reset();

/**
 * Set connection details for main server.
 *
 * @param srv The main server connection details structure.
 */
void ICACHE_FLASH_ATTR
esp_det_get_srv(esp_det_srv *srv);

/**
 * Return number of times device was started up.
 *
 * @return Start count.
 */
uint32_t ICACHE_FLASH_ATTR
get_start_cnt();

#endif //ESP_DET_H
