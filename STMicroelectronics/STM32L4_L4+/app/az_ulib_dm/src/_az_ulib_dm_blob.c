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

#include <stdint.h>
#include <stdbool.h>

#define             USER_AGENT_NAME     "User-Agent: "
#define             USER_AGENT_VALUE    "Azure RTOS Device (STM32)"
#define             DNS_SERVER_ADDRESS  IP_ADDRESS(192,168,1,1)
#define             DCF_PACKET_SIZE     (NX_WEB_HTTP_CLIENT_MIN_PACKET_SIZE * 2)
#define             DCF_PACKET_COUNT    2
#define             DCF_POOL_SIZE       ((DCF_PACKET_SIZE + sizeof(NX_PACKET)) * DCF_PACKET_COUNT)
static UCHAR        dcf_ip_pool[DCF_POOL_SIZE]; // DCF
static CHAR         receive_buffer[DCF_PACKET_SIZE + 1];

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
    AZ_ULIB_THROW_IF_ERROR(((next = az_span_find(url, AZ_SPAN_FROM_STR("://"))) != -1), AZ_ERROR_UNEXPECTED_CHAR);
    if(protocol != NULL)
    {
      *protocol = az_span_slice(url, 0, next);
    }

    /* Get address. */
    AZ_ULIB_THROW_IF_ERROR(((next = slice_next_char(url, next + 3, '/', address)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

    /* Get resource span. */
    if(resource != NULL)
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
  NX_DNS client_dns;
  NX_PACKET* packet_ptr;
  NX_PACKET_POOL client_pool;

  AZ_ULIB_TRY
  {
    /* Enable UDP traffic because DNS is a UDP based protocol.  */
    AZ_ULIB_THROW_IF_ERROR((nx_udp_enable(&nx_ip) == NX_SUCCESS), AZ_ERROR_ITEM_NOT_FOUND);

    /* Create a DNS instance for the Client.  Note this function will create
       the DNS Client packet pool for creating DNS message packets intended
       for querying its DNS server. */
    AZ_ULIB_THROW_IF_ERROR(
        (nx_dns_create(&client_dns, &nx_ip, (UCHAR *)"DNS Client") == NX_SUCCESS), 
        AZ_ERROR_ULIB_SYSTEM);
    AZ_ULIB_THROW_IF_ERROR(
        (nx_packet_pool_create(
            &client_pool, 
            "DCF Packet Pool", 
            DCF_PACKET_SIZE, 
            dcf_ip_pool, 
            DCF_POOL_SIZE) == NX_SUCCESS),
        AZ_ERROR_ULIB_SYSTEM);
    
    /* Use the packet pool created above which has appropriate payload size
       for DNS messages. */
    AZ_ULIB_THROW_IF_ERROR(
        (nx_dns_packet_pool_set(&client_dns, &client_pool) == NX_SUCCESS), 
        AZ_ERROR_ULIB_SYSTEM);
    AZ_ULIB_THROW_IF_ERROR(
        (nx_packet_allocate(
            &client_pool, &packet_ptr, NX_TCP_PACKET, NX_WAIT_FOREVER) == NX_SUCCESS), 
        AZ_ERROR_ULIB_SYSTEM);

    /* Add an IPv4 server address to the Client list. */
    AZ_ULIB_THROW_IF_ERROR(
        (nx_dns_server_add(&client_dns, DNS_SERVER_ADDRESS) == NX_SUCCESS), 
        AZ_ERROR_ULIB_SYSTEM);

    /* Get the IPv4 for the URI using DNS. */
    char uri_str[50];
    az_span_to_str(uri_str, sizeof(uri_str), uri);
    AZ_ULIB_THROW_IF_ERROR(
        (nxd_dns_host_by_name_get(
            &client_dns,
            (UCHAR*)uri_str, 
            ip, 
            NX_IP_PERIODIC_RATE, 
            NX_IP_VERSION_V4) == NX_SUCCESS), 
        AZ_ERROR_ULIB_SYSTEM);

  } AZ_ULIB_CATCH(...) {}

  nx_dns_delete(&client_dns);
  nx_packet_pool_delete(&client_pool);

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_ulib_dm_blob_get_package_name(az_span url, az_span* name)
{
  AZ_ULIB_TRY
  {
    AZ_ULIB_THROW_IF_AZ_ERROR(split_url(url, NULL, NULL, NULL, NULL, NULL, name, NULL));
    AZ_ULIB_THROW_IF_ERROR((slice_next_char(*name, 0, '.', name) != -1), AZ_ERROR_UNEXPECTED_CHAR);
  } AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_ulib_dm_blob_get_size(az_span url, int32_t* size)
{
  (void)url;
  (void)size;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result _az_ulib_dm_blob_download(void* address, az_span url)
{
  (void)address;
  NX_WEB_HTTP_CLIENT http_client;
  NX_PACKET_POOL client_pool;
  NX_PACKET* packet_ptr;
  bool shall_delete_http_client = false;
  bool shall_delete_client_pool = false;

  AZ_ULIB_TRY
  {
    az_span uri = AZ_SPAN_EMPTY;
    NXD_ADDRESS ip;
    az_span resource;
    
    AZ_ULIB_THROW_IF_AZ_ERROR(split_url(url, NULL, &uri, &resource, NULL, NULL, NULL, NULL));
    AZ_ULIB_THROW_IF_AZ_ERROR(get_ip_from_uri(uri, &ip)); 
    
    AZ_ULIB_THROW_IF_ERROR(
        (nx_packet_pool_create(
            &client_pool, 
            "DCF Packet Pool", 
            DCF_PACKET_SIZE, 
            dcf_ip_pool, 
            DCF_POOL_SIZE) == NX_SUCCESS),
        AZ_ERROR_ULIB_SYSTEM);
    shall_delete_client_pool = true;

    AZ_ULIB_THROW_IF_ERROR(
        (nx_web_http_client_create(&http_client, "HTTP Client", &nx_ip,
					&client_pool, 600) == NX_SUCCESS), AZ_ERROR_ULIB_SYSTEM);
    shall_delete_http_client = true;

    /* Allocate memory for NX_PACKET */
    nx_packet_allocate(&client_pool, &packet_ptr, NX_TCP_PACKET, NX_WAIT_FOREVER);

    /* Connect to Server */
    AZ_ULIB_THROW_IF_ERROR(
        (nx_web_http_client_connect(&http_client, 
                                    &ip, 
                                    NX_WEB_HTTP_SERVER_PORT,
                                    NX_WAIT_FOREVER) == NX_SUCCESS), 
        AZ_ERROR_ULIB_SYSTEM);

    /* Initialize HTTP request. */
    char host[50];
    az_span_to_str(host, sizeof(host), uri);
    char resource_str[200];
    az_span_to_str(resource_str, sizeof(resource_str), resource);
    AZ_ULIB_THROW_IF_ERROR(
        (nx_web_http_client_request_initialize(&http_client,
                                              NX_WEB_HTTP_METHOD_GET, /* GET, PUT, DELETE, POST, HEAD */
                                              resource_str, /* "resource" (usually includes auth headers)*/
                                              host, /* "host" */
                                              0, /* Used by PUT and POST */
                                              NX_FALSE, /* If true, input_size is ignored. */
                                              NX_NULL, /* "name" */
                                              NX_NULL, /* "password" */
                                              NX_WAIT_FOREVER) == NX_SUCCESS),
        AZ_ERROR_ULIB_SYSTEM);

    /* Add User-Agent header */                       
    AZ_ULIB_THROW_IF_ERROR(
        (nx_web_http_client_request_header_add(&http_client, 
                                          USER_AGENT_NAME, 
                                          sizeof(USER_AGENT_NAME) - 1, 
                                          USER_AGENT_VALUE, 
                                          sizeof(USER_AGENT_VALUE) - 1, 
                                          NX_WAIT_FOREVER) == NX_SUCCESS),
        AZ_ERROR_ULIB_SYSTEM);

    /* Send GET request */
    AZ_ULIB_THROW_IF_ERROR(
        (nx_web_http_client_request_send(&http_client, 
                                        NX_WAIT_FOREVER) == NX_SUCCESS),
        AZ_ERROR_ULIB_SYSTEM);

    /* Receive response data from the server. Loop until all data is received. */
    printf("Received package:\r\n");
    UINT get_status = NX_SUCCESS;
    while(get_status == NX_SUCCESS)
    {
      get_status = nx_web_http_client_response_body_get(&http_client, &packet_ptr, 500);
      if((get_status == NX_SUCCESS) || (get_status == NX_WEB_HTTP_GET_DONE))
      {
        az_span data = az_span_create(
            packet_ptr->nx_packet_prepend_ptr, 
            packet_ptr->nx_packet_length);

        // TODO: Save `data` to the Flash instead of print it.
        // TODO: Remove all printf() from this function.
        // TODO: Delete `receive_buffer`. 
        az_span_to_str(receive_buffer, DCF_PACKET_SIZE + 1, data);
        printf("%s", receive_buffer);
        
        nx_packet_release(packet_ptr);
      }
    } 
    if(get_status != NX_WEB_HTTP_GET_DONE)
    {
      AZ_ULIB_THROW(AZ_ERROR_ULIB_SYSTEM);
    }
    printf("\r\nEnd of package\r\n");

  } AZ_ULIB_CATCH(...) {}

  if(shall_delete_http_client)
  {
    nx_web_http_client_delete(&http_client);
  }
  if(shall_delete_client_pool)
  {
    nx_packet_pool_delete(&client_pool);
  }

  return AZ_ULIB_TRY_RESULT;
}

