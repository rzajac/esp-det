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

#include <esp_eb.h>
#include <esp_sdo.h>
#include <user_interface.h>


#define EVENT_BATTERY_LOW "batteryLow"
#define EVENT_BATTERY_FULL "batteryFull"
#define EVENT_THROTTLED "throttled"

uint32_t arg1, arg2;

void ICACHE_FLASH_ATTR
my_event1_cb(const char *event, void *arg)
{
  os_printf("event1_cb %s handled with arg: %d\n", event, *(uint32_t *) arg);
}

void ICACHE_FLASH_ATTR
my_event2_cb(const char *event, void *arg)
{
  os_printf("event2_cb %s handled with arg: %d\n", event, *(uint32_t *) arg);
}

void ICACHE_FLASH_ATTR
my_event_throttled_cb(const char *event, void *arg)
{
  os_printf("event throttled %s handled with arg: %d\n", event, *(uint32_t *) arg);
}

void ICACHE_FLASH_ATTR
sys_init_done(void)
{
  arg1 = 123;
  arg2 = 321;

  os_printf("system initialized\n");
  os_printf("callback pointers:\n event1: %p\n event2: %p\n", my_event1_cb, my_event2_cb);

  // Add two subscribers to batteryLow event.
  esp_eb_attach(EVENT_BATTERY_LOW, my_event1_cb);
  esp_eb_attach(EVENT_BATTERY_LOW, my_event2_cb);

  // Adding already existing subscriber will not do anything.
  esp_eb_attach(EVENT_BATTERY_LOW, my_event2_cb);

  // List should have two entries.
  os_printf("\n");
  esp_eb_print_list();

  // Stop subscribing to batteryLow event.
  esp_eb_detach(EVENT_BATTERY_LOW, my_event1_cb);

  // Un-subscribing not existing is OK.
  esp_eb_detach(EVENT_BATTERY_FULL, my_event1_cb);

  // Add batteryFull subscriber.
  esp_eb_attach(EVENT_BATTERY_FULL, my_event2_cb);

  // List should have one batteryLow and batteryFull subscriber.
  os_printf("\n");
  esp_eb_print_list();

  // Trigger events.
  esp_eb_trigger(EVENT_BATTERY_LOW, &arg1);
  esp_eb_trigger(EVENT_BATTERY_FULL, &arg2);
  esp_eb_trigger_delayed(EVENT_BATTERY_FULL, 5000, &arg2);

  os_printf("\n");
  esp_eb_attach_throttled(EVENT_THROTTLED, my_event_throttled_cb, 40000);
  esp_eb_trigger(EVENT_THROTTLED, &arg1);
  esp_eb_trigger(EVENT_THROTTLED, &arg1);
  esp_eb_trigger(EVENT_THROTTLED, &arg1);
  esp_eb_trigger(EVENT_THROTTLED, &arg1);
  os_delay_us(40000);
  esp_eb_trigger(EVENT_THROTTLED, &arg1);

  esp_eb_print_list();
}

void ICACHE_FLASH_ATTR
user_init()
{
  // No need for wifi for this example.
  wifi_station_disconnect();
  wifi_set_opmode_current(NULL_MODE);

  stdout_init(BIT_RATE_74880);
  system_init_done_cb(sys_init_done);
}
