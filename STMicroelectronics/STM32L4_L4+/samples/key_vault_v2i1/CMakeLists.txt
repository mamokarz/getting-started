#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. 
#See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.10)

# Define the Project
project(key_vault_v2i1 C ASM)

add_executable(key_vault_v2i1
  startup/gnu/txm_module_preamble.s
  startup/gnu/gcc_setup.s
  startup/gnu/tx_initialize_low_level.s
  startup/gnu/precondition_fail.c
  ${CMAKE_CURRENT_LIST_DIR}/key_vault_v2i1_interface.c
  ${CMAKE_CURRENT_LIST_DIR}/key_vault_v2i1.c
)

set_target_properties(key_vault_v2i1
PROPERTIES 
    SUFFIX ".elf"
)

target_include_directories(key_vault_v2i1
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_options(key_vault_v2i1
    PRIVATE 
        -T${CMAKE_CURRENT_LIST_DIR}/startup/gnu/STM32L475xx_FLASH.ld -Wl,-Map=key_vault_v2i1.map)

target_link_libraries(key_vault_v2i1
  az::ulib
  az::core
)

add_custom_command(TARGET key_vault_v2i1 
    POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} ARGS -O binary 
        ${CMAKE_CURRENT_LIST_DIR}/../../build/samples/key_vault_v2i1/key_vault_v2i1.elf
        ${CMAKE_CURRENT_LIST_DIR}/../../build/samples/key_vault_v2i1/key_vault_v2i1.bin
    COMMENT "Converting the ELF output to a binary file"
)
