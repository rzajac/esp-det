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


find_package(esp_sdo REQUIRED)
find_package(esp_aes REQUIRED)

find_package(esp_eb REQUIRED)
find_package(esp_cmd REQUIRED)
find_package(esp_cfg REQUIRED)
find_package(esp_json REQUIRED)

add_executable(esp_det_ex
    main.c
    ${ESP_USER_CONFIG})

target_include_directories(esp_det_ex PUBLIC
    ${esp_det_SOURCE_DIR}/include
    ${esp_sdo_INCLUDE_DIRS}
    ${esp_aes_INCLUDE_DIRS}
    ${esp_eb_INCLUDE_DIRS}
    ${esp_cmd_INCLUDE_DIRS}
    ${esp_cfg_INCLUDE_DIRS}
    ${esp_json_INCLUDE_DIRS}
    ${ESP_USER_CONFIG_DIR})

target_link_libraries(esp_det_ex
    ${esp_sdo_LIBRARIES}
    ${esp_aes_LIBRARIES}
    esp_det)

esp_gen_exec_targets(esp_det_ex)
