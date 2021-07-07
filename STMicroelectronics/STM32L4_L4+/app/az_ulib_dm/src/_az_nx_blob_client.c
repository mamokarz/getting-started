// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "_az_nx_blob_client.h"
#include "nx_wifi.h"
#include "stm_networking.h"
#include "wifi.h"

#define USER_AGENT_NAME "User-Agent: "
#define USER_AGENT_VALUE "Azure RTOS Device (STM32)"
#define AZ_ULIB_BLOB_CLIENT_WINDOW_SIZE 1536

static az_result result_from_nx_status(UINT nx_status)
{
  switch (nx_status)
  {
    case NX_SUCCESS:
      return AZ_OK;
    case NX_WEB_HTTP_GET_DONE:
      return AZ_ULIB_EOF;
    case NX_WEB_HTTP_POOL_ERROR:
      return AZ_ERROR_NOT_ENOUGH_SPACE;
    case NX_NO_PACKET:
      return AZ_ERROR_NOT_ENOUGH_SPACE;
    case NX_INVALID_PARAMETERS:
      return AZ_ERROR_NOT_SUPPORTED;
    case NX_OPTION_ERROR:
      return AZ_ERROR_ARG;
    case NX_PTR_ERROR:
      return AZ_ERROR_ARG;
    case NX_CALLER_ERROR:
      return AZ_ERROR_ARG;
    case NX_WEB_HTTP_NOT_READY:
      return AZ_ERROR_ULIB_BUSY;
    case NX_WEB_HTTP_STATUS_CODE_FORBIDDEN:
      return AZ_ERROR_ULIB_SYSTEM; // should be AZ_ERROR_ULIB_FORBIDDEN
    case NX_WAIT_ABORTED:
      return AZ_ERROR_ULIB_SYSTEM; // should be AZ_ERROR_ULIB_TIME_OUT
                                   // or AZ_ERROR_ULIB_ABORTED
    default:
      return AZ_ERROR_ULIB_SYSTEM;
  }
}

AZ_NODISCARD az_result
_az_nx_blob_client_init(NX_WEB_HTTP_CLIENT* http_client, NXD_ADDRESS* ip, ULONG wait_option)
{
  AZ_ULIB_TRY
  {
    // create http blob client
    AZ_ULIB_THROW_IF_AZ_ERROR(result_from_nx_status(nx_web_http_client_create(
        http_client, "HTTP Client", &nx_ip, &nx_pool, AZ_ULIB_BLOB_CLIENT_WINDOW_SIZE)));

    // connect to server
    AZ_ULIB_THROW_IF_AZ_ERROR(result_from_nx_status(
        nx_web_http_client_connect(http_client, ip, NX_WEB_HTTP_SERVER_PORT, wait_option)));
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_nx_blob_client_grab_chunk(
    NX_WEB_HTTP_CLIENT* http_client,
    NX_PACKET** packet_ptr_ref,
    ULONG wait_option)
{
  AZ_ULIB_TRY
  {
    // release packet_ptr from last nx_web_http_client_response_body_get()
    if (*packet_ptr_ref != NX_NULL)
    {
      AZ_ULIB_THROW_IF_AZ_ERROR(result_from_nx_status(nx_packet_release(*packet_ptr_ref)));
    }

    // grab next chunk
    AZ_ULIB_THROW_IF_AZ_ERROR(result_from_nx_status(
        nx_web_http_client_response_body_get(http_client, packet_ptr_ref, wait_option)));
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_nx_blob_client_request_send(
    NX_WEB_HTTP_CLIENT* http_client,
    int8_t* resource,
    int8_t* host,
    ULONG wait_option)
{
  AZ_ULIB_TRY
  {
    // initialize get request
    AZ_ULIB_THROW_IF_AZ_ERROR(result_from_nx_status(nx_web_http_client_request_initialize(
        http_client,
        NX_WEB_HTTP_METHOD_GET, /* GET, PUT, DELETE, POST, HEAD */
        (CHAR*)resource, /* "resource" (usually includes auth headers)*/
        (CHAR*)host, /* "host" */
        0, /* Used by PUT and POST */
        NX_FALSE, /* If true, input_size is ignored. */
        NX_NULL, /* "name" */
        NX_NULL, /* "password" */
        wait_option)));

    // add user agent
    AZ_ULIB_THROW_IF_AZ_ERROR(result_from_nx_status(nx_web_http_client_request_header_add(
        http_client,
        USER_AGENT_NAME,
        sizeof(USER_AGENT_NAME) - 1,
        USER_AGENT_VALUE,
        sizeof(USER_AGENT_VALUE) - 1,
        wait_option)));

    // send request
    AZ_ULIB_THROW_IF_AZ_ERROR(
        result_from_nx_status(nx_web_http_client_request_send(http_client, wait_option)));
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

AZ_NODISCARD az_result _az_nx_blob_client_dispose(NX_WEB_HTTP_CLIENT* http_client, NX_PACKET** packet_ptr_ref)
{
  AZ_ULIB_TRY
  {
    // release packet
    AZ_ULIB_THROW_IF_AZ_ERROR(result_from_nx_status(nx_packet_release(*packet_ptr_ref)));

    // dispose of http client
    AZ_ULIB_THROW_IF_AZ_ERROR(result_from_nx_status(nx_web_http_client_delete(http_client)));
  }
  AZ_ULIB_CATCH(...){}

  return AZ_ULIB_TRY_RESULT;
}
