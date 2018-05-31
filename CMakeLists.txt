# Copyright 2018 Rafal Zajac <rzajac@gmail.com>.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

# Make sure ESPROOT environemt varaiabnle is set.
if (NOT EXISTS $ENV{ESPROOT})
    message("The ESPROOT environment varaible is not set. Using $ENV{HOME}/esproot.")
    set(ENV{ESPROOT} "$ENV{HOME}/esproot")
endif()

# Bootstrap before call to project().
include("$ENV{ESPROOT}/esp-cmake/ESP8266.bootstrap.cmake")
cmake_minimum_required(VERSION 3.5)

project(esp_det C)
set(CMAKE_C_STANDARD 99)

# The directory containing global user_congih.h header file.
set(ESP_USER_CONFIG_DIR "${CMAKE_CURRENT_LIST_DIR}/example/include")
set(ESP_USER_CONFIG "${ESP_USER_CONFIG_DIR}/user_config.h")
set(ESP_LIB_BIN_DIR "${CMAKE_CURRENT_LIST_DIR}/bin")

# Magic.
include("${ESP_CMAKE_DIR}/ESP8266.cmake")

# Main application.
# Must define esp_main executable target.
add_subdirectory(src)
add_subdirectory(example)

set(CPACK_GENERATOR "TGZ")
include(CPack)
