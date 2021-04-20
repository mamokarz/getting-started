// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/********************************************************************
 * This code was auto-generated from sprinkler v1 DL and shall not be
 * modified.
 ********************************************************************/

#ifndef SPRINKLER_1_INTERFACE_H
#define SPRINKLER_1_INTERFACE_H

#include "az_ulib_ipc_api.h"
#include "az_ulib_result.h"
#include "azure/az_core.h"

#ifdef __cplusplus
#include <cstdint>
extern "C"
{
#else
#include <stdint.h>
#endif

/*
 * interface definition
 */
#define SPRINKLER_1_INTERFACE_NAME "sprinkler"
#define SPRINKLER_1_INTERFACE_VERSION 1
#define SPRINKLER_1_CAPABILITY_SIZE 2

/*
 * Define water_now command on sprinkler interface.
 */
#define SPRINKLER_1_WATER_NOW_COMMAND (az_ulib_capability_index)0
#define SPRINKLER_1_WATER_NOW_COMMAND_NAME "water_now"
#define SPRINKLER_1_TIMER_NAME "timer"
  typedef struct
  {
    int32_t timer;
  } sprinkler_1_water_now_model_in;

/*
 * Define stop command on sprinkler interface.
 */
#define SPRINKLER_1_STOP_COMMAND (az_ulib_capability_index)1
#define SPRINKLER_1_STOP_COMMAND_NAME "stop"

#ifdef __cplusplus
}
#endif

#endif /* SPRINKLER_1_INTERFACE_H */
