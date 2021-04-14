#include "dcf_tcp_gateways.h"

az_result dcf_tcp_client_send(NX_PACKET_POOL* pool_ptr, ULONG server_ip_address)
{
    az_result result; // this is a dcf function, so we will return az_result
    UINT status; // for wifi functions

    // initialize TCP connection objects
    NX_TCP_SOCKET socket_ptr;
    NXD_ADDRESS server_ip;
    server_ip.nxd_ip_version = NX_IP_VERSION_V4;
    server_ip.nxd_ip_address.v4 = server_ip_address;
    UINT server_port = 9999;

    // packet variables
    NX_PACKET* packet_ptr;
    ULONG packet_length;

    // Connect to server
    printf("Connecting to TCP server\r\n\r\n");
    if ((status = nx_wifi_tcp_client_socket_connect(&socket_ptr,
                                        &server_ip,
                                        server_port,
                                        0)))
    {
        printf("Error connecting to TCP server (0x%04x)\r\n", status);
        result = AZ_ERROR_CANCELED;
    }

    // allocate packet
    else if((status =  nx_packet_allocate(pool_ptr, &packet_ptr, NX_TCP_PACKET, NX_WAIT_FOREVER)))
    {
        printf("Error allocating packet (0x%04x)\r\n", status);
        result = AZ_ERROR_CANCELED;
    }

    // write buffer to packet
    else if((status = nx_packet_data_append(packet_ptr, DEMO_DATA, sizeof(DEMO_DATA), pool_ptr, TX_WAIT_FOREVER)))
    {
        printf("Error appending data to packet (0x%04x)\r\n", status);
        result = AZ_ERROR_CANCELED;
    }

    // if packet length is not the right size, throw the same error
    else if ((status =  nx_packet_length_get(packet_ptr, &packet_length)) || (packet_length != sizeof(DEMO_DATA)))
    {
        printf("Error appending data to packet (0x%04x)\r\n", status);
        result = AZ_ERROR_CANCELED;
    }

    // send packet to tcp server
    else if ((status = nx_wifi_tcp_socket_send(&socket_ptr, packet_ptr, NX_WAIT_FOREVER)))
    {
        printf("Error sending packet to TCP server (0x%04x)\r\n", status);
        result = AZ_ERROR_CANCELED;
    }
    else
    {
        printf("Successfully sent packet: %s\r\n", DEMO_DATA);
        result = AZ_OK;
    }

    // Disconnect from server
    if ((status = nx_wifi_tcp_socket_disconnect(&socket_ptr, 0)))
    {
        printf("Error disconnecting from TCP server (0x%04x)\r\n", status);
        result = AZ_ERROR_CANCELED;
    }

    return result;
}
