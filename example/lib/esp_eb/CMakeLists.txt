# This is not complete file. It has bare minimum to use this project with CLion.

cmake_minimum_required(VERSION 3.8)
project(esp_eb)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_C_COMPILER "$ENV{HOME}/lib/esp-open-sdk/xtensa-lx106-elf/bin/xtensa-lx106-elf-gcc")
SET(CMAKE_CXX_COMPILER "$ENV{HOME}/lib/esp-open-sdk/xtensa-lx106-elf/bin/xtensa-lx106-elf-g++")

if(NOT DEFINED ENV{ESP_OPEN_SDK})
    message("Seting ESP_OPEN_SDK to $ENV{HOME}/lib/esp-open-sdk")
    set(ENV{ESP_OPEN_SDK} "$ENV{HOME}/lib/esp-open-sdk")
endif()

message("Open SDK at $ENV{ESP_OPEN_SDK}")

include_directories(${CMAKE_CURRENT_LIST_DIR}/example/lib/esp_tim/esp_tim/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/example/lib/esp_pin/esp_pin/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/example/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/esp_eb/include)
include_directories($ENV{ESP_OPEN_SDK}/sdk/include)

set(HEADER_FILES
        esp_eb/include/esp_eb.h
        example/lib/esp_tim/esp_tim/include/esp_tim.h
        example/lib/esp_pin/esp_pin/include/esp_sdo.h
        example/include/user_config.h esp_eb/include/esp_eb.h
)

set(SOURCE_FILES
        example/main.c
        example/lib/esp_tim/esp_tim/esp_tim.c
        example/lib/esp_pin/esp_pin/esp_sdo.c
        esp_eb/esp_eb.c
)

add_library(esp_aes ${SOURCE_FILES} ${HEADER_FILES})