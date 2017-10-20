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

#ifndef ESP_CMD_WIFI_H
#define ESP_CMD_WIFI_H

#include <c_types.h>
#include <user_interface.h>

/** Connect to WiFi access point. */
void ICACHE_FLASH_ATTR wifi_connect();

/** WiFi event callback */
void ICACHE_FLASH_ATTR wifi_event_cb(System_Event_t *event);

// Espressif SDK missing includes.
void ets_isr_mask(unsigned intr);

void ets_isr_unmask(unsigned intr);

#endif //ESP_CMD_WIFI_H
