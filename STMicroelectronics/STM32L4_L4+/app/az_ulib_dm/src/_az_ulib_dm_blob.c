// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "_az_ulib_dm_blob.h"
#include "az_ulib_dm_blob_ustream_interface.h"
#include "az_ulib_result.h"
#include "az_ulib_ustream.h"
#include "azure/az_core.h"
#include "stm32l475_flash_driver.h"
#include <stdbool.h>
#include <stdint.h>

// TODO: Move to gateway
#include "stm_networking.h"

static int32_t slice_next_char(az_span span, int32_t start, uint8_t c, az_span* slice)
{
  int32_t returned_size = az_span_size(span);
  uint8_t* buf = az_span_ptr(span);
  int32_t end = start;
  while (end < returned_size)
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

    // Get directories, if exist. */
    if ((next = slice_next_char(url, next + 1, '/', directories)) != -1)
    {
      // TODO: Get more than one directory.
    }

    // Get file_name. */
    AZ_ULIB_THROW_IF_ERROR(
        ((next = slice_next_char(url, next + 1, '?', file_name)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

    // Get sas_token. */
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
    AZ_ULIB_THROW_IF_ERROR((slice_next_char(*name, 0, '.', name) != -1), AZ_ERROR_UNEXPECTED_CHAR);
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_ulib_dm_blob_get_size(az_span url, int32_t* returned_size)
{
  (void)url;
  (void)returned_size;
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

static az_result write_ustream_to_flash(az_ulib_ustream* ustream_instance, void* address)
{
  az_result result;
  HAL_StatusTypeDef hal_status;

  size_t returned_size = 0;
  static uint8_t local_buffer[256] = { 0 };

  if ((result = az_ulib_ustream_get_remaining_size(ustream_instance, &returned_size)) == AZ_OK)
  {
    // erase flash
    if ((returned_size & 0x07FF) != 0x000)
    {
      returned_size = (returned_size & 0xFFFFF800) + 0x0800;
    }
    if ((hal_status = internal_flash_erase((UCHAR*)address, returned_size)) != HAL_OK)
    {
      result = result_from_hal_status(hal_status);
    }

    // start ustream data transfer from blob client
    else
    {
      do
      {
        // grab next buffer-full from ustream_instance
        if ((result = az_ulib_ustream_read(
                 ustream_instance, local_buffer, sizeof(local_buffer), &returned_size))
            == AZ_OK) // should not use EOF
        {
          // write to flash if we have not reached the end of this chunk of data
          if ((hal_status = internal_flash_write((uint8_t*)address, local_buffer, returned_size))
              != HAL_OK)
          {
            result = result_from_hal_status(hal_status);
          }
          // increment the write address by the last write-size
          address += returned_size;
        }
      } while (result == AZ_OK);
    }

    if (result == AZ_ULIB_EOF)
    {
      result = AZ_OK;
    }
  }

  return result;
}

static az_result copy_blob_to_flash(NXD_ADDRESS* ip, CHAR* resource, CHAR* host, void* address)
{

  AZ_ULIB_TRY
  {
    (void)address;
    az_ulib_ustream ustream_instance;

    static az_blob_http_cb blob_http_cb;
    az_ulib_ustream_data_cb ustream_data_cb;

    // create ustream_instance and blob client
    AZ_ULIB_THROW_IF_AZ_ERROR(az_blob_create_ustream_from_blob(
        &ustream_instance,
        &ustream_data_cb,
        NULL,
        &blob_http_cb,
        NULL,
        ip,
        (int8_t*)resource,
        (int8_t*)host));

    // use ustream_instance to write blob package to flash
    AZ_ULIB_THROW_IF_AZ_ERROR(write_ustream_to_flash(&ustream_instance, address));

    // free up connection and ustream_instance resources
    AZ_ULIB_THROW_IF_AZ_ERROR(az_ulib_ustream_dispose(&ustream_instance));
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
