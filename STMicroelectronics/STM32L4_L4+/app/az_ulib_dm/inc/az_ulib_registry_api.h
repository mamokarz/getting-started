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

#define AZ_ULIB_CONFIG_MAX_REGISTRY_ENTRIES 10

typedef enum
{
  REGISTRY_USED = 0x00,
  REGISTRY_FREE = 0xFF
} az_registry_entry_type;

/**
 * @brief   Structure for the registry control block
 *
 * Every entry into the registry of the device will have a registry control block that contains
 * relevant information to restore the state of the registry after device power cycle.
 *
 */

typedef struct
{
  /** The enum that declares whether a registry slot is occupied or not.*/
  az_registry_entry_type used_flag;

  /** The az_span that contains pointer to a location in flash storing the key char string and the
   * string's size.*/
  az_span key;

  /** The az_span that contains pointer to a location in flash storing the valye char string and the
   * string's size.*/
  az_span value;
} az_registry_block;

/**
 * @brief   This function gets the az_span value associated with the given az_span key from the
 * registry.
 *
 * This function goes through the registry comparing keys until it finds one that matches with the
 * input.
 *
 * @param[in]   key                  The az_span key to look for within the registry.
 * @param[in]   value                The az_span value corresponding to the input key
 *
 * @pre         \p key                            shall not be `NULL`.
 * @pre         \p sizeof(key)                    shall greater than zero.
 *
 * @return The #az_result with the result of the registry operations.
 *      @retval #AZ_OK                        If retrieving a value from the registry was
 *                                            successful
 *      @retval #AZ_ERROR_ITEM_NOT_FOUND      If there are no values that correspond to the
 *                                            given key within the registry. *
 *      @retval #AZ_ERROR_ARG                 If the parameters passed into`az_ulib_registry_get`
 *                                            violate requirements for this function.
 */
AZ_NODISCARD az_result az_ulib_registry_get(az_span key, az_span* value);

/**
 * @brief   This function adds an az_span key and an az_span value into the device registry.
 *
 * This function goes through the registry ensuring there are no duplicate elements before
 * adding a new key value pair to the registry.
 *
 * @param[in]   key                  The az_span key to add to the registry.
 * @param[in]   value                The az_span value to add to the registry.
 *
 * @pre         \p key                            shall not be `NULL`.
 * @pre         \p sizeof(key)                    shall greater than zero.
 * @pre         \p value                          shall not be `NULL`.
 * @pre         \p sizeof(value)                  shall greater than zero.
 *
 * @return The #az_result with the result of the registry operations.
 *      @retval #AZ_OK                            If adding a value to the registry was successful
 *      @retval #AZ_ERROR_ULIB_ELEMENT_DUPLICATE  If there is a key within the registry that is the
 *                                                same as the new key.
 *      @retval #AZ_ERROR_ULIB_SYSTEM             If the `az_ulib_registry_add` operation failed on
 *                                                the system level.
 *      @retval #AZ_ERROR_ULIB_BUSY               If the resources necesary for the
 *                                                `az_ulib_registry_add` operation are busy.
 *      @retval #AZ_ERROR_ARG                     If the parameters passed into
 *                                                `az_ulib_registry_add` violate requirements for
 *                                                this function.
 */
AZ_NODISCARD az_result az_ulib_registry_add(az_span key, az_span value);

AZ_NODISCARD az_result az_ulib_registry_delete(az_span key);

/**
 * @brief   This function initializes the device registry.
 *
 * This function restores the state of the registry after reboot and repopulates the registry editor
 * with registry control blocks.
 *
 * @return The #az_result with the result of the registry operations.
 *      @retval #AZ_OK                            If initializing the registry was successful. 
 */
AZ_NODISCARD az_result az_ulib_registry_init();

#include "azure/core/_az_cfg_suffix.h"

#endif /* AZ_ULIB_REGISTRY_API_H */
