// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "stm32l4xx_hal.h"

HAL_StatusTypeDef internal_flash_write(unsigned char* destination_ptr, unsigned char* source_ptr, unsigned int size);
HAL_StatusTypeDef internal_flash_erase(unsigned char* destination_ptr, unsigned int package_size);