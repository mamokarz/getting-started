// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#ifndef AZ_ULIB_DM_BLOB_USTREAM_FORWARD_INTERFACE_H
#define AZ_ULIB_DM_BLOB_USTREAM_FORWARD_INTERFACE_H

#include "az_ulib_result.h"
#include "az_ulib_ustream_forward.h"
#include "nx_api.h"
#include "nx_web_http_client.h"
#ifndef __cplusplus
#include <stdbool.h>
#endif

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
    /** The #NX_WEB_HTTP_CLIENT that owns the http connection to the targeted blob. */
    NX_WEB_HTTP_CLIENT http_client;

    /** The #NX_PACKET* pointing to the data associated with the http response. */
    NX_PACKET* packet_ptr;
  } _internal;
} az_blob_http_cb;

/**
 * @brief   Create a ustream_forward with an associated blob http control block.
 * 
 * This API initializes `ustream_forward`, allowing the caller to gain access to the data inside 
 *    a blob located at the provided blob storage `ip`, `host`, and `resource`. The http connection
 *    to the blob storage server shall be established in this API and owned by the `blob_http_cb`.
 *    The first chunk of data will also be retrieved and the total size of the blob assessed using
 *    the associated http response header. In addition, this `ustream_forward` takes ownership of 
 *    the memory associated with `blob_http_cb` and will release this memory when the 
 *    `ustream_forward` is disposed.
 *
 * @param[in]   ustream_forward                   The #az_ulib_ustream_forward* with the interface  
 *                                                of the ustream_forward.
 * @param[in]   ustream_forward_release_callback  The #az_ulib_release_callback function signature 
 *                                                to release the ustream control block.
 * @param[in]   blob_http_cb_release_callback     The #az_ulib_release_callback function signature 
 *                                                to release the ustream data buffer.
 * @param[in]   blob_http_cb                      The #az_blob_http_cb* to perform the blob http
 *                                                actions.
 * @param[in]   ip                                The #NXD_ADDRESS* server ip address.
 * @param[in]   resource                          The `int8_t*` blob resource /0 terminated string.
 * @param[in]   host                              The `int8_t*` blob storage host /0 terminated
 *                                                string.
 *
 * @pre         \p ustream_forward                shall not be `NULL`.
 * @pre         \p blob_http_cb                   shall not be `NULL`.
 * @pre         \p ip                             shall not be `NULL`.
 * @pre         \p sizeof(resource)               shall greater than zero.
 * @pre         \p sizeof(host)                   shall greater than zero.
 *
 * @return The #az_result with the result of the contained NX operations.
 *      @retval #AZ_OK                        If the ustream and associated blob http actions were
 *                                            completed successfully.
 *      @retval #AZ_ERROR_ULIB_BUSY           If the resources necessary for the
 *                                            `create_ustream_forward_from_blob` operation is busy.
 *      @retval #AZ_ERROR_NOT_ENOUGH_SPACE    If there is not enough memory to execute the
 *                                            `create_ustream_from_blob` operation.
 *      @retval #AZ_ERROR_ULIB_SYSTEM         If the `create_ustream_forward_from_blob` operation 
 *                                            failed on the system level.
 *      @retval #AZ_ERROR_NOT_SUPPORTED       If the parameters passed into
 *                                            `create_ustream_forward_from_blob` do not satisfy
 *                                            the requirements put forth by the NX functions called
 *                                            within this function.
 *      @retval #AZ_ERROR_ARG                 If the parameters passed into 
 *                                            `create_ustream_forward_from_blob` violate the option, 
 *                                            pointer, or caller requirements put for in the NX
 *                                            functions called within this function.
 */
AZ_NODISCARD az_result az_blob_create_ustream_forward_from_blob(
    az_ulib_ustream_forward* ustream_forward,
    az_ulib_release_callback ustream_forward_release_callback,
    az_blob_http_cb* blob_http_cb,
    az_ulib_release_callback blob_http_cb_release_callback,
    NXD_ADDRESS* ip,
    int8_t* resource,
    int8_t* host);

#include "azure/core/_az_cfg_suffix.h"

#endif /* AZ_ULIB_DM_BLOB_USTREAM_FORWARD_INTERFACE_H */
