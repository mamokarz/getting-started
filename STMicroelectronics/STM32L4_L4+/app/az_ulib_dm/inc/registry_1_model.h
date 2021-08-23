// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/********************************************************************
 * This code was auto-generated from registry.1 DL and shall not be
 * modified.
 ********************************************************************/

#ifndef REGISTRY_1_MODEL_H
#define REGISTRY_1_MODEL_H

//#include "az_ulib_ipc_api.h"
#include "az_ulib_capability_api.h"
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
 * Interface definition
 */
#define REGISTRY_1_INTERFACE_NAME "registry"
#define REGISTRY_1_INTERFACE_VERSION 1

/*
 * Define get with key command on registry editor interface.
 */
#define REGISTRY_1_TRY_GET_VALUE_COMMAND (az_ulib_capability_index)0
#define REGISTRY_1_TRY_GET_VALUE_COMMAND_NAME "try_get_value"
#define REGISTRY_1_TRY_GET_VALUE_REGISTRY_KEY_NAME "key"
#define REGISTRY_1_TRY_GET_VALUE_REGISTRY_VALUE_NAME "value"
  typedef az_span registry_1_try_get_value_model_in;
  typedef struct
  {
    az_span* value;
  } registry_1_try_get_value_model_out;

/*
 * Define add command on registry editor interface.
 */
#define REGISTRY_1_ADD_COMMAND (az_ulib_capability_index)1
#define REGISTRY_1_ADD_COMMAND_NAME "add"
#define REGISTRY_1_ADD_REGISTRY_KEY_NAME "key"
#define REGISTRY_1_ADD_REGISTRY_VALUE_NAME "value"
  typedef struct
  {
    az_span key;
    az_span value;
  } registry_1_add_model_in;

/*
 * Define delete with key command on registry editor interface.
 */
#define REGISTRY_1_DELETE_COMMAND (az_ulib_capability_index)2
#define REGISTRY_1_DELETE_COMMAND_NAME "delete"
#define REGISTRY_1_DELETE_REGISTRY_KEY_NAME "key"
  typedef az_span registry_1_delete_model_in;

#ifdef __cplusplus
}
#endif

#endif /* REGISTRY_1_MODEL_H */

