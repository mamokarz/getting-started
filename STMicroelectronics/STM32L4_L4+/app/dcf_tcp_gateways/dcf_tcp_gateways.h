/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

#ifndef _DCF_TCP_GATEWAYS_H
#define _DCF_TCP_GATEWAYS_H

#include "tx_api.h"
#include "nx_api.h"
#include "nx_wifi.h"
#include "az_ulib_result.h"

#define DEMO_DATA "----- Test Packet -----\n"
#define DCF_PACKET_COUNT 5
#define DCF_PACKET_SIZE  1200 // Set the default value to 1200 since WIFI payload size (ES_WIFI_PAYLOAD_SIZE) is 1200
#define DCF_POOL_SIZE    ((DCF_PACKET_SIZE + sizeof(NX_PACKET)) * DCF_PACKET_COUNT)

az_result dcf_tcp_client_send(NX_PACKET_POOL* pool_ptr, ULONG server_ip_address);

#endif // _DCF_TCP_GATEWAYS_H
