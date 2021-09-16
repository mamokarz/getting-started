// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "azure/az_core.h"

static void az_precondition_failed_default(void)
{
  /* By default, when a precondition fails the calling thread spins forever */
  while (1)
  {
  }
}

az_precondition_failed_fn az_precondition_failed_get_callback(void)
{
  return az_precondition_failed_default;
}
