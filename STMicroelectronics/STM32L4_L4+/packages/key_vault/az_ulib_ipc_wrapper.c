// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include <stdint.h>

#include "az_ulib_ipc_wrapper.h"
#include "az_ulib_result.h"
#include "azure/az_core.h"

#include <azure/core/internal/az_precondition_internal.h>

typedef struct
{
    AZ_NODISCARD az_result (*publish)(
        const az_ulib_interface_descriptor* interface_descriptor,
        az_ulib_ipc_interface_handle* interface_handle);
    AZ_NODISCARD az_result (*unpublish)(
        const az_ulib_interface_descriptor* interface_descriptor,
        uint32_t wait_option_ms);
    AZ_NODISCARD az_result (*try_get_interface)(
        az_span name,
        az_ulib_version version,
        az_ulib_version_match_criteria match_criteria,
        az_ulib_ipc_interface_handle* interface_handle);
    AZ_NODISCARD az_result (*try_get_capability)(
        az_ulib_ipc_interface_handle interface_handle,
        az_span name,
        az_ulib_capability_index* capability_index);
    AZ_NODISCARD az_result (*get_interface)(
        az_ulib_ipc_interface_handle original_interface_handle,
        az_ulib_ipc_interface_handle* interface_handle);
    AZ_NODISCARD az_result (*release_interface)(az_ulib_ipc_interface_handle interface_handle);
    AZ_NODISCARD az_result (*call)(
        az_ulib_ipc_interface_handle interface_handle,
        az_ulib_capability_index command_index,
        az_ulib_model_in model_in,
        az_ulib_model_out model_out);
    AZ_NODISCARD az_result (*call_w_str)(
        az_ulib_ipc_interface_handle interface_handle,
        az_ulib_capability_index command_index,
        az_span model_in_span,
        az_span* model_out_span);
    AZ_NODISCARD az_result
        (*query)(az_span query, az_span* result, uint32_t* continuation_token);
    AZ_NODISCARD az_result (*query_next)(uint32_t* continuation_token, az_span* result);
} ipc_vtable;

static ipc_vtable* _vtable = NULL;

AZ_NODISCARD az_result az_ulib_ipc_publish(
    const az_ulib_interface_descriptor* interface_descriptor,
    az_ulib_ipc_interface_handle* interface_handle)
{
  if(_vtable != NULL)
    return _vtable->publish(interface_descriptor, interface_handle);
  return AZ_ERROR_ULIB_NOT_INITIALIZED;
}

AZ_NODISCARD az_result az_ulib_ipc_unpublish(
    const az_ulib_interface_descriptor* interface_descriptor,
    uint32_t wait_option_ms)
{
  if(_vtable != NULL)
    return _vtable->unpublish(interface_descriptor, wait_option_ms);
  return AZ_ERROR_ULIB_NOT_INITIALIZED;
}

AZ_NODISCARD az_result az_ulib_ipc_try_get_interface(
    az_span name,
    az_ulib_version version,
    az_ulib_version_match_criteria match_criteria,
    az_ulib_ipc_interface_handle* interface_handle)
{
  if(_vtable != NULL)
    return _vtable->try_get_interface(name, version, match_criteria, interface_handle);
  return AZ_ERROR_ULIB_NOT_INITIALIZED;
}

AZ_NODISCARD az_result az_ulib_ipc_try_get_capability(
    az_ulib_ipc_interface_handle interface_handle,
    az_span name,
    az_ulib_capability_index* capability_index)
{
  if(_vtable != NULL)
    return _vtable->try_get_capability(interface_handle, name, capability_index);
  return AZ_ERROR_ULIB_NOT_INITIALIZED;
}

AZ_NODISCARD az_result az_ulib_ipc_get_interface(
    az_ulib_ipc_interface_handle original_interface_handle,
    az_ulib_ipc_interface_handle* interface_handle)
{
  if(_vtable != NULL)
    return _vtable->get_interface(original_interface_handle, interface_handle);
  return AZ_ERROR_ULIB_NOT_INITIALIZED;
}

AZ_NODISCARD az_result az_ulib_ipc_release_interface(az_ulib_ipc_interface_handle interface_handle)
{
  if(_vtable != NULL)
    return _vtable->release_interface(interface_handle);
  return AZ_ERROR_ULIB_NOT_INITIALIZED;
}

AZ_NODISCARD az_result az_ulib_ipc_call(
    az_ulib_ipc_interface_handle interface_handle,
    az_ulib_capability_index command_index,
    az_ulib_model_in model_in,
    az_ulib_model_out model_out)
{
  if(_vtable != NULL)
    return _vtable->call(interface_handle, command_index, model_in, model_out);
  return AZ_ERROR_ULIB_NOT_INITIALIZED;
}

AZ_NODISCARD az_result az_ulib_ipc_call_w_str(
    az_ulib_ipc_interface_handle interface_handle,
    az_ulib_capability_index command_index,
    az_span model_in_span,
    az_span* model_out_span)
{
  if(_vtable != NULL)
    return _vtable->call_w_str(interface_handle, command_index, model_in_span, model_out_span);
  return AZ_ERROR_ULIB_NOT_INITIALIZED;
}

AZ_NODISCARD az_result
    az_ulib_ipc_query(az_span query, az_span* result, uint32_t* continuation_token)
{
  if(_vtable != NULL)
    return _vtable->query(query, result, continuation_token);
  return AZ_ERROR_ULIB_NOT_INITIALIZED;
}

AZ_NODISCARD az_result az_ulib_ipc_query_next(uint32_t* continuation_token, az_span* result)
{
  if(_vtable != NULL)
    return _vtable->query_next(continuation_token, result);
  return AZ_ERROR_ULIB_NOT_INITIALIZED;
}
