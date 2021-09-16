// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/********************************************************************
 * This code was auto-generated from az_dm DL and shall not be
 * modified.
 ********************************************************************/

#ifndef _az_ULIB_DM_INTERFACE_H
#define _az_ULIB_DM_INTERFACE_H

#include "az_ulib_result.h"

#ifdef __cplusplus
extern "C"
{
#else
#endif

#define DM_1_PACKAGE_NAME "dm"
#define DM_1_PACKAGE_VERSION 1

  /*
   * Publish Device Manager interface.
   */
  az_result _az_ulib_dm_interface_publish(void);

  /*
   * Unpublish Device Manager interface.
   */
  az_result _az_ulib_dm_interface_unpublish(void);

#ifdef __cplusplus
}
#endif

#endif /* _az_ULIB_DM_INTERFACE_H */
