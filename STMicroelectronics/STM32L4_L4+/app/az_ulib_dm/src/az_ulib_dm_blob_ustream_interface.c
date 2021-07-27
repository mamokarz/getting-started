// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "_az_nx_blob_client.h"
#include "az_ulib_dm_blob_ustream_interface.h"
#include "az_ulib_port.h"
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
  __pragma(warning(push));    \
  __pragma(warning(suppress : 6387));
#define RESUME_WARNINGS __pragma(warning(pop));
#endif // __clang__

static az_result concrete_set_position(az_ulib_ustream* ustream_instance, offset_t position);
static az_result concrete_reset(az_ulib_ustream* ustream_instance);
static az_result concrete_read(
    az_ulib_ustream* ustream_instance,
    uint8_t* const buffer,
    size_t buffer_length,
    size_t* const size);
static az_result concrete_get_remaining_size(az_ulib_ustream* ustream_instance, size_t* const size);
static az_result concrete_get_position(az_ulib_ustream* ustream_instance, offset_t* const position);
static az_result concrete_release(az_ulib_ustream* ustream_instance, offset_t position);
static az_result concrete_clone(
    az_ulib_ustream* ustream_instance_clone,
    az_ulib_ustream* ustream_instance,
    offset_t offset);
static az_result concrete_dispose(az_ulib_ustream* ustream_instance);
static const az_ulib_ustream_interface api
    = { concrete_set_position, concrete_reset,   concrete_read,  concrete_get_remaining_size,
        concrete_get_position, concrete_release, concrete_clone, concrete_dispose };

static void init_instance(
    az_ulib_ustream* ustream_instance,
    az_ulib_ustream_data_cb* control_block,
    offset_t inner_current_position,
    offset_t offset,
    size_t data_buffer_length)
{
  ustream_instance->inner_current_position = inner_current_position;
  ustream_instance->inner_first_valid_position = inner_current_position;
  ustream_instance->offset_diff = offset - inner_current_position;
  ustream_instance->control_block = control_block;
  ustream_instance->length = data_buffer_length;
}

static void destroy_control_block(az_ulib_ustream_data_cb* control_block)
{
  if (control_block->data_release)
  {
    /* If `data_release` was provided is because `ptr` is not `const`. So, we have an Warning
     * exception here to remove the `const` qualification of the `ptr`. */
    IGNORE_POINTER_TYPE_QUALIFICATION
    control_block->data_release(control_block->ptr);
    RESUME_WARNINGS
  }

  if (control_block->control_block_release)
  {
    control_block->control_block_release(control_block);
  }
}

static az_result concrete_set_position(az_ulib_ustream* ustream_instance, offset_t position)
{
  (void)ustream_instance;
  (void)position;

  return AZ_ERROR_NOT_SUPPORTED;
}

static az_result concrete_reset(az_ulib_ustream* ustream_instance)
{
  (void)ustream_instance;

  return AZ_ERROR_NOT_SUPPORTED;
}

static az_result concrete_read(
    az_ulib_ustream* ustream_instance,
    uint8_t* const buffer,
    size_t buffer_length,
    size_t* const size)
{
  _az_PRECONDITION(AZ_ULIB_USTREAM_IS_TYPE_OF(ustream_instance, api));
  _az_PRECONDITION_NOT_NULL(buffer);
  _az_PRECONDITION(buffer_length > 0);
  _az_PRECONDITION_NOT_NULL(size);

  az_result result;

  az_ulib_ustream_data_cb* control_block = ustream_instance->control_block;
  az_blob_http_cb* blob_http_cb = (az_blob_http_cb*)control_block->ptr;

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
        - (size_t)ustream_instance->offset_diff;

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
          blob_http_cb->_internal.packet_ptr->nx_packet_prepend_ptr + ustream_instance->offset_diff,
          size_to_copy);

      // decrease the remaining size in our local buffer
      remain_size_local_buffer -= size_to_copy;

      // increase the passed size for the caller
      total_size_copied += size_to_copy;

      // increase offset to keep track of our position in the inner buffer
      ustream_instance->offset_diff += size_to_copy;

      // set result to AZ_OK for next loop 
      result = AZ_OK;
    }

    // else if we have not already grabbed the last chunk, grab another
    else if (!blob_http_cb->_internal.last_chunk)
    {
      result = _az_nx_blob_client_grab_chunk(
          &blob_http_cb->_internal.http_client,
          &blob_http_cb->_internal.packet_ptr,
          BLOB_CLIENT_NX_API_WAIT_TIME);
      if ((result == AZ_OK) || (result == AZ_ULIB_EOF))
      {
        // increase current position by size of last chunk
        ustream_instance->inner_current_position += ustream_instance->offset_diff;
        // reset current offset
        ustream_instance->offset_diff = 0;
      }

      // raise last_chunk flag so we can exit once done processing this last chunk
      if (result == AZ_ULIB_EOF)
      {
        blob_http_cb->_internal.last_chunk = true;
      }
    }

    // else this is the last buffer-full to copy, exit
    else
    {
      break;
    }
  } while ((remain_size_local_buffer > 0) && ((result == AZ_OK) || (result == AZ_ULIB_EOF)));

  // load total size copied into returned size if successful
  if ((result == AZ_OK) || (result == AZ_ULIB_EOF))
  {
    *size = total_size_copied;
    result = AZ_OK;
  }

  // set result to EOF if we have finished processing the last chunk
  if (blob_http_cb->_internal.last_chunk && (remain_size_inner_buffer <= 0))
  {
    result = AZ_ULIB_EOF;
  }

  return result;
}

static az_result concrete_get_remaining_size(az_ulib_ustream* ustream_instance, size_t* const size)
{
  *size = ustream_instance->length - ustream_instance->inner_current_position;

  return AZ_OK;
}

static az_result concrete_get_position(az_ulib_ustream* ustream_instance, offset_t* const position)
{
  (void)ustream_instance;
  (void)position;

  return AZ_ERROR_NOT_SUPPORTED;
}

static az_result concrete_release(az_ulib_ustream* ustream_instance, offset_t position)
{
  (void)ustream_instance;
  (void)position;

  return AZ_OK;
}

static az_result concrete_clone(
    az_ulib_ustream* ustream_instance_clone,
    az_ulib_ustream* ustream_instance,
    offset_t offset)
{
  (void)ustream_instance_clone;
  (void)ustream_instance;
  (void)offset;

  return AZ_ERROR_NOT_SUPPORTED;
}

static az_result concrete_dispose(az_ulib_ustream* ustream_instance)
{
  AZ_ULIB_TRY
  {
    az_ulib_ustream_data_cb* control_block = ustream_instance->control_block;
    az_blob_http_cb* blob_http_cb = (az_blob_http_cb*)control_block->ptr;

    // dispose of http client control block
    AZ_ULIB_THROW_IF_AZ_ERROR(_az_nx_blob_client_dispose(
        &blob_http_cb->_internal.http_client, &blob_http_cb->_internal.packet_ptr));

    // dispose of the ustream control block
    destroy_control_block(control_block);
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static az_result az_blob_ustream_init(
    az_ulib_ustream* ustream_instance,
    az_ulib_ustream_data_cb* ustream_data_cb,
    az_ulib_release_callback ustream_data_cb_release_callback,
    const uint8_t* const data_buffer,
    size_t data_buffer_length,
    az_ulib_release_callback blob_http_cb_release_callback)
{
  ustream_data_cb->api = &api;
  ustream_data_cb->ptr = (const az_ulib_ustream_data*)data_buffer;
  ustream_data_cb->data_release = blob_http_cb_release_callback;
  ustream_data_cb->control_block_release = ustream_data_cb_release_callback;

  init_instance(ustream_instance, ustream_data_cb, 0, 0, data_buffer_length);

  return AZ_OK;
}

AZ_NODISCARD az_result az_blob_create_ustream_from_blob(
    az_ulib_ustream* ustream_instance,
    az_ulib_ustream_data_cb* ustream_data_cb,
    az_ulib_release_callback ustream_data_cb_release_callback,
    az_blob_http_cb* blob_http_cb,
    az_ulib_release_callback blob_http_cb_release_callback,
    NXD_ADDRESS* ip,
    int8_t* resource,
    int8_t* host)
{
  _az_PRECONDITION_NOT_NULL(ustream_instance);
  _az_PRECONDITION_NOT_NULL(ustream_data_cb);
  _az_PRECONDITION_NOT_NULL(blob_http_cb);
  _az_PRECONDITION_NOT_NULL(ip);
  _az_PRECONDITION(strlen((char*)resource) > 0);
  _az_PRECONDITION(strlen((char*)host) > 0);

  AZ_ULIB_TRY
  {
    // initialize blob http control block
    blob_http_cb->_internal.last_chunk = false;

    // initialize blob client
    AZ_ULIB_THROW_IF_AZ_ERROR(_az_nx_blob_client_init(
        &blob_http_cb->_internal.http_client,
        ip,
        BLOB_CLIENT_NX_API_WAIT_TIME));

    // send request
    AZ_ULIB_THROW_IF_AZ_ERROR(_az_nx_blob_client_request_send(
        &blob_http_cb->_internal.http_client,
        resource,
        host,
        BLOB_CLIENT_NX_API_WAIT_TIME));

    // grab first chunk and save package size
    AZ_ULIB_THROW_IF_AZ_ERROR(_az_nx_blob_client_grab_chunk(
        &blob_http_cb->_internal.http_client,
        &blob_http_cb->_internal.packet_ptr,
        BLOB_CLIENT_NX_API_WAIT_TIME));

    // grab blob package size for ustream init
    uint32_t package_size
        = (uint32_t)blob_http_cb->_internal.http_client.nx_web_http_client_total_receive_bytes;
    if ((package_size & 0x07FF) != 0x000)
    {
      package_size = (package_size & 0xFFFFF800) + 0x0800;
    }

    // initialize ustream
    AZ_ULIB_THROW_IF_AZ_ERROR(az_blob_ustream_init(
        ustream_instance,
        ustream_data_cb,
        ustream_data_cb_release_callback,
        (const uint8_t*)blob_http_cb,
        package_size,
        blob_http_cb_release_callback));
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}
