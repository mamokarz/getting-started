// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#ifndef SPRINKLER_V1I1_H
#define SPRINKLER_V1I1_H

#include "az_ulib_result.h"
#include "azure/az_core.h"

#ifdef __cplusplus
#include <cstdint>
extern "C"
{
#else
#include <stdint.h>
#endif

  az_result sprinkler_v1i1_create(void);
  az_result sprinkler_v1i1_destroy(void);

  az_result sprinkler_v1i1_water_now(int32_t area, int32_t timer);

  az_result sprinkler_v1i1_stop(int32_t area);

#ifdef __cplusplus
}
#endif

#endif /* SPRINKLER_V1I1_H */
