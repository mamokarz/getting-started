// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "stm32l475xx.h"
#include "stm32l4xx_hal.h"

#include "az_ulib_result.h"
#include "sprinkler.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

az_result sprinkler_v1i1_water_now(int32_t area, int32_t timer)
{
  if ((timer != 0) || (area != 0))
  {
    return AZ_ERROR_NOT_SUPPORTED;
  }

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);

  return AZ_OK;
}

az_result sprinkler_v1i1_stop(int32_t area)
{
  if (area != 0)
  {
    return AZ_ERROR_NOT_SUPPORTED;
  }

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);

  return AZ_OK;
}

az_result sprinkler_v1i1_end(void) { return sprinkler_v1i1_stop(0); }
