// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "_az_ulib_dm_blob.h"
#include "_az_ulib_dm_interface.h"
#include "az_ulib_result.h"
#include "az_ulib_dm_api.h"
#include "az_ulib_ipc_api.h"
#include "az_ulib_ipc_interface.h"
#include "az_ulib_pal_os_api.h"
#include "az_ulib_port.h"
#include "azure/az_core.h"
#include "cipher_v1i1.h"
#include "cipher_v2i1.h"
#include "dm_1_model.h"
#include "sprinkler_v1i1.h"
#include <azure/core/internal/az_precondition_internal.h>
#include <string.h>

/*
 * DM is a singleton component, and shall be initialized only once.
 *
 * Make it volatile to avoid any compilation optimization.
 */
static az_ulib_dm* volatile _az_dm_cb = NULL;

typedef az_result (*publish_interface)(const az_ulib_ipc_vtable* const vtable);
typedef az_result (*unpublish_interface)(const az_ulib_ipc_vtable* const vtable);

static _az_ulib_dm_package* get_first_package_free(void)
{
  for(int i = 0; i < AZ_ULIB_CONFIG_MAX_DM_PACKAGES; i++)
  {
    if(_az_dm_cb->_internal.package_list[i].address == NULL)
    {
      return &_az_dm_cb->_internal.package_list[i];
    }
  }
  return NULL;
}

static _az_ulib_dm_package* get_package(az_span name)
{
  for(int i = 0; i < AZ_ULIB_CONFIG_MAX_DM_PACKAGES; i++)
  {
    if(_az_dm_cb->_internal.package_list[i].address != NULL)
    {
      if(az_span_is_content_equal(_az_dm_cb->_internal.package_list[i].name, name))
      {
        return &_az_dm_cb->_internal.package_list[i];
      }
    }
  }
  return NULL;
}

AZ_NODISCARD az_result az_ulib_dm_init(az_ulib_dm* dm_handle)
{
  _az_PRECONDITION_IS_NULL(_az_dm_cb);
  _az_PRECONDITION_NOT_NULL(dm_handle);

  _az_dm_cb = dm_handle;

  az_pal_os_lock_init(&(_az_dm_cb->_internal.lock));

  for(int i = 0; i < AZ_ULIB_CONFIG_MAX_DM_PACKAGES; i++)
  {
    _az_ulib_dm_package* package = &(_az_dm_cb->_internal.package_list[i]);
    package->address = NULL;
    package->name = az_span_create(package->name_buf, AZ_ULIB_CONFIG_MAX_DM_PACKAGE_NAME);
    az_span_fill(package->name, '\0');
  }

  return _az_ulib_dm_interface_publish();
}

AZ_NODISCARD az_result az_ulib_dm_deinit(void)
{
  _az_PRECONDITION_NOT_NULL(_az_dm_cb);

  az_pal_os_lock_deinit(&(_az_dm_cb->_internal.lock));
  _az_dm_cb = NULL;

  return _az_ulib_dm_interface_unpublish();
}

static az_result install_built_in(az_span package_name)
{
  _az_PRECONDITION_NOT_NULL(_az_dm_cb);
  az_result result;

  if(az_span_is_content_equal(package_name, AZ_SPAN_FROM_STR("cipher_v1i1")))
  {
    result = cipher_v1i1_create();
  }
  else if(az_span_is_content_equal(package_name, AZ_SPAN_FROM_STR("cipher_v2i1")))
  {
    result = cipher_v2i1_create();
  }
  else if(az_span_is_content_equal(package_name, AZ_SPAN_FROM_STR("sprinkler_v1i1")))
  {
    result = sprinkler_v1i1_create();
  }
  else
  {
    result = AZ_ERROR_ITEM_NOT_FOUND;
  }

  return result;
}

static az_result install_in_memory(void* base_address, az_span package_name)
{
  _az_PRECONDITION_NOT_NULL(_az_dm_cb);

  AZ_ULIB_TRY
  {
    /* Is this a knowing package? */
    AZ_ULIB_THROW_IF_ERROR((*(uint32_t*)base_address == 0x4D4F4455), AZ_ERROR_ULIB_INCOMPATIBLE_VERSION);

    /* Does the package already exist? */
    AZ_ULIB_THROW_IF_ERROR((get_package(package_name) == NULL), AZ_ERROR_ULIB_ELEMENT_DUPLICATE);

    /* Check for available space to store the package information. */
    _az_ulib_dm_package* package = get_first_package_free();
    AZ_ULIB_THROW_IF_ERROR((package != NULL), AZ_ERROR_NOT_ENOUGH_SPACE);

    /* Call the publsh interface function in the package. */
    uint32_t publish_position = *((uint32_t*)base_address + AZ_ULIB_DM_PACKAGE_PUBLISH);
    publish_interface publish = (publish_interface)((uint8_t*)base_address + publish_position + 
        (AZ_ULIB_DM_PACKAGE_PUBLISH << 2));
    const az_ulib_ipc_vtable* vtable = az_ulib_ipc_get_vtable();
    AZ_ULIB_THROW_IF_AZ_ERROR(publish(vtable));

    /* Store package information. */
    az_span_copy(package->name, package_name);
    package->name = az_span_create(az_span_ptr(package->name), az_span_size(package_name));
    package->address = base_address;
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static az_result install_from_blob(void* base_address, az_span package_name)
{
  _az_PRECONDITION_NOT_NULL(_az_dm_cb);

  AZ_ULIB_TRY
  {
    AZ_ULIB_THROW_IF_AZ_ERROR(_az_ulib_dm_blob_download(base_address, package_name));

    az_span name = AZ_SPAN_EMPTY;
    AZ_ULIB_THROW_IF_AZ_ERROR(_az_ulib_dm_blob_get_package_name(package_name, &name));
    AZ_ULIB_THROW_IF_AZ_ERROR(install_in_memory(base_address, name));
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static az_result install_from_cli(void* base_address, az_span package_name)
{
  _az_PRECONDITION_NOT_NULL(_az_dm_cb);
  (void)base_address;
  (void)package_name;

  /* TODO: implement intall from cli here. */

  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_ulib_dm_install(dm_1_source_type source_type, void* base_address, az_span package_name)
{
  _az_PRECONDITION_NOT_NULL(_az_dm_cb);
  az_result result;

  az_pal_os_lock_acquire(&(_az_dm_cb->_internal.lock));
  {
    switch(source_type)
    {
      case DM_1_SOURCE_TYPE_IN_MEMORY:
        result = install_in_memory(base_address, package_name);
        break;
      case DM_1_SOURCE_TYPE_BLOB:
        result = install_from_blob(base_address, package_name);
        break;
      case DM_1_SOURCE_TYPE_CLI:
        result = install_from_cli(base_address, package_name);
        break;
      case DM_1_SOURCE_TYPE_BUILT_IN:
        result = install_built_in(package_name);
        break;
      default:
        result = AZ_ERROR_UNEXPECTED_CHAR;
        break;
    }
  }
  az_pal_os_lock_release(&(_az_dm_cb->_internal.lock));

  return result;
}

AZ_NODISCARD az_result az_ulib_dm_uninstall(az_span package_name)
{
  _az_PRECONDITION_NOT_NULL(_az_dm_cb);
  az_result result;
  _az_ulib_dm_package* package;

  az_pal_os_lock_acquire(&(_az_dm_cb->_internal.lock));
  {
    if(az_span_is_content_equal(package_name, AZ_SPAN_FROM_STR("cipher_v1i1")))
    {
      result = cipher_v1i1_destroy();
    }
    else if(az_span_is_content_equal(package_name, AZ_SPAN_FROM_STR("cipher_v2i1")))
    {
      result = cipher_v2i1_destroy();
    }
    else if(az_span_is_content_equal(package_name, AZ_SPAN_FROM_STR("sprinkler_v1i1")))
    {
      result = sprinkler_v1i1_destroy();
    }
    else if((package = get_package(package_name)) != NULL)
    {
      uint32_t unpublish_position = *((uint32_t*)package->address + AZ_ULIB_DM_PACKAGE_UNPUBLISH);
      unpublish_interface unpublish = 
          (unpublish_interface)((uint8_t*)package->address + unpublish_position + 
              (AZ_ULIB_DM_PACKAGE_UNPUBLISH << 2));
      const az_ulib_ipc_vtable* vtable = az_ulib_ipc_get_vtable();
      if((result = unpublish(vtable)) == AZ_OK)
      {
        package->address = NULL;
        package->name = az_span_create(package->name_buf, AZ_ULIB_CONFIG_MAX_DM_PACKAGE_NAME);
        az_span_fill(package->name, '\0');
      }
    }
    else
    {
      result = AZ_ERROR_ITEM_NOT_FOUND;
    }
  }
  az_pal_os_lock_release(&(_az_dm_cb->_internal.lock));

  return result;
}
