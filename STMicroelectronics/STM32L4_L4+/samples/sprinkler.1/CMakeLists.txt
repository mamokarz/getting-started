#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. 
#See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.10)

# Define the Project
project(sprinkler.1 C ASM)

add_executable(sprinkler.1
  startup/gnu/txm_module_preamble.s
  startup/gnu/gcc_setup.s
  startup/gnu/tx_initialize_low_level.s
  startup/gnu/precondition_fail.c
  ${CMAKE_CURRENT_LIST_DIR}/sprinkler_interface.c
  ${CMAKE_CURRENT_LIST_DIR}/sprinkler.c
)

set_target_properties(sprinkler.1
PROPERTIES 
    SUFFIX ".elf"
)

target_include_directories(sprinkler.1
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_options(sprinkler.1
    PRIVATE
        -fpie -fno-plt -fno-jump-tables ${PIC_FLAGS})

target_link_options(sprinkler.1
    PRIVATE 
        -T${CMAKE_CURRENT_LIST_DIR}/startup/gnu/STM32L475xx_FLASH.ld -pie -Wl,-Map=sprinkler.1.map)

target_link_libraries(sprinkler.1
  stm32cubel4
  az::ulib
  az::core_pic
)

add_custom_command(TARGET sprinkler.1 
    POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} ARGS -O binary -j .preamble -j .dynsym -j dynstr -j .rel.dyn 
          -j .rel.plt -j .plt -j .interp -j .hash -j .text -j .dtors -j .ctors -j .got -j .rodata 
          -j .fast -j .fast_run -j .data -j .data_run -j .bss -j .non_init -j .heap
        ${CMAKE_CURRENT_LIST_DIR}/../../build/samples/sprinkler.1/sprinkler.1.elf
        ${CMAKE_CURRENT_LIST_DIR}/../../build/samples/sprinkler.1/sprinkler.1.bin
    COMMENT "Converting the ELF output to a binary file"
)
