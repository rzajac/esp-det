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

#include <esp_det.h>
#include <esp_aes.h>
#include <esp_sdo.h>

// AES encryption configuration.
#define AES_KEY_INIT { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c }
#define AES_VI_INIT { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f }

// AES encryption key.
static const uint8_t AES_KEY[] = AES_KEY_INIT;
// AES encryption vector.
static const uint8_t AES_VI[] = AES_VI_INIT;

static void ICACHE_FLASH_ATTR
run_main_program(esp_det_err err)
{
  os_printf("running main program, esp_det error: %d\n", err);

  // Do your stuff...
}

static void ICACHE_FLASH_ATTR
wifi_disconnected()
{
  os_printf("wifi disconnected :(\n");

  // Stop network activities and wait for run_main_program to be called again.
}

static uint16 ICACHE_FLASH_ATTR
encrypt(uint8_t *output, const uint8_t *input, uint16 length)
{
  return (uint16) esp_aes_encrypt(output, (uint8_t *) input, length, AES_KEY, AES_VI);
}

static uint16 ICACHE_FLASH_ATTR
decrypt(uint8_t *output, const uint8_t *input, uint16 length)
{
  return (uint16) esp_aes_decrypt(output, (uint8_t *) input, length, AES_KEY, AES_VI);
}

void ICACHE_FLASH_ATTR
sys_init_done()
{
  os_printf("System initialized. Starting ESP detection.\n");
  os_printf("RTC cycle: %d\n", system_rtc_clock_cali_proc() >> 12);

  esp_det_err err = esp_det_start("password",
                                  6,
                                  run_main_program,
                                  wifi_disconnected,
                                  encrypt,
                                  decrypt,
                                  false);

  // No encryption case.
  //esp_det_err err = esp_det_start("password",
  //                                6,
  //                                run_main_program,
  //                                wifi_disconnected,
  //                                NULL,
  //                                NULL,
  //                                false);

  if (err != ESP_DET_OK) {
    os_printf("ESP detect failed with: %d\n", err);
  }
}

void ICACHE_FLASH_ATTR
user_init()
{
  // Initialize UART.
  stdout_init(BIT_RATE_74880);

  // Set callback when system is done initializing.
  system_init_done_cb(sys_init_done);
}
