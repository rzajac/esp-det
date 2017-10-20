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

#ifndef ESP_CFG_H
#define ESP_CFG_H

#include <c_types.h>
#include <spi_flash.h>

// The start address for configuration sectors.
#ifndef ESP_CFG_START_SECTOR
  #define ESP_CFG_START_SECTOR 0xC
#endif

// Defines the number of configuration sectors available.
#ifndef ESP_CFG_NUMBER
  #define ESP_CFG_NUMBER 2
#endif

#define ESP_CFG_MAX_INDEX (ESP_CFG_NUMBER - 1)

// This has the same order as SpiFlashOpResult with additional result codes.
// We do that so we can easily cast SpiFlashOpResult to esp_cfg_err.
typedef enum {
  ESP_CFG_OK,
  ESP_CFG_ERR,
  ESP_CFG_ERR_TIMEOUT,

  // Additional results.
  ESP_CFG_ERR_MEM = 200,    // Out of memory.
  ESP_CFG_ERR_INIT,         // Configuration not initialized.
  ESP_CFG_ERR_INIT_TWICE,   // Configuration already initialized.
  ESP_CFG_ERR_NOT_SUPPORTED // Not supported configuration number.
} esp_cfg_err;

// Espressif SDK missing includes.
void ets_isr_mask(unsigned intr);
void ets_isr_unmask(unsigned intr);

/**
 * Initialize user config.
 *
 * This MUST be called before reading or writing custom configuration.
 *
 * @param cfg_num The config number. See ESP_CFG_NUMBER.
 * @param config  The pointer to custom config structure.
 * @param size    The size of the custom config structure.
 *
 * @return Error code or ESP_CFG_OK on success.
 */
esp_cfg_err ICACHE_FLASH_ATTR
esp_cfg_init(uint8_t cfg_num, void *config, uint32 size);

/**
 * Read custom config from flash memory.
 *
 * @param cfg_num The config number. See ESP_CFG_NUMBER.
 *
 * @return The status of read operation.
 */
esp_cfg_err ICACHE_FLASH_ATTR
esp_cfg_read(uint8_t cfg_num);

/**
 * Write custom configuration to flash.
 *
 * @param cfg_num The config number. See ESP_CFG_NUMBER.
 *
 * @return The status of write operation.
 */
esp_cfg_err ICACHE_FLASH_ATTR
esp_cfg_write(uint8_t cfg_num);

#endif //ESP_CFG_H
