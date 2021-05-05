// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/********************************************************************
 * This code was auto-generated from dm v1 DL and shall not be
 * modified.
 ********************************************************************/

#ifndef DM_1_MODEL_H
#define DM_1_MODEL_H

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
#define DM_1_INTERFACE_NAME "dm"
#define DM_1_INTERFACE_VERSION 1
#define DM_1_CAPABILITY_SIZE 2

typedef enum 
{ 
  DM_1_SOURCE_TYPE_IN_MEMORY = 0, 
  DM_1_SOURCE_TYPE_BLOB = 1, 
  DM_1_SOURCE_TYPE_CLI = 2,
  DM_1_SOURCE_TYPE_BUILT_IN = 3
} dm_1_source_type;

/*
 * Define install command on dm interface.
 */
#define DM_1_INSTALL_COMMAND (az_ulib_capability_index)0
#define DM_1_INSTALL_COMMAND_NAME "install"
#define DM_1_INSTALL_SOURCE_NAME "source_type"
#define DM_1_INSTALL_ADDRESS_NAME "address"
#define DM_1_INSTALL_PACKAGE_NAME_NAME "package_name"
  typedef struct
  {
    dm_1_source_type source_type;
    void* address;
    az_span package_name;
  } dm_1_install_model_in;

/*
 * Define uninstall command on dm interface.
 */
#define DM_1_UNINSTALL_COMMAND (az_ulib_capability_index)1
#define DM_1_UNINSTALL_COMMAND_NAME "uninstall"
#define DM_1_UNINSTALL_PACKAGE_NAME_NAME "package_name"
  typedef struct
  {
    az_span package_name;
  } dm_1_uninstall_model_in;

#ifdef __cplusplus
}
#endif

#endif /* DM_1_MODEL_H */
