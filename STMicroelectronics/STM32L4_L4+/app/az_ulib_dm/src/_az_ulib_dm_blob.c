// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "_az_ulib_dm_blob.h"
#include "az_ulib_result.h"
#include "azure/az_core.h"
#include "stm32l475_flash_driver.h"
#include "_az_ulib_dm_blob_ustream_interface.h"
#include "az_ulib_ustream.h"
#include <stdbool.h>
#include <stdint.h>

#define USER_BUFFER_SIZE 5
#define SPLIT_POSITION 12

static const char USTREAM_ONE_STRING[] = "Split BeforeSplit After";

static az_result print_ustream(az_ulib_ustream* ustream)
{
  az_result result;
  size_t returned_size;
  uint8_t user_buf[USER_BUFFER_SIZE] = { 0 };

  // Read ustream until receive AZIOT_ULIB_EOF
  (void)printf("\r\n------printing the ustream------\r\n");
  while ((result = az_ulib_ustream_read(ustream, user_buf, USER_BUFFER_SIZE - 1, &returned_size))
         == AZ_OK)
  {
    user_buf[returned_size] = '\0';
    (void)printf("%s", user_buf);
  }
  (void)printf("\r\n-----------end of ustream------------\r\n\r\n");

  // Change return to AZ_OK if last returned value was AZ_ULIB_EOF
  if (result == AZ_ULIB_EOF)
  {
    result = AZ_OK;
  }
  return result;
}

static int32_t slice_next_char(az_span span, int32_t start, uint8_t c, az_span* slice)
{
  int32_t size = az_span_size(span);
  uint8_t* buf = az_span_ptr(span);
  int32_t end  = start;
  while (end < size)
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

static az_result split_url(az_span url,
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
    AZ_ULIB_THROW_IF_ERROR(((next = az_span_find(url, AZ_SPAN_FROM_STR("://"))) != -1), AZ_ERROR_UNEXPECTED_CHAR);
    if (protocol != NULL)
    {
      *protocol = az_span_slice(url, 0, next);
    }

    /* Get address. */
    AZ_ULIB_THROW_IF_ERROR(((next = slice_next_char(url, next + 3, '/', address)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

    /* Get resource span. */
    if (resource != NULL)
    {
      *resource = az_span_slice_to_end(url, next + 1);
    }

    /* Get container name. */
    AZ_ULIB_THROW_IF_ERROR(((next = slice_next_char(url, next + 1, '/', container)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

    // Get directories, if exist. */
    // TODO: add code to read directories here.

    // Get file_name. */
    AZ_ULIB_THROW_IF_ERROR(((next = slice_next_char(url, next + 1, '?', file_name)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

    // Get sas_token. */
    if (sas_token != NULL)
    {
      *sas_token = az_span_slice_to_end(url, next + 1);
    }
  }
  AZ_ULIB_CATCH(...)
  {
    return AZ_ULIB_TRY_RESULT;
  }

  return AZ_OK;
}

static az_result get_ip_from_uri(az_span uri, NXD_ADDRESS* ip)
{
  AZ_ULIB_TRY
  {
    /* Get the IPv4 for the URI using DNS. */
    char uri_str[50];
    az_span_to_str(uri_str, sizeof(uri_str), uri);
    UINT status = nxd_dns_host_by_name_get(&nx_dns_client, (UCHAR*)uri_str, ip, NX_IP_PERIODIC_RATE, NX_IP_VERSION_V4);
    AZ_ULIB_THROW_IF_ERROR((status == NX_SUCCESS), AZ_ERROR_ULIB_SYSTEM);
  }
  AZ_ULIB_CATCH(...)
  {
  }

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_ulib_dm_blob_get_package_name(az_span url, az_span* name)
{
  AZ_ULIB_TRY
  {
    AZ_ULIB_THROW_IF_AZ_ERROR(split_url(url, NULL, NULL, NULL, NULL, NULL, name, NULL));
    AZ_ULIB_THROW_IF_ERROR((slice_next_char(*name, 0, '.', name) != -1), AZ_ERROR_UNEXPECTED_CHAR);
  }
  AZ_ULIB_CATCH(...)
  {
  }

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_ulib_dm_blob_get_size(az_span url, int32_t* size)
{
  (void)url;
  (void)size;
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

static az_result try_ustream()
{
  az_result result;

  az_ulib_ustream_data_cb* data_cb;
  if ((data_cb = (az_ulib_ustream_data_cb*)malloc(sizeof(az_ulib_ustream_data_cb))) != NULL)
  {
    az_ulib_ustream ustream_instance;
    if ((result = az_ulib_ustream_init(
             &ustream_instance,
             data_cb,
             free,
             (const uint8_t*)USTREAM_ONE_STRING,
             sizeof(USTREAM_ONE_STRING),
             NULL))
        != AZ_OK)
    {
      printf("Could not initialize ustream_instance\r\n");
    }
    else if ((result = print_ustream(&ustream_instance)) != AZ_OK)
    {
      az_ulib_ustream_dispose(&ustream_instance);
      printf("Could not print the original ustream_instance\r\n");
    }
    else if ((result = az_ulib_ustream_reset(&ustream_instance)) != AZ_OK)
    {
      az_ulib_ustream_dispose(&ustream_instance);
      printf("Could not reset ustream_instance\r\n");
    }
    else
    {
      az_ulib_ustream ustream_instance_split;

      if ((result
           = az_ulib_ustream_split(&ustream_instance, &ustream_instance_split, SPLIT_POSITION))
          != AZ_OK)
      {
        printf("Could not split ustream_instance\r\n");
      }
      else
      {
        if ((result = print_ustream(&ustream_instance)) != AZ_OK)
        {
          printf("Could not print the split ustream_instance\r\n");
        }
        else if ((result = print_ustream(&ustream_instance_split)) != AZ_OK)
        {
          printf("Could not print ustream_instance_split\r\n");
        }

        if ((result = az_ulib_ustream_dispose(&ustream_instance_split)) != AZ_OK)
        {
          printf("Could not dispose of ustream_instance_split\r\n");
        }
      }

      if ((result = az_ulib_ustream_dispose(&ustream_instance)) != AZ_OK)
      {
        printf("Could not dispose of ustream_instance\r\n");
      }
    }
  }
  else
  {
    result = AZ_ERROR_OUT_OF_MEMORY;
  }

  return result;
}

static az_result copy_blob_to_flash(NXD_ADDRESS* ip, CHAR* resource, CHAR* host, void* address)
{
  (void)address;

  NX_WEB_HTTP_CLIENT http_client;
  NX_PACKET* packet_ptr = NULL;
  UINT nx_status;
  HAL_StatusTypeDef hal_status;
  uint8_t download_complete = 0;
  az_result result;

  if ((result = try_ustream()) != AZ_OK)
  {
    printf("shit\r\n");
  }
  
  if((nx_status = blob_client_init(ip, &http_client, packet_ptr, resource, host)) == NX_SUCCESS)
  {
    // send request
    if ((result = blob_client_request_send(&http_client, DCF_WAIT_TIME)) == AZ_OK)
    {
      // grab first chunk
      if ((result = blob_client_grab_chunk(&http_client, &packet_ptr, &download_complete)) == AZ_OK)
      {
        // save pointer to first chunk
        az_span data = az_span_create(packet_ptr->nx_packet_prepend_ptr, packet_ptr->nx_packet_length);

        // calculate destination pointer
        uint8_t* dest_ptr = (uint8_t*)(address);

        // grab package size and round up to nearest 2KB (0x0800)
        uint32_t package_size = (uint32_t)http_client.nx_web_http_client_total_receive_bytes;
        if ((package_size & 0x07FF) != 0x000)
        {
          package_size = (package_size & 0xFFFFF800) + 0x0800;
        }

        // erase flash
        if ((hal_status = internal_flash_erase((UCHAR*)address, package_size)) == HAL_OK)
        {
          // write first chunk to flash
          if ((hal_status = internal_flash_write(dest_ptr, az_span_ptr(data), az_span_size(data))) == HAL_OK)
          {
            // if there are more chunks to store, loop over them until done
            while (!download_complete)
            {
              // grab next chunk
              if ((result = blob_client_grab_chunk(&http_client, &packet_ptr, &download_complete)) == AZ_OK)
              {
                // increase destination pointer by size of last chunk
                dest_ptr += az_span_size(data);

                // save pointer to chunk
                data = az_span_create(packet_ptr->nx_packet_prepend_ptr, packet_ptr->nx_packet_length);
                
                // call store to flash
                if ((hal_status = internal_flash_write(dest_ptr, az_span_ptr(data), az_span_size(data))) !=
                    HAL_OK)
                {
                  break;
                }
              }
            }
          }
        }
      }
    }

    result = blob_client_dispose(&http_client, &packet_ptr);
  }

  if (hal_status != HAL_OK)
  {
    return result_from_hal_status(hal_status);
  }
  return result;
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
  AZ_ULIB_CATCH(...)
  {
  }

  return AZ_ULIB_TRY_RESULT;
}
