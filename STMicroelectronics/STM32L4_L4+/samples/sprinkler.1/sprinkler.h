// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#ifndef SPRINKLER_H
#define SPRINKLER_H

#include "az_ulib_result.h"
#include "azure/az_core.h"

#ifdef __cplusplus
#include <cstdint>
extern "C"
{
#else
#include <stdint.h>
#endif

#define SPRINKLER_1_PACKAGE_NAME "sprinkler"
#define SPRINKLER_1_PACKAGE_VERSION 1

  az_result sprinkler_v1i1_water_now(int32_t zone, int32_t timer);

  az_result sprinkler_v1i1_stop(int32_t zone);

  az_result sprinkler_v1i1_end(void);

#ifdef __cplusplus
}
#endif

#endif /* SPRINKLER_H */
