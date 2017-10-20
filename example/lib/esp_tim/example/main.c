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

#include <esp_tim.h>
#include <user_interface.h>
#include <uart.h>
#include <osapi.h>
#include <mem.h>

typedef struct {
  uint8_t val1;
  int8_t val2;
} my_data;

void ICACHE_FLASH_ATTR
my_cb(void *arg)
{
  esp_tim_timer *timer = arg;
  my_data *data = timer->payload;

  os_printf("Callback val1: %d, val2: %d\n", data->val1, data->val2);

  if (data->val1 == 8) {
    os_printf("END\n");
    esp_tim_stop(timer);
    return;
  }

  data->val1++;
  data->val2--;
  timer->delay *= 2;

  esp_tim_continue(timer);
}

void ICACHE_FLASH_ATTR
sys_init_done(void)
{
  my_data *data = os_zalloc(sizeof(my_data));
  if (data == NULL) {
    os_printf("No memory!\n");
    return;
  }

  esp_tim_start(my_cb, data);
}

void ICACHE_FLASH_ATTR
user_init()
{
  // No need for wifi for this example.
  wifi_station_disconnect();
  wifi_set_opmode_current(NULL_MODE);

  system_init_done_cb(sys_init_done);
  uart_init(BIT_RATE_74880, BIT_RATE_74880);
}
