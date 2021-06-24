#include "_az_nx_blob_client.h"

#define USER_AGENT_NAME "User-Agent: "
#define USER_AGENT_VALUE "Azure RTOS Device (STM32)"

static az_result result_from_nx_status(UINT nx_status)
{
  switch (nx_status)
  {
    case NX_SUCCESS:
      return AZ_OK;
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
    case NX_WAIT_ABORTED:
      return AZ_ERROR_ULIB_SYSTEM; // should be AZ_ERROR_ULIB_TIME_OUT
                                   // or AZ_ERROR_ULIB_ABORTED
    default:
      return AZ_ERROR_ULIB_SYSTEM;
  }
}

az_result az_nx_blob_client_init(
    NXD_ADDRESS* ip,
    NX_WEB_HTTP_CLIENT* http_client,
    NX_PACKET* packet_ptr,
    CHAR* resource,
    CHAR* host,
    ULONG wait_option)
{
  UINT nx_status;
  az_result result = AZ_OK;

  // create http blob client
  if ((nx_status = nx_web_http_client_create(http_client, "HTTP Client", &nx_ip, &nx_pool, 1536))
      != NX_SUCCESS)
  {
    result = result_from_nx_status(nx_status);
  }

  // connect to server
  else if (
      (nx_status
       = nx_web_http_client_connect(http_client, ip, NX_WEB_HTTP_SERVER_PORT, wait_option))
      != NX_SUCCESS)
  {
    result = result_from_nx_status(nx_status);
  }

  // initialize get request
  else if (
      (nx_status = nx_web_http_client_request_initialize(
           http_client,
           NX_WEB_HTTP_METHOD_GET, /* GET, PUT, DELETE, POST, HEAD */
           resource, /* "resource" (usually includes auth headers)*/
           host, /* "host" */
           0, /* Used by PUT and POST */
           NX_FALSE, /* If true, input_size is ignored. */
           NX_NULL, /* "name" */
           NX_NULL, /* "password" */
           wait_option))
      != NX_SUCCESS)
  {
    result = result_from_nx_status(nx_status);
  }

  // add user agent
  else if (
      (nx_status = nx_web_http_client_request_header_add(
           http_client,
           USER_AGENT_NAME,
           sizeof(USER_AGENT_NAME) - 1,
           USER_AGENT_VALUE,
           sizeof(USER_AGENT_VALUE) - 1,
           wait_option))
      != NX_SUCCESS)
  {
    result = result_from_nx_status(nx_status);
  }

  return result;
}

az_result az_nx_blob_client_grab_chunk(
    NX_WEB_HTTP_CLIENT* http_client_ptr,
    NX_PACKET** packet_ptr_ref)
{
  UINT nx_status;

  // release packet_ptr from last nx_web_htt_client_response_body_get()
  nx_packet_release(*packet_ptr_ref);

  // grab next chunk
  nx_status = nx_web_http_client_response_body_get(http_client_ptr, packet_ptr_ref, 500);

  // done processing response body
  if (nx_status == NX_WEB_HTTP_GET_DONE)
  {
    return AZ_ULIB_EOF;
  }
  // there was an error
  else if (nx_status != NX_SUCCESS)
  {
    return result_from_nx_status(nx_status);
  }
  // successfully grabbed chunk and there is more to grab
  else
  {
    return AZ_OK;
  }
}

az_result az_nx_blob_client_request_send(NX_WEB_HTTP_CLIENT* http_client_ptr, ULONG wait_option)
{
  UINT nx_status;

  if ((nx_status = nx_web_http_client_request_send(http_client_ptr, wait_option)) != NX_SUCCESS)
  {
    return result_from_nx_status(nx_status);
  }

  return AZ_OK;
}

az_result az_nx_blob_client_dispose(NX_WEB_HTTP_CLIENT* http_client_ptr, NX_PACKET** packet_ptr_ref)
{
  UINT nx_status;

  nx_packet_release(*packet_ptr_ref);

  if ((nx_status = nx_web_http_client_delete(http_client_ptr)) != NX_SUCCESS)
  {
    return result_from_nx_status(nx_status);
  }

  return AZ_OK;
}
