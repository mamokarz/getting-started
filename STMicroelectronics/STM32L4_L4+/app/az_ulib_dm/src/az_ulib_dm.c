// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "_az_ulib_dm_blob.h"
#include "_az_ulib_dm_interface.h"
#include "_az_ulib_dm_package.h"
#include "az_ulib_dm_api.h"
#include "az_ulib_ipc_api.h"
#include "az_ulib_ipc_interface.h"
#include "az_ulib_pal_os_api.h"
#include "az_ulib_port.h"
#include "az_ulib_result.h"
#include "azure/az_core.h"
#include "packages_1_model.h"
#include <azure/core/internal/az_precondition_internal.h>
#include <stdbool.h>
#include <string.h>

/*
 * DM is a singleton component, and shall be initialized only once.
 *
 * Make it volatile to avoid any compilation optimization.
 */
static az_ulib_dm* volatile _az_dm_cb = NULL;

extern void* __dcf_pgk_start;
extern uint32_t __SIZEOF_DCF_PKG;

static _az_ulib_dm_package* get_first_package_free(void)
{
  for (int i = 0; i < AZ_ULIB_CONFIG_MAX_DM_PACKAGES; i++)
  {
    if (_az_dm_cb->_internal.package_list[i].address == NULL)
    {
      return &_az_dm_cb->_internal.package_list[i];
    }
  }
  return NULL;
}

static _az_ulib_dm_package* get_package(az_span name)
{
  for (int i = 0; i < AZ_ULIB_CONFIG_MAX_DM_PACKAGES; i++)
  {
    if (_az_dm_cb->_internal.package_list[i].address != NULL)
    {
      if (az_span_is_content_equal(_az_dm_cb->_internal.package_list[i].name, name))
      {
        return &_az_dm_cb->_internal.package_list[i];
      }
    }
  }
  return NULL;
}

static bool does_package_fit(void* start_address, uint32_t size)
{
  void* end_address = (void*)((uint8_t*)start_address + size);

  if ((end_address >= (void*)((uint8_t*)&__dcf_pgk_start + (uint32_t)&__SIZEOF_DCF_PKG))
      || (start_address < (void*)&__dcf_pgk_start))
  {
    // If the package bypass the end of the reserved flash for packages.
    // Or it is before the starting of this area.
    // Return that package doesn't fits.
    return false;
  }

  // Look into all packages installed to figure out if the package fits.
  for (int i = 0; i < AZ_ULIB_CONFIG_MAX_DM_PACKAGES; i++)
  {
    void* installed_start_address = _az_dm_cb->_internal.package_list[i].address;
    if (installed_start_address != NULL)
    {
      void* installed_end_address = installed_start_address
          + *((uint32_t*)installed_start_address + _AZ_ULIB_DM_PACKAGE_PREAMBLE_CODE_SIZE);

      /* It shall cover all possibilities.
       * Package to install               |=========================|
       * ---------------------------------:---- Fail conditions ----:---------------------------
       * Package invade start         |===:===|                     :
       * Package invade end               :                     |===:========|
       * Old package inside               :        |==========|     :
       * New package inside           |===:=========================:====|
       * ---------------------------------:-- Success conditions ---:---------------------------
       * Old package before     |=======| :                         :
       * Old package after                :                         :    |=============|
       */
      if ((start_address <= installed_end_address) && (end_address >= installed_start_address))
      {
        return false;
      }
    }
  }
  return true;
}

AZ_NODISCARD az_result az_ulib_dm_init(az_ulib_dm* dm_handle)
{
  _az_PRECONDITION_IS_NULL(_az_dm_cb);
  _az_PRECONDITION_NOT_NULL(dm_handle);

  _az_dm_cb = dm_handle;

  az_pal_os_lock_init(&(_az_dm_cb->_internal.lock));

  for (int i = 0; i < AZ_ULIB_CONFIG_MAX_DM_PACKAGES; i++)
  {
    _az_ulib_dm_package* package = &(_az_dm_cb->_internal.package_list[i]);
    package->address = NULL;
    package->name = az_span_create(package->name_buf, sizeof(package->name_buf));
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

static az_result install_in_memory(void* base_address, az_span package_name)
{
  AZ_ULIB_TRY
  {
    uint32_t* preamble = (uint32_t*)base_address;
    
    /* Is this a known package? */
    AZ_ULIB_THROW_IF_ERROR(
        (*(preamble + _AZ_ULIB_DM_PACKAGE_PREAMBLE_ID) == _AZ_ULIB_DM_PACKAGE_ID),
        AZ_ERROR_ULIB_INCOMPATIBLE_VERSION);

    /* Does the package already exist? */
    AZ_ULIB_THROW_IF_ERROR((get_package(package_name) == NULL), AZ_ERROR_ULIB_ELEMENT_DUPLICATE);

    /* Does the package fits in the proposed space? */
    uint32_t size = *(preamble + _AZ_ULIB_DM_PACKAGE_PREAMBLE_CODE_SIZE);
    AZ_ULIB_THROW_IF_ERROR(does_package_fit(base_address, size), AZ_ERROR_NOT_ENOUGH_SPACE);

    /* Check for available space to store the package information. */
    _az_ulib_dm_package* package = get_first_package_free();
    AZ_ULIB_THROW_IF_ERROR((package != NULL), AZ_ERROR_NOT_ENOUGH_SPACE);

    /* Does the package RAM fits in the package data? */
    AZ_ULIB_THROW_IF_ERROR(
        *(preamble + _AZ_ULIB_DM_PACKAGE_PREAMBLE_DATA_SIZE) < sizeof(package->data),
        AZ_ERROR_NOT_ENOUGH_SPACE);

    /* Check if the package name fits in the reserved buffer. */
    if (az_span_size(package_name) > (int32_t)sizeof(package->name_buf))
    {
      AZ_ULIB_THROW(AZ_ERROR_NOT_ENOUGH_SPACE);
    }

    /*
     * Find the shell entry point in the package.
     */
    /* Get the shell entry point offset in the preamble. */
    uint32_t shell_entry_point_offset
        = *(preamble + _AZ_ULIB_DM_PACKAGE_PREAMBLE_SHELL_ENTRY_POINT);
    /* The position is relative to the offset position in the preamble. */
    shell_entry_point_offset += (_AZ_ULIB_DM_PACKAGE_PREAMBLE_SHELL_ENTRY_POINT << 2);
    /* Convert the offset to address by adding the base_address. */
    _az_ulib_dm_package_shell_entry_point entry_point
        = (_az_ulib_dm_package_shell_entry_point)((uint8_t*)base_address + shell_entry_point_offset);

    /*
     * Find the publsh interface function in the package.
     */
    /* Get the publish interface offset in the preamble. */
    uint32_t publish_interface_offset = *(preamble + _AZ_ULIB_DM_PACKAGE_PREAMBLE_PUBLISH);
    /* The position is relative to the offset position in the preamble. */
    publish_interface_offset += (_AZ_ULIB_DM_PACKAGE_PREAMBLE_PUBLISH << 2);
    /* Convert the offset to address by adding the base_address. */
    _az_ulib_dm_package_publish_interface publish
        = (_az_ulib_dm_package_publish_interface)((uint8_t*)base_address + publish_interface_offset);

    const az_ulib_ipc_table* table = az_ulib_ipc_get_table();
    AZ_ULIB_PORT_SET_DATA_CONTEXT(&(package->data));
    if (entry_point != NULL)
    {
      entry_point(base_address);
    }
    AZ_ULIB_THROW_IF_AZ_ERROR(publish(table));

    /* Store package information. */
    az_span_to_str((char*)package->name_buf, (int32_t)sizeof(package->name_buf), package_name);
    package->name = az_span_create(package->name_buf, az_span_size(package_name));
    package->address = base_address;
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static void* find_available_flash(uint32_t size)
{
  void* first_that_fit = (void*)&__dcf_pgk_start;
  for (int i = 0; i < AZ_ULIB_CONFIG_MAX_DM_PACKAGES; i++)
  {
    if (_az_dm_cb->_internal.package_list[i].address != NULL)
    {
      uint32_t installed_size
          = *((uint32_t*)(_az_dm_cb->_internal.package_list[i].address)
              + _AZ_ULIB_DM_PACKAGE_PREAMBLE_CODE_SIZE);
      installed_size = (installed_size & 0xFFFFF800) + 0x00000800;
      void* next_available_address
          = (uint8_t*)(_az_dm_cb->_internal.package_list[i].address) + installed_size;
      if (does_package_fit(next_available_address, size))
      {
        first_that_fit = next_available_address;
        break;
      }
      else
      {
        first_that_fit = NULL;
      }
    }
  }
  return first_that_fit;
}

static az_result install_from_blob(void* base_address, az_span package_name)
{
  AZ_ULIB_TRY
  {
    if (base_address == NULL)
    {
      // We are assuming that the package size if 0xFFFF, which is the maximum size defined in the
      // linker, however, this is not correct, we should get the size from the file.
      //
      // TODO: Replace the size 0xFFFF to the correct size of the file provided by the source of the
      // file.
      AZ_ULIB_THROW_IF_ERROR(
          ((base_address = find_available_flash(0xFFFF)) != NULL), AZ_ERROR_NOT_ENOUGH_SPACE);
    }

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
  (void)base_address;
  (void)package_name;

  /* TODO: implement intall from cli here. */

  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result
az_ulib_dm_install(packages_1_source_type source_type, void* base_address, az_span package_name)
{
  _az_PRECONDITION_NOT_NULL(_az_dm_cb);
  az_result result;

  az_pal_os_lock_acquire(&(_az_dm_cb->_internal.lock));
  {
    switch (source_type)
    {
      case PACKAGES_1_SOURCE_TYPE_IN_MEMORY:
        result = install_in_memory(base_address, package_name);
        break;
      case PACKAGES_1_SOURCE_TYPE_BLOB:
        result = install_from_blob(base_address, package_name);
        break;
      case PACKAGES_1_SOURCE_TYPE_CLI:
        result = install_from_cli(base_address, package_name);
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
    if ((package = get_package(package_name)) != NULL)
    {
      /* Get the unpublish interface offset in the preamble. */
      uint32_t unpublish_interface_offset
          = *((uint32_t*)package->address + _AZ_ULIB_DM_PACKAGE_PREAMBLE_UNPUBLISH);
      /* The position is relative to the offset position in the preamble. */
      unpublish_interface_offset += (_AZ_ULIB_DM_PACKAGE_PREAMBLE_UNPUBLISH << 2);
      /* Convert the offset to address by adding the base_address. */
      _az_ulib_dm_package_unpublish_interface unpublish
          = (_az_ulib_dm_package_unpublish_interface)((uint8_t*)package->address + unpublish_interface_offset);
      const az_ulib_ipc_table* table = az_ulib_ipc_get_table();

      AZ_ULIB_PORT_SET_DATA_CONTEXT(&(package->data));
      if ((result = unpublish(table)) == AZ_OK)
      {
        package->address = NULL;
        package->name = az_span_create(package->name_buf, sizeof(package->name_buf));
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
