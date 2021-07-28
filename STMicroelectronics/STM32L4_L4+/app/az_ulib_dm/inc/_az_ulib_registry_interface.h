// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/********************************************************************
 * This code was auto-generated from az_registry DL and shall not be
 * modified.
 ********************************************************************/

#ifndef _az_ULIB_REGISTRY_INTERFACE_H
#define _az_ULIB_REGISTRY_INTERFACE_H

#include "az_ulib_result.h"

#ifdef __cplusplus
extern "C"
{
#else
#endif

#define REGISTRY_1_PACKAGE_NAME "registry"
#define REGISTRY_1_PACKAGE_VERSION 1

  /*
   * Publish Registry Editor interface.
   */
  az_result _az_ulib_registry_interface_publish(void);

  /*
   * Unpublish Registry Editor interface.
   */
  az_result _az_ulib_registry_interface_unpublish(void);

#ifdef __cplusplus
}
#endif

#endif /* _az_ULIB_REGISTRY_INTERFACE_H */

