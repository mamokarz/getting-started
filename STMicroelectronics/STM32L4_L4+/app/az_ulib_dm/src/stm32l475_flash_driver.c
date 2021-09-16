// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "stm32l475_flash_driver.h"
#include "stm32l4xx.h"
#include "stm32l475xx.h"
#include "stm32l4xx_hal.h"

#include <stdint.h>

/* Define the bank2 address for new firmware.  */
#define FLASH_BANK2_ADDR                (FLASH_BASE + FLASH_BANK_SIZE)

/* Define the internal variables.  */
static uint32_t total_write_size;
static uint32_t write_size;
static union 
{
    uint64_t write_buffer_int64;
    unsigned char write_buffer_char[8];
}write_buffer;

static int remainder_count;

/* Write firmware into internal flash.  */
HAL_StatusTypeDef internal_flash_write(unsigned char* destination_ptr, unsigned char* source_ptr, unsigned int size)
{

HAL_StatusTypeDef       status;


    write_size += size;
    
    for (; (remainder_count >= 0) && (remainder_count < 8) && (size > 0); destination_ptr++, size--)
    {
        write_buffer.write_buffer_char[remainder_count++] = *source_ptr++;
    }
    
    HAL_FLASH_Unlock();
    
    // definitely will be 8 but destination pointer has been increased by 8 for unknown reason and now we are taking it off
    // write the first write buffer int64 into flash
    if (remainder_count == 8)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)(destination_ptr - 8), write_buffer.write_buffer_int64); 
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return status;
        }
        remainder_count = -1;
    }

    for (; size > 8; size -= 8, destination_ptr += 8, source_ptr += 8)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)(destination_ptr), (uint64_t)*(uint32_t*)source_ptr | ((uint64_t)*(uint32_t*)(source_ptr + 4)) << 32); 
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return status;
        }
    }
    if (size > 0)
    {
        remainder_count = 0;
    }
    for (; size > 0; size --)
    {
        write_buffer.write_buffer_char[remainder_count++] = *source_ptr++;
        destination_ptr++;
    }
    if ((remainder_count >= 0) && (write_size >= total_write_size))
    {
        while (remainder_count < 8)
        {
            write_buffer.write_buffer_char[remainder_count++] = 0xFF;
            destination_ptr++;
        }

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)(destination_ptr - 8), write_buffer.write_buffer_int64); 
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return status;
        }
    }
    HAL_FLASH_Lock();
    return HAL_OK;
}

// Specific helper function for erasing flash for STM32L4, only erases the last page
HAL_StatusTypeDef internal_flash_erase(unsigned char* destination_ptr, unsigned int size)
{
    // calculate the page where destination_ptr is at and erase
    uint32_t firstPage = 0;
    uint32_t numPages = (size + 2047) / 2048;
    uint32_t bank;
    
    if ((unsigned int)destination_ptr <= FLASH_BANK1_END) 
    {
        bank = FLASH_BANK_1;
        firstPage = ((unsigned int)destination_ptr-FLASH_BASE)/2048 - 1;
    }
    else
    {
        bank = FLASH_BANK_2;
        firstPage = ((unsigned int)destination_ptr-FLASH_BANK2_ADDR)/2048 -1;
    }

    

	// unlock flash
	HAL_FLASH_Unlock();
	HAL_FLASH_OB_Unlock();

	FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks = bank;
	EraseInitStruct.Page = firstPage; // bank 2 page 0 is 0x80800000, using last page
	EraseInitStruct.NbPages = numPages;
        
	uint32_t PageError;
    HAL_StatusTypeDef status;
    if ((status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError)) == HAL_ERROR) 
    {
        // HAL_FLASHEx_Erase() requires two calls to work, if fails a second time then exit
        // lock flash
        status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    }
	
	// Lock flash
	HAL_FLASH_OB_Lock();
	HAL_FLASH_Lock();		
	
	return status;
}
                    