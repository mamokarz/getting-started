// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

/********************************************************************
 * This code was auto-generated from dm v2 DL.
 *
 * Implement the code under the concrete functions.
 *
 ********************************************************************/

#include "_az_ulib_dm_interface.h"
#include "az_ulib_capability_api.h"
#include "az_ulib_descriptor_api.h"
#include "az_ulib_ipc_api.h"
#include "az_ulib_result.h"
#include "az_ulib_dm_api.h"
#include "dm_1_model.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static az_result dm_1_install_concrete(az_ulib_model_in model_in, az_ulib_model_out model_out)
{
  (void)model_out;
  const dm_1_install_model_in* const in = (const dm_1_install_model_in* const)model_in;
  return az_ulib_dm_install(in->source_type, in->address, in->package_name);
}

static az_result dm_1_install_span_wrapper(az_span model_in_span, az_span* model_out_span)
{
  AZ_ULIB_TRY
  {
    // Unmarshalling JSON in model_in_span to install_model_in.
    az_json_reader jr;
    dm_1_install_model_in install_model_in = { 0 };
    AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_init(&jr, model_in_span, NULL));
    AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
    while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
    {
      if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR(DM_1_INSTALL_SOURCE_NAME)))
      {
        int32_t type;
        AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
        AZ_ULIB_THROW_IF_AZ_ERROR(az_span_atoi32(jr.token.slice, &type));
        if((type < DM_1_SOURCE_TYPE_IN_MEMORY) || (type > DM_1_SOURCE_TYPE_BUILT_IN))
        {
          AZ_ULIB_THROW(AZ_ERROR_UNEXPECTED_CHAR);
        }        
        install_model_in.source_type = (dm_1_source_type)type;
      }
      else if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR(DM_1_INSTALL_ADDRESS_NAME)))
      {
        uint32_t add;
        AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
        AZ_ULIB_THROW_IF_AZ_ERROR(az_span_atou32(jr.token.slice, &add));
        install_model_in.address = NULL + add;
      }
      else if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR(DM_1_INSTALL_PACKAGE_NAME_NAME)))
      {
        AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
        install_model_in.package_name
            = az_span_create(az_span_ptr(jr.token.slice), az_span_size(jr.token.slice));
      }
      AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
    }
    AZ_ULIB_THROW_IF_AZ_ERROR(AZ_ULIB_TRY_RESULT);

    // Call.
    AZ_ULIB_THROW_IF_AZ_ERROR(dm_1_install_concrete((az_ulib_model_in)&install_model_in, NULL));

    // Marshalling empty install_model_out to JSON in model_out_span.
    *model_out_span = az_span_create_from_str("{}");
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static az_result dm_1_uninstall_concrete(az_ulib_model_in model_in, az_ulib_model_out model_out)
{
  (void)model_out;
  const dm_1_uninstall_model_in* const in = (const dm_1_uninstall_model_in* const)model_in;
  return az_ulib_dm_uninstall(in->package_name);
}

static az_result dm_1_uninstall_span_wrapper(az_span model_in_span, az_span* model_out_span)
{
  AZ_ULIB_TRY
  {
    // Unmarshalling JSON in model_in_span to uninstall_model_in.
    az_json_reader jr;
    dm_1_uninstall_model_in uninstall_model_in = { 0 };
    AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_init(&jr, model_in_span, NULL));
    AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
    while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
    {
      if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR(DM_1_UNINSTALL_PACKAGE_NAME_NAME)))
      {
        AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
        uninstall_model_in.package_name
            = az_span_create(az_span_ptr(jr.token.slice), az_span_size(jr.token.slice));
      }
      AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
    }
    AZ_ULIB_THROW_IF_AZ_ERROR(AZ_ULIB_TRY_RESULT);

    // Call.
    AZ_ULIB_THROW_IF_AZ_ERROR(dm_1_uninstall_concrete((az_ulib_model_in)&uninstall_model_in, NULL));

    // Marshalling empty uninstall_model_out to JSON in model_out_span.
    *model_out_span = az_span_create_from_str("{}");
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static const az_ulib_capability_descriptor DM_1_CAPABILITIES[DM_1_CAPABILITY_SIZE] = {
  AZ_ULIB_DESCRIPTOR_ADD_COMMAND(
      DM_1_INSTALL_COMMAND_NAME,
      dm_1_install_concrete,
      dm_1_install_span_wrapper),
  AZ_ULIB_DESCRIPTOR_ADD_COMMAND(
      DM_1_UNINSTALL_COMMAND_NAME, 
      dm_1_uninstall_concrete,
      dm_1_uninstall_span_wrapper)
};

static const az_ulib_interface_descriptor DM_1_DESCRIPTOR = AZ_ULIB_DESCRIPTOR_CREATE(
    DM_1_INTERFACE_NAME,
    DM_1_INTERFACE_VERSION,
    DM_1_CAPABILITY_SIZE,
    DM_1_CAPABILITIES);

az_result _az_ulib_dm_interface_publish(void)
{
  return az_ulib_ipc_publish(&DM_1_DESCRIPTOR, NULL);
}

az_result _az_ulib_dm_interface_unpublish(void)
{
#ifdef AZ_ULIB_CONFIG_IPC_UNPUBLISH
  return az_ulib_ipc_unpublish(&DM_1_DESCRIPTOR, AZ_ULIB_NO_WAIT);
#else
  return AZ_OK;
#endif // AZ_ULIB_CONFIG_IPC_UNPUBLISH
}
