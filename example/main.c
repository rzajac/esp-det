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

#include <esp_aes.h>
#include <user_interface.h>
#include <osapi.h>
#include <mem.h>
#include <esp_sdo.h>

// AES key and initialization vector.
#define AES_KEY_INIT { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c }
#define AES_VI_INIT { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f }

// AES encryption key.
static const uint8_t AES_KEY[] = AES_KEY_INIT;
// AES encryption vector.
static const uint8_t AES_VI[] = AES_VI_INIT;

static void ICACHE_FLASH_ATTR
dump_bytes(char *label, const uint8_t *buf, uint16 len)
{
  uint8_t i;

  os_printf("DUMP: %s (%d)\n", label, len);
  for (i = 0; i < len; i++) {
    os_printf("%02x ", buf[i]);
    if ((i + 1) % 8 == 0) os_printf(" ");
    if ((i + 1) % 16 == 0) os_printf("\n");
  }
  os_printf("\n");
  os_printf("\n");
}

void ICACHE_FLASH_ATTR sys_init_done(void)
{
  uint32_t enc_len; // Number of bytes in ciphertext.

  char clear_text[] = "Super secret text."; // The text to encrypt.
  char *encrypted_text = os_zalloc(50); // Storage for encrypted and decrypted text.
  char *decrypted_text = os_zalloc(50); // Storage for decrypted text.

  os_printf("Clear text: %s\n", clear_text);
  dump_bytes("Clear text bytes:", (const uint8_t *) clear_text, (uint16) strlen(clear_text));

  os_printf("Encrypting %d characters.\n", strlen(clear_text));
  enc_len = esp_aes_encrypt((uint8_t *) encrypted_text, (uint8_t *) clear_text, strlen(clear_text), AES_KEY, AES_VI);
  dump_bytes("Encrypted text bytes:", (const uint8_t *) encrypted_text, (uint16) enc_len);

  os_printf("Decrypting.\n");
  esp_aes_decrypt((uint8_t *) decrypted_text, (uint8_t *) encrypted_text, enc_len, AES_KEY, AES_VI);
  os_printf("Decrypted text: %s\n", decrypted_text);
  dump_bytes("Decrypted text bytes:", (const uint8_t *) decrypted_text, (uint16) strlen(decrypted_text));

  os_free(encrypted_text);
  os_free(decrypted_text);
}

void ICACHE_FLASH_ATTR user_init()
{
  // No need for wifi for this example.
  wifi_station_disconnect();
  wifi_set_opmode_current(NULL_MODE);

  system_init_done_cb(sys_init_done);
  stdout_init(BIT_RATE_74880);
}
