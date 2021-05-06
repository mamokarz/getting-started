// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "stm32l475e_iot01.h"
#include "stm32l475e_iot01_tsensor.h"

#include "az_ulib_result.h"
#include "azure/az_core.h"
#include "sprinkler_v1i1.h"
#include "sprinkler_v1i1_interface.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

az_result sprinkler_v1i1_create(void)
{
  return sprinkler_v1i1_interface_publish();
}

az_result sprinkler_v1i1_destroy(void)
{
  return sprinkler_v1i1_interface_unpublish();
}

az_result sprinkler_v1i1_water_now(int32_t area, int32_t timer)
{
  if((timer != 0) || (area != 0))
  {
    return AZ_ERROR_NOT_SUPPORTED;
  }

  BSP_LED_On(LED_GREEN);

  return AZ_OK;
}

az_result sprinkler_v1i1_stop(int32_t area)
{
  if(area != 0)
  {
    return AZ_ERROR_NOT_SUPPORTED;
  }

  BSP_LED_Off(LED_GREEN);

  return AZ_OK;
}
