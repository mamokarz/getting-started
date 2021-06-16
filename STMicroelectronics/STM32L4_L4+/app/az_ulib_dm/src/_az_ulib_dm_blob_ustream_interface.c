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

NX_WEB_HTTP_CLIENT _blob_http_client;
NX_PACKET _blob_packet;

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

AZ_NODISCARD az_result blob_client_grab_chunk(NX_WEB_HTTP_CLIENT* http_client_ptr, NX_PACKET** packet_ptr_ref, uint8_t* done)
{
  UINT nx_status; 

  // release packet_ptr from last nx_web_htt_client_response_body_get()
  nx_packet_release(*packet_ptr_ref);

  // grab next chunk
  nx_status = nx_web_http_client_response_body_get(http_client_ptr, packet_ptr_ref, 500);
  
  // done processing response body
  if (nx_status == NX_WEB_HTTP_GET_DONE)
  {
    *done = 1;
    return AZ_OK;
  }
  // there was an error
  else if (nx_status != NX_SUCCESS)
  {
    *done = 1;
    return result_from_nx_status(nx_status);
  }
  // successfully grabbed chunk and there is more to grab
  else
  {
    *done = 0;
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
    printf("HTTP Client request send fail with nx_status = (%0x02)\r\n", nx_status);
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

  az_result result;

  az_ulib_ustream_data_cb* control_block = ustream_instance->control_block;
  az_blob_ustream_interface blob_interface = (az_blob_ustream_interface)control_block->ptr;

  if ((result = blob_client_grab_chunk(blob_interface->http_client_ptr, &blob_interface->packet_ptr, &blob_interface->download_complete)) == AZ_OK)
  {
    // increase destination pointer by size of last chunk
    blob_interface->destination_ptr += az_span_size(blob_interface->data);

    // save pointer to chunk
    blob_interface->data = az_span_create(blob_interface->packet_ptr->nx_packet_prepend_ptr, blob_interface->packet_ptr->nx_packet_length);
  }


  // if (ustream_instance->inner_current_position >= ustream_instance->length)
  // {
  //   *size = 0;
  //   result = AZ_ULIB_EOF;
  // }
  // else
  // {
  //   size_t remain_size
  //       = ustream_instance->length - (size_t)ustream_instance->inner_current_position;
  //   *size = (buffer_length < remain_size) ? buffer_length : remain_size;
  //   IGNORE_MEMCPY_TO_NULL
  //   memcpy(
  //       buffer,
  //       (const uint8_t*)blob_interface->data + ustream_instance->inner_current_position,
  //       *size);
  //   RESUME_WARNINGS
  //   ustream_instance->inner_current_position += *size;
  //   result = AZ_OK;
  // }

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

AZ_NODISCARD az_result create_ustream_from_blob(az_ulib_ustream_data_cb* data_cb, az_ulib_ustream* ustream_instance_ptr, 
                                                uint8_t* user_buffer, uint32_t user_buffer_size,
                                                az_blob_ustream_interface blob_ustream_interface, 
                                                NXD_ADDRESS* ip_ptr, CHAR* resource, CHAR* host)
{
  az_result result;
  UINT nx_status;

  // initialize ustream
  if ((result = az_ulib_ustream_init(
            ustream_instance_ptr,
            data_cb,
            free,
            (const uint8_t*)user_buffer,
            user_buffer_size,
            NULL))
      != AZ_OK)
  {
    printf("Could not initialize ustream_instance\r\n");
  }

  // initialize blob ustream control block and point ustream instance control block to it
  blob_ustream_interface->http_client_ptr = &_blob_http_client;
  blob_ustream_interface->packet_ptr = &_blob_packet;
  blob_ustream_interface->ip_ptr = ip_ptr;
  blob_ustream_interface->resource = resource;
  blob_ustream_interface->host = host;
  blob_ustream_interface->download_complete = 0;
  // blob_ustream_interface->destination_ptr = (uint8_t*)NULL;
  ustream_instance_ptr->control_block->ptr = (const az_ulib_ustream_data*)blob_ustream_interface;

  //Read the file, it will return the first packet.
  //  using:   nx_web_http_client_response_boby_get(&http_client, &packet_ptr, 500);

  // initialize blob client
  if((nx_status = blob_client_init(blob_ustream_interface->ip_ptr, blob_ustream_interface->http_client_ptr, blob_ustream_interface->packet_ptr, 
                                  blob_ustream_interface->resource, blob_ustream_interface->host)) != NX_SUCCESS)
  { 
    printf("Initialize blob client failed with nx_status = (%0x02)\r\n", nx_status);
    result = result_from_nx_status(nx_status);
  }
  
  // send request
  else if ((result = blob_client_request_send(blob_ustream_interface->http_client_ptr, DCF_WAIT_TIME)) != AZ_OK)
  {
    printf("Blob client request send failed with nx_status = (%0x02)\r\n", nx_status);
  }
  
  // grab first chunk and save package size
  else if ((result = blob_client_grab_chunk(blob_ustream_interface->http_client_ptr, &blob_ustream_interface->packet_ptr, &blob_ustream_interface->download_complete)) != AZ_OK)
  {
    printf("Blob client grab chunk failed with nx_status = (%0x02)\r\n", nx_status);
  }

  // populate blob cb with package data and metadata
  else
  {
    // save pointer to first chunk
    blob_ustream_interface->data = az_span_create(blob_ustream_interface->packet_ptr->nx_packet_prepend_ptr, blob_ustream_interface->packet_ptr->nx_packet_length);

    // set ustream length to total package size and round up to nearest 2KB (0x0800)
    ustream_instance_ptr->length = (uint32_t)blob_ustream_interface->http_client_ptr->nx_web_http_client_total_receive_bytes;
    if ((ustream_instance_ptr->length & 0x07FF) != 0x000)
    {
      ustream_instance_ptr->length = (ustream_instance_ptr->length & 0xFFFFF800) + 0x0800;
    }
  }

  return result;
}

AZ_NODISCARD az_result ustream_blob_client_dispose(az_ulib_ustream* ustream_instance_ptr,
                                                  NX_WEB_HTTP_CLIENT* http_client_ptr, NX_PACKET** packet_ptr_ref)
{
  az_result result;

  if ((result = az_ulib_ustream_dispose(ustream_instance_ptr)) != AZ_OK)
  {
    printf("Could not dispose of ustream_instance\r\n");
  }

  result = blob_client_dispose(http_client_ptr, packet_ptr_ref);

  return result;
}

// AZ_NODISCARD az_result ustream_print_blob_package(az_ulib_ustream* ustream_instance_ptr, uint32_t user_buffer_size,
//                                     NX_WEB_HTTP_CLIENT* http_client_ptr, NX_PACKET** packet_ptr_ref)
// {
//   az_result result;
//   size_t returned_size;
//   uint8_t user_buf[user_buffer_size] = { 0 };

//   // Read ustream until receive AZIOT_ULIB_EOF
//   (void)printf("\r\n------printing the ustream------\r\n");
//   while ((result = az_ulib_ustream_read(ustream_instance_ptr, user_buf, user_buffer_size - 1, &returned_size))
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
