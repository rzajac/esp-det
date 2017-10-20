/**
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

#include <user_interface.h>
#include <uart.h>
#include <osapi.h>
#include <esp_json.h>
#include <mem.h>

#define EXAMPLE_JSON "{\"cmd\": \"test\", \"code\": 23, \"fast\": true}"

void ICACHE_FLASH_ATTR print_success_json()
{
  cJSON *resp = cJSON_CreateObject();
  cJSON_AddItemToObject(resp, "success", cJSON_CreateBool(1));
  cJSON_AddItemToObject(resp, "code", cJSON_CreateNumber((double) 0));
  cJSON_AddItemToObject(resp, "error", cJSON_CreateString(""));

  char *json = cJSON_PrintUnformatted(resp);
  os_printf("Success message: %s\n", json);

  cJSON_Delete(resp);
  os_free(json);
}

void ICACHE_FLASH_ATTR parse_example_json()
{
  cJSON *json = cJSON_Parse(EXAMPLE_JSON);

  char *cmd = cJSON_GetObjectItem(json, "cmd")->valuestring;
  int code = cJSON_GetObjectItem(json, "code")->valueint;
  int fast = cJSON_GetObjectItem(json, "fast")->valueint;

  os_printf("Parsed message: cmd: %s, code: %d, fast: %d\n", cmd, code, fast);

  cJSON_Delete(json);
}

void ICACHE_FLASH_ATTR sys_init_done(void)
{
  os_printf("System initialized.\n");

  print_success_json();
  parse_example_json();
}

void ICACHE_FLASH_ATTR user_init()
{
  // No need for wifi for this example.
  wifi_station_disconnect();
  wifi_set_opmode_current(NULL_MODE);

  // Initialize UART.
  uart_init(BIT_RATE_74880, BIT_RATE_74880);

  // Set callback when system is done initializing.
  system_init_done_cb(sys_init_done);
}
