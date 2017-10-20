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

#ifndef ESP_CMD_H
#define ESP_CMD_H

#include <c_types.h>
#include <osapi.h>
#include <ip_addr.h>
#include <espconn.h>

// Maximum number of TCP connections.
#define ESP_CMD_MAX_CONN 3
// Maximum response length.
#define ESP_CMD_MAX_RESP_LEN 255

#ifndef ESP_CMD_DEBUG_ON
  #define ESP_CMD_DEBUG_ON 0
#endif

#ifndef DEBUG_ON
  #define DEBUG_ON 0
#endif

#if ESP_CMD_DEBUG_ON || DEBUG_ON
  #define ESP_CMD_DEBUG(format, ...) os_printf("CMD DBG: " format, ## __VA_ARGS__ )
#else
  #define ESP_CMD_DEBUG(format, ...) do {} while(0)
#endif

#define ESP_CMD_ERROR(format, ...) os_printf("CMD ERR: " format, ## __VA_ARGS__ )

// Error returned when server already started.
#define ESP_CMD_ERR_ALREADY_STARTED 100
// Error returned when server already stopped.
#define ESP_CMD_ERR_ALREADY_STOPPED 101

/**
 * The command handler callback prototype.
 *
 * @param res     The response buffer.
 * @param res_len The response buffer maximum length.
 * @param req     The request.
 * @param req_len The request length.
 *
 * @return Number of bytes written to the res buffer.
 */
typedef uint16 (esp_cmd_cb)(uint8_t *res, uint16 res_len, const uint8_t *req, uint16_t req_len);

/**
 * The callback when stopping command server using esp_cmd_schedule_stop.
 *
 * @param err The error code returned from esp_cmd_stop function.
 */
typedef void (esp_cmd_stop_cb)(sint8 err);

/**
 * Start command server.
 *
 * @param port     The TCP port.
 * @param max_conn The maximum number of connections allowed.
 * @param cb       The callback called every time server receives TCP message.
 *
 * @return The one of the ESPCONN_* error constants defined in espconn.h file or
 *         ESP_CMD_ERR_ALREADY_STARTED, ESP_CMD_ERR_ALREADY_STOPPED.
 */
sint8 ICACHE_FLASH_ATTR
esp_cmd_start(int port, uint8 max_conn, esp_cmd_cb *cb);

/**
 * Stop command server.
 *
 * Do not call this in espconn callbacks. Use esp_cmd_schedule_server_stop instead.
 *
 * @return The one of the ESPCONN_* error constants defined in espconn.h file or
 *         ESP_CMD_ERR_ALREADY_STARTED, ESP_CMD_ERR_ALREADY_STOPPED.
 */
sint8 ICACHE_FLASH_ATTR
esp_cmd_stop();

/**
 * Schedule command server stop.
 *
 * This must be used if you wish to stop command server from espconn callback.
 *
 * @param cb The callback to be called when command server has stopped.
 *
 * @return The one of the ESPCONN_* error constants defined in espconn.h file or
 *         ESP_CMD_ERR_ALREADY_STARTED, ESP_CMD_ERR_ALREADY_STOPPED.
 */
sint8 ICACHE_FLASH_ATTR
esp_cmd_schedule_stop(esp_cmd_stop_cb *cb);

#endif //ESP_CMD_H
