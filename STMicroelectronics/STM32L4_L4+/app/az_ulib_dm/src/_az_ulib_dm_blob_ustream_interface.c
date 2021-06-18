#include "_az_ulib_dm_blob_ustream_interface.h"
#include "az_ulib_port.h"
#include <azure/core/internal/az_precondition_internal.h>

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
  __pragma(warning(push));  \
  __pragma(warning(suppress: 6387));
#define RESUME_WARNINGS __pragma(warning(pop));
#endif // __clang__

#define USER_AGENT_NAME  "User-Agent: "
#define USER_AGENT_VALUE "Azure RTOS Device (STM32)"

NX_WEB_HTTP_CLIENT blob_http_client;

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


AZ_NODISCARD UINT blob_client_init(NXD_ADDRESS* ip, NX_WEB_HTTP_CLIENT* http_client, NX_PACKET* packet_ptr,
                                  CHAR* resource, CHAR* host)
{
  UINT nx_status; 

  // create http blob client
  if ((nx_status = nx_web_http_client_create(http_client, "HTTP Client", &nx_ip, &nx_pool, 1536)) != NX_SUCCESS)
  {
    printf("HTTP Client Create failed with nx_status = (%0x02)\r\n", nx_status);
  }

  // connect to server
  else if ((nx_status = nx_web_http_client_connect(http_client, ip, NX_WEB_HTTP_SERVER_PORT, DCF_WAIT_TIME)) !=
      NX_SUCCESS)
  {
    printf("HTTP Client Connect failed with nx_status = (%0x02)\r\n", nx_status);
  }

  // initialize get request
  else if ((nx_status = nx_web_http_client_request_initialize(http_client,
          NX_WEB_HTTP_METHOD_GET, /* GET, PUT, DELETE, POST, HEAD */
          resource,               /* "resource" (usually includes auth headers)*/
          host,                   /* "host" */
          0,                      /* Used by PUT and POST */
          NX_FALSE,               /* If true, input_size is ignored. */
          NX_NULL,                /* "name" */
          NX_NULL,                /* "password" */
          DCF_WAIT_TIME)) != NX_SUCCESS)
  {
    printf("HTTP Client Request Initialize failed with nx_status = (%0x02)\r\n", nx_status);
  }

  // add user agent
  else if ((nx_status = nx_web_http_client_request_header_add(http_client,
            USER_AGENT_NAME,
            sizeof(USER_AGENT_NAME) - 1,
            USER_AGENT_VALUE,
            sizeof(USER_AGENT_VALUE) - 1,
            DCF_WAIT_TIME)) != NX_SUCCESS)
  {
    printf("HTTP Client Request Header Add failed with nx_status = (%0x02)\r\n", nx_status);
  }

  return nx_status;
}

AZ_NODISCARD az_result blob_client_grab_chunk(NX_WEB_HTTP_CLIENT* http_client_ptr, NX_PACKET** packet_ptr_ref)
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

AZ_NODISCARD az_result blob_client_request_send(NX_WEB_HTTP_CLIENT* http_client_ptr, ULONG wait_option)
{
  UINT nx_status;
  
  if ((nx_status = nx_web_http_client_request_send(http_client_ptr, wait_option)) != NX_SUCCESS)
  {
    printf("HTTP Client request send fail with nx_status = (%0x02)\r\n", nx_status);
    return result_from_nx_status(nx_status);
  }

  return AZ_OK;
}

AZ_NODISCARD az_result blob_client_dispose(NX_WEB_HTTP_CLIENT* http_client_ptr, NX_PACKET** packet_ptr_ref)
{
  UINT nx_status;

  nx_packet_release(*packet_ptr_ref);

  if ((nx_status = nx_web_http_client_delete(http_client_ptr)) != NX_SUCCESS)
  {
    printf("HTTP Client delete fail with nx_status = (%0x02)\r\n", nx_status);
    return result_from_nx_status(nx_status);
  }

  return AZ_OK;
}

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
  AZ_ULIB_PORT_ATOMIC_INC_W(&(ustream_instance->control_block->ref_count));
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
  _az_PRECONDITION(AZ_ULIB_USTREAM_IS_TYPE_OF(ustream_instance, api));

  az_result result;

  offset_t inner_position = position - ustream_instance->offset_diff;

  if ((inner_position > (offset_t)(ustream_instance->length))
      || (inner_position < ustream_instance->inner_first_valid_position))
  {
    result = AZ_ERROR_ITEM_NOT_FOUND;
  }
  else
  {
    ustream_instance->inner_current_position = inner_position;
    result = AZ_OK;
  }

  return result;
}

static az_result concrete_reset(az_ulib_ustream* ustream_instance)
{
  _az_PRECONDITION(AZ_ULIB_USTREAM_IS_TYPE_OF(ustream_instance, api));

  ustream_instance->inner_current_position = ustream_instance->inner_first_valid_position;

  return AZ_OK;
}

/*
data is packet ptr 
calling this should copy packet_ptr to buffer
*/
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

  az_result result = AZ_OK;

  az_ulib_ustream_data_cb* control_block = ustream_instance->control_block;
  az_blob_http_cb* blob_http_cb = (az_blob_http_cb*)control_block->ptr;
  
  // keep track of our position in the local buffer
  size_t remain_size_local_buffer = buffer_length;

  // reset size
  *size = 0;

  // while we still have room left in the local buffer and we have not reached the end of the package
  do
  {
    // keep track of our position in the inner buffer (packer)
    size_t remain_size_inner_buffer = blob_http_cb->packet_ptr->nx_packet_length - (size_t)ustream_instance->offset_diff;

    // if we have not reached the end of the inner buffer, copy the next data to the local buffer
    if(remain_size_inner_buffer)
    {
      // size to copy is the smaller of the remaining size in the inner buffer or the local buffer
      size_t size_to_copy = remain_size_inner_buffer < remain_size_local_buffer ? 
                            remain_size_inner_buffer : remain_size_local_buffer;

      // copy data from inner buffer to local buffer
      IGNORE_MEMCPY_TO_NULL
      memcpy(
          buffer + (buffer_length - remain_size_local_buffer),
          blob_http_cb->packet_ptr->nx_packet_prepend_ptr + ustream_instance->offset_diff,
          size_to_copy);
      RESUME_WARNINGS
      
      // decrease the remaining size in our local buffer
      remain_size_local_buffer -= size_to_copy;

      // increase the passed size for the caller
      *size += size_to_copy;

      // increase offset to keep track of our position in the inner buffer 
      ustream_instance->offset_diff += size_to_copy;
    }

    // else we must grab another packet
    else
    {
      if ((result = blob_client_grab_chunk(blob_http_cb->http_client_ptr, &blob_http_cb->packet_ptr)) == AZ_OK)
      {
        // increase current position by size of last chunk
        ustream_instance->inner_current_position += ustream_instance->offset_diff;
        // reset current offset
        ustream_instance->offset_diff = 0;
      }
    } 
  } while (remain_size_local_buffer && (result == AZ_OK));

  

  // while write_buffer_offset > buffer_length
  /*
    if(remain_size_inner_buffer == 0 && remain_size_local_buffer > 0)
      result = blob_client_grab_chunk()
      if (result == AZ_ULIB_EOF)
        break;
    else
      break;
    
    size_to_copy = remain_size_inner_buffer (packet) < remain_size_local_buffer (local buffer) ? rmsib : 
    memcpy(local_buffer, packet + remain_Szie_inner_buffer, size_to_copy)
  */

  return result;
}

static az_result concrete_get_remaining_size(az_ulib_ustream* ustream_instance, size_t* const size)
{
  _az_PRECONDITION(AZ_ULIB_USTREAM_IS_TYPE_OF(ustream_instance, api));
  _az_PRECONDITION_NOT_NULL(size);

  *size = ustream_instance->length - ustream_instance->inner_current_position;

  return AZ_OK;
}

static az_result concrete_get_position(az_ulib_ustream* ustream_instance, offset_t* const position)
{
  _az_PRECONDITION(AZ_ULIB_USTREAM_IS_TYPE_OF(ustream_instance, api));
  _az_PRECONDITION_NOT_NULL(position);

  *position = ustream_instance->inner_current_position + ustream_instance->offset_diff;

  return AZ_OK;
}

static az_result concrete_release(az_ulib_ustream* ustream_instance, offset_t position)
{
  _az_PRECONDITION(AZ_ULIB_USTREAM_IS_TYPE_OF(ustream_instance, api));

  az_result result;

  offset_t inner_position = position - ustream_instance->offset_diff;

  if ((inner_position >= ustream_instance->inner_current_position)
      || (inner_position < ustream_instance->inner_first_valid_position))
  {
    result = AZ_ERROR_ARG;
  }
  else
  {
    ustream_instance->inner_first_valid_position = inner_position + (offset_t)1;
    result = AZ_OK;
  }

  return result;
}

static az_result concrete_clone(
    az_ulib_ustream* ustream_instance_clone,
    az_ulib_ustream* ustream_instance,
    offset_t offset)
{
  _az_PRECONDITION(AZ_ULIB_USTREAM_IS_TYPE_OF(ustream_instance, api));
  _az_PRECONDITION_NOT_NULL(ustream_instance_clone);

  az_result result;

  if (offset > (UINT32_MAX - ustream_instance->length))
  {
    result = AZ_ERROR_ARG;
  }
  else
  {
    init_instance(
        ustream_instance_clone,
        ustream_instance->control_block,
        ustream_instance->inner_current_position,
        offset,
        ustream_instance->length);
    result = AZ_OK;
  }

  return result;
}

static az_result concrete_dispose(az_ulib_ustream* ustream_instance)
{
  _az_PRECONDITION(AZ_ULIB_USTREAM_IS_TYPE_OF(ustream_instance, api));

  az_ulib_ustream_data_cb* control_block = ustream_instance->control_block;

  AZ_ULIB_PORT_ATOMIC_DEC_W(&(control_block->ref_count));
  if (control_block->ref_count == 0)
  {
    destroy_control_block(control_block);
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_ulib_ustream_init(
    az_ulib_ustream* ustream_instance,
    az_ulib_ustream_data_cb* ustream_control_block,
    az_ulib_release_callback control_block_release,
    const uint8_t* const data_buffer,
    size_t data_buffer_length,
    az_ulib_release_callback data_buffer_release)
{
  _az_PRECONDITION_NOT_NULL(ustream_instance);
  _az_PRECONDITION_NOT_NULL(ustream_control_block);
  _az_PRECONDITION_NOT_NULL(data_buffer);
  _az_PRECONDITION(data_buffer_length > 0);

  ustream_control_block->api = &api;
  ustream_control_block->ptr = (const az_ulib_ustream_data*)data_buffer;
  ustream_control_block->ref_count = 0;
  ustream_control_block->data_release = data_buffer_release;
  ustream_control_block->control_block_release = control_block_release;

  init_instance(ustream_instance, ustream_control_block, 0, 0, data_buffer_length);

  return AZ_OK; 
}

AZ_NODISCARD az_result create_ustream_from_blob(az_ulib_ustream* ustream_instance, az_ulib_ustream_data_cb* ustream_data_cb, 
                                                az_blob_http_cb* blob_http_cb, 
                                                NXD_ADDRESS* ip, CHAR* resource, CHAR* host)
{
  az_result result;
  UINT nx_status;

  // initialize blob http control block
  blob_http_cb->http_client_ptr = &blob_http_client;
  blob_http_cb->ip = ip;
  blob_http_cb->resource = resource;
  blob_http_cb->host = host;

  // initialize blob client
  if((nx_status = blob_client_init(blob_http_cb->ip, blob_http_cb->http_client_ptr, blob_http_cb->packet_ptr, 
                                  blob_http_cb->resource, blob_http_cb->host)) != NX_SUCCESS)
  { 
    printf("Initialize blob client failed with nx_status = (%0x02)\r\n", nx_status);
    result = result_from_nx_status(nx_status);
  }
  
  // send request
  else if ((result = blob_client_request_send(blob_http_cb->http_client_ptr, DCF_WAIT_TIME)) != AZ_OK)
  {
    printf("Blob client request send failed with nx_status = (%0x02)\r\n", nx_status);
  }
  
  // grab first chunk and save package size
  else if ((result = blob_client_grab_chunk(blob_http_cb->http_client_ptr, &blob_http_cb->packet_ptr)) != AZ_OK)
  {
    printf("Blob client grab chunk failed with nx_status = (%0x02)\r\n", nx_status);
  }

  // populate blob cb with package data and metadata
  else
  {
    // grab blob package size for ustream init
    uint32_t package_size = (uint32_t)blob_http_client.nx_web_http_client_total_receive_bytes;
    if ((package_size & 0x07FF) != 0x000)
    {
      package_size = (package_size & 0xFFFFF800) + 0x0800;
    }

    // initialize ustream with blob http control block
    if ((result = az_ulib_ustream_init(
              ustream_instance,
              ustream_data_cb,
              free,
              (const uint8_t*)blob_http_cb,
              package_size,
              NULL))
        != AZ_OK)
    {
      printf("Could not initialize ustream_instance\r\n");
    }
  }

  return result;
}

az_result ustream_blob_client_dispose(az_ulib_ustream* ustream_instance,
                                                  NX_WEB_HTTP_CLIENT* http_client_ptr, NX_PACKET** packet_ptr_ref)
{
  az_result result;

  if ((result = az_ulib_ustream_dispose(ustream_instance)) != AZ_OK)
  {
    printf("Could not dispose of ustream_instance\r\n");
  }

  result = blob_client_dispose(http_client_ptr, packet_ptr_ref);

  return result;
}
