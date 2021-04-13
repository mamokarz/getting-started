/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

#include "nx_client.h"

#include <stdio.h>

#include "stm32l475e_iot01.h"
#include "stm32l475e_iot01_tsensor.h"

#include "nx_api.h"
#include "nx_azure_iot_hub_client.h"
#include "nx_azure_iot_json_reader.h"
#include "nx_azure_iot_provisioning_client.h"

#include "azure_iot_nx_client.h"
#include "nx_azure_iot_pnp_helpers.h"

#include "azure_config.h"
#include "azure_device_x509_cert_config.h"
#include "azure_pnp_info.h"
#include "az_ulib_dm.h"
#include "az_ulib_ipc_api.h"
#include "azure/az_core.h"

#include "nx_wifi.h" // DCF
#define DEMO_DATA "----- Test Packet -----\n"
#define DCF_PACKET_COUNT 5
#define DCF_PACKET_SIZE  1200 // Set the default value to 1200 since WIFI payload size (ES_WIFI_PAYLOAD_SIZE) is 1200
#define DCF_POOL_SIZE    ((DCF_PACKET_SIZE + sizeof(NX_PACKET)) * DCF_PACKET_COUNT)

#define IOT_MODEL_ID "dtmi:azurertos:devkit:gsg;1"

#define TELEMETRY_TEMPERATURE       "temperature"
#define TELEMETRY_INTERVAL_PROPERTY "telemetryInterval"
#define LED_STATE_PROPERTY          "ledState"
#define SET_LED_STATE_COMMAND       "setLedState"
#define INSTALL_COMMAND             "install"
#define UNINSTALL_COMMAND           "uninstall"
#define TCP_SEND_COMMAND            "tcp_send"

#define TELEMETRY_INTERVAL_EVENT 1

static AZURE_IOT_NX_CONTEXT azure_iot_nx_client;
static TX_EVENT_FLAGS_GROUP azure_iot_flags;

static int32_t telemetry_interval = 10;

static UCHAR dcf_ip_pool[DCF_POOL_SIZE]; // DCF

az_result dcf_tcp_client_gateway_entry(NX_PACKET_POOL* pool_ptr)
{
    az_result result; // this is a dcf function, so we will return az_result
    UINT status; // for wifi functions

    // initialize TCP connection objects
    NX_TCP_SOCKET socket_ptr;
    NXD_ADDRESS server_ip;
    server_ip.nxd_ip_version = NX_IP_VERSION_V4;
    server_ip.nxd_ip_address.v4 = IP_ADDRESS(192, 168, 1, 29);
    UINT server_port = 9999;

    // packet variables
    NX_PACKET* packet_ptr;
    ULONG packet_length;

    // loop forever
    // while(1)
    // {
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
        if((status =  nx_packet_allocate(pool_ptr, &packet_ptr, NX_TCP_PACKET, NX_WAIT_FOREVER)))
        {
            printf("Error allocating packet (0x%04x)\r\n", status);
            result = AZ_ERROR_CANCELED;
        }

        // write buffer to packet
        nx_packet_data_append(packet_ptr, DEMO_DATA, sizeof(DEMO_DATA), pool_ptr, TX_WAIT_FOREVER);

        status =  nx_packet_length_get(packet_ptr, &packet_length);
        if ((status) || (packet_length != sizeof(DEMO_DATA)))
        {
            nx_packet_release(packet_ptr);
            printf("Error appending data to packet (0x%04x)\r\n", status);
            result = AZ_ERROR_CANCELED;
        }
        printf("Sending packet: %s\r\n", DEMO_DATA);
        if ((status = nx_wifi_tcp_socket_send(&socket_ptr, packet_ptr, NX_WAIT_FOREVER)))
        {
            printf("Error sending packet to TCP server (0x%04x)\r\n", status);
            result = AZ_ERROR_CANCELED;
        }

        // release packet
        nx_packet_release(packet_ptr);

        // Disconnect from server
        if ((status = nx_wifi_tcp_socket_disconnect(&socket_ptr, 0)))
        {
            printf("Error disconnecting from TCP server (0x%04x)\r\n", status);
            result = AZ_ERROR_CANCELED;
        }
        else
        {
            result = AZ_OK;
        }

        // sleep
    //     _tx_thread_sleep(50);
    // }
    
    return result;
}

static UINT append_device_info_properties(NX_AZURE_IOT_JSON_WRITER* json_writer, VOID* context)
{
    if (nx_azure_iot_json_writer_append_property_with_string_value(json_writer,
            (UCHAR*)DEVICE_INFO_MANUFACTURER_PROPERTY_NAME,
            sizeof(DEVICE_INFO_MANUFACTURER_PROPERTY_NAME) - 1,
            (UCHAR*)DEVICE_INFO_MANUFACTURER_PROPERTY_VALUE,
            sizeof(DEVICE_INFO_MANUFACTURER_PROPERTY_VALUE) - 1) ||
        nx_azure_iot_json_writer_append_property_with_string_value(json_writer,
            (UCHAR*)DEVICE_INFO_MODEL_PROPERTY_NAME,
            sizeof(DEVICE_INFO_MODEL_PROPERTY_NAME) - 1,
            (UCHAR*)DEVICE_INFO_MODEL_PROPERTY_VALUE,
            sizeof(DEVICE_INFO_MODEL_PROPERTY_VALUE) - 1) ||
        nx_azure_iot_json_writer_append_property_with_string_value(json_writer,
            (UCHAR*)DEVICE_INFO_SW_VERSION_PROPERTY_NAME,
            sizeof(DEVICE_INFO_SW_VERSION_PROPERTY_NAME) - 1,
            (UCHAR*)DEVICE_INFO_SW_VERSION_PROPERTY_VALUE,
            sizeof(DEVICE_INFO_SW_VERSION_PROPERTY_VALUE) - 1) ||
        nx_azure_iot_json_writer_append_property_with_string_value(json_writer,
            (UCHAR*)DEVICE_INFO_OS_NAME_PROPERTY_NAME,
            sizeof(DEVICE_INFO_OS_NAME_PROPERTY_NAME) - 1,
            (UCHAR*)DEVICE_INFO_OS_NAME_PROPERTY_VALUE,
            sizeof(DEVICE_INFO_OS_NAME_PROPERTY_VALUE) - 1) ||
        nx_azure_iot_json_writer_append_property_with_string_value(json_writer,
            (UCHAR*)DEVICE_INFO_PROCESSOR_ARCHITECTURE_PROPERTY_NAME,
            sizeof(DEVICE_INFO_PROCESSOR_ARCHITECTURE_PROPERTY_NAME) - 1,
            (UCHAR*)DEVICE_INFO_PROCESSOR_ARCHITECTURE_PROPERTY_VALUE,
            sizeof(DEVICE_INFO_PROCESSOR_ARCHITECTURE_PROPERTY_VALUE) - 1) ||
        nx_azure_iot_json_writer_append_property_with_string_value(json_writer,
            (UCHAR*)DEVICE_INFO_PROCESSOR_MANUFACTURER_PROPERTY_NAME,
            sizeof(DEVICE_INFO_PROCESSOR_MANUFACTURER_PROPERTY_NAME) - 1,
            (UCHAR*)DEVICE_INFO_PROCESSOR_MANUFACTURER_PROPERTY_VALUE,
            sizeof(DEVICE_INFO_PROCESSOR_MANUFACTURER_PROPERTY_VALUE) - 1) ||
        nx_azure_iot_json_writer_append_property_with_double_value(json_writer,
            (UCHAR*)DEVICE_INFO_TOTAL_STORAGE_PROPERTY_NAME,
            sizeof(DEVICE_INFO_TOTAL_STORAGE_PROPERTY_NAME) - 1,
            DEVICE_INFO_TOTAL_STORAGE_PROPERTY_VALUE,
            2) ||
        nx_azure_iot_json_writer_append_property_with_double_value(json_writer,
            (UCHAR*)DEVICE_INFO_TOTAL_MEMORY_PROPERTY_NAME,
            sizeof(DEVICE_INFO_TOTAL_MEMORY_PROPERTY_NAME) - 1,
            DEVICE_INFO_TOTAL_MEMORY_PROPERTY_VALUE,
            2))
    {
        return NX_NOT_SUCCESSFUL;
    }

    return NX_AZURE_IOT_SUCCESS;
}

static UINT append_device_telemetry(NX_AZURE_IOT_JSON_WRITER* json_writer, VOID* context)
{
    float temperature = BSP_TSENSOR_ReadTemp();

    if (nx_azure_iot_json_writer_append_property_with_double_value(
            json_writer, (UCHAR*)TELEMETRY_TEMPERATURE, sizeof(TELEMETRY_TEMPERATURE) - 1, temperature, 2))
    {
        return NX_NOT_SUCCESSFUL;
    }

    return NX_AZURE_IOT_SUCCESS;
}

static void set_led_state(bool level)
{
    if (level)
    {
        printf("LED is turned ON\r\n");
        BSP_LED_On(LED_GREEN);
    }
    else
    {
        printf("LED is turned OFF\r\n");
        BSP_LED_Off(LED_GREEN);
    }
}

static void direct_method_cb(AZURE_IOT_NX_CONTEXT* nx_context,
    const UCHAR* method,
    USHORT method_length,
    UCHAR* payload,
    USHORT payload_length,
    VOID* context,
    USHORT context_length)
{
    UINT status;
    UINT http_status    = 501;
    CHAR* http_response = "{}";

    if (strncmp((CHAR*)method, SET_LED_STATE_COMMAND, method_length) == 0)
    {
        bool arg = (strncmp((CHAR*)payload, "true", payload_length) == 0);
        set_led_state(arg);

        azure_iot_nx_client_publish_bool_property(&azure_iot_nx_client, LED_STATE_PROPERTY, arg);

        http_status = 200;
    }
    else if (strncmp((CHAR*)method, INSTALL_COMMAND, method_length) == 0)
    {
        az_result result;
        if((result = az_ulib_dm_install((CHAR*)payload, payload_length)) == AZ_OK)
        {
            http_status = 200;
        }
        else
        {
            switch(result)
            {
                case AZ_ERROR_ULIB_ELEMENT_DUPLICATE:
                    http_status = 409;
                    http_response = "{ \"description\":\"Package expose an already published interface.\" }";
                break;
                case AZ_ERROR_NOT_ENOUGH_SPACE:
                    http_status = 507;
                    http_response = "{ \"description\":\"There is no more space to store the new package.\" }";
                break;
                default:
                    http_status = 400;
                    http_response = "{ \"description\":\"Unknow error.\" }";
                break;
            }
        }
    }
    else if (strncmp((CHAR*)method, UNINSTALL_COMMAND, method_length) == 0)
    {
        az_result result;
        if((result = az_ulib_dm_uninstall((CHAR*)payload, payload_length)) == AZ_OK)
        {
            http_status = 200;
        }
        else
        {
            switch(result)
            {
                case AZ_ERROR_ITEM_NOT_FOUND:
                    http_status = 404;
                    http_response = "{ \"description\":\"Package not found.\" }";
                break;
                case AZ_ERROR_ULIB_BUSY:
                    http_status = 500;
                    http_response = "{ \"description\":\"Interface call in progress.\" }";
                break;
                default:
                    http_status = 400;
                    http_response = "{ \"description\":\"Unknow error.\" }";
                break;
            }
        }
    }
    else if (strncmp((CHAR*)method, TCP_SEND_COMMAND, method_length) == 0)
    {
        az_result result;
        NX_PACKET_POOL tcp_pool;
        printf("Send TCP Packet!\r\n");
        // create temporary packet pool
        status = nx_packet_pool_create(&tcp_pool, "DCF Packet Pool", DCF_PACKET_SIZE, dcf_ip_pool, DCF_POOL_SIZE);
        if (status != NX_SUCCESS)
        {
            nx_packet_pool_delete(&tcp_pool);
            printf("ERROR: Packet pool create fail.\r\n");
            return;
        }
        if((result = dcf_tcp_client_gateway_entry(&tcp_pool)) == AZ_OK)
        {
            nx_packet_pool_delete(&tcp_pool);
            printf("TCP Send Success!\r\n");
            return;
        }
        else
        {   
            nx_packet_pool_delete(&tcp_pool);
            printf("TCP Send Failure!\r\n");
            return;
        }
    }
    else
    {
        az_result result;
        char buf[100];
        char result_buf[200];
        az_span result_span = AZ_SPAN_FROM_BUFFER(result_buf);
        az_ulib_ipc_interface_handle handle = NULL;

        strncpy((char*)buf, (const char*)method, (size_t)method_length);
        buf[method_length] = '\0';
        
        az_span method_span = az_span_create_from_str(buf);
        int32_t split_pos = az_span_find(method_span, AZ_SPAN_FROM_STR("."));
        az_span interface_span = az_span_slice(method_span, 0, split_pos);
        method_span = az_span_slice_to_end(method_span, split_pos+1);
        split_pos = az_span_find(method_span, AZ_SPAN_FROM_STR("."));
        az_span version_span = az_span_slice(method_span, 0, split_pos);
        uint32_t version;
        method_span = az_span_slice_to_end(method_span, split_pos+1);

        if(az_span_atou32(version_span, &version) != AZ_OK)
        {
            http_status = 400;
            http_response = "{ \"description\":\"Invalid method version.\" }";
        }
        else if((result = az_ulib_ipc_try_get_interface(interface_span, version, AZ_ULIB_VERSION_EQUALS_TO, &handle)) == AZ_OK)
        {
            az_span payload_span = az_span_create(payload, (int32_t)payload_length);
            if((result = az_ulib_ipc_call_w_str(handle, method_span, payload_span, &result_span)) == AZ_OK)
            {
                http_status = 200;
                http_response = (char*)az_span_ptr(result_span);
                http_response[az_span_size(result_span)] = '\0';
            }
            else
            {
                switch(result)
                {
                    case AZ_ERROR_ITEM_NOT_FOUND:
                        http_status = 404;
                        http_response = "{ \"description\":\"Command not found.\" }";
                    break;
                    case AZ_ERROR_NOT_SUPPORTED:
                        http_status = 405;
                        http_response = "{ \"description\":\"Interface does not support JSON call.\" }";
                    break;
                    default:
                        http_status = 400;
                        http_response = "{ \"description\":\"Unknow error.\" }";
                    break;
                }
            }
        }
        else
        {
            switch(result)
            {
                case AZ_ERROR_ITEM_NOT_FOUND:
                    http_status = 404;
                    http_response = "{ \"description\":\"Interface not found.\" }";
                break;
                default:
                    http_status = 400;
                    http_response = "{ \"description\":\"Unknow error.\" }";
                break;
            }
        }
    }

    if ((status = nx_azure_iot_hub_client_direct_method_message_response(&nx_context->iothub_client,
             http_status,
             context,
             context_length,
             (UCHAR*)http_response,
             strlen(http_response),
             NX_WAIT_FOREVER)))
    {
        printf("Direct method response failed! (0x%08x)\r\n", status);
        return;
    }
}

static void device_twin_desired_property_cb(UCHAR* component_name,
    UINT component_name_len,
    UCHAR* property_name,
    UINT property_name_len,
    NX_AZURE_IOT_JSON_READER property_value_reader,
    UINT version,
    VOID* userContextCallback)
{
    UINT status;
    AZURE_IOT_NX_CONTEXT* nx_context = (AZURE_IOT_NX_CONTEXT*)userContextCallback;

    if (strncmp((CHAR*)property_name, TELEMETRY_INTERVAL_PROPERTY, property_name_len) == 0)
    {
        status = nx_azure_iot_json_reader_token_int32_get(&property_value_reader, &telemetry_interval);
        if (status == NX_AZURE_IOT_SUCCESS)
        {
            // Confirm reception back to hub
            azure_nx_client_respond_int_writeable_property(
                nx_context, TELEMETRY_INTERVAL_PROPERTY, telemetry_interval, 200, version);

            // Set a telemetry event so we pick up the change immediately
            tx_event_flags_set(&azure_iot_flags, TELEMETRY_INTERVAL_EVENT, TX_OR);
        }
    }
}

static void device_twin_property_cb(UCHAR* component_name,
    UINT component_name_len,
    UCHAR* property_name,
    UINT property_name_len,
    NX_AZURE_IOT_JSON_READER property_value_reader,
    UINT version,
    VOID* userContextCallback)
{
    if (strncmp((CHAR*)property_name, TELEMETRY_INTERVAL_PROPERTY, property_name_len) == 0)
    {
        nx_azure_iot_json_reader_token_int32_get(&property_value_reader, &telemetry_interval);
    }
}

UINT azure_iot_nx_client_entry(
    NX_IP* ip_ptr, NX_PACKET_POOL* pool_ptr, NX_DNS* dns_ptr, UINT (*unix_time_callback)(ULONG* unix_time))
{
    UINT status;
    ULONG events = 0;

    if ((status = tx_event_flags_create(&azure_iot_flags, "Azure IoT flags")))
    {
        printf("FAIL: Unable to create nx_client event flags (0x%08x)\r\n", status);
        return status;
    }

    status =
        azure_iot_nx_client_create(&azure_iot_nx_client, ip_ptr, pool_ptr, dns_ptr, unix_time_callback, IOT_MODEL_ID);
    if (status != NX_SUCCESS)
    {
        printf("ERROR: azure_iot_nx_client_create failed (0x%08x)\r\n", status);
        return status;
    }

#ifdef ENABLE_X509
    status = azure_iot_nx_client_cert_set(&azure_iot_nx_client,
        (UCHAR*)iot_x509_device_cert,
        iot_x509_device_cert_len,
        (UCHAR*)iot_x509_private_key,
        iot_x509_private_key_len);
#else
    status = azure_iot_nx_client_sas_set(&azure_iot_nx_client, IOT_DEVICE_SAS_KEY);
#endif
    if (status != NX_SUCCESS)
    {
        printf("ERROR: azure_iot_nx_client_[sas|cert]_set failed (0x%08x)\r\n", status);
        return status;
    }

#ifdef ENABLE_DPS
    azure_iot_nx_client_dps_create(&azure_iot_nx_client, IOT_DPS_ID_SCOPE, IOT_DPS_REGISTRATION_ID);
#else
    azure_iot_nx_client_hub_create(&azure_iot_nx_client, IOT_HUB_HOSTNAME, IOT_HUB_DEVICE_ID);
#endif
    if (status != NX_SUCCESS)
    {
        printf("ERROR: azure_iot_nx_client_[hub|dps]_create failed (0x%08x)\r\n", status);
        return status;
    }

    // Register the callbacks
    azure_iot_nx_client_register_direct_method(&azure_iot_nx_client, direct_method_cb);
    azure_iot_nx_client_register_device_twin_desired_prop(&azure_iot_nx_client, device_twin_desired_property_cb);
    azure_iot_nx_client_register_device_twin_prop(&azure_iot_nx_client, device_twin_property_cb);

    if ((status = azure_iot_nx_client_connect(&azure_iot_nx_client)))
    {
        printf("ERROR: failed to connect nx client (0x%08x)\r\n", status);
        return status;
    }

    // Request the device twin for writeable property update
    if ((status = azure_iot_nx_client_device_twin_request_and_wait(&azure_iot_nx_client)))
    {
        printf("ERROR: azure_iot_nx_client_device_twin_request_and_wait failed (0x%08x)\r\n", status);
        return status;
    }

    // Send out property updates
    azure_iot_nx_client_publish_int_writeable_property(
        &azure_iot_nx_client, TELEMETRY_INTERVAL_PROPERTY, telemetry_interval);
    azure_iot_nx_client_publish_bool_property(&azure_iot_nx_client, LED_STATE_PROPERTY, false);
    azure_iot_nx_client_publish_properties(
        &azure_iot_nx_client, DEVICE_INFO_COMPONENT_NAME, append_device_info_properties);

    printf("\r\nStarting Main loop\r\n");

    while (true)
    {
        tx_event_flags_get(
            &azure_iot_flags, TELEMETRY_INTERVAL_EVENT, TX_OR_CLEAR, &events, telemetry_interval * NX_IP_PERIODIC_RATE);

        azure_iot_nx_client_publish_telemetry(&azure_iot_nx_client, append_device_telemetry);
    }

    return NX_SUCCESS;
}
