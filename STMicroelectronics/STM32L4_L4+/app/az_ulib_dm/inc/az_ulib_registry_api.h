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
#include "registry_1_model.h"

#ifndef __cplusplus
#include <stdint.h>
#else
#include <cstdint>
#endif /* __cplusplus */

#include "azure/core/_az_cfg_prefix.h"

#define AZ_ULIB_CONFIG_MAX_REGISTRY_ENTRIES 10

// any needed internal data structure
typedef struct
{
  az_span key;
  az_span value;
} _az_ulib_registry_entry;

/**
 * @brief Registry Editor handle.
 */
typedef struct az_ulib_registry_tag
{
  struct
  {
    _az_ulib_registry_entry registry_entry[AZ_ULIB_CONFIG_MAX_REGISTRY_ENTRIES];
    uint8_t num_entries;
  } _internal;
} az_ulib_registry;

AZ_NODISCARD az_result az_ulib_registry_get_value(az_span key, az_span value);
AZ_NODISCARD az_result az_ulib_registry_add(az_span key, az_span value);

#include "azure/core/_az_cfg_suffix.h"

#endif /* AZ_ULIB_REGISTRY_API_H */
