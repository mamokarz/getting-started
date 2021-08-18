## Creating your own package

The following steps create a new package to be installed in the device. This package will implements a second version of the sprinkler that will expose the interface sprinkler.1.

### Infrastructure

Create a new directory called sprinkler.2 inside of the directory "samples".

```bash
cd samples
mkdir sprinkler.2
cd sprinkler.2
```

Similar to key_vault, the sprinkler needs a startup code. Copy the entire directory key_vault.1/startup to sprinkler.2.

```bash
copy -r ..\key_vault.1\startup\* startup
```

### Interface

The interface shall be composed by a description of the model and a concrete implementation, both following the DTDL definition. 

The sprinkler.2 will use the same interface of sprinkler.1, the main difference will be in the business logic, the version 2 will be able to control 3 different zones independently, while version 1 can control only 1 zone.

#### Model

The model shall describe the interface, including its name and version, and a list of capabilities with the input and output arguments for each capability.

Create a file called sprinkler_1_model.h with the following content in the directory "sprinkler.2".

```c
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#ifndef SPRINKLER_1_MODULE_H
#define SPRINKLER_1_MODULE_H

#include "az_ulib_capability_api.h"
#include "az_ulib_result.h"

#ifdef __cplusplus
#include <cstdint>
extern "C"
{
#else
#include <stdint.h>
#endif

/*
 * interface definition
 */
#define SPRINKLER_1_INTERFACE_NAME "sprinkler"
#define SPRINKLER_1_INTERFACE_VERSION 1

/*
 * Define water_now command on sprinkler interface.
 */
#define SPRINKLER_1_WATER_NOW_COMMAND (az_ulib_capability_index)0
#define SPRINKLER_1_WATER_NOW_COMMAND_NAME "water_now"
#define SPRINKLER_1_ZONE_NAME "zone"
#define SPRINKLER_1_TIMER_NAME "timer"
  typedef struct
  {
    int32_t zone;
    int32_t timer;
  } sprinkler_1_water_now_model_in;

/*
 * Define stop command on sprinkler interface.
 */
#define SPRINKLER_1_STOP_COMMAND (az_ulib_capability_index)1
#define SPRINKLER_1_STOP_COMMAND_NAME "stop"
  typedef struct
  {
    int32_t zone;
  } sprinkler_1_stop_model_in;

#ifdef __cplusplus
}
#endif

#endif /* SPRINKLER_1_MODULE_H */
```

This file starts with an interface definition, which defines that this is the "sprinkler" interface version 1 and contains 2 capabilities. After that, it describes the first capability, which is a command called "water_now" with input arguments "zone" and "timer", followed by the second capability "stop" with input argument "zone".

#### Concrete implementation

Create a file called sprinkler_interface.c with the following content in the directory "sprinkler.2" .

```c
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "az_ulib_capability_api.h"
#include "az_ulib_descriptor_api.h"
#include "az_ulib_ipc_interface.h"
#include "az_ulib_result.h"
#include "azure/az_core.h"
#include "sprinkler.h"
#include "sprinkler_1_model.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static az_result sprinkler_1_water_now_concrete(
    const sprinkler_1_water_now_model_in* const in,
    az_ulib_model_out out)
{
  (void)out;
  return sprinkler_v2i1_water_now(in->zone, in->timer);
}

static az_result sprinkler_1_water_now_span_wrapper(az_span model_in_span, az_span* model_out_span)
{
  AZ_ULIB_TRY
  {
    // Unmarshalling JSON in model_in_span to water_now_model_in.
    az_json_reader jr;
    sprinkler_1_water_now_model_in water_now_model_in = { 0 };
    AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_init(&jr, model_in_span, NULL));
    AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
    while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
    {
      if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR(SPRINKLER_1_ZONE_NAME)))
      {
        AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
        AZ_ULIB_THROW_IF_AZ_ERROR(az_span_atoi32(jr.token.slice, &(water_now_model_in.zone)));
      }
      if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR(SPRINKLER_1_TIMER_NAME)))
      {
        AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
        AZ_ULIB_THROW_IF_AZ_ERROR(az_span_atoi32(jr.token.slice, &(water_now_model_in.timer)));
      }
      AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
    }
    AZ_ULIB_THROW_IF_AZ_ERROR(AZ_ULIB_TRY_RESULT);

    // Call.
    AZ_ULIB_THROW_IF_AZ_ERROR(
        sprinkler_1_water_now_concrete((az_ulib_model_in)&water_now_model_in, NULL));

    // Marshalling empty water_now_model_out to JSON in model_out_span.
    *model_out_span = az_span_create_from_str("{}");
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static az_result sprinkler_1_stop_concrete(
    const sprinkler_1_stop_model_in* const in,
    az_ulib_model_out out)
{
  (void)out;
  return sprinkler_v2i1_stop(in->zone);
}

static az_result sprinkler_1_stop_span_wrapper(az_span model_in_span, az_span* model_out_span)
{
  AZ_ULIB_TRY
  {
    // Unmarshalling empty JSON in model_in_span to stop_model_in.
    az_json_reader jr;
    sprinkler_1_stop_model_in stop_model_in = { 0 };
    AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_init(&jr, model_in_span, NULL));
    AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
    AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
    while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
    {
      if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR(SPRINKLER_1_ZONE_NAME)))
      {
        AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
        AZ_ULIB_THROW_IF_AZ_ERROR(az_span_atoi32(jr.token.slice, &(stop_model_in.zone)));
      }
      AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
    }
    AZ_ULIB_THROW_IF_AZ_ERROR(AZ_ULIB_TRY_RESULT);

    // Call.
    AZ_ULIB_THROW_IF_AZ_ERROR(sprinkler_1_stop_concrete((az_ulib_model_in)&stop_model_in, NULL));

    // Marshalling empty stop_model_out to JSON in model_out_span.
    *model_out_span = az_span_create_from_str("{}");
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static const az_ulib_capability_descriptor SPRINKLER_1_CAPABILITIES[]
    = { AZ_ULIB_DESCRIPTOR_ADD_CAPABILITY(
            SPRINKLER_1_WATER_NOW_COMMAND_NAME,
            sprinkler_1_water_now_concrete,
            sprinkler_1_water_now_span_wrapper),
        AZ_ULIB_DESCRIPTOR_ADD_CAPABILITY(
            SPRINKLER_1_STOP_COMMAND_NAME,
            sprinkler_1_stop_concrete,
            sprinkler_1_stop_span_wrapper) };

static const az_ulib_interface_descriptor SPRINKLER_1_DESCRIPTOR = AZ_ULIB_DESCRIPTOR_CREATE(
    SPRINKLER_1_PACKAGE_NAME,
    SPRINKLER_1_PACKAGE_VERSION,
    SPRINKLER_1_INTERFACE_NAME,
    SPRINKLER_1_INTERFACE_VERSION,
    SPRINKLER_1_CAPABILITIES);

az_result publish_interface(const az_ulib_ipc_table* const table)
{
  return table->publish(&SPRINKLER_1_DESCRIPTOR, NULL);
}

az_result unpublish_interface(const az_ulib_ipc_table* const table)
{
  /**
   * @note: It is the user's responsibility to halt any processes that should not persist
   *        beyond package uninstall.
   */
  AZ_ULIB_TRY
  {
    // end sprinkler operations
    AZ_ULIB_THROW_IF_AZ_ERROR(sprinkler_v2i1_end());

    // unpublish interface
    AZ_ULIB_THROW_IF_AZ_ERROR(table->unpublish(&SPRINKLER_1_DESCRIPTOR, AZ_ULIB_NO_WAIT));
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}
```

Each capability may contain 2 functions, the `[capability]_concrete`, and `[capability]_span_wrapper`. Only the `[capability]_concrete` is required, it is the concrete implementation for the capability and shall be filled with the business logic. In this sample, the business logic is implemented in a separated file, so the concrete functions are only wrappers for the real business logic implementation. 

For example, in the `sprinkler_1_water_now_concrete`, the business logic in implemented in `sprinkler_v2i1_water_now`.

```c
static az_result sprinkler_1_water_now_concrete(
    const sprinkler_1_water_now_model_in* const in,
    az_ulib_model_out out)
{
  (void)out;
  return sprinkler_v2i1_water_now(in->zone, in->timer);
}
```

The `[capability]_span_wrapper` are optional, they are used by the IPC to call the capability using the arguments in JSON `az_span`. Those functions parse the JSON and call the `[capability]_concrete`. If a capability does not contains the `[capability]_span_wrapper`, the IPC will return *AZ_ERROR_NOT_SUPPORTED* if someone try to call this capability using JSON arguments. In the current implementation, we use an static library `az_core` to parse those JSON.

To publish an interface, it is necessary to create the interface descriptor `az_ulib_interface_descriptor`. IPC provides the macro `AZ_ULIB_DESCRIPTOR_CREATE` that will properly organize the interface information inside of the descriptor, this macro requires a list of capabilities. The macro `AZ_ULIB_DESCRIPTOR_ADD_CAPABILITY` helps to create each capability.

If a capability doesn't support JSON, you shall pass `NULL` in the 4th argument of `AZ_ULIB_DESCRIPTOR_ADD_CAPABILITY`.

The Device Manager will call the `publish_interface` after bring the code to the memory, and `unpublish_interface` before remove it from the memory. Both functions are required as is. 

### Business Logic

Create a file called "sprinkler.h" with the following content in the directory "sprinkler.2". 

```c
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#ifndef SPRINKLER_H
#define SPRINKLER_H

#include "az_ulib_result.h"
#include "azure/az_core.h"

#ifdef __cplusplus
#include <cstdint>
extern "C"
{
#else
#include <stdint.h>
#endif

#define SPRINKLER_1_PACKAGE_NAME "sprinkler"
#define SPRINKLER_1_PACKAGE_VERSION 2

  az_result sprinkler_v1i1_water_now(int32_t zone, int32_t timer);

  az_result sprinkler_v1i1_stop(int32_t zone);

  az_result sprinkler_v1i1_end(void);

#ifdef __cplusplus
}
#endif

#endif /* SPRINKLER_H */
```

And another file called "sprinkler.c" with the following content.

```c
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "stm32l475xx.h"
#include "stm32l4xx_hal.h"

#include "az_ulib_result.h"
#include "sprinkler.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

az_result sprinkler_v2i1_water_now(int32_t zone, int32_t timer)
{
  if (timer != 0)
  {
    return AZ_ERROR_NOT_SUPPORTED;
  }

  switch (zone)
  {
    case 0:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
      break;
    case 1:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
      break;
    case 2:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
      break;
    default:
      return AZ_ERROR_NOT_SUPPORTED;
  }

  return AZ_OK;
}

az_result sprinkler_v2i1_stop(int32_t zone)
{
  switch (zone)
  {
    case 0:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
      break;
    case 1:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
      break;
    case 2:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
      break;
    default:
      return AZ_ERROR_NOT_SUPPORTED;
  }

  return AZ_OK;
}

az_result sprinkler_v1i1_end(void) 
{ 
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
  return AZ_OK;
}
```

Both `water_now` and `stop` access the MCU pins using the STM32 HAL provided by the static library `stm32cubel4`. In this case, 3 solenoids will control the water flow in 3 different zones. To open and close those solenoids, we will use pins PB14, PB2 and PA4.

It is possible that your implementation need to take some actions before be uninstalled, for example, in this sample we don't want water open forever if the sprinkler is uninstalled. To do that, the `unpublish_interface` will call `sprinkler_v2i1_end` that close all zones.

### Building the package using CMake.

Create a file called CMakeLists.txt with the following content in the directory "sprinkler.2".

```
#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. 
#See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.10)

# Define the Project
project(sprinkler.2 C ASM)

add_executable(sprinkler.2
  startup/gnu/txm_module_preamble.s
  startup/gnu/gcc_setup.s
  startup/gnu/tx_initialize_low_level.s
  startup/gnu/precondition_fail.c
  ${CMAKE_CURRENT_LIST_DIR}/sprinkler_interface.c
  ${CMAKE_CURRENT_LIST_DIR}/sprinkler.c
)

set_target_properties(sprinkler.2
PROPERTIES 
    SUFFIX ".elf"
)

target_include_directories(sprinkler.2
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_options(sprinkler.2
    PRIVATE
        -fpie -fno-jump-tables ${PIC_FLAGS})

target_link_options(sprinkler.2
    PRIVATE 
        -T${CMAKE_CURRENT_LIST_DIR}/startup/gnu/STM32L475xx_FLASH.ld -pie -Wl,-Map=sprinkler.1.map)

target_link_libraries(sprinkler.2
  stm32cubel4
  az::ulib
  az::core_pic
)

add_custom_command(TARGET sprinkler.2 
    POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} ARGS -O binary -j .preamble -j .dynsym -j dynstr -j .rel.dyn 
          -j .rel.plt -j .plt -j .interp -j .hash -j .text -j .dtors -j .ctors -j .got -j .rodata 
          -j .fast -j .fast_run -j .data -j .data_run -j .bss -j .non_init -j .heap
        ${CMAKE_CURRENT_LIST_DIR}/../../build/samples/sprinkler.1/sprinkler.1.elf
        ${CMAKE_CURRENT_LIST_DIR}/../../build/samples/sprinkler.1/sprinkler.1.bin
    COMMENT "Converting the ELF output to a binary file. See https://manned.org/arm-none-eabi-objcopy for more information."
)
```

At this point, your sprinkler.2 project tree shall looks like:
```
└───sprinkler.2
    │   CMakeLists.txt
    │   sprinkler_1_model.h
    │   sprinkler.c
    │   sprinkler.h
    │   sprinkler_interface.c
    │
    └───startup
        └───gnu
                gcc_setup.s
                precondition_fail.c
                STM32L475xx_FLASH.ld
                txm_module_preamble.S
                tx_initialize_low_level.S
```

This CMake will create an executable sprinkler.2.elf that contains the files in the startup directory plus the concrete implementation of the interface and the business logic. Because it will expose an IPC interface, the library "az::ulib" is required. The library "az::core_pic" brings the implementation for the JSON parser, and "stm32cubel4" the HAL to access the MCU pins. 

Sprinkler.2.elf contains the binary code to run in the MCU together with a few more information, tools like jTag knows that and extract the binary when loading the .elf to the MCU. In our case, we need to extract the binary to write directly in the memory. The POST_BUILD ObjCopy do this extraction creating the sprinkler.2.bin.

Include the new sample in the CMake of the samples by adding `add_subdirectory(sprinkler.2)` in the samples/CMakeLists.txt.

To build the project, call again ".\tools\rebuild.bat". It will build the entire project creating all executables, including the new sprinkler.2.elf

```
[945/950] Linking C executable samples\sprinkler.2\sprinkler.2.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:          0 GB        64 KB      0.00%
             RAM:         152 B        256 B     59.38%
```

You can check the generated files at "/build/samples/sprinkler.2", there you will find sprinkler.2.elf and the sprinkler.2.bin that is the binary to upload to the MCU. 

## Upload Binary to MCU Flash

Use the same steps as earlier to flash this new package (for example, into address `0x08057000`) with the [GDB commands](../../DCF_Demo.md###GDB-Commands) or [ST-Link Utility tool](../../DCF_Demo.md###STM32-ST-Link-Utility).

## Test the New Package
Now, similar to key_vault.1, you can install sprinkler.2 using `invoke-device-method`. To install sprinkler.2 package in the address 134574080 [0x08057000] 
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.packages.1.install" --mp "{\"source_type\":0,\"address\":134574080,\"package_name\":\"sprinker.2\"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```
