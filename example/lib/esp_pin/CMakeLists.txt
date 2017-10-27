# This is not complete file. It has bare minimum to use this project with CLion.

cmake_minimum_required(VERSION 3.1)
project(esp_pin)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_C_COMPILER "$ENV{HOME}/lib/esp-open-sdk/xtensa-lx106-elf/bin/xtensa-lx106-elf-gcc")
SET(CMAKE_CXX_COMPILER "$ENV{HOME}/lib/esp-open-sdk/xtensa-lx106-elf/bin/xtensa-lx106-elf-g++")

if(NOT DEFINED ENV{ESP_OPEN_SDK})
    message("Seting ESP_OPEN_SDK to $ENV{HOME}/lib/esp-open-sdk")
    set(ENV{ESP_OPEN_SDK} "$ENV{HOME}/lib/esp-open-sdk")
endif()

message("Open SDK at $ENV{ESP_OPEN_SDK}")

include_directories(${CMAKE_CURRENT_LIST_DIR}/example/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/esp_pin/include)
include_directories($ENV{ESP_OPEN_SDK}/sdk/include)

set(HEADER_FILES
        example/include/user_config.h
        esp_pin/include/esp_gpio.h
        esp_pin/include/esp_gpio_debug.h
        esp_pin/include/esp_sdo.h
)

set(SOURCE_FILES
        example/main.c
        esp_pin/esp_gpio.c
        esp_pin/esp_gpio_debug.c
        esp_pin/esp_sdo.c
)

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})