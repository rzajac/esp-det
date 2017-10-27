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

#include <esp_cfg.h>

#include <osapi.h>
#include <user_interface.h>
#include <esp_sdo.h>

// The my_config0 magic number.
#define MY_CFG_MAGIC_0 0xA3

// The my_config1 magic number.
#define MY_CFG_MAGIC_1 0xA5

struct STORE_ATTR {
  int magic;
  int cfg1;
  int cfg2;
} my_config0;

struct STORE_ATTR {
  int magic;
  int cfg1;
} my_config1;

esp_cfg_err ICACHE_FLASH_ATTR
write_config(uint8_t cfg_num)
{
  esp_cfg_err result = esp_cfg_write(cfg_num);

  if (result == ESP_CFG_OK) {
    os_printf("Config %d written.\n", cfg_num);
  } else {
    os_printf("Config %d write error: %d.\n", cfg_num, result);
  }

  return result;
}

void ICACHE_FLASH_ATTR
sys_init_done(void)
{
  bool restart = false;
  esp_cfg_err init_err;

  os_printf("Initializing configs.\n");
  init_err = esp_cfg_init(0, &my_config0, sizeof(my_config0));
  if (init_err != ESP_CFG_OK) {
    os_printf("my_config0 initialization error %d\n", init_err);
    return;
  }

  init_err = esp_cfg_init(1, &my_config1, sizeof(my_config1));
  if (init_err != ESP_CFG_OK) {
    os_printf("my_config1 initialization error %d\n", init_err);
    return;
  }

  os_printf("Reading configs.\n");
  esp_cfg_err result0 = esp_cfg_read(0);
  esp_cfg_err result1 = esp_cfg_read(1);

  if (result0 != ESP_CFG_OK || result1 != ESP_CFG_OK) {
    os_printf("Config read error. my_config0: %d, my_config1: %d\n", result0, result1);
    return;
  }

  if (my_config0.magic != MY_CFG_MAGIC_0) {
    os_printf("Writing my_config0 for the first time.\n");
    my_config0.magic = MY_CFG_MAGIC_0;
    my_config0.cfg1 = 0;
    my_config0.cfg2 = 0;

    write_config(0);
    restart = true;
  }

  if (my_config1.magic != MY_CFG_MAGIC_1) {
    os_printf("Writing my_config0 for the first time.\n");
    my_config1.magic = MY_CFG_MAGIC_1;
    my_config1.cfg1 = 0;

    write_config(1);
    restart = true;
  }

  // We just wrote configs for the first time
  // so we restart and go with the rest of the flow.
  if (restart) {
    os_printf("Please press reset...\n");
    return;
  }

  os_printf("my_config0 values: %d %d\n", my_config0.cfg1, my_config0.cfg2);
  my_config0.cfg1++;
  my_config0.cfg2--;

  os_printf("my_config1 values: %d\n", my_config1.cfg1);
  my_config1.cfg1 += 10;

  os_printf("Writing new config values.\n");

  result0 = esp_cfg_write(0);
  if (result0 != ESP_CFG_OK) {
    os_printf("Error writing my_config0 (%d)\n", result0);
  }

  result1 = esp_cfg_write(1);
  if (result0 != ESP_CFG_OK) {
    os_printf("Error writing my_config1 (%d)\n", result1);
  }

  os_printf("Press reset for next iteration...\n");
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