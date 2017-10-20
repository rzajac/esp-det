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

#include <esp_cfg.h>

#include <ets_sys.h>
#include <mem.h>

// Type defining user configuration structure.
typedef struct {
  uint32 cfg_size; // The size of the user configuration structure.
  void *cfg;       // The pointer to user config structure.
} user_config;

// Pointers to initialized configuration structures.
static user_config *user_configs[ESP_CFG_NUMBER];


static esp_cfg_err ICACHE_FLASH_ATTR
esp_cfg_check(uint8_t cfg_num) {
  if (cfg_num > ESP_CFG_MAX_INDEX) return ESP_CFG_ERR_NOT_SUPPORTED;
  if (user_configs[cfg_num] == NULL) return ESP_CFG_ERR_INIT;

  return ESP_CFG_OK;
}

esp_cfg_err ICACHE_FLASH_ATTR
esp_cfg_init(uint8_t cfg_num, void *config, uint32 size) {
  esp_cfg_err err = esp_cfg_check(cfg_num);

  // We ignore ESP_CFG_ERR_INIT in this case.
  if (!(err == ESP_CFG_OK || err == ESP_CFG_ERR_INIT)) return err;

  user_configs[cfg_num] = os_zalloc(sizeof(user_config));
  if (user_configs[cfg_num] == NULL) return ESP_CFG_ERR_MEM;

  user_configs[cfg_num]->cfg = config;
  user_configs[cfg_num]->cfg_size = size;

  return ESP_CFG_OK;
}

esp_cfg_err ICACHE_FLASH_ATTR
esp_cfg_read(uint8_t cfg_num) {
  esp_cfg_err err = esp_cfg_check(cfg_num);
  if (err != ESP_CFG_OK) return err;

  uint32 sec_adr = (uint32) ((ESP_CFG_START_SECTOR + cfg_num) * SPI_FLASH_SEC_SIZE);
  SpiFlashOpResult result = spi_flash_read(sec_adr, (uint32 *) user_configs[cfg_num]->cfg,
                                           user_configs[cfg_num]->cfg_size);

  return (esp_cfg_err) result;
}

esp_cfg_err ICACHE_FLASH_ATTR
esp_cfg_write(uint8_t cfg_num) {
  esp_cfg_err err = esp_cfg_check(cfg_num);
  if (err != ESP_CFG_OK) return err;

  uint32 sec_adr = (uint32) ((ESP_CFG_START_SECTOR + cfg_num) * SPI_FLASH_SEC_SIZE);

  ETS_UART_INTR_DISABLE();
  SpiFlashOpResult result = spi_flash_erase_sector((uint16) (ESP_CFG_START_SECTOR + cfg_num));
  if (result == SPI_FLASH_RESULT_OK) {
    result = spi_flash_write(sec_adr, (uint32 *) user_configs[cfg_num]->cfg,
                             user_configs[cfg_num]->cfg_size);
  }
  ETS_UART_INTR_ENABLE();

  return (esp_cfg_err) result;
}
