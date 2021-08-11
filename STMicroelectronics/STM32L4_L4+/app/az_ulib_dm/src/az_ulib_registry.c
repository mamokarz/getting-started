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

extern uint32_t __REGISTRYINFO;
extern uint32_t __REGISTRY;

static az_ulib_pal_os_lock registry_lock;

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
    // if ((hal_status = internal_flash_write_doubleword((uint32_t)address, REGISTRY_READY)) !=
    // HAL_OK)
    // {
    //   result = result_from_hal_status(hal_status);
    //   AZ_ULIB_THROW_IF_AZ_ERROR(result);
    // }
    AZ_ULIB_THROW_IF_AZ_ERROR(
        result_from_hal_status(internal_flash_write_doubleword((uint32_t)address, REGISTRY_READY)));
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static az_result set_registry_node_delete_flag(uint8_t* address)
{
  AZ_ULIB_TRY
  {
    // if ((hal_status = internal_flash_write_doubleword(
    //          ((uint32_t)address + AZ_ULIB_REGISTRY_FLAG_SIZE), REGISTRY_DELETED))
    //     != HAL_OK)
    // {
    //   result = result_from_hal_status(hal_status);
    //   AZ_ULIB_THROW_IF_AZ_ERROR(result);
    // }
    AZ_ULIB_THROW_IF_AZ_ERROR(result_from_hal_status(internal_flash_write_doubleword(
        ((uint32_t)address + AZ_ULIB_REGISTRY_FLAG_SIZE), REGISTRY_DELETED)));
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static az_result store_registry_node(az_ulib_registry_node node, uint8_t** node_ptr)
{
  AZ_ULIB_TRY
  {
    az_ulib_registry_node* runner;
    int32_t node_index;

    runner = (az_ulib_registry_node*)(&__REGISTRYINFO);

    /* Look for next available spot */
    for (node_index = 0; node_index < MAX_AZ_ULIB_REGISTRY_ENTRIES; node_index++)
    {
      if (runner->_internal.status.ready_flag == REGISTRY_FREE)
      {
        break;
      }
      runner++;
    }

    /* Handle case if all nodes were used */
    AZ_ULIB_THROW_IF_ERROR((node_index < MAX_AZ_ULIB_REGISTRY_ENTRIES), AZ_ERROR_OUT_OF_MEMORY);

    /* Store pointers to key value pair into flash */
    AZ_ULIB_THROW_IF_AZ_ERROR(result_from_hal_status(internal_flash_write(
        (uint8_t*)(runner) + sizeof(az_ulib_registry_node_status), // add size of flags
        (uint8_t*)&(node._internal.data),
        sizeof(az_ulib_registry_node_data))));

    /* Return pointer to this node for setting flags later */
    *node_ptr = (uint8_t*)runner;
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

/* Find pointer to the next available space in flash to store registry key/value */
static uint8_t* flash_buf_get()
{
  uint8_t* flash_ptr;
  az_ulib_registry_node node;

  /* Is there an empty registry? */
  node = *(az_ulib_registry_node*)(&__REGISTRYINFO);
  if (node._internal.status.ready_flag == REGISTRY_FREE)
  {
    flash_ptr = (uint8_t*)(&__REGISTRY);
  }
  else
  {
    uint8_t* max_address = (uint8_t*)(&__REGISTRY); /* The beginning of buffer space */
    az_ulib_registry_node* last_node;
    az_ulib_registry_node* runner;

    /* Find occupied node that has maximum address such that a new key-value pair can be stored */
    runner = (az_ulib_registry_node*)(uint8_t*)(&__REGISTRYINFO);
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

    /* Last_node should have reference to the last node in available memory, with rounded address */
    int32_t value_buf_size;
    value_buf_size = ((((az_span_size(last_node->_internal.data.value) - 1) >> 3) + 1) << 3);

    /* Calculate final flash_ptr */
    flash_ptr = az_span_ptr(last_node->_internal.data.value) + value_buf_size;
  }

  return flash_ptr;
}

AZ_NODISCARD az_result az_ulib_registry_init(void)
{
  /* Initialize lock */
  az_pal_os_lock_init(&registry_lock);

  return AZ_OK;
}

AZ_NODISCARD az_result az_ulib_registry_deinit(void)
{
  /* Deinitialize lock */
  az_pal_os_lock_deinit(&registry_lock);

  return AZ_OK;
}

AZ_NODISCARD az_result az_ulib_registry_delete(az_span key)
{
  /* Precondition check */
  _az_PRECONDITION_VALID_SPAN(key, 1, false);

  az_result result;
  az_ulib_registry_node* runner;

  /* Loop through registry for entry that matches the key */
  runner = (az_ulib_registry_node*)(&__REGISTRYINFO);
  for (int i = 0; i < MAX_AZ_ULIB_REGISTRY_ENTRIES; i++)
  {
    if (runner->_internal.status.ready_flag == REGISTRY_READY)
    {
      if (az_span_is_content_equal(key, runner->_internal.data.key))
      {
        set_registry_node_delete_flag((uint8_t*)runner);
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
  /* Precondition check */
  _az_PRECONDITION_VALID_SPAN(key, 1, false);

  az_result result;
  az_ulib_registry_node* runner;

  /* Find node containing key */
  runner = (az_ulib_registry_node*)(&__REGISTRYINFO);
  for (int i = 0; i < MAX_AZ_ULIB_REGISTRY_ENTRIES; i++)
  {
    if(runner->_internal.status.delete_flag != REGISTRY_DELETED)
    {
      /* If the content is not deleted and ready for use */
      if (runner->_internal.status.ready_flag == REGISTRY_READY)
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
    }
    runner++;
  }

  return result;
}

AZ_NODISCARD az_result az_ulib_registry_add(az_span key, az_span value)
{
  /* Precondition check */
  _az_PRECONDITION_VALID_SPAN(key, 1, false);
  _az_PRECONDITION_VALID_SPAN(value, 1, false);

  AZ_ULIB_TRY
  {
    az_span sample_value;
    az_result result;
    az_ulib_registry_node new_node;
    uint8_t* new_node_ptr;
    uint8_t* key_dest_ptr;
    uint8_t* val_dest_ptr;

    az_pal_os_lock_acquire(&registry_lock);
    {
      /* Validate for duplicates before adding new entry */
      result = az_ulib_registry_get(key, &sample_value);
      AZ_ULIB_THROW_IF_ERROR((result == AZ_ERROR_ITEM_NOT_FOUND), AZ_ERROR_ULIB_ELEMENT_DUPLICATE);

      /* Find destination in flash buffer */
      key_dest_ptr = flash_buf_get();
      val_dest_ptr
          = (uint8_t*)((int32_t)key_dest_ptr + ((((az_span_size(key) - 1) >> 3) + 1) << 3));

      /* Handle out of space scenario */
      AZ_ULIB_THROW_IF_ERROR(
          (key_dest_ptr < ((uint8_t*)(&__REGISTRY) + MAX_AZ_ULIB_REGISTRY_BUFFER)),
          AZ_ERROR_OUT_OF_MEMORY);
      AZ_ULIB_THROW_IF_ERROR(
          (val_dest_ptr < ((uint8_t*)(&__REGISTRY) + MAX_AZ_ULIB_REGISTRY_BUFFER)),
          AZ_ERROR_OUT_OF_MEMORY);

      /* Set free node information */
      new_node._internal.data.key = az_span_create(key_dest_ptr, az_span_size(key));
      new_node._internal.data.value = az_span_create(val_dest_ptr, az_span_size(value));

      /* Update registry node in flash */
      new_node_ptr = NULL;
      AZ_ULIB_THROW_IF_AZ_ERROR(store_registry_node(new_node, &new_node_ptr));

      /* Write key to flash */
      AZ_ULIB_THROW_IF_AZ_ERROR(result_from_hal_status(
          internal_flash_write(key_dest_ptr, az_span_ptr(key), az_span_size(key))));

      /* Write value to flash */
      AZ_ULIB_THROW_IF_AZ_ERROR(result_from_hal_status(
          internal_flash_write(val_dest_ptr, az_span_ptr(value), az_span_size(value))));

      /* After successful storage of registry node and actual key value pair, set flag in node to
      indicate the entry is now ready to use  */
      AZ_ULIB_THROW_IF_AZ_ERROR(set_registry_node_ready_flag(new_node_ptr));
    }
    az_pal_os_lock_release(&registry_lock);

    AZ_ULIB_THROW_IF_AZ_ERROR(result);
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}
