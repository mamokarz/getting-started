// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/********************************************************************
 * This code was auto-generated from key_vault.1 DL and shall not be
 * modified.
 ********************************************************************/

#ifndef KEY_VAULT_1_MODEL_H
#define KEY_VAULT_1_MODEL_H

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
 * interface definition
 */
#define KEY_VAULT_1_INTERFACE_NAME "key_vault"
#define KEY_VAULT_1_INTERFACE_VERSION 1

/*
 * Define encrypt command on key_vault interface.
 */
#define KEY_VAULT_1_ENCRYPT_COMMAND (az_ulib_capability_index)0
#define KEY_VAULT_1_ENCRYPT_COMMAND_NAME "encrypt"
#define KEY_VAULT_1_ENCRYPT_ALGORITHM_NAME "algorithm"
#define KEY_VAULT_1_ENCRYPT_SRC_NAME "src"
#define KEY_VAULT_1_ENCRYPT_DEST_NAME "dest"
  typedef struct
  {
    uint32_t algorithm;
    az_span src;
  } key_vault_1_encrypt_model_in;
  typedef struct
  {
    az_span* dest;
  } key_vault_1_encrypt_model_out;

/*
 * Define decrypt command on key_vault interface.
 */
#define KEY_VAULT_1_DECRYPT_COMMAND (az_ulib_capability_index)1
#define KEY_VAULT_1_DECRYPT_COMMAND_NAME "decrypt"
#define KEY_VAULT_1_DECRYPT_SRC_NAME "src"
#define KEY_VAULT_1_DECRYPT_DEST_NAME "dest"
  typedef struct
  {
    az_span src;
  } key_vault_1_decrypt_model_in;
  typedef struct
  {
    az_span* dest;
  } key_vault_1_decrypt_model_out;

#ifdef __cplusplus
}
#endif

#endif /* KEY_VAULT_1_MODEL_H */
