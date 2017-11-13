# Copyright 2017 Rafal Zajac <rzajac@gmail.com>.
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


project(esp_det C)

find_package(esp_eb REQUIRED)
find_package(esp_cmd REQUIRED)
find_package(esp_cfg REQUIRED)
find_package(esp_json REQUIRED)

add_library(esp_det STATIC
    esp_det.c
    include/esp_det.h)

target_include_directories(esp_det PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${esp_eb_INCLUDE_DIRS}
    ${esp_cmd_INCLUDE_DIRS}
    ${esp_cfg_INCLUDE_DIRS}
    ${esp_json_INCLUDE_DIRS}
    ${ESP_USER_CONFIG_DIR})

esp_gen_lib(esp_det
    ${esp_eb_LIBRARIES}
    ${esp_cmd_LIBRARIES}
    ${esp_cfg_LIBRARIES}
    ${esp_json_LIBRARIES}
    ${ESP_CMAKE_FIND_DIR})
