# This is not complete file. It has bare minimum to use this project with CLion.

cmake_minimum_required(VERSION 3.8)
project(esp_cmd)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_C_COMPILER "$ENV{HOME}/lib/esp-open-sdk/xtensa-lx106-elf/bin/xtensa-lx106-elf-gcc")
SET(CMAKE_CXX_COMPILER "$ENV{HOME}/lib/esp-open-sdk/xtensa-lx106-elf/bin/xtensa-lx106-elf-g++")

if(NOT DEFINED ENV{ESP_OPEN_SDK})
    message("Seting ESP_OPEN_SDK to $ENV{HOME}/lib/esp-open-sdk")
    set(ENV{ESP_OPEN_SDK} "$ENV{HOME}/lib/esp-open-sdk")
endif()

message("Open SDK at $ENV{ESP_OPEN_SDK}")

include_directories(${CMAKE_CURRENT_LIST_DIR}/example/lib/uart/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/example/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/esp_cmd/include)
include_directories($ENV{ESP_OPEN_SDK}/sdk/include)

set(HEADER_FILES
        esp_cmd/include/esp_cmd.h
        example/lib/uart/include/uart.h
        example/lib/uart/include/uart_register.h
        example/include/user_config.h
        esp_cmd/include/esp_cmd.h
        example/include/wifi.h
        )

set(SOURCE_FILES
        example/main.c
        example/lib/uart/uart.c
        esp_cmd/esp_cmd.c
        example/wifi.c
        )

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})