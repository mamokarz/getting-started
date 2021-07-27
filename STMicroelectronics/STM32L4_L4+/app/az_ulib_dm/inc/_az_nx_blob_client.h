// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#ifndef _az_NX_BLOB_CLIENT_H
#define _az_NX_BLOB_CLIENT_H

#include "az_ulib_result.h"
#include "nx_api.h"
#include "nx_web_http_client.h"

#include "azure/core/_az_cfg_prefix.h"

AZ_NODISCARD az_result
_az_nx_blob_client_init(NX_WEB_HTTP_CLIENT* http_client, NXD_ADDRESS* ip, ULONG wait_option);

AZ_NODISCARD az_result _az_nx_blob_client_grab_chunk(
    NX_WEB_HTTP_CLIENT* http_client_ptr,
    NX_PACKET** packet_ptr_ref,
    ULONG wait_option);

AZ_NODISCARD az_result _az_nx_blob_client_request_send(
    NX_WEB_HTTP_CLIENT* http_client_ptr,
    int8_t* resource,
    int8_t* host,
    ULONG wait_option);

AZ_NODISCARD az_result
_az_nx_blob_client_dispose(NX_WEB_HTTP_CLIENT* http_client_ptr, NX_PACKET** packet_ptr_ref);

#include "azure/core/_az_cfg_suffix.h"

#endif /* _az_NX_BLOB_CLIENT_H */
