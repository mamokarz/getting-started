#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. 
#See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.10)

# Define the Project
project(key_vault C ASM)

add_executable(key_vault
  startup/gnu/txm_module_preamble.s
  startup/gnu/gcc_setup.s
  startup/gnu/tx_initialize_low_level.s
  ${CMAKE_CURRENT_LIST_DIR}/az_ulib_ipc_wrapper.c
  ${CMAKE_CURRENT_LIST_DIR}/key_vault_v1i1_interface.c
  ${CMAKE_CURRENT_LIST_DIR}/key_vault_v1i1.c
)

set_target_properties(key_vault
PROPERTIES 
    SUFFIX ".elf"
)

target_include_directories(key_vault
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CORE_LIB_DIR}/azure-ulib-c/inc
    ${CORE_LIB_DIR}/azure-ulib-c/config
    ${CORE_LIB_DIR}/azure-ulib-c/deps/azure-core-c/inc
)

target_compile_options(key_vault PRIVATE -fpie -fno-plt -mpic-data-is-text-relative -msingle-pic-base)

target_link_options(key_vault
    PRIVATE 
        -T${CMAKE_CURRENT_LIST_DIR}/startup/gnu/STM32L475xx_FLASH.ld -e _txm_module_thread_shell_entry -Wl,-Map=key_vault.map)

target_link_libraries(key_vault
  azrtos::txm
  az::core
)

