// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "_az_ulib_registry_interface.h"
#include "az_ulib_capability_api.h"
#include "az_ulib_descriptor_api.h"
#include "az_ulib_ipc_api.h"
#include "az_ulib_registry_api.h"
#include "az_ulib_result.h"
#include "registry_1_model.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static az_result registry_1_get_value_concrete(
    az_ulib_model_in model_in,
    az_ulib_model_out model_out)
{
  (void)model_out;
  const registry_1_get_model_in* const in = (const registry_1_get_model_in* const)model_in;
  const registry_1_get_model_out* const out = (const registry_1_get_model_out* const)model_out;
  return az_ulib_registry_get(in->registry_key, out->registry_value);
}

static az_result registry_1_add_concrete(az_ulib_model_in model_in, az_ulib_model_out model_out)
{
  (void)model_out;
  const registry_1_add_model_in* const in = (const registry_1_add_model_in* const)model_in;
  return az_ulib_registry_add(in->new_key, in->new_value);
}

static az_result registry_1_delete_concrete(az_ulib_model_in model_in, az_ulib_model_out model_out)
{
  (void)model_out;
  const registry_1_delete_model_in* const in = (const registry_1_delete_model_in* const)model_in;
  return az_ulib_registry_delete(in->registry_key);
}

static const az_ulib_capability_descriptor REGISTRY_1_CAPABILITIES[] = {
  AZ_ULIB_DESCRIPTOR_ADD_CAPABILITY(
      REGISTRY_1_GET_COMMAND_NAME,
      registry_1_get_value_concrete,
      NULL),
  AZ_ULIB_DESCRIPTOR_ADD_CAPABILITY(REGISTRY_1_ADD_COMMAND_NAME, registry_1_add_concrete, NULL),
  AZ_ULIB_DESCRIPTOR_ADD_CAPABILITY(
      REGISTRY_1_DELETE_COMMAND_NAME,
      registry_1_delete_concrete,
      NULL)
};

static const az_ulib_interface_descriptor REGISTRY_1_DESCRIPTOR = AZ_ULIB_DESCRIPTOR_CREATE(
    REGISTRY_1_PACKAGE_NAME,
    REGISTRY_1_PACKAGE_VERSION,
    REGISTRY_1_INTERFACE_NAME,
    REGISTRY_1_INTERFACE_VERSION,
    REGISTRY_1_CAPABILITIES);

az_result _az_ulib_registry_interface_publish(void)
{
  return az_ulib_ipc_publish(&REGISTRY_1_DESCRIPTOR, NULL);
}

az_result _az_ulib_registry_interface_unpublish(void)
{
#ifdef AZ_ULIB_CONFIG_IPC_UNPUBLISH
  return az_ulib_ipc_unpublish(&REGISTRY_1_DESCRIPTOR, AZ_ULIB_NO_WAIT);
#else
  return AZ_OK;
#endif // AZ_ULIB_CONFIG_IPC_UNPUBLISH
}
