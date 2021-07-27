// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "_az_ulib_registry_interface.h"
#include "az_ulib_ipc_api.h"
#include "az_ulib_ipc_interface.h"
#include "az_ulib_registry_api.h"
#include "az_ulib_result.h"
#include "stm32l475_flash_driver.h"
#include <azure/core/internal/az_precondition_internal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MAX_AZ_ULIB_REGISTRY_ENTRIES 10

static az_registry_block registry_editor[MAX_AZ_ULIB_REGISTRY_ENTRIES];

extern uint32_t __REGISTRYINFO;
extern uint32_t __REGISTRY;

static az_result result_from_hal_status(HAL_StatusTypeDef status)
{
  switch (status)
  {
    case HAL_OK:
      return AZ_OK;
    case HAL_ERROR:
      return AZ_ERROR_ULIB_SYSTEM;
    case HAL_BUSY:
      return AZ_ERROR_ULIB_BUSY;
    case HAL_TIMEOUT:
      return AZ_ERROR_ULIB_SYSTEM; // should be AZ_ERROR_ULIB_TIME_OUT
    default:
      return AZ_ERROR_ULIB_SYSTEM;
  }
}

static HAL_StatusTypeDef store_registry()
{
  HAL_StatusTypeDef hal_status;
  int32_t size = sizeof(az_registry_block) * MAX_AZ_ULIB_REGISTRY_ENTRIES;

  // erase registry
  if ((hal_status = internal_flash_erase((uint8_t*)(&__REGISTRYINFO), size)) != HAL_OK)
  {
    return hal_status;
  }

  if ((hal_status = internal_flash_write((uint8_t*)(&__REGISTRYINFO),
           (uint8_t*)&registry_editor,
           sizeof(az_registry_block) * MAX_AZ_ULIB_REGISTRY_ENTRIES))
      != HAL_OK)
  {
    return hal_status;
  }

  return hal_status;
}

AZ_NODISCARD az_result az_ulib_registry_init()
{
  // read registry_editor from flash
  memcpy(&registry_editor,
      (uint8_t*)(&__REGISTRYINFO),
      sizeof(az_registry_block) * MAX_AZ_ULIB_REGISTRY_ENTRIES);

  return AZ_OK;
}

// Find pointer to the next available space in flash to store registry key/value
// Will always be called after init
static uint8_t* flash_buf_get()
{
  az_registry_block last_block;

  // empty registry
  if (registry_editor[0].used_flag == REGISTRY_FREE)
  {
    return (uint8_t*)(&__REGISTRY);
  }

  // The beginning of buffer space
  uint8_t* max_address = (uint8_t*)(&__REGISTRY);

  // registry with content
  for (int i = 0; i < MAX_AZ_ULIB_REGISTRY_ENTRIES; i++)
  {
    if (registry_editor[i].used_flag == REGISTRY_USED)
    {
      if (az_span_ptr(registry_editor[i].value) > max_address)
      {
        max_address = az_span_ptr(registry_editor[i].value);
        last_block  = registry_editor[i];
      }
    }
  }

  // last_block should have reference to the last block in available memory
  uint8_t* new_block_ptr
      = az_span_ptr(last_block.value) + ((((az_span_size(last_block.value) - 1) >> 3) + 1) << 3);
  return new_block_ptr;
}

AZ_NODISCARD az_result az_ulib_registry_get(az_span key, az_span* value)
{
  // registry with content
  for (int i = 0; i < MAX_AZ_ULIB_REGISTRY_ENTRIES; i++)
  {
    if (registry_editor[i].used_flag == REGISTRY_USED)
    {
      if (az_span_is_content_equal(key, registry_editor[i].key))
      {
        *value = registry_editor[i].value;
        return AZ_OK;
      }
    }
    else
    {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
  }

  // item not found in registry
  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD az_result az_ulib_registry_add(az_span new_key, az_span new_value)
{
  int32_t free_block_index;
  HAL_StatusTypeDef hal_status;
  az_result result;

  // validate input parameters 

  // validate for duplicates before adding new entry
  result = az_ulib_registry_get(new_key, &new_value);

  if (result == AZ_OK)
  {
    return AZ_ERROR_ULIB_ELEMENT_DUPLICATE;
  }
  if (result != AZ_ERROR_ITEM_NOT_FOUND)
  {
    return AZ_ERROR_ULIB_SYSTEM;
  }

  // look for next available spot in list
  for (int i = 0; i < MAX_AZ_ULIB_REGISTRY_ENTRIES; i++)
  {
    if (registry_editor[i].used_flag == REGISTRY_FREE)
    {
      free_block_index = i;
      break;
    }
  }

  // find destination in flash buffer
  uint8_t* key_dest_ptr = flash_buf_get();
  uint8_t* val_dest_ptr = key_dest_ptr
      + ((((az_span_size(new_key) - 1) >> 3) + 1) << 3); // round to the nearest word address

  // set free block information
  registry_editor[free_block_index].used_flag = REGISTRY_USED;
  registry_editor[free_block_index].key       = az_span_create(key_dest_ptr, az_span_size(new_key));
  registry_editor[free_block_index].value = az_span_create(val_dest_ptr, az_span_size(new_value));

  // write key to flash
  if ((hal_status = internal_flash_write(key_dest_ptr, az_span_ptr(new_key), az_span_size(new_key)))
      != HAL_OK)
  {
    return result_from_hal_status(hal_status);
  }

  // write value to flash
  if ((hal_status
          = internal_flash_write(val_dest_ptr, az_span_ptr(new_value), az_span_size(new_value)))
      != HAL_OK)
  {
    return result_from_hal_status(hal_status);
  }

  // store registry state to flash
  hal_status = store_registry();
  if (hal_status != HAL_OK)
  {
    return result_from_hal_status(hal_status);
  }

  return AZ_OK;
}
