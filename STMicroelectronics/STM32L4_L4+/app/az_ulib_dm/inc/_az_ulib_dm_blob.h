// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#ifndef AZ_ULIB_DM_BLOB_H
#define AZ_ULIB_DM_BLOB_H

#include "az_ulib_result.h"
#include "azure/az_core.h"
#include "stm32l475_flash_driver.h"


#ifndef __cplusplus
#include <stdint.h>
#else
#include <cstdint>
#endif /* __cplusplus */

#include "azure/core/_az_cfg_prefix.h"

AZ_NODISCARD az_result _az_ulib_dm_blob_get_package_name(az_span url, az_span* name);

AZ_NODISCARD az_result _az_ulib_dm_blob_get_size(az_span url, int32_t* size);

AZ_NODISCARD az_result _az_ulib_dm_blob_download(void* address, az_span url);

#include "azure/core/_az_cfg_suffix.h"

#endif /* AZ_ULIB_DM_API_H */
