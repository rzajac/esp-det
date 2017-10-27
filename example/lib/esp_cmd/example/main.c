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

#include <esp_cmd.h>

#include <user_interface.h>
#include <wifi.h>
#include <esp_sdo.h>

uint8_t cmd_count;

void server_stop_cb(sint8 err)
{
  os_printf("Sever stopped with err: %d\n", err);
}

// Handle commends received through TCP.
uint16 ICACHE_FLASH_ATTR
handle_cmd(uint8_t *resp, uint16 res_len, const uint8_t *cmd, uint16_t cmd_len)
{
  cmd_count += 1;
  os_printf("Received command (%d): %s\n", cmd_count, cmd);
  os_strncpy((char *) resp, "OK", 2);

  // After two received commands stop the TCP server.
  // We must use esp_cmd_schedule_stop instead of
  // esp_cmd_stop because we are running inside the espconn
  // callback and SDK docs clearly say not to call some APIs
  // in this case.
  if (cmd_count == 2) esp_cmd_schedule_stop(&server_stop_cb);

  return 2;
}

void ICACHE_FLASH_ATTR
sys_init_done(void)
{
  os_printf("System initialized.\n");
  wifi_connect();

  // Start command server.
  sint8_t err = esp_cmd_start(7802, 3, handle_cmd);
  os_printf("Server start err: %d\n", err);
}

void ICACHE_FLASH_ATTR
user_init()
{
  stdout_init(BIT_RATE_74880);

  // Set callback when system is done initializing.
  system_init_done_cb(sys_init_done);
}


