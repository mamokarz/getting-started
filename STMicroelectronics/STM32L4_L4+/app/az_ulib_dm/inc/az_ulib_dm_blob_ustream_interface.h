// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#ifndef AZ_ULIB_DM_BLOB_USTREAM_INTERFACE_H
#define AZ_ULIB_DM_BLOB_USTREAM_INTERFACE_H

#include "az_ulib_result.h"
#include "az_ulib_ustream.h"
#include "nx_api.h"
#include "nx_web_http_client.h"
#include "stdbool.h"

#include "azure/core/_az_cfg_prefix.h"

/**
 * @brief   Structure for the blob http control block.
 *
 * For any given ustream created using `az_blob_create_ustream_from_blob`, an associated
 *    #az_blob_http_cb is created and initialized, owning the http connection to the blob located
 *    at `ip`, `host`, and `resource`.
 *
 * @note    This structure should be viewed and used as internal to the implementation of the
 *          ustream. Users should therefore not act on it directly and only allocate the memory
 *          necessary for it to be passed to the ustream.
 *
 */

typedef struct az_blob_http_cb_tag
{
  struct
  {
    /** The #NX_WEB_HTTP_CLIENT that owns the http connection to the targeted blob.*/
    NX_WEB_HTTP_CLIENT http_client;

    /** The #NX_PACKET* pointing to the data associated with the http response.*/
    NX_PACKET* packet_ptr;

    /** The `bool` flag used to let this implementation of the ustream api `read` know when the
     *  last chunk of data has been retrieved from the http connection to the targeted blob. 
    */
    bool last_chunk;
  } _internal;
} az_blob_http_cb;

/**
 * @brief   Create a ustream with an associated blob http control block.
 * 
 * This API initializes `ustream_instance`, allowing the caller to gain access to the data inside 
 *    a blob located at the provided blob storage `ip`, `host`, and `resource`. The http connection
 *    to the blob storage server shall be established in this API and owned by the `blob_http_cb`. 
 *    The first chunk of data will also be retrieved and the total size of the blob assessed using
 *    the associated http response header. In addition, this `ustream_instance` takes ownership of 
 *    the memory associated with `blob_http_cb` and will release this memory when the ref count of
 *    the `ustream_data_cb` goes to zero.
 *
 * @param[in]   ustream_instance                  The #az_ulib_ustream* with the interface of the 
 *                                                ustream.
 * @param[in]   ustream_data_cb                   The #az_ulib_ustream_data_cb* to point to the 
 *                                                ustream data.
 * @param[in]   blob_http_cb_release_callback     The #az_ulib_release_callback function signature 
 *                                                to release the ustream data buffer.
 * @param[in]   blob_http_cb                      The #az_blob_http_cb* to perform the blob http 
 *                                                actions.
 * @param[in]   ustream_data_cb_release_callback  The #az_ulib_release_callback function signature 
 *                                                to release the ustream control block.
 * @param[in]   ip                                The #NXD_ADDRESS* server ip address.
 * @param[in]   resource                          The `int8_t*` blob resource string.
 * @param[in]   host                              The `int8_t*` blob storage host string.
 *
 * @pre         \p ustream_instance               shall not be `NULL`.
 * @pre         \p ustream_data_cb                shall not be `NULL`.
 * @pre         \p blob_http_cb                   shall not be `NULL`.
 * @pre         \p ip                             shall not be `NULL`.
 * @pre         \p sizeof(resource)               shall greater than zero.
 * @pre         \p sizeof(host)                   shall greater than zero.
 * 
 * @return The #az_result with the result of the contained NX operations.
 *      @retval #AZ_OK                        If the ustream and associated blob http actions were
 *                                            completed successfully.
 *      @retval #AZ_ERROR_ULIB_BUSY           If the resources necessary for the
 *                                            `create_ustream_from_blob` operation is busy.
 *      @retval #AZ_ERROR_NOT_ENOUGH_SPACE    If there is not enough memory to execute the
 *                                            `create_ustream_from_blob` operation.
 *      @retval #AZ_ERROR_ULIB_SYSTEM         If the `create_ustream_from_blob` operation failed on
 *                                            the system level.
 *      @retval #AZ_ERROR_NOT_SUPPORTED       If the parameters passed into
 *                                            `create_ustream_from_blob` do not satisfy
 *                                            the requirements put forth by the NX functions called
 *                                            within this function.
 *      @retval #AZ_ERROR_ARG                 If the parameters passed into 
 *                                            `create_ustream_from_blob` violate the option, 
 *                                            pointer, or caller requirements put for in the NX
 *                                            functions called within this function.
 */
AZ_NODISCARD az_result az_blob_create_ustream_from_blob(
    az_ulib_ustream* ustream_instance,
    az_ulib_ustream_data_cb* ustream_data_cb,
    az_ulib_release_callback ustream_data_cb_release_callback,
    az_blob_http_cb* blob_http_cb,
    az_ulib_release_callback blob_http_cb_release_callback,
    NXD_ADDRESS* ip,
    int8_t* resource,
    int8_t* host);

#include "azure/core/_az_cfg_suffix.h"

#endif /* AZ_ULIB_DM_BLOB_USTREAM_INTERFACE_H */
