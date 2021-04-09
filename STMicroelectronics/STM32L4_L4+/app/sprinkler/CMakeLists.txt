#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. 
#See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.10)

add_library(sprinkler
  ${CMAKE_CURRENT_LIST_DIR}/src/sprinkler_v1i1.c
  ${CMAKE_CURRENT_LIST_DIR}/src/sprinkler_v1i1_interface.c
)

target_include_directories(sprinkler
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

target_link_libraries(sprinkler
  stm32cubel4

  az::core
  az::ulib
)

