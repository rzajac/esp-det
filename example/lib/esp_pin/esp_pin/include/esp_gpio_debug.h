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

#ifndef ESP_GPIO_DEBUG_H
#define ESP_GPIO_DEBUG_H

#include <c_types.h>

/**
 * Dump binary representation of the value.
 *
 * @param value
 */
void ICACHE_FLASH_ATTR
esp_gpiod_dump_binary(uint32_t value);

/**
 * Dump MUX register address for given GPIO number.
 *
 * @param gpio_num The GPIO pin number.
 */
void ICACHE_FLASH_ATTR
esp_gpiod_dump_mux_addr(uint8_t gpio_num);

/**
 * Dump binary representation of GPIO register.
 *
 * @param reg_addr The register address.
 */
void ICACHE_FLASH_ATTR
esp_gpiod_dump_reg(uint32_t reg_addr);

/**
 * Dump binary representation of GPIO MUX register for given GPIO number.
 *
 * @param gpio_num The GPIO pin number.
 */
void ICACHE_FLASH_ATTR
esp_gpiod_dump_mux_reg(uint8_t gpio_num);

/** Dump binary representation of GPIO_ENABLE register */
void ICACHE_FLASH_ATTR
esp_gpiod_dump_en();

/** Dump binary representation of GPIO_OUT register */
void ICACHE_FLASH_ATTR
esp_gpiod_dump_out();

/** Dump binary representation of GPIO_IN register */
void ICACHE_FLASH_ATTR
esp_gpiod_dump_in();

#endif //ESP_GPIO_DEBUG_H
