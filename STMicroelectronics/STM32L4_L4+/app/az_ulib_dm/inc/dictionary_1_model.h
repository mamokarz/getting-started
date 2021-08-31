// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/********************************************************************
 * This code was auto-generated from dictionary.1 DL and shall not be
 * modified.
 ********************************************************************/

#ifndef DICTIONARY_1_MODEL_H
#define DICTIONARY_1_MODEL_H

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
#define DICTIONARY_1_INTERFACE_NAME "dictionary"
#define DICTIONARY_1_INTERFACE_VERSION 1

/*
 * Define try get value command in the dictionary interface.
 */
#define DICTIONARY_1_TRY_GET_VALUE_COMMAND (az_ulib_capability_index)0
#define DICTIONARY_1_TRY_GET_VALUE_COMMAND_NAME "try_get_value"
#define DICTIONARY_1_TRY_GET_VALUE_KEY_NAME "key"
#define DICTIONARY_1_TRY_GET_VALUE_VALUE_NAME "value"
  typedef az_span dictionary_1_try_get_value_model_in;
  typedef struct
  {
    az_span* value;
  } dictionary_1_try_get_value_model_out;

/*
 * Define add command in the dictionary interface.
 */
#define DICTIONARY_1_ADD_COMMAND (az_ulib_capability_index)1
#define DICTIONARY_1_ADD_COMMAND_NAME "add"
#define DICTIONARY_1_ADD_KEY_NAME "key"
#define DICTIONARY_1_ADD_VALUE_NAME "value"
  typedef struct
  {
    az_span key;
    az_span value;
  } dictionary_1_add_model_in;

/*
 * Define delete with key command on dictionary interface.
 */
#define DICTIONARY_1_DELETE_COMMAND (az_ulib_capability_index)2
#define DICTIONARY_1_DELETE_COMMAND_NAME "delete"
#define DICTIONARY_1_DELETE_KEY_NAME "key"
  typedef az_span dictionary_1_delete_model_in;

#ifdef __cplusplus
}
#endif

#endif /* DICTIONARY_1_MODEL_H */

