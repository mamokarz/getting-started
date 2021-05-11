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

#define             DNS_SERVER_ADDRESS IP_ADDRESS(192,168,1,1)
#define             DCF_PACKET_SIZE  (NX_WEB_HTTP_CLIENT_MIN_PACKET_SIZE * 2)
#define             DCF_PACKET_COUNT 5
#define             DCF_POOL_SIZE    ((DCF_PACKET_SIZE + sizeof(NX_PACKET)) * DCF_PACKET_COUNT)
static UCHAR        dcf_ip_pool[DCF_POOL_SIZE]; // DCF

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
  az_span* container, 
  az_span* directories, 
  az_span* file_name, 
  az_span* sas_token)
{
  AZ_ULIB_TRY
  {
    int32_t next;

    /* Get protocol http:// or http:// */
    AZ_ULIB_THROW_IF_ERROR(((next = az_span_find(url, AZ_SPAN_FROM_STR("://"))) != -1), AZ_ERROR_UNEXPECTED_CHAR);
    if(protocol != NULL)
    {
      *protocol = az_span_slice(url, 0, next);
    }

    /* Get address. */
    AZ_ULIB_THROW_IF_ERROR(((next = slice_next_char(url, next + 3, '/', address)) != -1), AZ_ERROR_UNEXPECTED_CHAR);

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

/* Setup a callback to print out header information as it is processed. */
VOID http_response_callback(NX_WEB_HTTP_CLIENT *client_ptr, CHAR *field_name,
    UINT field_name_length, CHAR *field_value,
    UINT field_value_length)
{
    CHAR name[100];
    CHAR value[100];
    memset(name, 0, sizeof(name));

    memset(value, 0, sizeof(value));

    strncpy(name, field_name, field_name_length);
    strncpy(value, field_value, field_value_length);

    printf("Received header: \n");
    printf("\tField name: %s (%d bytes)\n", name, field_name_length);
    printf("\tValue: %s (%d bytes)\n\n", value, field_value_length);
}

AZ_NODISCARD az_result _az_ulib_dm_blob_get_package_name(az_span url, az_span* name)
{
  AZ_ULIB_TRY
  {
    AZ_ULIB_THROW_IF_AZ_ERROR(split_url(url, NULL, NULL, NULL, NULL, name, NULL));
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

  AZ_ULIB_TRY
  {
    az_span uri = AZ_SPAN_EMPTY;
    NXD_ADDRESS ip;

    AZ_ULIB_THROW_IF_AZ_ERROR(split_url(url, NULL, &uri, NULL, NULL, NULL, NULL));
    AZ_ULIB_THROW_IF_AZ_ERROR(get_ip_from_uri(uri, &ip)); 
    
    // AZ_ULIB_THROW_IF_ERROR(
    //     (nx_packet_pool_create(
    //         &client_pool, 
    //         "DCF Packet Pool", 
    //         DCF_PACKET_SIZE, 
    //         dcf_ip_pool, 
    //         DCF_POOL_SIZE) == NX_SUCCESS),
    //     AZ_ERROR_ULIB_SYSTEM);

    // AZ_ULIB_THROW_IF_ERROR((nx_web_http_client_create(&http_client, "HTTP Client", &nx_ip,
		// 			&client_pool, 600) == NX_SUCCESS), AZ_ERROR_ULIB_SYSTEM);

    // char resource_str[300];
    // az_span_to_str(resource_str, sizeof(resource_str), url);

    // /* Assign the callback to the HTTP client instance. */
    // AZ_ULIB_THROW_IF_ERROR(
    //     (nx_web_http_client_response_header_callback_set(&http_client, 
    //         http_response_callback) == NX_SUCCESS), 
    //     AZ_ERROR_ULIB_SYSTEM);

    // /* Start a GET operation to get a response from the HTTP server. */
    // AZ_ULIB_THROW_IF_ERROR(
    //     (nx_web_http_client_get_start(&http_client, &ip, NX_WEB_HTTP_SERVER_PORT,
    //                  resource_str, "windows.net", NX_NULL, NX_NULL, 500) == NX_SUCCESS), 
    //     AZ_ERROR_ULIB_SYSTEM)

  } AZ_ULIB_CATCH(...) {}

  nx_web_http_client_delete(&http_client);
  nx_packet_pool_delete(&client_pool);

  return AZ_ULIB_TRY_RESULT;
}

