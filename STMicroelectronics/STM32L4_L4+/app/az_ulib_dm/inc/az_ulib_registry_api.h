// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/**
 * @file
 *
 * @brief   The Registry Editor.
 *
 * The Registry Editor is capable of storing key value pairs and stored into flash.
 */

#ifndef AZ_ULIB_REGISTRY_API_H
#define AZ_ULIB_REGISTRY_API_H

#include "az_ulib_result.h"
#include "azure/az_core.h"

#ifndef __cplusplus
#include <stdint.h>
#else
#include <cstdint>
#endif /* __cplusplus */

#include "azure/core/_az_cfg_prefix.h"

extern uint32_t __SIZEOF_REGISTRYINFO;
extern uint32_t __SIZEOF_REGISTRY;

/* Each az_ulib_registry_node is 32 bytes. RegistryInfo is a 2k section in FLASH */
#define AZ_ULIB_REGISTRY_NODE_SIZE 32
#define MAX_AZ_ULIB_REGISTRY_ENTRIES ((uint32_t)(&__SIZEOF_REGISTRYINFO))/AZ_ULIB_REGISTRY_NODE_SIZE
#define MAX_AZ_ULIB_REGISTRY_BUFFER (uint32_t)(&__SIZEOF_REGISTRY)
#define AZ_ULIB_REGISTRY_FLAG_SIZE 8 // in bytes

#define REGISTRY_FREE 0xFFFFFFFFFFFFFFFF
#define REGISTRY_READY 0x0000000000000000
#define REGISTRY_DELETED 0x0000000000000000

/**
 * @brief   Structure for the registry node status.
 *
 * Every entry into the registry will have two flags that will indicate the status of the node.
 * Status can be free, ready to use, and deleted.
 *
 */
typedef struct
{
  uint64_t ready_flag;
  uint64_t delete_flag;
} az_ulib_registry_node_status;

/**
 * @brief   Structure for the registry data containing key value pair of #az_span pointing to
 *          location of data in flash.
 */
typedef struct
{
  /** The #az_span that contains a pointer to a location in flash storing the key char string and
   * the string's size.*/
  az_span key;

  /** The #az_span that contains pointer to a location in flash storing the value char string and
   * the string's size.*/
  az_span value;
} az_ulib_registry_node_data;

/**
 * @brief   Structure for the registry control block
 *
 * Every entry into the registry of the device will have a registry control block that contains
 * relevant information to restore the state of the registry after device power cycle.
 *
 */
typedef struct
{
  struct
  {
    /** Flags that shows the status of the registry (free or deleted).*/
    az_ulib_registry_node_status status;

    /** Struct that contains #az_span pair of key value pairs */
    az_ulib_registry_node_data data;
  } _internal;
} az_ulib_registry_node;

/**
 * @brief   This function gets the #az_span value associated with the given #az_span key from the
 * registry.
 *
 * This function goes through the registry comparing keys until it finds one that matches the input.
 *
 * @param[in]   key                  The #az_span key to look for within the registry.
 * @param[in]   value                The #az_span value corresponding to the input key.
 *
 * @pre         \p key                            shall not be `#AZ_SPAN_EMPTY`.
 * @pre         \p sizeof(key)                    shall greater than zero.
 *
 * @return The #az_result with the result of the registry operations.
 *      @retval #AZ_OK                        If retrieving a value from the registry was
 *                                            successful.
 *      @retval #AZ_ERROR_ITEM_NOT_FOUND      If there are no values that correspond to the
 *                                            given key within the registry.
 */
AZ_NODISCARD az_result az_ulib_registry_try_get_value(az_span key, az_span* value);

/**
 * @brief   This function adds an #az_span key and an #az_span value into the device registry.
 *
 * This function goes through the registry ensuring there are no duplicate elements before
 * adding a new key value pair to the registry.
 *
 * @param[in]   key                  The #az_span key to add to the registry.
 * @param[in]   value                The #az_span value to add to the registry.
 *
 * @pre         \p key                            shall not be `#AZ_SPAN_EMPTY`.
 * @pre         \p sizeof(key)                    shall greater than zero.
 * @pre         \p value                          shall not be `#AZ_SPAN_EMPTY`.
 * @pre         \p sizeof(value)                  shall greater than zero.
 *
 * @return The #az_result with the result of the registry operations.
 *      @retval #AZ_OK                            If adding a value to the registry was successful.
 *      @retval #AZ_ERROR_ULIB_ELEMENT_DUPLICATE  If there is a key within the registry that is the
 *                                                same as the new key.
 *      @retval #AZ_ERROR_ULIB_SYSTEM             If the `az_ulib_registry_add` operation failed on
 *                                                the system level.
 *      @retval #AZ_ERROR_ULIB_BUSY               If the resources necesary for the
 *                                                `az_ulib_registry_add` operation are busy.
 *      @retval #AZ_ERROR_OUT_OF_MEMORY           If the flash space for `az_ulib_registry_add`
 *                                                is not enough for a new registry entry.
 */
AZ_NODISCARD az_result az_ulib_registry_add(az_span key, az_span value);

/**
 * @brief   This function removes an #az_span key and its corresponding #az_span value from the
 * device registry.
 *
 * This function goes through the registry and marks the node with the given key value pair as
 * deleted so that it can no longer be accessible.
 *
 * @param[in]   key                  The #az_span key to remove from the registry.
 *
 * @pre         \p key                            shall not be `#AZ_SPAN_EMPTY`.
 * @pre         \p sizeof(key)                    shall greater than zero.
 *
 * @return The #az_result with the result of the registry operations.
 *      @retval #AZ_OK                            If removing a value to the registry was
 * successful.
 *      @retval #AZ_ERROR_ULIB_SYSTEM             If the `az_ulib_registry_delete` operation failed
 *                                                on the system level.
 *      @retval #AZ_ERROR_ULIB_BUSY               If the resources necesary for the
 *                                                `az_ulib_registry_delete` operation are busy.
 */
AZ_NODISCARD az_result az_ulib_registry_delete(az_span key);

/**
 * @brief   This function initializes the device registry.
 *
 * This function initializes components that the registry needs upon reboot.
 */
void az_ulib_registry_init();

/**
 * @brief   This function deinitializes the device registry.
 *
 * This function deinitializes components that the registry used. The registry can be reinitialized.
 */
void az_ulib_registry_deinit();

#include "azure/core/_az_cfg_suffix.h"

#endif /* AZ_ULIB_REGISTRY_API_H */
 