// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "_az_ulib_dm_blob.h"
#include "az_ulib_dm_blob_ustream_forward_interface.h"
#include "az_ulib_result.h"
#include "az_ulib_ustream_forward.h"
#include "azure/az_core.h"
#include "stm32l475_flash_driver.h"
#include <stdbool.h>
#include <stdint.h>

// TODO: Move to gateway
#include "stm_networking.h"

static int32_t slice_next_char(az_span span, int32_t start, uint8_t c, az_span* slice)
{
  int32_t ustream_forward_size = az_span_size(span);
  uint8_t* buf = az_span_ptr(span);
  int32_t end = start;
  while (end < ustream_forward_size)
  {
    if (buf[end] == c)
    {
      if (slice != NULL)
      {
        *slice = az_span_slice(span, start, end);
      }
      return end;
    }
    end++;
  }
  return -1;
}

static int32_t slice_latest_char(az_span span, int32_t start, uint8_t c, az_span* slice)
{
  uint8_t* buf = az_span_ptr(span) + start;
  uint8_t* end_buf = az_span_ptr(span) + az_span_size(span);
  int32_t latest = start;
  while (buf < end_buf)
  {
    if (*buf == c)
    {
      latest = buf - az_span_ptr(span);
    }
    buf++;
  }
  if (latest != start)
  {
    if (slice != NULL)
    {
      *slice = az_span_slice(span, start, latest);
    }
    return latest;
  }
  return -1;
}

static az_result split_url(
    az_span url,
    az_span* protocol,
    az_span* address,
    az_span* resource,
    az_span* container,
    az_span* directories,
    az_span* file_name,
    az_span* sas_token)
{
  AZ_ULIB_TRY
  {
    int32_t next;

    /* Get protocol http:// or https:// */
    AZ_ULIB_THROW_IF_ERROR(
        ((next = az_span_find(url, AZ_SPAN_FROM_STR("://"))) != -1), AZ_ERROR_UNEXPECTED_CHAR);
    if (protocol != NULL)
    {
      *protocol = az_span_slice(url, 0, next);
    }

    /* Get address. */
    AZ_ULIB_THROW_IF_ERROR(
        ((next = slice_next_char(url, next + 3, '/', address)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

    /* Get resource span. */
    if (resource != NULL)
    {
      *resource = az_span_slice_to_end(url, next + 1);
    }

    /* Get container name. */
    AZ_ULIB_THROW_IF_ERROR(
        ((next = slice_next_char(url, next + 1, '/', container)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

    /* Get directories, if it exists. */
    if ((next = slice_latest_char(url, next + 1, '/', directories)) == -1)
    {
      *directories = AZ_SPAN_EMPTY;
    }

    /* Get file_name. */
    AZ_ULIB_THROW_IF_ERROR(
        ((next = slice_next_char(url, next + 1, '?', file_name)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

    /* Get sas_token. */
    if (sas_token != NULL)
    {
      *sas_token = az_span_slice_to_end(url, next + 1);
    }
  }
  AZ_ULIB_CATCH(...) { return AZ_ULIB_TRY_RESULT; }

  return AZ_OK;
}

// TODO: Move to gateway
static az_result get_ip_from_uri(az_span uri, NXD_ADDRESS* ip)
{
  AZ_ULIB_TRY
  {
    /* Get the IPv4 for the URI using DNS. */
    char uri_str[50];
    az_span_to_str(uri_str, sizeof(uri_str), uri);
    UINT status = nxd_dns_host_by_name_get(
        &nx_dns_client, (UCHAR*)uri_str, ip, NX_IP_PERIODIC_RATE, NX_IP_VERSION_V4);
    AZ_ULIB_THROW_IF_ERROR((status == NX_SUCCESS), AZ_ERROR_ULIB_SYSTEM);
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_ulib_dm_blob_get_package_name(az_span url, az_span* name)
{
  AZ_ULIB_TRY
  {
    AZ_ULIB_THROW_IF_AZ_ERROR(split_url(url, NULL, NULL, NULL, NULL, NULL, name, NULL));
    AZ_ULIB_THROW_IF_ERROR(
        (slice_latest_char(*name, 0, '.', name) != -1), AZ_ERROR_UNEXPECTED_CHAR);
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_ulib_dm_blob_get_size(az_span url, int32_t* ustream_forward_size)
{
  (void)url;
  (void)ustream_forward_size;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

static az_result result_from_hal_status(HAL_StatusTypeDef status)
{
  switch (status)
  {
    case HAL_OK:
      return AZ_OK;
    case HAL_ERROR:
      return AZ_ERROR_ULIB_SYSTEM;
    case HAL_BUSY:
      return AZ_ERROR_ULIB_BUSY;
    case HAL_TIMEOUT:
      return AZ_ERROR_ULIB_SYSTEM; // should be AZ_ERROR_ULIB_TIME_OUT
    default:
      return AZ_ERROR_ULIB_SYSTEM;
  }
}

typedef struct
{
  void* address;
} flush_write_to_flash_context;

// flush callback to write package to flash
static az_result flush_callback(
    const uint8_t* const buffer,
    size_t size,
    az_ulib_callback_context flush_callback_context)
{
  az_result result;
  HAL_StatusTypeDef hal_status;

  // handle buffer
  flush_write_to_flash_context* flush_context
      = (flush_write_to_flash_context*)flush_callback_context;

  // write to flash if we have not reached the end of this chunk of data
  if ((hal_status = internal_flash_write((uint8_t*)flush_context->address, (uint8_t*)buffer, size))
      != HAL_OK)
  {
    result = result_from_hal_status(hal_status);
  }

  else
  {
    result = AZ_OK;
  }

  // adjust offset
  flush_context->address += size;

  return result;
}

static az_result write_ustream_forward_to_flash(
    az_ulib_ustream_forward* ustream_forward,
    void* address)
{
  az_result result;
  HAL_StatusTypeDef hal_status;

  size_t ustream_forward_size = 0;
  flush_write_to_flash_context flush_context = { address };

  ustream_forward_size = az_ulib_ustream_forward_get_size(ustream_forward);

  // erase flash
  if ((hal_status = internal_flash_erase((UCHAR*)address, ustream_forward_size)) != HAL_OK)
  {
    result = result_from_hal_status(hal_status);
  }

  // start ustream data transfer from blob client
  else
  {
    // flush blob client data to flash
    result = az_ulib_ustream_forward_flush(ustream_forward, flush_callback, &flush_context);
  }

  if (result == AZ_ULIB_EOF)
  {
    result = AZ_OK;
  }

  return result;
}

static az_result copy_blob_to_flash(NXD_ADDRESS* ip, CHAR* resource, CHAR* host, void* address)
{

  AZ_ULIB_TRY
  {
    (void)address;
    az_ulib_ustream_forward ustream_forward;

    static az_blob_http_cb blob_http_cb;

    // create ustream_forward and blob client
    AZ_ULIB_THROW_IF_AZ_ERROR(az_blob_create_ustream_forward_from_blob(
        &ustream_forward, NULL, &blob_http_cb, NULL, ip, (int8_t*)resource, (int8_t*)host));

    // use ustream_forward to write blob package to flash
    AZ_ULIB_THROW_IF_AZ_ERROR(write_ustream_forward_to_flash(&ustream_forward, address));

    // free up connection and ustream_forward resources
    AZ_ULIB_THROW_IF_AZ_ERROR(az_ulib_ustream_forward_dispose(&ustream_forward));
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_ulib_dm_blob_download(void* address, az_span url)
{
  AZ_ULIB_TRY
  {
    az_span uri = AZ_SPAN_EMPTY;
    NXD_ADDRESS ip;
    az_span resource;

    AZ_ULIB_THROW_IF_AZ_ERROR(split_url(url, NULL, &uri, &resource, NULL, NULL, NULL, NULL));

    AZ_ULIB_THROW_IF_AZ_ERROR(get_ip_from_uri(uri, &ip));

    char host[50];
    az_span_to_str(host, sizeof(host), uri);
    char resource_str[200];
    az_span_to_str(resource_str, sizeof(resource_str), resource);
    AZ_ULIB_THROW_IF_AZ_ERROR(copy_blob_to_flash(&ip, resource_str, host, address));
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}
