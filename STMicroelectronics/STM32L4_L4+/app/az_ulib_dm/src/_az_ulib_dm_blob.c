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

// #define USER_BUFFER_SIZE 5
// #define SPLIT_POSITION 12

// static const char USTREAM_ONE_STRING[] = "Split BeforeSplit After";

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

// static az_result print_ustream(az_ulib_ustream* ustream)
// {
//   az_result result;
//   size_t returned_size;
//   uint8_t user_buf[USER_BUFFER_SIZE] = { 0 };

//   // Read ustream until receive AZIOT_ULIB_EOF
//   (void)printf("\r\n------printing the ustream------\r\n");
//   while ((result = az_ulib_ustream_read(ustream, user_buf, USER_BUFFER_SIZE - 1, &returned_size))
//          == AZ_OK)
//   {
//     user_buf[returned_size] = '\0';
//     (void)printf("%s", user_buf);
//   }
//   (void)printf("\r\n-----------end of ustream------------\r\n\r\n");

//   // Change return to AZ_OK if last returned value was AZ_ULIB_EOF
//   if (result == AZ_ULIB_EOF)
//   {
//     result = AZ_OK;
//   }
//   return result;
// }


// static az_result try_ustream()
// {
//   az_result result;

//   az_ulib_ustream_data_cb* data_cb;
//   if ((data_cb = (az_ulib_ustream_data_cb*)malloc(sizeof(az_ulib_ustream_data_cb))) != NULL)
//   {
//     az_ulib_ustream ustream_instance;
//     if ((result = az_ulib_ustream_init(
//              &ustream_instance,
//              data_cb,
//              free,
//              (const uint8_t*)USTREAM_ONE_STRING,
//              sizeof(USTREAM_ONE_STRING),
//              NULL))
//         != AZ_OK)
//     {
//       printf("Could not initialize ustream_instance\r\n");
//     }
//     else if ((result = print_ustream(&ustream_instance)) != AZ_OK)
//     {
//       az_ulib_ustream_dispose(&ustream_instance);
//       printf("Could not print the original ustream_instance\r\n");
//     }
//     else if ((result = az_ulib_ustream_reset(&ustream_instance)) != AZ_OK)
//     {
//       az_ulib_ustream_dispose(&ustream_instance);
//       printf("Could not reset ustream_instance\r\n");
//     }
//     else
//     {
//       az_ulib_ustream ustream_instance_split;

//       if ((result
//            = az_ulib_ustream_split(&ustream_instance, &ustream_instance_split, SPLIT_POSITION))
//           != AZ_OK)
//       {
//         printf("Could not split ustream_instance\r\n");
//       }
//       else
//       {
//         if ((result = print_ustream(&ustream_instance)) != AZ_OK)
//         {
//           printf("Could not print the split ustream_instance\r\n");
//         }
//         else if ((result = print_ustream(&ustream_instance_split)) != AZ_OK)
//         {
//           printf("Could not print ustream_instance_split\r\n");
//         }

//         if ((result = az_ulib_ustream_dispose(&ustream_instance_split)) != AZ_OK)
//         {
//           printf("Could not dispose of ustream_instance_split\r\n");
//         }
//       }

//       if ((result = az_ulib_ustream_dispose(&ustream_instance)) != AZ_OK)
//       {
//         printf("Could not dispose of ustream_instance\r\n");
//       }
//     }
//   }
//   else
//   {
//     result = AZ_ERROR_OUT_OF_MEMORY;
//   }

//   return result;
// }

az_blob_ustream_cb blob_ustream_cb;

// az_retusn write_ustream_to_flash(az_ulib_ustream stream, void* address)
// {
//   uint32_t ustream_buffer_size = 128;
//   uint8_t ustream_buffer[128] = {0};
//   az_result result;
//   size_t size;
//   do
//   {
//     result = az_ulib_ustream_read(ustram, ustream_buffer, sizeof(ustream_buffer), &size);
//     if(result == AZ_OK)
//     {
//       /* Wrtie ustream_buffer/size to flash.*/

//       /* address += size;*/
//     }
//   } while (result!= AZ_OK);
//   if(result == AZ_ULIB_EOF)
//   {
//     result = AZ_OK;
//   }
//   return result;
// }

static az_result copy_blob_to_flash(NXD_ADDRESS* ip, CHAR* resource, CHAR* host, void* address)
{
  (void)address;

  // nx
  az_blob_ustream_interface blob_ustream_interface = &blob_ustream_cb;
  HAL_StatusTypeDef hal_status;

  az_result result;

  az_ulib_ustream_data_cb* data_cb;
  az_ulib_ustream ustream_instance;
  uint32_t ustream_buffer_size = 128;
  uint8_t ustream_buffer[128] = {0};
  size_t returned_size;
  
  // if ((result = try_ustream()) != AZ_OK)
  // {
  //   printf("demo failed\r\n");
  // }

  // allocate memory for data control block
  if ((data_cb = (az_ulib_ustream_data_cb*)malloc(sizeof(az_ulib_ustream_data_cb))) == NULL)
  {
    printf("Could not malloc data control block\r\n");
  }

  // create ustream and blob client
  else if((result = create_ustream_from_blob(data_cb, &ustream_instance, ustream_buffer, ustream_buffer_size, 
                                            blob_ustream_interface, ip, resource, host)) == AZ_OK)
  {

// wrtie_ustrem_to_flash(ustream_instance, address);

    // calculate destination pointer
    blob_ustream_interface->destination_ptr = (uint8_t*)(address);

    // erase flash
    if ((hal_status = internal_flash_erase((UCHAR*)address, ustream_instance.length)) == HAL_OK)
    {
      // write first chunk to flash
      if ((hal_status = internal_flash_write(blob_ustream_interface->destination_ptr, az_span_ptr(blob_ustream_interface->data), 
          az_span_size(blob_ustream_interface->data))) == HAL_OK)
      {
        // if there are more chunks to store, loop over them until done
        while (!blob_ustream_interface->download_complete)
        {
          // grab next chunk
          if ((result = az_ulib_ustream_read(&ustream_instance, ustream_buffer, sizeof(ustream_buffer) - 1, &returned_size))
              == AZ_OK)
          {
            // call store to flash
            if ((hal_status = internal_flash_write(blob_ustream_interface->destination_ptr, az_span_ptr(blob_ustream_interface->data), 
                az_span_size(blob_ustream_interface->data))) != HAL_OK)
            {
              break;
            }
          }
        }
      }
    }

    result = ustream_blob_client_dispose(&ustream_instance, blob_ustream_interface->http_client_ptr, 
                                          &blob_ustream_interface->packet_ptr);
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
