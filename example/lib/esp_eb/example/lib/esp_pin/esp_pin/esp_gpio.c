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

#include <esp_gpio.h>

// GPIO MUX register address offsets.
uint16_t esp_gpio_mux[17] = {0x834, 0x818, 0x838, 0x814,
                             0x83C, 0x840, 0x81C, 0x820,
                             0x824, 0x828, 0x82C, 0x830,
                             0x804, 0x808, 0x80C, 0x810,
                             0x7A0};

void ICACHE_FLASH_ATTR
esp_gpio_setup(uint8_t gpio_num, uint8_t mode)
{
  // Set pin as GPIO.
  GPIO_MUX(gpio_num) = GPIO_FUNC(GPIO_FUNC_NUM(gpio_num));

  if (mode == GPIO_MODE_OUTPUT || mode == GPIO_MODE_OUTPUT_OPEN_DRAIN) {
    // Reset all the bits except bits [10:7].
    GPIO_CFG(gpio_num) = (GPIO_CFG(gpio_num) & (0xF << GPIO_CFG_ITR_TYPE));

    // Set open drain bit.
    if (mode == GPIO_MODE_OUTPUT_OPEN_DRAIN) GPIO_CFG(gpio_num) |= (0x1 << GPIO_CFG_DRV);

    // Set output.
    GPIO_OUT_MAKE(gpio_num);
  } else if (mode == GPIO_MODE_INPUT || mode == GPIO_MODE_INPUT_PULLUP) {
    // Set as input.
    GPIO_IN_MAKE(gpio_num);

    // Reset all the bits except bits [10:7] and set bit [2].
    GPIO_CFG(gpio_num) = (GPIO_CFG(gpio_num) & (0xF << GPIO_CFG_ITR_TYPE)) | (0x1 << GPIO_CFG_DRV);
    if (mode == GPIO_MODE_INPUT_PULLUP) GPIO_MUX(gpio_num) |= (0x1 << GPIO_MUX_PUU);
  }
}
