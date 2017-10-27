# This is not complete file. It has bare minimum to use this project with CLion.

cmake_minimum_required(VERSION 3.8)
project(esp_det)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

SET(CMAKE_C_COMPILER "$ENV{HOME}/lib/esp-open-sdk/xtensa-lx106-elf/bin/xtensa-lx106-elf-gcc")
SET(CMAKE_CXX_COMPILER "$ENV{HOME}/lib/esp-open-sdk/xtensa-lx106-elf/bin/xtensa-lx106-elf-g++")

if(NOT DEFINED ENV{ESP_OPEN_SDK})
    message("Seting ESP_OPEN_SDK to $ENV{HOME}/lib/esp-open-sdk")
    set(ENV{ESP_OPEN_SDK} "$ENV{HOME}/lib/esp-open-sdk")
endif()

message("Open SDK at $ENV{ESP_OPEN_SDK}")

include_directories(${CMAKE_CURRENT_LIST_DIR}/example/lib/esp_aes/esp_aes/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/example/lib/esp_cmd/esp_cmd/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/example/lib/esp_cfg/esp_cfg/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/example/lib/esp_eb/esp_eb/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/example/lib/esp_json/esp_json/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/example/lib/esp_tim/esp_tim/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/example/lib/esp_pin/esp_pin/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/example/lib/uart/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/example/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/esp_det/include)
include_directories($ENV{ESP_OPEN_SDK}/sdk/include)

set(HEADER_FILES
        example/lib/esp_aes/esp_aes/include/esp_aes.h
        example/lib/esp_cmd/esp_cmd/include/esp_cmd.h
        example/lib/esp_cfg/esp_cfg/include/esp_cfg.h
        example/lib/esp_eb/esp_eb/include/esp_eb.h
        example/lib/esp_json/esp_json/include/esp_json.h
        example/lib/esp_tim/esp_tim/include/esp_tim.h
        example/lib/esp_pin/esp_pin/include/esp_sdo.h

        example/include/user_config.h
        esp_det/include/esp_det.h
)

set(SOURCE_FILES
        example/main.c

        example/lib/esp_aes/esp_aes/esp_aes.c
        example/lib/esp_cmd/esp_cmd/esp_cmd.c
        example/lib/esp_cfg/esp_cfg/esp_cfg.c
        example/lib/esp_eb/esp_eb/esp_eb.c
        example/lib/esp_json/esp_json/esp_json.c
        example/lib/esp_tim/esp_tim/esp_tim.c
        example/lib/esp_pin/esp_pin/esp_sdo.c

        esp_det/esp_det.c
)

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})