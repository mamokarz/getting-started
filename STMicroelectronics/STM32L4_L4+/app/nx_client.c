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

#include "az_ulib_dm_api.h"
#include "az_ulib_ipc_api.h"
#include "azure/az_core.h"
#include "azure_config.h"
#include "azure_device_x509_cert_config.h"
#include "azure_pnp_info.h"

#define IOT_MODEL_ID "dtmi:azurertos:devkit:gsg;2"

#define TELEMETRY_TEMPERATURE "temperature"
#define TELEMETRY_INTERVAL_PROPERTY "telemetryInterval"
#define LED_STATE_PROPERTY "ledState"

#define TELEMETRY_INTERVAL_EVENT 1

#define TEMP_BUFFER_SIZE 300

#define MAX_SESSIONS_IN_METHOD 5

static AZURE_IOT_NX_CONTEXT azure_iot_nx_client;
static TX_EVENT_FLAGS_GROUP azure_iot_flags;

static LONG telemetry_interval = 10;

static UINT append_device_info_properties(NX_AZURE_IOT_JSON_WRITER* json_writer, VOID* context)
{
  if (nx_azure_iot_json_writer_append_property_with_string_value(
          json_writer,
          (UCHAR*)DEVICE_INFO_MANUFACTURER_PROPERTY_NAME,
          sizeof(DEVICE_INFO_MANUFACTURER_PROPERTY_NAME) - 1,
          (UCHAR*)DEVICE_INFO_MANUFACTURER_PROPERTY_VALUE,
          sizeof(DEVICE_INFO_MANUFACTURER_PROPERTY_VALUE) - 1)
      || nx_azure_iot_json_writer_append_property_with_string_value(
          json_writer,
          (UCHAR*)DEVICE_INFO_MODEL_PROPERTY_NAME,
          sizeof(DEVICE_INFO_MODEL_PROPERTY_NAME) - 1,
          (UCHAR*)DEVICE_INFO_MODEL_PROPERTY_VALUE,
          sizeof(DEVICE_INFO_MODEL_PROPERTY_VALUE) - 1)
      || nx_azure_iot_json_writer_append_property_with_string_value(
          json_writer,
          (UCHAR*)DEVICE_INFO_SW_VERSION_PROPERTY_NAME,
          sizeof(DEVICE_INFO_SW_VERSION_PROPERTY_NAME) - 1,
          (UCHAR*)DEVICE_INFO_SW_VERSION_PROPERTY_VALUE,
          sizeof(DEVICE_INFO_SW_VERSION_PROPERTY_VALUE) - 1)
      || nx_azure_iot_json_writer_append_property_with_string_value(
          json_writer,
          (UCHAR*)DEVICE_INFO_OS_NAME_PROPERTY_NAME,
          sizeof(DEVICE_INFO_OS_NAME_PROPERTY_NAME) - 1,
          (UCHAR*)DEVICE_INFO_OS_NAME_PROPERTY_VALUE,
          sizeof(DEVICE_INFO_OS_NAME_PROPERTY_VALUE) - 1)
      || nx_azure_iot_json_writer_append_property_with_string_value(
          json_writer,
          (UCHAR*)DEVICE_INFO_PROCESSOR_ARCHITECTURE_PROPERTY_NAME,
          sizeof(DEVICE_INFO_PROCESSOR_ARCHITECTURE_PROPERTY_NAME) - 1,
          (UCHAR*)DEVICE_INFO_PROCESSOR_ARCHITECTURE_PROPERTY_VALUE,
          sizeof(DEVICE_INFO_PROCESSOR_ARCHITECTURE_PROPERTY_VALUE) - 1)
      || nx_azure_iot_json_writer_append_property_with_string_value(
          json_writer,
          (UCHAR*)DEVICE_INFO_PROCESSOR_MANUFACTURER_PROPERTY_NAME,
          sizeof(DEVICE_INFO_PROCESSOR_MANUFACTURER_PROPERTY_NAME) - 1,
          (UCHAR*)DEVICE_INFO_PROCESSOR_MANUFACTURER_PROPERTY_VALUE,
          sizeof(DEVICE_INFO_PROCESSOR_MANUFACTURER_PROPERTY_VALUE) - 1)
      || nx_azure_iot_json_writer_append_property_with_double_value(
          json_writer,
          (UCHAR*)DEVICE_INFO_TOTAL_STORAGE_PROPERTY_NAME,
          sizeof(DEVICE_INFO_TOTAL_STORAGE_PROPERTY_NAME) - 1,
          DEVICE_INFO_TOTAL_STORAGE_PROPERTY_VALUE,
          2)
      || nx_azure_iot_json_writer_append_property_with_double_value(
          json_writer,
          (UCHAR*)DEVICE_INFO_TOTAL_MEMORY_PROPERTY_NAME,
          sizeof(DEVICE_INFO_TOTAL_MEMORY_PROPERTY_NAME) - 1,
          DEVICE_INFO_TOTAL_MEMORY_PROPERTY_VALUE,
          2))
  {
    return NX_NOT_SUCCESSFUL;
  }

  return NX_AZURE_IOT_SUCCESS;
}

static void direct_method_cb(
    AZURE_IOT_NX_CONTEXT* nx_context,
    const UCHAR* method,
    USHORT method_length,
    UCHAR* payload,
    USHORT payload_length,
    VOID* context,
    USHORT context_length)
{
  UINT status;
  UINT http_status;
  char buf[TEMP_BUFFER_SIZE];
  char result_buf[TEMP_BUFFER_SIZE];
  az_span http_response = AZ_SPAN_FROM_BUFFER(result_buf);
  az_ulib_ipc_interface_handle handle = NULL;

  AZ_ULIB_TRY
  {
    strncpy((char*)buf, (const char*)method, (size_t)method_length);
    buf[method_length] = '\0';
    az_span method_full_name = az_span_create_from_str(buf);

    az_span device_name;
    az_span package_name;
    ULONG package_version;
    az_span interface_name;
    ULONG interface_version;
    az_span capability_name;
    AZ_ULIB_THROW_IF_AZ_ERROR(az_ulib_ipc_split_method_name(
        method_full_name,
        &device_name,
        &package_name,
        &package_version,
        &interface_name,
        &interface_version,
        &capability_name));
    AZ_ULIB_THROW_IF_ERROR((az_span_size(capability_name) > 0), AZ_ERROR_UNEXPECTED_CHAR);

    AZ_ULIB_THROW_IF_AZ_ERROR(az_ulib_ipc_try_get_interface(
        device_name, package_name, package_version, interface_name, interface_version, &handle));

    az_span payload_span = az_span_create(payload, (LONG)payload_length);

    az_ulib_capability_index command_index;
    AZ_ULIB_THROW_IF_AZ_ERROR(
        az_ulib_ipc_try_get_capability(handle, capability_name, &command_index));

    AZ_ULIB_THROW_IF_AZ_ERROR(
        az_ulib_ipc_call_with_str(handle, command_index, payload_span, &http_response));

    http_status = 200;
  }
  AZ_ULIB_CATCH(...)
  {
    const char* error_str;
    switch (AZ_ULIB_TRY_RESULT)
    {
      case AZ_ULIB_EOF:
        http_status = 416;
        error_str = "AZ_ULIB_EOF";
        break;
      case AZ_ERROR_ULIB_ELEMENT_DUPLICATE:
        http_status = 409;
        error_str = "AZ_ERROR_ULIB_ELEMENT_DUPLICATE";
        break;
      case AZ_ERROR_NOT_ENOUGH_SPACE:
        http_status = 507;
        error_str = "AZ_ERROR_NOT_ENOUGH_SPACE";
        break;
      case AZ_ERROR_UNEXPECTED_CHAR:
        http_status = 400;
        error_str = "AZ_ERROR_UNEXPECTED_CHAR";
        break;
      case AZ_ERROR_ARG:
        http_status = 400;
        error_str = "AZ_ERROR_ARG";
        break;
      case AZ_ERROR_ULIB_SYSTEM:
        http_status = 500;
        error_str = "AZ_ERROR_ULIB_SYSTEM";
        break;
      case AZ_ERROR_ITEM_NOT_FOUND:
        http_status = 404;
        error_str = "AZ_ERROR_ITEM_NOT_FOUND";
        break;
      case AZ_ERROR_NOT_SUPPORTED:
        http_status = 405;
        error_str = "AZ_ERROR_NOT_SUPPORTED";
        break;
      case AZ_ERROR_NOT_IMPLEMENTED:
        http_status = 501;
        error_str = "AZ_ERROR_NOT_IMPLEMENTED";
        break;
      case AZ_ERROR_ULIB_BUSY:
        http_status = 503;
        error_str = "AZ_ERROR_ULIB_BUSY";
        break;
      case AZ_ERROR_ULIB_INCOMPATIBLE_VERSION:
        http_status = 400;
        error_str = "AZ_ERROR_ULIB_INCOMPATIBLE_VERSION";
        break;
      default:
        http_status = 400;
        error_str = "Unknow error.";
        break;
    }
    (void)snprintf(result_buf, TEMP_BUFFER_SIZE, "{ \"description\":\"%s\" }", error_str);
    http_response = az_span_create_from_str(result_buf);
  }

  if (handle != NULL)
  {
    az_result result = az_ulib_ipc_release_interface(handle);
    (void)result;
  }

  if ((status = nx_azure_iot_hub_client_direct_method_message_response(
           &nx_context->iothub_client,
           http_status,
           context,
           context_length,
           (UCHAR*)az_span_ptr(http_response),
           (size_t)az_span_size(http_response),
           NX_WAIT_FOREVER)))
  {
    printf("Direct method response failed! (0x%08x)\r\n", status);
    return;
  }
}

static void device_twin_desired_property_cb(
    UCHAR* component_name,
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

static void device_twin_property_cb(
    UCHAR* component_name,
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
    NX_IP* ip_ptr,
    NX_PACKET_POOL* pool_ptr,
    NX_DNS* dns_ptr,
    UINT (*unix_time_callback)(ULONG* unix_time))
{
  UINT status;
  ULONG events = 0;

  if ((status = tx_event_flags_create(&azure_iot_flags, "Azure IoT flags")))
  {
    printf("FAIL: Unable to create nx_client event flags (0x%08x)\r\n", status);
    return status;
  }

  status = azure_iot_nx_client_create(
      &azure_iot_nx_client, ip_ptr, pool_ptr, dns_ptr, unix_time_callback, IOT_MODEL_ID);
  if (status != NX_SUCCESS)
  {
    printf("ERROR: azure_iot_nx_client_create failed (0x%08x)\r\n", status);
    return status;
  }

#ifdef ENABLE_X509
  status = azure_iot_nx_client_cert_set(
      &azure_iot_nx_client,
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
<<<<<<< HEAD
<<<<<<< HEAD
    status = azure_iot_nx_client_dps_create(&azure_iot_nx_client, IOT_DPS_ID_SCOPE, IOT_DPS_REGISTRATION_ID);
#else
    status = azure_iot_nx_client_hub_create(&azure_iot_nx_client, IOT_HUB_HOSTNAME, IOT_HUB_DEVICE_ID);
=======
  azure_iot_nx_client_dps_create(&azure_iot_nx_client, IOT_DPS_ID_SCOPE, IOT_DPS_REGISTRATION_ID);
#else
  azure_iot_nx_client_hub_create(&azure_iot_nx_client, IOT_HUB_HOSTNAME, IOT_HUB_DEVICE_ID);
>>>>>>> d207dcc (Add the management interface to the IPC with the capability of change the default interface. (#17))
=======
  azure_iot_nx_client_dps_create(&azure_iot_nx_client, IOT_DPS_ID_SCOPE, IOT_DPS_REGISTRATION_ID);
#else
  azure_iot_nx_client_hub_create(&azure_iot_nx_client, IOT_HUB_HOSTNAME, IOT_HUB_DEVICE_ID);
>>>>>>> 2aadf94142c2561b5ab0f45e65c1206a8651604d
#endif
  if (status != NX_SUCCESS)
  {
    printf("ERROR: azure_iot_nx_client_[hub|dps]_create failed (0x%08x)\r\n", status);
    return status;
  }

  // Register the callbacks
  azure_iot_nx_client_register_direct_method(&azure_iot_nx_client, direct_method_cb);
  azure_iot_nx_client_register_device_twin_desired_prop(
      &azure_iot_nx_client, device_twin_desired_property_cb);
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

  printf("\r\nReady!\r\n");

  while (true)
  {
    tx_event_flags_get(
        &azure_iot_flags,
        TELEMETRY_INTERVAL_EVENT,
        TX_OR_CLEAR,
        &events,
        telemetry_interval * NX_IP_PERIODIC_RATE);
  }

  return NX_SUCCESS;
}
