// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/********************************************************************
 * This code was auto-generated from packages v1 DL and shall not be
 * modified.
 ********************************************************************/

#ifndef PACKAGES_1_MODEL_H
#define PACKAGES_1_MODEL_H

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
#define PACKAGES_1_INTERFACE_NAME "packages"
#define PACKAGES_1_INTERFACE_VERSION 1

  typedef enum
  {
    PACKAGES_1_SOURCE_TYPE_IN_MEMORY = 0,
    PACKAGES_1_SOURCE_TYPE_BLOB = 1,
    PACKAGES_1_SOURCE_TYPE_CLI = 2
  } packages_1_source_type;

/*
 * Define install command on packages interface.
 */
#define PACKAGES_1_INSTALL_COMMAND (az_ulib_capability_index)0
#define PACKAGES_1_INSTALL_COMMAND_NAME "install"
#define PACKAGES_1_INSTALL_SOURCE_NAME "source_type"
#define PACKAGES_1_INSTALL_ADDRESS_NAME "address"
#define PACKAGES_1_INSTALL_PACKAGE_NAME_NAME "package_name"
  typedef struct
  {
    packages_1_source_type source_type;
    void* address;
    az_span package_name;
  } packages_1_install_model_in;

/*
 * Define uninstall command on packages interface.
 */
#define PACKAGES_1_UNINSTALL_COMMAND (az_ulib_capability_index)1
#define PACKAGES_1_UNINSTALL_COMMAND_NAME "uninstall"
#define PACKAGES_1_UNINSTALL_PACKAGE_NAME_NAME "package_name"
  typedef struct
  {
    az_span package_name;
  } packages_1_uninstall_model_in;

#ifdef __cplusplus
}
#endif

#endif /* PACKAGES_1_MODEL_H */
