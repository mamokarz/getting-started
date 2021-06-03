#ifndef AZ_ULIB_DM_BLOB_USTREAM_INTERFACE_H
#define AZ_ULIB_DM_BLOB_USTREAM_INTERFACE_H

#include "az_ulib_result.h"
#include "azure/az_core.h"
#include "nx_api.h"
#include "nx_web_http_client.h"
#include "nx_wifi.h"
#include "nxd_dns.h"
#include "stm_networking.h"
#include "wifi.h"

#ifndef __cplusplus
#include <stdint.h>
#else
#include <cstdint>
#endif /* __cplusplus */

#define DCF_WAIT_TIME    600

#include "azure/core/_az_cfg_prefix.h"

AZ_NODISCARD UINT blob_client_init(NXD_ADDRESS* ip, NX_WEB_HTTP_CLIENT* http_client, NX_PACKET* packet_ptr,
                                  CHAR* resource, CHAR* host);

AZ_NODISCARD az_result blob_client_grab_chunk(NX_WEB_HTTP_CLIENT* http_client_ptr, NX_PACKET** packet_ptr_ref, uint8_t* done);

AZ_NODISCARD az_result blob_client_request_send(NX_WEB_HTTP_CLIENT* http_client_ptr, ULONG wait_option);

AZ_NODISCARD az_result blob_client_dispose(NX_WEB_HTTP_CLIENT* http_client_ptr, NX_PACKET** packet_ptr_ref);

#include "azure/core/_az_cfg_suffix.h"

#endif /* AZ_ULIB_DM_BLOB_USTREAM_INTERFACE_H */