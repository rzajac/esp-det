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

#include <wifi.h>

#include <osapi.h>


void ICACHE_FLASH_ATTR wifi_connect()
{
  struct station_config station_config;

  wifi_set_event_handler_cb(wifi_event_cb);
  wifi_set_opmode_current(STATION_MODE);
  os_memset(&station_config, 0, sizeof(struct station_config));
  os_memcpy(station_config.ssid, AP_NAME, (unsigned int) os_strlen(AP_NAME));
  os_memcpy(station_config.password, AP_PASS, (unsigned int) os_strlen(AP_PASS));

  ETS_UART_INTR_DISABLE();
  if (!wifi_station_set_config(&station_config)) {
    os_printf("setting wifi config failed\n");
  }
  ETS_UART_INTR_ENABLE();
  wifi_station_connect();
}

void ICACHE_FLASH_ATTR wifi_event_cb(System_Event_t *event)
{
  switch (event->event) {
    case EVENT_STAMODE_CONNECTED:
      os_printf("wifi event: EVENT_STAMODE_CONNECTED\n");
      break;

    case EVENT_STAMODE_DISCONNECTED:
      os_printf("wifi event: EVENT_STAMODE_DISCONNECTED\n");
      break;

    case EVENT_STAMODE_AUTHMODE_CHANGE:
      os_printf("wifi event: EVENT_STAMODE_AUTHMODE_CHANGE\n");
      break;

    case EVENT_STAMODE_GOT_IP:
      // Got IP from hive access point.
      os_printf("got IP: " IPSTR "\n", IP2STR(&(event->event_info.got_ip.ip)));
      os_printf("msk IP: " IPSTR "\n", IP2STR(&(event->event_info.got_ip.mask)));
      break;

    case EVENT_STAMODE_DHCP_TIMEOUT:
      os_printf("wifi event: EVENT_STAMODE_AUTHMODE_CHANGE\n");
      break;

    case EVENT_SOFTAPMODE_STACONNECTED:
      os_printf("wifi event: EVENT_SOFTAPMODE_STACONNECTED\n");
      break;

    case EVENT_SOFTAPMODE_STADISCONNECTED:
      os_printf("wifi event: EVENT_SOFTAPMODE_STADISCONNECTED\n");
      break;

    case EVENT_SOFTAPMODE_PROBEREQRECVED:
      // I basically ignore logging this one because
      // there is a lot of them and its making log less readable.
      break;

    case EVENT_OPMODE_CHANGED:
      os_printf("wifi event: EVENT_OPMODE_CHANGED\n");
      break;

    default:
      os_printf("unexpected wifi event: %d\n", event->event);
      break;
  }
}