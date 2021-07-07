#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. 
#See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.10)

add_library(az_ulib_dm
  ${CMAKE_CURRENT_LIST_DIR}/src/az_ulib_dm_blob_ustream_interface.c
  ${CMAKE_CURRENT_LIST_DIR}/src/_az_ulib_dm_blob.c
  ${CMAKE_CURRENT_LIST_DIR}/src/_az_nx_blob_client.c
  ${CMAKE_CURRENT_LIST_DIR}/src/az_ulib_dm_interface.c
  ${CMAKE_CURRENT_LIST_DIR}/src/az_ulib_dm.c
  ${CMAKE_CURRENT_LIST_DIR}/src/stm32l475_flash_driver.c
)

target_include_directories(az_ulib_dm
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(az_ulib_dm
  azrtos::threadx
  azrtos::netxduo
  
  az::core
  az::ulib
  
  stm32cubel4
  netx_driver
)

add_library(az::dm ALIAS az_ulib_dm)
