// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#ifndef STM32L475_FLASH_DRIVER_H
#define STM32L475_FLASH_DRIVER_H

#include "stm32l4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

void internal_flash_flush();

HAL_StatusTypeDef internal_flash_write_doubleword(uint8_t* destination, uint64_t source);

HAL_StatusTypeDef internal_flash_write(
    unsigned char* destination_ptr, unsigned char* source_ptr, uint32_t size);
    
HAL_StatusTypeDef internal_flash_erase(
    unsigned char* destination_ptr, uint32_t package_size);

#ifdef __cplusplus
}
#endif

#endif /* STM32L475_FLASH_DRIVER_H */
