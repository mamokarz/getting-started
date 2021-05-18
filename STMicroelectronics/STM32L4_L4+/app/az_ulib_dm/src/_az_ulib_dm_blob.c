// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "_az_ulib_dm_blob.h"
#include "az_ulib_result.h"
#include "azure/az_core.h"
#include "nx_api.h"
#include "nxd_dns.h"
#include "nx_web_http_client.h"
#include "stm_networking.h"
#include "nx_wifi.h"
#include "wifi.h"
#include "stm32l475_flash_driver.h"

#include <stdint.h>
#include <stdbool.h>

#define             USER_AGENT_NAME     "User-Agent: "
#define             USER_AGENT_VALUE    "Azure RTOS Device (STM32)"
#define             DCF_WAIT_TIME       600

static int32_t slice_next_char(az_span span, int32_t start, uint8_t c, az_span* slice)
{
  int32_t size = az_span_size(span);
  uint8_t* buf = az_span_ptr(span);
  int32_t end = start;
  while(end < size)
  {
    if(buf[end] == c)
    {
      if(slice != NULL)
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
    if(protocol != NULL)
    {
      *protocol = az_span_slice(url, 0, next);
    }

    /* Get address. */
    AZ_ULIB_THROW_IF_ERROR(
        ((next = slice_next_char(url, next + 3, '/', address)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

    /* Get resource span. */
    if(resource != NULL)
    {
      *resource = az_span_slice_to_end(url, next + 1);
    }

    /* Get container name. */
    AZ_ULIB_THROW_IF_ERROR(
        ((next = slice_next_char(url, next + 1, '/', container)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

    // Get directories, if exist. */
    // TODO: add code to read directories here.

    // Get file_name. */
    AZ_ULIB_THROW_IF_ERROR(
        ((next = slice_next_char(url, next + 1, '?', file_name)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

    // Get sas_token. */
    if(sas_token != NULL) 
    {
      *sas_token = az_span_slice_to_end(url, next + 1);
    }
  } AZ_ULIB_CATCH(...) 
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
    UINT status = nxd_dns_host_by_name_get(
            &nx_dns_client,
            (UCHAR*)uri_str, 
            ip, 
            NX_IP_PERIODIC_RATE, 
            NX_IP_VERSION_V4);
    AZ_ULIB_THROW_IF_ERROR((status == NX_SUCCESS), AZ_ERROR_ULIB_SYSTEM);

  } AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_ulib_dm_blob_get_package_name(az_span url, az_span* name)
{
  AZ_ULIB_TRY
  {
    AZ_ULIB_THROW_IF_AZ_ERROR(split_url(url, NULL, NULL, NULL, NULL, NULL, name, NULL));
    AZ_ULIB_THROW_IF_ERROR(
        (slice_next_char(*name, 0, '.', name) != -1), AZ_ERROR_UNEXPECTED_CHAR);
  } AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_ulib_dm_blob_get_size(az_span url, int32_t* size)
{
  (void)url;
  (void)size;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

static az_result result_from_nx_status(UINT nx_status)
{
  switch(nx_status)
  {
    case NX_SUCCESS:              return AZ_OK;
    case NX_WEB_HTTP_POOL_ERROR:  return AZ_ERROR_NOT_ENOUGH_SPACE;
    case NX_NO_PACKET:            return AZ_ERROR_NOT_ENOUGH_SPACE;
    case NX_INVALID_PARAMETERS:   return AZ_ERROR_NOT_SUPPORTED;
    case NX_OPTION_ERROR:         return AZ_ERROR_ARG;
    case NX_PTR_ERROR:            return AZ_ERROR_ARG;
    case NX_CALLER_ERROR:         return AZ_ERROR_ARG;
    case NX_WEB_HTTP_NOT_READY:   return AZ_ERROR_ULIB_BUSY;
    case NX_WAIT_ABORTED:         return AZ_ERROR_ULIB_SYSTEM;  //should be AZ_ERROR_ULIB_TIME_OUT
                                                                // or AZ_ERROR_ULIB_ABORTED
    default:                      return AZ_ERROR_ULIB_SYSTEM; 
  }
}

static az_result result_from_hal_status(HAL_StatusTypeDef status)
{
  switch(status)
  {
    case HAL_OK:                  return AZ_OK;
    case HAL_ERROR:               return AZ_ERROR_ULIB_SYSTEM;
    case HAL_BUSY:                return AZ_ERROR_ULIB_BUSY;
    case HAL_TIMEOUT:             return AZ_ERROR_ULIB_SYSTEM;  //should be AZ_ERROR_ULIB_TIME_OUT
    default:                      return AZ_ERROR_ULIB_SYSTEM; 
  }
}

static az_result copy_blob_to_flash(NXD_ADDRESS* ip, CHAR* resource, CHAR* host, void* address)
{
  (void)address;

  NX_WEB_HTTP_CLIENT http_client;
  NX_PACKET* packet_ptr = NULL;
  UINT nx_status;
  HAL_StatusTypeDef hal_status;

  if((nx_status = nx_web_http_client_create(&http_client, "HTTP Client", 
                              &nx_ip, &nx_pool, 1536)) == NX_SUCCESS)
  {
    if((nx_status = nx_web_http_client_connect(&http_client, ip, 
                              NX_WEB_HTTP_SERVER_PORT, DCF_WAIT_TIME)) == NX_SUCCESS)
    {
      // prepare flash by erasing pages
      // TODO: Replace package_size with actual package size fetched from blob
      unsigned int package_size =  0x00007000; // for the first package
      if ((hal_status = internal_flash_erase((UCHAR*)address, package_size)) == HAL_OK)
      {
        if((nx_status = nx_web_http_client_request_initialize(&http_client,
                                  NX_WEB_HTTP_METHOD_GET, /* GET, PUT, DELETE, POST, HEAD */
                                  resource, /* "resource" (usually includes auth headers)*/
                                  host, /* "host" */
                                  0, /* Used by PUT and POST */
                                  NX_FALSE, /* If true, input_size is ignored. */
                                  NX_NULL, /* "name" */
                                  NX_NULL, /* "password" */
                                  DCF_WAIT_TIME)) == NX_SUCCESS)
        {
          if((nx_status = nx_web_http_client_request_header_add(&http_client, 
                                    USER_AGENT_NAME, 
                                    sizeof(USER_AGENT_NAME) - 1, 
                                    USER_AGENT_VALUE, 
                                    sizeof(USER_AGENT_VALUE) - 1, 
                                    DCF_WAIT_TIME)) == NX_SUCCESS)
          {
            nx_status = nx_web_http_client_request_send(&http_client, DCF_WAIT_TIME);
            // Download destination pointer, increase after every packet is stored
            uint32_t total_downloaded_size = 0;
            while(nx_status == NX_SUCCESS)
            {
              nx_status = nx_web_http_client_response_body_get(&http_client, &packet_ptr, 500);
              if((nx_status == NX_SUCCESS) || (nx_status == NX_WEB_HTTP_GET_DONE))
              {
                az_span data = az_span_create(packet_ptr->nx_packet_prepend_ptr, 
                                              packet_ptr->nx_packet_length);

                // calculate destination pointer
                uint8_t* dest_ptr = (uint8_t*)(address) + total_downloaded_size;

                // call store to flash
                if((hal_status = internal_flash_write(
                                      dest_ptr, az_span_ptr(data), az_span_size(data))) != HAL_OK)
                {
                  nx_status = nx_packet_release(packet_ptr);
                  break;
                }

                // update total package size
                total_downloaded_size += az_span_size(data);

                nx_status = nx_packet_release(packet_ptr);
              }
            } 

            if(nx_status == NX_WEB_HTTP_GET_DONE)
            {
              nx_status = NX_SUCCESS;
            }            
          }
        }
      }
    }
    nx_status = nx_web_http_client_delete(&http_client);
  }

  if(nx_status != NX_SUCCESS)
  {
    return result_from_nx_status(nx_status);
  }
  if(hal_status != HAL_OK)
  {
    return result_from_hal_status(hal_status);
  }
  return AZ_OK;
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

  } AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}
