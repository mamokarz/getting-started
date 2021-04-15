// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "az_ulib_result.h"
#include "az_ulib_dm_api.h"
#include "_az_ulib_dm_interface.h"
#include "azure/az_core.h"
#include "cipher_v1i1.h"
#include "cipher_v2i1.h"
#include "sprinkler_v1i1.h"
#include <azure/core/internal/az_precondition_internal.h>
#include <string.h>

/*
 * DM is a singleton component, and shall be initialized only once.
 *
 * Make it volatile to avoid any compilation optimization.
 */
static az_ulib_dm* volatile _az_dm_cb = NULL;

AZ_NODISCARD az_result az_ulib_dm_init(az_ulib_dm* dm_handle)
{
  _az_PRECONDITION_IS_NULL(_az_dm_cb);
  _az_PRECONDITION_NOT_NULL(dm_handle);

  _az_dm_cb = dm_handle;

  return _az_ulib_dm_interface_publish();
}

AZ_NODISCARD az_result az_ulib_dm_deinit(void)
{
  _az_PRECONDITION_NOT_NULL(_az_dm_cb);

  return _az_ulib_dm_interface_unpublish();
}

AZ_NODISCARD az_result az_ulib_dm_install(az_span package_name)
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

AZ_NODISCARD az_result az_ulib_dm_uninstall(az_span package_name)
{
  _az_PRECONDITION_NOT_NULL(_az_dm_cb);
  az_result result;

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
  else
  {
    result = AZ_ERROR_ITEM_NOT_FOUND;
  }

  return result;
}
