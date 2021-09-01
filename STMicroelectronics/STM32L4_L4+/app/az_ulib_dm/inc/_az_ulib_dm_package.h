// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#ifndef _AZ_ULIB_DM_PACKAGE_H
#define _AZ_ULIB_DM_PACKAGE_H

#include "az_ulib_ipc_interface.h"
#include "az_ulib_result.h"
#include "azure/az_core.h"

#ifndef __cplusplus
#include <stdint.h>
#else
#include <cstdint>
#endif /* __cplusplus */

#include "azure/core/_az_cfg_prefix.h"

/**
 * @brief Package shell entry point expected signature.
 *
 * @param[in]   code    The pointer to the code.
 */
typedef void (*_az_ulib_dm_package_shell_entry_point)(void* code);

/**
 * @brief Publish interface expected signature.
 *
 * @param[in]   table   The pointer to #az_ulib_ipc_table with the IPC functions.
 *
 * @return The #az_result with the publish result.
 *  @retval #AZ_OK                  If the interface was published with success.
 *  @retval non-AZ_OK               If there was any issue publishing the interface.
 */
typedef az_result (*_az_ulib_dm_package_publish_interface)(const az_ulib_ipc_table* const table);

/**
 * @brief Unpublish interface expected signature.
 *
 * @param[in]   table   The pointer to #az_ulib_ipc_table with the IPC functions.
 *
 * @return The #az_result with the publish result.
 *  @retval #AZ_OK                  If the interface was unpublished with success.
 *  @retval non-AZ_OK               If there was any issue unpublishing the interface.
 */
typedef az_result (*_az_ulib_dm_package_unpublish_interface)(const az_ulib_ipc_table* const table);

/** Define the Module ID, which is used to indicate a module is valid.  */
#define _AZ_ULIB_DM_PACKAGE_ID 0x4D4F4455

/*
 * Define the overlay for the module's preamble.
 */

/** Module ID. It shall be _AZ_ULIB_DM_PACKAGE_ID (0x4D4F4455). */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_ID 0

/** Major Version ID.*/
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_VERSION_MAJOR 1

/** Minor Version ID. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_VERSION_MINOR 2

/** Module Preamble Size, in 32-bit words. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_PREAMBLE_SIZE 3

/** Module ID (application defined). */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_APPLICATION_MODULE_ID 4

/** Properties Bit Map. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_PROPERTY_FLAGS 5

/** Module shell Entry Function. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_SHELL_ENTRY_POINT 6

/** Module Thread Start Function. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_START_FUNCTION 7

/** Module Thread Stop Function. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_STOP_FUNCTION 8

/** Module Start/Stop Thread Priority. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_START_STOP_PRIORITY 9

/** Module Start/Stop Thread Priority. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_START_STOP_STACK_SIZE 10

/** Module Callback Thread Function. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_CALLBACK_FUNCTION 11

/** Module Callback Thread Priority. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_CALLBACK_PRIORITY 12

/** Module Callback Thread Stack Size. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_CALLBACK_STACK_SIZE 13

/** Module Instruction Area Size. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_CODE_SIZE 14

/** Module Data Area Size. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_DATA_SIZE 15

/** Reserved. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_PUBLISH 16

/** Reserved. */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_UNPUBLISH 17

/** Module Instruction Area Checksum [Optional].  */
#define _AZ_ULIB_DM_PACKAGE_PREAMBLE_CHECKSUM 31

/*
 * End of preamble.
 */

/** Maximum number of chars that can compose the package name. */
#define _AZ_ULIB_CONFIG_MAX_DM_PACKAGE_NAME 32

/** Maximum number of bytes (uint8_t) that can compose the package RAM. */
#define _AZ_ULIB_DM_PACKAGE_MAX_DATA_SIZE 0x100

typedef struct
{
  /** Copy of the package name. */
  az_span name;

  /** Buffer to store the package name. */
  uint8_t name_buf[_AZ_ULIB_CONFIG_MAX_DM_PACKAGE_NAME];

  /** Pre-allocated memory to be used as the package RAM. */
  uint8_t data[_AZ_ULIB_DM_PACKAGE_MAX_DATA_SIZE];

  /** Pointer to the code in the Flash. */
  void* address;
} _az_ulib_dm_package;

#include "azure/core/_az_cfg_suffix.h"

#endif /* _AZ_ULIB_DM_PACKAGE_H */
