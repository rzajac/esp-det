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
#include <mem.h>
#include <osapi.h>


bool ICACHE_FLASH_ATTR
esp_tim_start_delay(os_timer_func_t cb, void *payload, uint32_t delay)
{
  esp_tim_timer *timer = os_zalloc(sizeof(esp_tim_timer));
  if (timer == NULL) return false;

  timer->_os_timer = os_zalloc(sizeof(os_timer_t));
  if (timer->_os_timer == NULL) {
    os_free(timer);
    return false;
  }

  timer->os_timer_cb = cb;
  timer->delay = delay;
  timer->payload = payload;

  os_timer_setfn(timer->_os_timer, timer->os_timer_cb, (void *) timer);
  os_timer_arm(timer->_os_timer, delay, false);
}

bool ICACHE_FLASH_ATTR
esp_tim_start(os_timer_func_t cb, void *payload)
{
  return esp_tim_start_delay(cb, payload, ESP_TIM_DELAY_DEF);
}

void ICACHE_FLASH_ATTR
esp_tim_disarm(esp_tim_timer *timer)
{
  os_timer_disarm(timer->_os_timer);
}

void ICACHE_FLASH_ATTR
esp_tim_stop(esp_tim_timer *timer)
{
  os_timer_disarm(timer->_os_timer);
  os_free(timer->_os_timer);
  os_free(timer);
}

void ICACHE_FLASH_ATTR
esp_tim_continue(esp_tim_timer *timer)
{
  os_timer_disarm(timer->_os_timer);
  os_timer_setfn(timer->_os_timer, timer->os_timer_cb, (void *) timer);
  os_timer_arm(timer->_os_timer, timer->delay, false);
}