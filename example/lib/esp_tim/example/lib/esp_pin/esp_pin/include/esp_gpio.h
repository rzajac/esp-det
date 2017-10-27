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

#include <c_types.h>

#ifndef ESP_GPIO_H
#define ESP_GPIO_H

#define ESP_REG(addr) *((volatile uint32_t *)(0x60000000+(addr)))

///////////////////////////////////////////////////////////////////////////////
// GPIO bit manipulation and constants.                                      //
///////////////////////////////////////////////////////////////////////////////

/*
 * List of GPIOs.
 *
 * GPIO#  Function
 * 0      During boot: high - normal operation, low - program via UART
 * 1      TX0
 * 2      During boot: high - normal operation and for UART programing
 * 3      RX0
 * 4      SDA (I²C)
 * 5      SCL (I²C)
 * 6 - 11 Flash
 * 12     MISO (SPI)
 * 13     MOSI (SPI)
 * 14     SCK (SPI)
 * 15     During boot: low - normal operation, high - SD card boot
 * 16     Sleep wake up
 */
#define GPIO0 0
#define GPIO1 1
#define GPIO2 2
#define GPIO3 3
#define GPIO4 4
#define GPIO5 5
#define GPIO6 6
#define GPIO7 7
#define GPIO8 8
#define GPIO9 9
#define GPIO10 10
#define GPIO11 11
#define GPIO12 12
#define GPIO13 13
#define GPIO14 14
#define GPIO15 15
#define GPIO16 16

// The bit values for given GPIOs.
#define GPIO_B0 0x1     // 0000 0000 0000 0001
#define GPIO_B1 0x2     // 0000 0000 0000 0010
#define GPIO_B2 0x4     // 0000 0000 0000 0100
#define GPIO_B3 0x8     // 0000 0000 0000 1000
#define GPIO_B4 0x10    // 0000 0000 0001 0000
#define GPIO_B5 0x20    // 0000 0000 0010 0000
#define GPIO_B6 0x40    // 0000 0000 0100 0000
#define GPIO_B7 0x80    // 0000 0000 1000 0000
#define GPIO_B8 0x100   // 0000 0001 0000 0000
#define GPIO_B9 0x200   // 0000 0010 0000 0000
#define GPIO_B10 0x400  // 0000 0100 0000 0000
#define GPIO_B11 0x800  // 0000 1000 0000 0000
#define GPIO_B12 0x1000 // 0001 0000 0000 0000
#define GPIO_B13 0x2000 // 0010 0000 0000 0000
#define GPIO_B14 0x4000 // 0100 0000 0000 0000
#define GPIO_B15 0x8000 // 1000 0000 0000 0000
#define GPIO_B(gpio_num) ((uint32_t) (0x1 << (gpio_num)))

#define GPIO_MODE_INPUT             0x0
#define GPIO_MODE_INPUT_PULLUP      0x1
#define GPIO_MODE_OUTPUT            0x2
#define GPIO_MODE_OUTPUT_OPEN_DRAIN 0x3

///////////////////////////////////////////////////////////////////////////////
// Macros for register addresses.                                            //
///////////////////////////////////////////////////////////////////////////////

// MUX registers for given GPIO.               Fn0      , Fn1       , Fn2          , Fn3       , Fn4
#define GPIO_MUX_0  ESP_REG(0x834) // GPIO0  - GPIO0    , I2SO_WS   , U1TXD        ,           , U0TXD
#define GPIO_MUX_1  ESP_REG(0x818) // GPIO1  - U0TXD    , SPICS1    ,              , GPIO1     , CLK_RTC
#define GPIO_MUX_2  ESP_REG(0x838) // GPIO2  - GPIO2    , I2SO_WS   , U1TXD        ,           , U0TXD
#define GPIO_MUX_3  ESP_REG(0x814) // GPIO3  - U0RXD    , I2SO_DATA ,              , GPIO3     , CLK_XTAL
#define GPIO_MUX_4  ESP_REG(0x83C) // GPIO4  - GPIO4    , CLK_XTAL  ,              ,           ,
#define GPIO_MUX_5  ESP_REG(0x840) // GPIO5  - GPIO5    , CLK_RTC   ,              ,           ,
#define GPIO_MUX_6  ESP_REG(0x81C) // GPIO6  - SD_CLK   , SIPCLK    ,              , GPIO6     , U1CTS
#define GPIO_MUX_7  ESP_REG(0x820) // GPIO7  - SD_DATA0 , SPIQ      ,              , GPIO7     , U1TXD
#define GPIO_MUX_8  ESP_REG(0x824) // GPIO8  - SD_DATA1 , SPID      ,              , GPIO8     , U1RXD
#define GPIO_MUX_9  ESP_REG(0x828) // GPIO9  - SD_DATA2 , SPIHD     ,              , GPIO9     , HSPIHD
#define GPIO_MUX_10 ESP_REG(0x82C) // GPIO10 - SD_DATA3 , SPIWP     ,              , GPIO10    , HSPIWP
#define GPIO_MUX_11 ESP_REG(0x830) // GPIO11 - CD_CMD   , SPICS0    ,              , GPIO11    , U1RTS
#define GPIO_MUX_12 ESP_REG(0x804) // GPIO12 - MTDI     , I2SI_DATA , HSPIQ (MISO) , GPIO12    , U0DTR
#define GPIO_MUX_13 ESP_REG(0x808) // GPIO13 - MTCK     , I2SI_BCK  , HSPID (MOSI) , GPIO13    , U0CTS
#define GPIO_MUX_14 ESP_REG(0x80C) // GPIO14 - MTMS     , I2SI_WS   , HSPICLK      , GPIO14    , U0DSR
#define GPIO_MUX_15 ESP_REG(0x810) // GPIO15 - MTDO     , I2SO_BCK  , HSPICS       , GPIO15    , U0RTS
#define GPIO_MUX_16 ESP_REG(0x7A0) // GPIO16 - XPD_DCDC , RTC_GPIO0 , EXT_WAKEUP   , DEEPSLEEP , BT_XTAL_EN
// Map between GPIO number and MUX register.
extern uint16_t esp_gpio_mux[17];
// GPIO MUX register address.
#define GPIO_MUX_ADR(gpio_num) (esp_gpio_mux[((gpio_num) & 0x1F)])
// The value of MUX register value.
#define GPIO_MUX(gpio_num) ESP_REG(GPIO_MUX_ADR(gpio_num))
// The function number corresponding to GPIO in MUX register (3 bits).
#define GPIO_FUNC_NUM(gpio_num) (((gpio_num)==0||(gpio_num)==2||(gpio_num)==4||(gpio_num)==5)?0:((gpio_num)==16)?1:3)
// Set GPIO function bits [8] and [5:4].
#define GPIO_FUNC(fn) ((uint32_t)(((((fn) & 4) != 0) << 8) | ((((fn) & 2) != 0) << 5) | ((((fn) & 1) != 0) << 4)))

// MUX register bits.
#define GPIO_MUX_SOE 0 // Sleep OE
#define GPIO_MUX_SLS 1 // Sleep Sel
#define GPIO_MUX_SLU 3 // Sleep pull up
#define GPIO_MUX_FU0 4 // Function bit 0
#define GPIO_MUX_FU1 5 // Function bit 1
#define GPIO_MUX_PUU 7 // Pull up
#define GPIO_MUX_FU3 8 // Function bit 2

// GPIO (0-15) configuration registers.
// Used to configure interrupt types, open drain, wakeup.
#define GPIO_CFG_0  ESP_REG(0x328)
#define GPIO_CFG_1  ESP_REG(0x32C)
#define GPIO_CFG_2  ESP_REG(0x330)
#define GPIO_CFG_3  ESP_REG(0x334)
#define GPIO_CFG_4  ESP_REG(0x338)
#define GPIO_CFG_5  ESP_REG(0x33C)
#define GPIO_CFG_6  ESP_REG(0x340)
#define GPIO_CFG_7  ESP_REG(0x344)
#define GPIO_CFG_8  ESP_REG(0x348)
#define GPIO_CFG_9  ESP_REG(0x34C)
#define GPIO_CFG_10 ESP_REG(0x350)
#define GPIO_CFG_11 ESP_REG(0x354)
#define GPIO_CFG_12 ESP_REG(0x358)
#define GPIO_CFG_13 ESP_REG(0x35C)
#define GPIO_CFG_14 ESP_REG(0x360)
#define GPIO_CFG_15 ESP_REG(0x364)
// GPIO configuration register address.
#define GPIO_CFG_ADR(gpio_num) (0x328 + (((gpio_num) & 0xF) * 4))
// The value of GPIO configuration register.
#define GPIO_CFG(gpio_num) ESP_REG(GPIO_CFG_ADR(gpio_num))

#define GPIO_CFG_WAKEUP_ENABLE 10 // Enable wake up on the pin only when GPIO_ITR_TYPE is 0x4 or 0x5.
#define GPIO_CFG_ITR_TYPE      7  // 3 bits - 0x0: disable; 0x1: positive edge; 0x2: negative edge; 0x3: both types of edge; 0x4: low-level; 0x5: highlevel
#define GPIO_CFG_DRV           2  // 0x0: normal; 0x1: open drain
#define GPIO_CFG_SRC           0  // 0x0: GPIO_DAT; 0x1: sigma-delta

// GPIO 0-15 type configuration registers.
// Used to configure GPIOs as input, output and check GPIO logic state.
#define GPIO_OUT      ESP_REG(0x300) // R/W - The output value when the GPIO pin is set as output.
#define GPIO_OUT_S    ESP_REG(0x304) // WO - Writing 1 into a bit in this register will set the related bit in GPIO_OUT_DATA.
#define GPIO_OUT_C    ESP_REG(0x308) // WO - Writing 1 into a bit in this register will clear the related bit in GPIO_OUT_DATA.
#define GPIO_EN       ESP_REG(0x30C) // R/W - The output enable register.
#define GPIO_OUT_EN_S ESP_REG(0x310) // WO - Writing 1 into a bit in this register will set the related bit in GPIO_ENABLE_DATA.
#define GPIO_OUT_EN_C ESP_REG(0x314) // WO - Writing 1 into a bit in this register will clear the related bit in GPIO_ENABLE_DATA.
#define GPIO_IN       ESP_REG(0x318) // RO - The values of the GPIO pins when the GPIO pin is set as input.
#define GPIO_ITR      ESP_REG(0x31C) // R/W - Interrupt enable register.
#define GPIO_ITR_S    ESP_REG(0x320) // WO - Writing 1 into a bit in this register will set the related bit in GPIO_STATUS_INTERRUPT.
#define GPIO_ITR_C    ESP_REG(0x324) // WO - Writing 1 into a bit in this register will clear the related bit in GPIO_STATUS_INTERRUPT.

///////////////////////////////////////////////////////////////////////////////
// GPIO macros.                                                              //
///////////////////////////////////////////////////////////////////////////////

// Make GPIO an input.
#define GPIO_IN_MAKE(gpio_num) (GPIO_OUT_EN_C = GPIO_B(gpio_num))

// Make GPIO an output.
#define GPIO_OUT_MAKE(gpio_num) (GPIO_OUT_EN_S = GPIO_B(gpio_num))

// Set given GPIO pin to high.
#define GPIO_OUT_HIGH(gpio_num) (GPIO_OUT_S = GPIO_B(gpio_num))

// Set given GPIO pin to low.
#define GPIO_OUT_LOW(gpio_num) (GPIO_OUT_C = GPIO_B(gpio_num))

// Get logic state of the pin.
#define GPIO_VALUE(gpio_num) ((GPIO_IN & GPIO_B(gpio_num))!=0)

// Set GPIO as output with low state.
#define GPIO_OUT_MAKE_LOW(gpio_num) do { \
  GPIO_OUT_LOW((gpio_num));              \
  GPIO_OUT_MAKE(gpio_num);               \
  } while (0)

// Set GPIO as output with low state.
#define GPIO_OUT_MAKE_HIGH(gpio_num) do { \
  GPIO_OUT_HIGH((gpio_num));              \
  GPIO_OUT_MAKE(gpio_num);               \
  } while (0)

// Enable GPIO pull up.
#define GPIO_PULLUP_EN(gpio_num) GPIO_MUX(gpio_num) |= (0x1 << GPIO_MUX_PUU)

// Disable GPIO pull up.
#define GPIO_PULLUP_DIS(gpio_num) GPIO_MUX(gpio_num) &= (~(0x1 << GPIO_MUX_PUU))

///////////////////////////////////////////////////////////////////////////////
// Non macro declarations.                                                   //
///////////////////////////////////////////////////////////////////////////////

/**
 * Setup pin as GPIO.
 *
 * @param gpio_num The GPIO number 0-15.
 * @param mode     One of GPIO_MODE_* defines.
 */
void ICACHE_FLASH_ATTR
esp_gpio_setup(uint8_t gpio_num, uint8_t mode);

#endif //ESP_GPIO_H
