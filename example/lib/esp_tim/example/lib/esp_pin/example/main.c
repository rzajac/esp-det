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
#include <osapi.h>
#include <esp_gpio.h>
#include <esp_sdo.h>

static os_timer_t timer;

void ICACHE_FLASH_ATTR
blink(void *arg)
{
  // If GPIO3 is low don't change GPIO2 state.
  if (!GPIO_VALUE(GPIO3)) return;

  bool state = GPIO_VALUE(GPIO2);

  os_printf("LED state: %d\n", state);
  state ? GPIO_OUT_LOW(GPIO2) : GPIO_OUT_HIGH(GPIO2);
}

void ICACHE_FLASH_ATTR
sys_init_done()
{
  esp_gpio_setup(GPIO2, GPIO_MODE_OUTPUT);
  esp_gpio_setup(GPIO3, GPIO_MODE_INPUT);

  //Setup timer to call our callback in 1 second intervals.
  os_timer_disarm(&timer);
  os_timer_setfn(&timer, (os_timer_func_t *) blink, 0);
  os_timer_arm(&timer, 1000, true);
}

void ICACHE_FLASH_ATTR
user_init()
{
  // We don't need WiFi for this example.
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);

  stdout_init(BIT_RATE_74880);
  system_init_done_cb(sys_init_done);
}
