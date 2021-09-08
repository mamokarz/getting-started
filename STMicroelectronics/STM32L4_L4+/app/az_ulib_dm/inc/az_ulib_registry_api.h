// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/**
 * @file
 *
 * @brief   The Registry Editor.
 *
 * The Registry Editor is capable of storing key value pairs into device flash.
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

/**
 * @brief Struct including two #az_span that each contains a pointer and a size for each key and
 * value to be stored in the flash. Used exclusively in az_ulib_registry_node structs.
 */
typedef struct
{
  /** The #az_span that contains a pointer to a location in flash storing the key char string and
   * the string's size.*/
  az_span key;

  /** The #az_span that contains pointer to a location in flash storing the value char string and
   * the string's size.*/
  az_span value;
} _az_ulib_registry_key_value_ptrs;

/**
 * @brief   Structure for the registry control node.
 *
 * Every entry into the registry of the device will have a registry control node that contains
 * relevant information to restore the state of the registry after device power cycle.
 *
 */
typedef struct
{
  struct
  {
    /** Two flags that shows the status of a node in the registry (ready, deleted). If both flags
     * are set, the node is deleted and cannot be used again. */
    uint64_t ready_flag;
    uint64_t delete_flag;
    _az_ulib_registry_key_value_ptrs key_value_ptrs;
  } _internal;
} az_ulib_registry_node;

/**
 * @brief   This function gets the #az_span value associated with the given #az_span key from the
 * registry.
 *
 * This function goes through the registry comparing keys until it finds one that matches the input.
 *
 * @param[in]   key                  The #az_span key to look for within the registry.
 * @param[out]   value                The #az_span value corresponding to the input key.
 *
 * @pre         \p key                            shall not be `#AZ_SPAN_EMPTY`.
 * @pre         \p value                          shall not be `NULL`.
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
 * @pre         \p value                          shall not be `#AZ_SPAN_EMPTY`.
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
 *
 * @return The #az_result with the result of the registry operations.
 *      @retval #AZ_OK                            If removing a value from the registry was
 *                                                successful.
 *      @retval #AZ_ERROR_ULIB_SYSTEM             If the `az_ulib_registry_delete` operation failed
 *                                                on the system level.
 *      @retval #AZ_ERROR_ULIB_BUSY               If the resources necesary for the
 *                                                `az_ulib_registry_delete` operation are busy.
 */
AZ_NODISCARD az_result az_ulib_registry_delete(az_span key);

/**
 * @brief   This function initializes the device registry.
 *
 * This function initializes components that the registry needs upon reboot. This function is not
 * thread safe and all other APIs shall only be invoked after the initialization ends.
 */
void az_ulib_registry_init();

/**
 * @brief   This function deinitializes the device registry.
 *
 * This function deinitializes components that the registry used. The registry can be reinitialized.
 * This function is not thread safe and all other APIs shall release the resource before calling 
 * the deinit() function. 
 */
void az_ulib_registry_deinit();

#include "azure/core/_az_cfg_suffix.h"

#endif /* AZ_ULIB_REGISTRY_API_H */
