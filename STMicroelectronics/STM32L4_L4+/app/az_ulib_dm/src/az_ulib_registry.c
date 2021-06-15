// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "_az_ulib_registry_interface.h"
#include "az_ulib_registry_api.h"
#include "az_ulib_ipc_api.h"
#include "az_ulib_ipc_interface.h"
#include "az_ulib_result.h"
#include "azure/az_core.h"
#include "registry_1_model.h"
#include <azure/core/internal/az_precondition_internal.h>
#include <string.h>

// reference location from linker to save registry info

// static functions

// verify_space in registry
// get all keys
// store num entries

AZ_NODISCARD az_result az_ulib_registry_get_value(az_span key, az_span value);

AZ_NODISCARD az_result az_ulib_registry_add(az_span key, az_span value);