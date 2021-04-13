#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. 
#See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.10)

add_library(dcf_tcp_gateways
  ${CMAKE_CURRENT_LIST_DIR}/dcf_tcp_gateways.c
)

#Add include directories for this target and anyone linking against it
target_include_directories(dcf_tcp_gateways
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(dcf_tcp_gateways
  azrtos::threadx
  azrtos::netxduo
  az::ulib
  netx_driver
)