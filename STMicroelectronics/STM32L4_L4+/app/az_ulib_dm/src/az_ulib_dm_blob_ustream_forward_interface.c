// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "_az_nx_blob_client.h"
#include "az_ulib_ustream_forward.h"
#include "az_ulib_dm_blob_ustream_forward_interface.h"
#include "az_ulib_port.h"
#include "az_ulib_result.h"
#include "azure/core/az_span.h"

#include <azure/core/internal/az_precondition_internal.h>

#define BLOB_CLIENT_NX_API_WAIT_TIME 600

#ifdef __clang__
#define IGNORE_POINTER_TYPE_QUALIFICATION \
  _Pragma("clang diagnostic push")        \
      _Pragma("clang diagnostic ignored \"-Wincompatible-pointer-types-discards-qualifiers\"")
#define IGNORE_MEMCPY_TO_NULL _Pragma("GCC diagnostic push")
#define RESUME_WARNINGS _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define IGNORE_POINTER_TYPE_QUALIFICATION \
  _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdiscarded-qualifiers\"")
#define IGNORE_MEMCPY_TO_NULL _Pragma("GCC diagnostic push")
#define RESUME_WARNINGS _Pragma("GCC diagnostic pop")
#else
#define IGNORE_POINTER_TYPE_QUALIFICATION __pragma(warning(push));
#define IGNORE_MEMCPY_TO_NULL \
  __pragma(warning(push));  \
  __pragma(warning(suppress: 6387));
#define RESUME_WARNINGS __pragma(warning(pop));
#endif // __clang__

static az_result concrete_flush(
    az_ulib_ustream_forward* ustream_forward,
    az_ulib_flush_callback flush_callback, 
    az_ulib_callback_context flush_callback_context);
static az_result concrete_read(
    az_ulib_ustream_forward* ustream_forward,
    uint8_t* const buffer,
    size_t buffer_length,
    size_t* const size);
static size_t concrete_get_size(
    az_ulib_ustream_forward* ustream_forward);
static az_result concrete_dispose(
    az_ulib_ustream_forward* ustream_forward);
static const az_ulib_ustream_forward_interface api
    = { concrete_flush, concrete_read,  concrete_get_size, concrete_dispose };

static az_result concrete_flush(
    az_ulib_ustream_forward* ustream_forward,
    az_ulib_flush_callback flush_callback, 
    az_ulib_callback_context flush_callback_context)
{
  // precondition checks
  _az_PRECONDITION_NOT_NULL(ustream_forward);
  _az_PRECONDITION(AZ_ULIB_USTREAM_FORWARD_IS_TYPE_OF(ustream_forward, api));
  _az_PRECONDITION_NOT_NULL(flush_callback);

  // get size of data
  size_t buffer_size = concrete_get_size(ustream_forward);

  // point to data
  const uint8_t* buffer = (const uint8_t*)ustream_forward->_internal.ptr;

  // invoke callback
  (*flush_callback)(buffer, buffer_size, flush_callback_context);

  return AZ_OK;
}

static az_result concrete_read(
    az_ulib_ustream_forward* ustream_forward,
    uint8_t* const buffer,
    size_t buffer_length,
    size_t* const size)
{
 _az_PRECONDITION(AZ_ULIB_USTREAM_FORWARD_IS_TYPE_OF(ustream_forward, api));
  _az_PRECONDITION_NOT_NULL(buffer);
  _az_PRECONDITION(buffer_length > 0);
  _az_PRECONDITION_NOT_NULL(size);

  az_result result;

  if (ustream_forward->_internal.inner_current_position == ustream_forward->_internal.length)
  {
    // All bytes already returned.
    result = AZ_ULIB_EOF;
  }
  else
  {
    az_blob_http_cb* blob_http_cb = (az_blob_http_cb*)ustream_forward->_internal.ptr;

    // keep track of our position in the local buffer
    size_t remain_size_local_buffer = buffer_length;

    // keep track of our position in the inner buffer (packet)
    size_t remain_size_inner_buffer;

    // keep track of total size copied
    size_t total_size_copied = 0;

    // while we still have room left in the local buffer and we have not reached the end of the
    // package
    do
    {
      // keep track of our position in the inner buffer
      remain_size_inner_buffer = blob_http_cb->_internal.packet_ptr->nx_packet_length
          - (size_t)blob_http_cb->_internal.read_offset;

      // if we have not reached the end of the inner buffer, copy the next data to the local buffer
      if (remain_size_inner_buffer > 0)
      {
        // size to copy is the smaller of the remaining size in the inner buffer or the local buffer
        size_t size_to_copy = remain_size_inner_buffer < remain_size_local_buffer
            ? remain_size_inner_buffer
            : remain_size_local_buffer;

        // copy data from inner buffer to local buffer
        memcpy(
            buffer + (buffer_length - remain_size_local_buffer),
            blob_http_cb->_internal.packet_ptr->nx_packet_prepend_ptr
                + blob_http_cb->_internal.read_offset,
            size_to_copy);

        // decrease the remaining size in our local buffer
        remain_size_local_buffer -= size_to_copy;

        // increase the passed size for the caller
        total_size_copied += size_to_copy;

        // increase offset to keep track of our position in the inner buffer
        blob_http_cb->_internal.read_offset += size_to_copy;

        // set result to AZ_OK for next loop
        result = AZ_OK;
      }

      // All bytes in the current packet was consumed.
      else
      {
        // increase current position by size of last chunk
        ustream_forward->_internal.inner_current_position += blob_http_cb->_internal.read_offset;
        // reset current offset
        blob_http_cb->_internal.read_offset = 0;

        // else if we have not already grabbed the last chunk, grab another
        if (ustream_forward->_internal.inner_current_position != ustream_forward->_internal.length)
        {
          if ((result = _az_nx_blob_client_grab_chunk(
                   &blob_http_cb->_internal.http_client,
                   &blob_http_cb->_internal.packet_ptr,
                   BLOB_CLIENT_NX_API_WAIT_TIME))
              == AZ_ULIB_EOF)
          {
            result = AZ_OK;
          }
        }

        // else this is the last buffer-full to copy, exit
        else
        {
          break;
        }
      }
    } while ((remain_size_local_buffer > 0) && (result == AZ_OK));

    // load total size copied into returned size if successful
    if (result == AZ_OK)
    {
      *size = total_size_copied;
    }
  }

  return result;
}

static size_t concrete_get_size(az_ulib_ustream_forward* ustream_forward)
{
  _az_PRECONDITION(AZ_ULIB_USTREAM_FORWARD_IS_TYPE_OF(ustream_forward, api));

  return ustream_forward->_internal.length - ustream_forward->_internal.inner_current_position;
}

static az_result concrete_dispose(az_ulib_ustream_forward* ustream_forward)
{
  _az_PRECONDITION(AZ_ULIB_USTREAM_FORWARD_IS_TYPE_OF(ustream_forward, api));

  AZ_ULIB_TRY
  {
    // dispose of blob http control block
    az_blob_http_cb* blob_http_cb = (az_blob_http_cb*)ustream_forward->_internal.ptr;
      AZ_ULIB_THROW_IF_AZ_ERROR(_az_nx_blob_client_dispose(
          &blob_http_cb->_internal.http_client, &blob_http_cb->_internal.packet_ptr));

    if (ustream_forward->_internal.data_release)
    {
      /* If `data_release` was provided it is because `ptr` is not `const`. So, we have a
          Warning exception here to remove the `const` qualification of the `ptr`. */
      IGNORE_POINTER_TYPE_QUALIFICATION
      ustream_forward->_internal.data_release(ustream_forward->_internal.ptr);
      RESUME_WARNINGS
    }
    if (ustream_forward->_internal.ustream_forward_release)
    {
      ustream_forward->_internal.ustream_forward_release(ustream_forward);
    }
  }
  AZ_ULIB_CATCH(...) {} 

    return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result az_ulib_ustream_forward_init(
    az_ulib_ustream_forward* ustream_forward,
    az_ulib_release_callback ustream_forward_release,
    const uint8_t* const data_buffer,
    size_t data_buffer_length,
    az_ulib_release_callback data_buffer_release)
{
  _az_PRECONDITION_NOT_NULL(ustream_forward);
  _az_PRECONDITION_NOT_NULL(data_buffer);
  _az_PRECONDITION(data_buffer_length > 0);

  ustream_forward->_internal.api = &api;
  ustream_forward->_internal.ptr = (const az_ulib_ustream_forward_data*)data_buffer;
  ustream_forward->_internal.data_release = data_buffer_release;
  ustream_forward->_internal.ustream_forward_release = ustream_forward_release;
  ustream_forward->_internal.inner_current_position = 0;
  ustream_forward->_internal.length = data_buffer_length;

  return AZ_OK;
}

AZ_NODISCARD az_result az_blob_create_ustream_forward_from_blob(
    az_ulib_ustream_forward* ustream_forward,
    az_ulib_release_callback ustream_forward_release_callback,
    az_blob_http_cb* blob_http_cb,
    az_ulib_release_callback blob_http_cb_release_callback,
    NXD_ADDRESS* ip,
    int8_t* resource,
    int8_t* host)
{
  _az_PRECONDITION_NOT_NULL(ustream_forward);
  _az_PRECONDITION_NOT_NULL(blob_http_cb);
  _az_PRECONDITION_NOT_NULL(ip);
  _az_PRECONDITION_NOT_NULL(resource);
  _az_PRECONDITION_NOT_NULL(host);
  _az_PRECONDITION(resource[0] != 0);
  _az_PRECONDITION(host[0] != 0);

  AZ_ULIB_TRY
  {
    // initialize blob client
    AZ_ULIB_THROW_IF_AZ_ERROR(_az_nx_blob_client_init(
        &blob_http_cb->_internal.http_client, ip, BLOB_CLIENT_NX_API_WAIT_TIME));

    // send request
    AZ_ULIB_THROW_IF_AZ_ERROR(_az_nx_blob_client_request_send(
        &blob_http_cb->_internal.http_client, resource, host, BLOB_CLIENT_NX_API_WAIT_TIME));

    // grab first chunk and save package size
    AZ_ULIB_THROW_IF_AZ_ERROR(_az_nx_blob_client_grab_chunk(
        &blob_http_cb->_internal.http_client,
        &blob_http_cb->_internal.packet_ptr,
        BLOB_CLIENT_NX_API_WAIT_TIME));

    // grab blob package size for ustream init
    uint32_t package_size
        = (uint32_t)blob_http_cb->_internal.http_client.nx_web_http_client_total_receive_bytes;

    // initialize ustream
    AZ_ULIB_THROW_IF_AZ_ERROR(az_ulib_ustream_forward_init(
      ustream_forward,
      ustream_forward_release_callback,
      (const uint8_t*)blob_http_cb,
      package_size,
      blob_http_cb_release_callback
    ));
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}