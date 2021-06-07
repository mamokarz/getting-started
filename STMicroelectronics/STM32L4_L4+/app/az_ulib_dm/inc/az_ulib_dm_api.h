// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/**
 * @file
 *
 * @brief   The Device Manager.
 *
 * The DM is the component responsible to manage the resources in the device.
 */

#ifndef AZ_ULIB_DM_API_H
#define AZ_ULIB_DM_API_H

#include "az_ulib_pal_os_api.h"
#include "az_ulib_port.h"
#include "az_ulib_result.h"
#include "azure/az_core.h"
#include "dm_1_model.h"

#ifndef __cplusplus
#include <stdint.h>
#else
#include <cstdint>
#endif /* __cplusplus */

#include "azure/core/_az_cfg_prefix.h"

#define AZ_ULIB_CONFIG_MAX_DM_PACKAGES 5
#define AZ_ULIB_CONFIG_MAX_DM_PACKAGE_NAME 32

#define AZ_ULIB_DM_PACKAGE_PUBLISH 16
#define AZ_ULIB_DM_PACKAGE_UNPUBLISH 17

typedef struct
{
  az_span name;
  uint8_t name_buf[AZ_ULIB_CONFIG_MAX_DM_PACKAGE_NAME];
  void* address;
} _az_ulib_dm_package;

/**
 * @brief DM handle.
 */
typedef struct az_ulib_dm_tag
{
  struct
  {
    az_ulib_pal_os_lock lock;
    _az_ulib_dm_package package_list[AZ_ULIB_CONFIG_MAX_DM_PACKAGES];
  } _internal;
} az_ulib_dm;

/**
 * @brief   Initialize the Device Manager.
 *
 * This API initialize the DM [Device Manager]. It shall be called only once, at the beginning of
 * the code execution.
 *
 * @note    This API **is not** thread safe, the other DM API shall only be called after the
 *          initialization process is completely done.
 *
 * @param[in]   dm_handle       The #az_ulib_dm* that points to a memory position where the DM
 *                              shall create its control block.
 *
 * @pre     \p dm_handle shall be different than `NULL`.
 * @pre     DM shall not been initialized.
 *
 * @return The #az_result with the result of the initialization.
 *  @retval #AZ_OK                              If the DM initialize with success.
 */
AZ_NODISCARD az_result az_ulib_dm_init(az_ulib_dm* dm_handle);

/**
 * @brief   De-initialize the Device Manager.
 *
 * This API will release all resources associated with the DM, but only if the DM is completely
 * free.
 *
 * If the system needs the DM again, it may call az_ulib_dm_init() again to reinitialize it.
 *
 * @note    This API **is not** thread safe, no other DM API may be called during the execution of
 *          this deinit, and no other DM API shall be running during the execution of this API.
 *
 * @pre     DM shall already been initialized.
 *
 * @return The #az_result with the result of the de-initialization.
 *  @retval #AZ_OK                              If the DM de-initialize with success.
 *  @retval #AZ_ERROR_ULIB_BUSY                 If the DM is not completely free.
 */
AZ_NODISCARD az_result az_ulib_dm_deinit(void);

/**
 * @brief   Install a new package in the device.
 *
 * @param[in]   source_type     The #dm_1_source_type with the package source type.
 * @param[in]   address         The `void*` with the memory where package shall be.
 * @param[in]   package_name    The `az_span` with the package name.
 *
 * @pre     DM shall already been initialized.
 *
 * @return The #az_result with the result of the installation.
 *  @retval #AZ_OK                              If the package was installed with success.
 *  @retval #AZ_ERROR_ITEM_NOT_FOUND            If the DM could not find the package.
 *  @retval #AZ_ERROR_ULIB_ELEMENT_DUPLICATE    If the package is already installed.
 *  @retval #AZ_ERROR_NOT_ENOUGH_SPACE          If there is not enough space to handle the package.
 */
AZ_NODISCARD az_result
az_ulib_dm_install(dm_1_source_type source_type, void* address, az_span package_name);

/**
 * @brief   Uninstall a package from the device.
 *
 * @param[in]   package_name    The `az_span` with the package name.
 *
 * @pre     DM shall already been initialized.
 *
 * @return The #az_result with the result of the installation.
 *  @retval #AZ_OK                              If the package was uninstalled with success.
 *  @retval #AZ_ERROR_ITEM_NOT_FOUND            If the DM could not find the package.
 *  @retval #AZ_ERROR_ULIB_BUSY                 If the interface is busy and cannot be unpublished.
 */
AZ_NODISCARD az_result az_ulib_dm_uninstall(az_span package_name);

#include "azure/core/_az_cfg_suffix.h"

#endif /* AZ_ULIB_DM_API_H */
