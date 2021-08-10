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

// __attribute__((section(".regdata"))) static az_ulib_registry_node
// registry_editor[MAX_AZ_ULIB_REGISTRY_ENTRIES];
// __attribute__((section(".regdata"))) static uint8_t registry_buffer[MAX_AZ_ULIB_REGISTRY_BUFFER];

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

static az_result set_registry_node_ready_flag(uint8_t* address)
{
  AZ_ULIB_TRY
  {
    az_result result;
    HAL_StatusTypeDef hal_status;

    if ((hal_status = internal_flash_write_doubleword((uint32_t)address, REGISTRY_READY)) != HAL_OK)
    {
      result = result_from_hal_status(hal_status);
      AZ_ULIB_THROW_IF_AZ_ERROR(result);
    }
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static az_result store_registry_node(az_ulib_registry_node node)
{
  AZ_ULIB_TRY
  {
    az_result result;
    HAL_StatusTypeDef hal_status;
    az_ulib_registry_node* runner;

    runner = (az_ulib_registry_node*)(&__REGISTRYINFO);
    // look for next available spot in list
    for (int i = 0; i < MAX_AZ_ULIB_REGISTRY_ENTRIES; i++)
    {
      if (runner->_internal.status.ready_flag == REGISTRY_FREE)
      {
        break;
      }
      runner++;
    }

    if ((hal_status = internal_flash_write(
                          (uint8_t*)(runner) + sizeof(az_ulib_registry_node_status), // add size of flags
                          (uint8_t*)&(node._internal.data),
                          sizeof(az_ulib_registry_node_data))
             != HAL_OK))
    {
      result = result_from_hal_status(hal_status);
      AZ_ULIB_THROW_IF_AZ_ERROR(result);
    }

    // after successful storage of key value pair
    result = set_registry_node_ready_flag((uint8_t*)runner);
    AZ_ULIB_THROW_IF_AZ_ERROR(result);
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static az_result set_registry_node_delete_flag(uint8_t* address)
{
  AZ_ULIB_TRY
  {
    az_result result;
    HAL_StatusTypeDef hal_status;

    if ((hal_status = internal_flash_write_doubleword(
             ((uint32_t)address + AZ_ULIB_REGISTRY_FLAG_SIZE), REGISTRY_DELETED))
        != HAL_OK)
    {
      result = result_from_hal_status(hal_status);
      AZ_ULIB_THROW_IF_AZ_ERROR(result);
    }
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result az_ulib_registry_init()
{
  // // initialize lock
  // set_registry_node_ready_flag((uint8_t*)(&__REGISTRYINFO));
  // set_registry_node_delete_flag((uint8_t*)(&__REGISTRYINFO));

  return AZ_OK;
}

// Find pointer to the next available space in flash to store registry key/value
// Will always be called after init
static uint8_t* flash_buf_get()
{
  uint8_t* flash_ptr;

  az_ulib_registry_node node;

  // empty registry
  node = *(az_ulib_registry_node*)(&__REGISTRYINFO);
  if (node._internal.status.ready_flag == REGISTRY_FREE)
  {
    flash_ptr = (uint8_t*)(&__REGISTRY);
  }
  else
  {
    az_ulib_registry_node* last_node;

    // The beginning of buffer space
    uint8_t* max_address = (uint8_t*)(&__REGISTRY);
    az_ulib_registry_node* runner = (az_ulib_registry_node*)(uint8_t*)(&__REGISTRYINFO);

    for (int i = 0; i < MAX_AZ_ULIB_REGISTRY_ENTRIES; i++)
    {
      if (runner->_internal.status.ready_flag == REGISTRY_READY)
      {
        if (az_span_ptr(runner->_internal.data.value) > max_address)
        {
          max_address = az_span_ptr(runner->_internal.data.value);
          last_node = runner;
        }
      }
      runner++;
    }

    // last_block should have reference to the last block in available memory, rounded address
    int32_t value_buf_size = ((((az_span_size(last_node->_internal.data.value) - 1) >> 3) + 1) << 3);
    flash_ptr = az_span_ptr(last_node->_internal.data.value) + value_buf_size;
  }

  return flash_ptr;
}

AZ_NODISCARD az_result az_ulib_registry_delete(az_span key)
{
  az_result result;

  az_ulib_registry_node* runner = (az_ulib_registry_node*)(&__REGISTRYINFO);

  // registry with content
  for (int i = 0; i < MAX_AZ_ULIB_REGISTRY_ENTRIES; i++)
  {
    if (runner->_internal.status.ready_flag == REGISTRY_READY)
    {
      if (az_span_is_content_equal(key, runner->_internal.data.key))
      {
        set_registry_node_delete_flag((uint8_t*)&runner);
        result = AZ_OK;
        break;
      }
    }
    else
    {
      result = AZ_ERROR_ITEM_NOT_FOUND;
      break;
    }
    runner++;
  }
  return result;
}

AZ_NODISCARD az_result az_ulib_registry_get(az_span key, az_span* value)
{
  az_result result;

  az_ulib_registry_node* runner = (az_ulib_registry_node*)(&__REGISTRYINFO);

  // registry with content
  for (int i = 0; i < MAX_AZ_ULIB_REGISTRY_ENTRIES; i++)
  {
    if (runner->_internal.status.ready_flag == REGISTRY_READY
        && runner->_internal.status.delete_flag != REGISTRY_DELETED)
    {
      if (az_span_is_content_equal(key, runner->_internal.data.key))
      {
        *value = runner->_internal.data.value;
        result = AZ_OK;
        break;
      }
    }
    else
    {
      result = AZ_ERROR_ITEM_NOT_FOUND;
      break;
    }
    runner++;
  }

  return result;
}

AZ_NODISCARD az_result az_ulib_registry_add(az_span new_key, az_span new_value)
{
  AZ_ULIB_TRY
  {
    HAL_StatusTypeDef hal_status;
    az_span sample_value;
    az_result result;
    az_ulib_registry_node new_node;

    // validate for duplicates before adding new entry
    result = az_ulib_registry_get(new_key, &sample_value);
    AZ_ULIB_THROW_IF_ERROR((result == AZ_ERROR_ITEM_NOT_FOUND), AZ_ERROR_ULIB_ELEMENT_DUPLICATE);

    // find destination in flash buffer
    uint8_t* key_dest_ptr = flash_buf_get();
    uint8_t* val_dest_ptr = key_dest_ptr + ((((az_span_size(new_key) - 1) >> 3) + 1) << 3);

    // set free block information
    new_node._internal.data.key = az_span_create(key_dest_ptr, az_span_size(new_key));
    new_node._internal.data.value = az_span_create(val_dest_ptr, az_span_size(new_value));

    // update registry node in flash
    result = store_registry_node(new_node);
    AZ_ULIB_THROW_IF_AZ_ERROR(result);

    // write key to flash
    if ((hal_status
         = internal_flash_write(key_dest_ptr, az_span_ptr(new_key), az_span_size(new_key)))
        != HAL_OK)
    {
      result = result_from_hal_status(hal_status);
    }

    // write value to flash
    if ((hal_status
         = internal_flash_write(val_dest_ptr, az_span_ptr(new_value), az_span_size(new_value)))
        != HAL_OK)
    {
      result = result_from_hal_status(hal_status);
    }

    AZ_ULIB_THROW_IF_AZ_ERROR(result);
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}
