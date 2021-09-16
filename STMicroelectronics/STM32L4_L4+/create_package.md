## Creating your own package

The following steps create a new package to be installed in the device. This package will implements a second version of the sprinkler that will expose the interface sprinkler.1.

### Infrastructure

Create a new directory called sprinkler_v2i1 inside of the directory "packages".

```bash
cd packages
mkdir sprinkler_v2i1
cd sprinkler_v2i1
```

Similar to key_vault, the sprinkler needs a startup code. Copy the entire directory key_vault_v1i1/startup to sprinkler_v2i1.

```bash
copy -r ..\key_vault_v1i1\startup\* startup
```

Because DM does not support code with independent-position position yet, you need to change the target address of the package, so, edit the file "startup\STM32L475xx_FLASH.ld" to change the address 0x0805000 to 0x080570000.

```
MEMORY
{
  FLASH (rx) : ORIGIN = 0x08057000, LENGTH = 0x00006FFF
  RAM   (wx) : ORIGIN = 0, LENGTH = 0
}
```
And change the Flash segment start and end.  
```
  __FLASH_segment_start__ = 0x08057000;
  __FLASH_segment_end__   = 0x0805DFFF;
```

### Interface

The interface shall be composed by a description of the model and a concrete implementation, both following the DTDL definition. 

The sprinkler_v2i1 will use the same interface of sprinkler_v1i1, the main difference will be in the business logic, the version 2 will be able to control 3 different areas independently, while version 1 can control only 1 area.

#### Model

The model shall describe the interface, including its name and version, and a list of capabilities with the input and output arguments for each capability.

Create a file called sprinkler_1_model.h with the following content in the directory "sprinkler_v2i1".

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
#define SPRINKLER_1_CAPABILITY_SIZE 2

/*
 * Define water_now command on sprinkler interface.
 */
#define SPRINKLER_1_WATER_NOW_COMMAND (az_ulib_capability_index)0
#define SPRINKLER_1_WATER_NOW_COMMAND_NAME "water_now"
#define SPRINKLER_1_AREA_NAME "area"
#define SPRINKLER_1_TIMER_NAME "timer"
  typedef struct
  {
    int32_t area;
    int32_t timer;
  } sprinkler_1_water_now_model_in;

/*
 * Define stop command on sprinkler interface.
 */
#define SPRINKLER_1_STOP_COMMAND (az_ulib_capability_index)1
#define SPRINKLER_1_STOP_COMMAND_NAME "stop"
  typedef struct
  {
    int32_t area;
  } sprinkler_1_stop_model_in;

#ifdef __cplusplus
}
#endif

#endif /* SPRINKLER_1_MODULE_H */
```

This file starts with an interface definition, which defines that this is the "sprinkler" interface version 1 and contains 2 capabilities. After that, it describes the first capability, which is a command called "water_now" with input arguments "area" and "timer", followed by the second capability "stop" with input argument "area".

#### Concrete implementation

Create a file called sprinkler_v2i1_interface.c with the following content in the directory "sprinkler_v2i1" .

```c
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "az_ulib_capability_api.h"
#include "az_ulib_descriptor_api.h"
#include "az_ulib_ipc_interface.h"
#include "az_ulib_result.h"
#include "azure/az_core.h"
#include "sprinkler_v2i1.h"
#include "sprinkler_1_model.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static az_result sprinkler_1_water_now_concrete(az_ulib_model_in model_in, az_ulib_model_out model_out)
{
  (void)model_out;
  const sprinkler_1_water_now_model_in* const in = (const sprinkler_1_water_now_model_in* const)model_in;
  return sprinkler_v2i1_water_now(in->area, in->timer);
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
    AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
    while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
    {
      if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR(SPRINKLER_1_AREA_NAME)))
      {
        AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
        AZ_ULIB_THROW_IF_AZ_ERROR(az_span_atoi32(jr.token.slice, &(water_now_model_in.area)));
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
    AZ_ULIB_THROW_IF_AZ_ERROR(sprinkler_1_water_now_concrete((az_ulib_model_in)&water_now_model_in, NULL));

    // Marshalling empty water_now_model_out to JSON in model_out_span.
    *model_out_span = az_span_create_from_str("{}");
  }
  AZ_ULIB_CATCH(...) {}

  return AZ_ULIB_TRY_RESULT;
}

static az_result sprinkler_1_stop_concrete(az_ulib_model_in model_in, az_ulib_model_out model_out)
{
  (void)model_in;
  const sprinkler_1_stop_model_in* const in = (const sprinkler_1_stop_model_in* const)model_in;
  return sprinkler_v2i1_stop(in->area);
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
      if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR(SPRINKLER_1_AREA_NAME)))
      {
        AZ_ULIB_THROW_IF_AZ_ERROR(az_json_reader_next_token(&jr));
        AZ_ULIB_THROW_IF_AZ_ERROR(az_span_atoi32(jr.token.slice, &(stop_model_in.area)));
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

static const az_ulib_capability_descriptor SPRINKLER_1_CAPABILITIES[SPRINKLER_1_CAPABILITY_SIZE] = {
  AZ_ULIB_DESCRIPTOR_ADD_COMMAND(
      SPRINKLER_1_WATER_NOW_COMMAND_NAME,
      sprinkler_1_water_now_concrete,
      sprinkler_1_water_now_span_wrapper),
  AZ_ULIB_DESCRIPTOR_ADD_COMMAND(
      SPRINKLER_1_STOP_COMMAND_NAME, 
      sprinkler_1_stop_concrete, 
      sprinkler_1_stop_span_wrapper)
};

static const az_ulib_interface_descriptor SPRINKLER_1_DESCRIPTOR = AZ_ULIB_DESCRIPTOR_CREATE(
    SPRINKLER_1_INTERFACE_NAME,
    SPRINKLER_1_INTERFACE_VERSION,
    SPRINKLER_1_CAPABILITY_SIZE,
    SPRINKLER_1_CAPABILITIES);

az_result publish_interface(const az_ulib_ipc_vtable* const vtable)
{
  return vtable->publish(&SPRINKLER_1_DESCRIPTOR, NULL);
}

az_result unpublish_interface(const az_ulib_ipc_vtable* const vtable)
{
  return vtable->unpublish(&SPRINKLER_1_DESCRIPTOR, AZ_ULIB_NO_WAIT);
}
```

Each capability may contain 2 functions, the `[capability]_concrete`, and `[capability]_span_wrapper`. Only the `[capability]_concrete` is required, it is the concrete implementation for the capability and shall be filled with the business logic. In this sample, the business logic is implemented in a separated file, so the concrete functions are only wrappers for the real business logic implementation. 

For example, in the `sprinkler_1_water_now_concrete`, the business logic in implemented in `sprinkler_v2i1_water_now`.

```c
static az_result sprinkler_1_water_now_concrete(az_ulib_model_in model_in, az_ulib_model_out model_out)
{
  (void)model_out;
  const sprinkler_1_water_now_model_in* const in = (const sprinkler_1_water_now_model_in* const)model_in;
  return sprinkler_v2i1_water_now(in->area, in->timer);
}
```

The `[capability]_span_wrapper` are optional, they are used by the IPC to call the capability using the arguments in JSON `az_span`. Those functions parse the JSON and call the `[capability]_concrete`. If a capability does not contains the `[capability]_span_wrapper`, the IPC will return *AZ_ERROR_NOT_SUPPORTED* if someone try to call this capability using JSON arguments. In the current implementation, we use an static library `az_core` to parse those JSON.

To publish an interface, it is necessary to create the interface descriptor `az_ulib_interface_descriptor`. IPC provides the macro `AZ_ULIB_DESCRIPTOR_CREATE` that will properly organize the interface information inside of the descriptor, this macro requires a list of capabilities. The macro `AZ_ULIB_DESCRIPTOR_ADD_COMMAND` helps to create each capability.

  **NOTE**: Currently IPC only support Commands. Add support for Properties, Telemetry and Command Async are in our backlog. 

If a capability doesn't support JSON, you shall pass `NULL` in the 4th argument of `AZ_ULIB_DESCRIPTOR_ADD_COMMAND`.

The Device Manager will call the `publish_interface` after bring the code to the memory, and `unpublish_interface` before remove it from the memory. Both functions are required as is. 

### Business Logic

Create a file called sprinkler_v2i1.h with the following content in the directory "sprinkler_v2i1". 

```c
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#ifndef SPRINKLER_V2I1_H
#define SPRINKLER_V2I1_H

#include "az_ulib_result.h"

#ifdef __cplusplus
#include <cstdint>
extern "C"
{
#else
#include <stdint.h>
#endif

  az_result sprinkler_v2i1_water_now(int32_t area, int32_t timer);

  az_result sprinkler_v2i1_stop(int32_t area);

#ifdef __cplusplus
}
#endif

#endif /* SPRINKLER_V2I1_H */
```

And another file called sprinkler_v2i1.c with the following content.

```c
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
// See LICENSE file in the project root for full license information.

#include "stm32l4xx_hal.h"
#include "stm32l475xx.h"

#include "az_ulib_result.h"
#include "sprinkler_v2i1.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

az_result sprinkler_v2i1_water_now(int32_t area, int32_t timer)
{
  if(timer != 0)
  {
    return AZ_ERROR_NOT_SUPPORTED;
  }

  switch(area)
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

az_result sprinkler_v2i1_stop(int32_t area)
{
  switch(area)
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
```

Both `water_now` and `stop` access the MCU pins using the STM32 HAL provided by the static library `stm32cubel4`. In this case, 3 solenoids will control the water flow in 3 different areas. To open and close those solenoids, we will use pins PB14, PB2 and PA4.

### Building the package using CMake.

Create a file called CMakeLists.txt with the following content in the directory "sprinkler_v2i1".

```
#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. 
#See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.10)

# Define the Project
project(sprinkler_v2i1 C ASM)

add_executable(sprinkler_v2i1
  startup/gnu/txm_module_preamble.s
  startup/gnu/gcc_setup.s
  startup/gnu/tx_initialize_low_level.s
  startup/gnu/precondition_fail.c
  ${CMAKE_CURRENT_LIST_DIR}/sprinkler_v2i1_interface.c
  ${CMAKE_CURRENT_LIST_DIR}/sprinkler_v2i1.c
)

set_target_properties(sprinkler_v2i1
PROPERTIES 
    SUFFIX ".elf"
)

target_include_directories(sprinkler_v2i1
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_options(sprinkler_v2i1
    PRIVATE 
        -T${CMAKE_CURRENT_LIST_DIR}/startup/gnu/STM32L475xx_FLASH.ld -Wl,-Map=sprinkler_v2i1.map)

target_link_libraries(sprinkler_v2i1
  stm32cubel4
  az::ulib
  az::core
)

add_custom_command(TARGET sprinkler_v2i1 
    POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} ARGS -O binary 
        ${CMAKE_CURRENT_LIST_DIR}/../../build/packages/sprinkler_v2i1/sprinkler_v2i1.elf
        ${CMAKE_CURRENT_LIST_DIR}/../../build/packages/sprinkler_v2i1/sprinkler_v2i1.bin
    COMMENT "Converting the ELF output to a binary file"
)
```

An add `add_subdirectory(sprinkler_v2i1)` in the packages/CMakeLists.txt.

At this point, your sprinkler_v2i1 project tree shall looks like:
```
└───sprinkler_v2i1
    │   CMakeLists.txt
    │   sprinkler_1_model.h
    │   sprinkler_v2i1.c
    │   sprinkler_v2i1.h
    │   sprinkler_v2i1_interface.c
    │
    └───startup
        └───gnu
                gcc_setup.s
                precondition_fail.c
                STM32L475xx_FLASH.ld
                txm_module_preamble.S
                tx_initialize_low_level.S
```

This CMake will create an executable sprinkler_v2i1.elf that contains the files in the startup directory plus the concrete implementation of the interface and the business logic. Because it will expose an IPC interface, the library "az::ulib" is required. The library "az::core" brings the implementation for the JSON parser, and "stm32cubel4" the HAL to access the MCU pins. 

Sprinkler_v1i1.elf contains the binary code to run in the MCU together with a few more information, tools like jTag knows that and extract the binary when loading the .elf to the MCU. In our case, we need to extract the binary to write directly in the memory. The POST_BUILD ObjCopy do this extraction creating the sprinkler_v2i1.bin.

To build the project, call again ".\tools\rebuild.bat". It will build the entire project creating 4 executables, including the new sprinkler_v2i1.elf

```
[923/931] Linking C executable packages\key_vault_v1i1\key_vault_v1i1.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:          0 GB      28671 B      0.00%
             RAM:          0 GB         0 GB     -1.#J%
[925/931] Linking C executable packages\sprinkler_v2i1\sprinkler_v2i1.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:          0 GB      28671 B      0.00%
             RAM:          0 GB         0 GB     -1.#J%
[928/931] Linking C executable app\stm32l475_azure_iot.elf
Memory region         Used Size  Region Size  %age Used
             RAM:       85216 B        96 KB     86.69%
            RAM2:          0 GB        32 KB      0.00%
           FLASH:      284144 B         1 MB     27.10%
[929/931] Linking C executable app\stm32l4S5_azure_iot.elf
Memory region         Used Size  Region Size  %age Used
             RAM:       85216 B       640 KB     13.00%
           FLASH:      284192 B         2 MB     13.55%
```

You can check the generated files at "/build/packages/sprinkler_v2i1", there you will find sprinkler_v2i1.elf and the sprinkler_v2i1.bin that is the binary to upload to the MCU. 

## Upload Binary to MCU Flash

### GDB Commands

To upload this code to the MCU, you can use, for example, GDB commands, it is important to remember that the code was build to the address 0x08057000.

```
restore build/packages/sprinkler_v2i1/sprinkler_v2i1.bin binary 0x08057000
add-symbol-file build/packages/sprinkler_v2i1/sprinkler_v2i1.elf 0x08057080
```

### STM32 ST-Link Utility

Use the same steps as earlier to flash this new package into address `0x08057000` with the ST-Link Utility tool. Reference this link [How to flash new package into device memory with ST-Link](../../DCF_Demo.md###STM32-ST-Link-Utility) from earlier. 

## Test the New Package
Now, similar to key_vault_v1i1, you can install sprinkler_v2i1 using `invoke-device-method`. To install sprinker_v2i1 package in the address 134574080 [0x08057000] 
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.1.install" --mp "{\"source_type\":0,\"address\":134574080,\"package_name\":\"sprinker_v2i1\"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```
