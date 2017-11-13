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

# Try to find esp_det
#
# Once done this will define:
#
#   esp_det_FOUND        - System found the library.
#   esp_det_INCLUDE_DIR  - The library include directory.
#   esp_det_INCLUDE_DIRS - If library has dependencies this will be set
#                          to <lib_name>_INCLUDE_DIR [<dep1_name_INCLUDE_DIRS>, ...].
#   esp_det_LIBRARY      - The path to the library.
#   esp_det_LIBRARIES    - The dependencies to link to use the library.
#                          It will have a form of <lib_name>_LIBRARY [dep1_name_LIBRARIES, ...].
#


find_path(esp_det_INCLUDE_DIR esp_det.h)
find_library(esp_det_LIBRARY NAMES esp_det)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(esp_det
    DEFAULT_MSG
    esp_det_LIBRARY
    esp_det_INCLUDE_DIR)

find_package(esp_eb REQUIRED)
find_package(esp_cmd REQUIRED)
find_package(esp_cfg REQUIRED)
find_package(esp_json REQUIRED)

set(esp_det_INCLUDE_DIRS
    ${esp_det_INCLUDE_DIR}
    ${esp_eb_INCLUDE_DIRS}
    ${esp_cmd_INCLUDE_DIRS}
    ${esp_cfg_INCLUDE_DIRS}
    ${esp_json_INCLUDE_DIRS})

set(esp_det_LIBRARIES
    ${esp_det_LIBRARY}
    ${esp_eb_LIBRARIES}
    ${esp_cmd_LIBRARIES}
    ${esp_cfg_LIBRARIES}
    ${esp_json_LIBRARIES})
