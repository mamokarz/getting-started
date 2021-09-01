# DCF Invoke Device Method Using Azure CLI

## Prerequisites 
Please make sure you have completed the Getting Started Guide for the STM32 board and flashed the binary to your device before continuing this demo. [Instructions for STM32 Getting Started Guide](https://github.com/mamokarz/getting-started/blob/master/README.md)

## About DCF [Device Composition Framework]
DCF is a technology created by Microsoft to improve the way that developers embedded their code in a device connected to the cloud. It isolates the RTOS image from the product business logic adding the ability to install and uninstall packages in a running device and publish interfaces from these packages.

DCF uses the concept of “publish once, use everywhere”. So, if authorized, a published interface can be accessed locally in the device, by other packages, as much as from other devices or services in the cloud. 

For this demo, DCF uses a Gateway, connecting these interfaces to the Azure IoT Invoke Device Method.

### IDM [Invoke Device Method] Gateway
The DCF Gateway is responsible for **Naming** and **Protocol** translation.

#### Naming
Translates the name from an external network to the DCF internal address. In IDM the capability is identified as `--mn` or "method name". For example (using the Azure CLI to invoke a method):
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc.*.query.1:query" --mp "{}"
```
The `--mn` uses the format `<package_name>.<package_version>.<interface_name>.<interface_version>:<capability_name>`, so, "ipc.*.query.1:query" means:
  * `package_name` = `ipc`: Stands for the device IPC [Inter-process Communication].
  * `package_version` = `*`: Which is the wildcard that represents the *default* version of the `ipc` package.
  * `interface_name` = `query`: This is a standard interface to query information from a package.
  * `interface_version` = `1`: This is the version of the interface `query` implemented by the `ipc` package.
  * `capability_name` = `query`: The interface `query` implements the command `query` that starts a new query, and `next` that retrieves the next portion of information using the continuation token. So, in this example, we are invoking the `query` command.

#### Protocol translation
Translates the input and output arguments from the format used by the external network to a binary format used by the interface capabilities. For example:
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "key_vault.*.cipher.1:encrypt" --mp "{\"algorithm\":0, \"src\":\"Welcome to Azure IoT!\"}" 
```
The `--mp` or "method payload" contains 2 arguments using JSON format, however the capability `encrypt` expect the binary data in the structure
```c
  typedef struct
  {
    uint32_t algorithm;
    az_span src;
  } cipher_1_encrypt_model_in;
```
So, the gateway uses a JSON parser to execute this conversion.

## Open up an instance of Azure CLI 
You can use the Azure Portal or an instance on Powershell/bash/other local environments

Device Manager currently supports installing code from 2 different sources.

0. **In memory**: Is the code that was brought to the memory using external interfaces (like JTag).
1. **From Blobs**: DM will download the code from Blobs using the provided URL.

## In Memory example

Installing an **In Memory** package assumes that the binary is already in the MCU memory when the install command arrives. The main goal of this installation is to debug package's code. 
If properly configured, all debug tools applicable to a code built in the OS can be equally applied to a package using the DCF in memory installation.

The current example produces the key_vault.1, key_vault.2 and sprinkler.1 packages. The CMake will create the `.elf` and `.bin` for each package in the ".\build\samples" directory.

```
  .\build\samples\key_vault.1\key_vault.1.elf
  .\build\samples\key_vault.1\key_vault.1.bin
  .\build\samples\key_vault.2\key_vault.2.elf
  .\build\samples\key_vault.2\key_vault.2.bin
  .\build\samples\sprinkler.1\sprinkler.1.elf
  .\build\samples\sprinkler.1\sprinkler.1.bin
```

All packages use Position Independent Code (PIC), so they can be installed in any available memory in the MCU. For example, you may install key_vault.1 in the address 0x08050000, and sprinkler.1 in the address 0x08057000. The DM reserves the first 320K of Flash for the RTOS, so, you can chose any memory starting at 0x08050000.

### Upload Binary to MCU Flash

Here are two ways to upload the binary to the MCU flash. Both examples use key_vault.1. You can do the same for key_vault.2 and sprinkler.1, replacing the package name and the target address.

<details>
<summary>GDB Commands</summary>
<br>

One of the ways to upload the binary file is to use the following GDB commands.

```
load build/samples/key_vault.1/key_vault.1.elf 0x08050000
```
If you are using GDB to debug your code, you can add the symbols as well. Symbols shall be attached from the .text section. to find it, you can use the readelf tool.
```bash
arm-none-eabi-readelf.exe -S build/samples/key_vault.1/key_vault.1.elf
There are 30 section headers, starting at offset 0x5aa78:

Section Headers:
  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            00000000 000000 000000 00      0   0  0
  [ 1] .preamble         PROGBITS        00000000 010000 000080 00   A  0   0  1
  [ 2] .dynsym           DYNSYM          00000080 010080 000030 10   A  3   3  4
  [ 3] .dynstr           STRTAB          000000b0 0100b0 000004 00   A  0   0  1
  [ 4] .rel.dyn          REL             000000b4 0100b4 000180 08   A  2   0  4
  [ 5] .rel.plt          REL             00000234 010234 000000 08  AI  2  10  4
  [ 6] .plt              PROGBITS        00000234 010234 000000 04  AX  0   0  4
  [ 7] .interp           PROGBITS        00000234 010234 000014 00   A  0   0  1
  [ 8] .hash             HASH            00000248 010248 000018 04   A  2   0  4
  [ 9] .text             PROGBITS        00000260 010260 00598c 00  AX  0   0 16
  [10] .got              PROGBITS        00005bec 015bec 000044 04  WA  0   0  4
  [11] .rodata           PROGBITS        00005c30 015c30 00018d 00   A  0   0  4
  [12] .fast_run         PROGBITS        10000044 030098 000000 00   W  0   0  1
  [13] .data             PROGBITS        10000044 020044 000088 00  WA  0   0  4
  [14] .dynamic          DYNAMIC         10000000 030000 000098 08  WA  3   0  4
  [15] .data_run         NOBITS          10000044 030044 000088 00  WA  0   0  1
  [16] .heap             PROGBITS        100000cc 030098 000000 00   W  0   0  1
  [17] .ARM.attributes   ARM_ATTRIBUTES  00000000 030098 000030 00      0   0  1
  [18] .comment          PROGBITS        00000000 0300c8 000079 01  MS  0   0  1
  [19] .debug_line       PROGBITS        00000000 030141 006755 00      0   0  1
  [20] .debug_info       PROGBITS        00000000 036896 00d8b0 00      0   0  1
  [21] .debug_abbrev     PROGBITS        00000000 044146 001715 00      0   0  1
  [22] .debug_aranges    PROGBITS        00000000 045860 000448 00      0   0  8
  [23] .debug_str        PROGBITS        00000000 045ca8 00dac7 01  MS  0   0  1
  [24] .debug_ranges     PROGBITS        00000000 05376f 000418 00      0   0  1
  [25] .debug_macro      PROGBITS        00000000 053b87 003797 00      0   0  1
  [26] .debug_frame      PROGBITS        00000000 057320 001230 00      0   0  4
  [27] .symtab           SYMTAB          00000000 058550 001500 10     28 243  4
  [28] .strtab           STRTAB          00000000 059a50 000f14 00      0   0  1
  [29] .shstrtab         STRTAB          00000000 05a964 000112 00      0   0  1
```
As a result, you can see in Section 9, that `.text` starts at the relative position 0x00000260, which we shall add to the installed address 0x08050000, resulting in the address 0x08050260. The address may be different in your compilation.
```
add-symbol-file build/samples/key_vault.1/key_vault.1.elf 0x08050260
```

Because you have the symbol attached to the code, you can now debug your package's code using any GDB tools, including breakpoints and variable inspections. 

</details>
<br>

<details>
<summary>STM32 ST-Link Utility</summary>
<br>

- Locate the ST-Link Utility application. 
- If you can't locate it, refer to [Local Debugging with Visual Studio](docs/debugging-local.md) to install STLink Driver or download directly from [ST-Link Download Link](https://www.st.com/en/development-tools/stsw-link004.html) here.

![ST_Link_utility1](STMicroelectronics/STM32L4_L4+/media/STLink1.PNG)

- Plug the STM32 dev board to your machine. 
- Use the `Connect to the target` button to connect to your dev board and you should be able to see the content of the memory address '0x08050000'. 
- Click the `Binary File` tab and locate the `build/samples/key_vault.1/key_vault.1.bin` file. 

![ST_Link_utility2](STMicroelectronics/STM32L4_L4+/media/STLink2.PNG)

You should be able to see the content of this `.bin` package. 

![ST_Link_utility3](STMicroelectronics/STM32L4_L4+/media/STLink3.PNG)

Use the `Target->Program` option to choose where you want to flash the DCF package. 

![ST_Link_utility4](STMicroelectronics/STM32L4_L4+/media/STLink4.PNG)

- Use Starting Address as `0x08050000` and locate the `.bin` file from earlier. 
- Click `Start`. You should be able to see the package loaded into FLASH memory.

![ST_Link_utility5](STMicroelectronics/STM32L4_L4+/media/STLink5.PNG)

</details>
<br>

### Test the New in memory Package

For the next steps, you shall have the package key_vault.1 uploaded to the memory address 134545408 [0x08050000] and sprinkler.1 to the address 134574080 [0x08057000]. With the package in the memory, you can make it available to the device using DCF commands via invoke_device_method.

<details>
<summary>Click here for Portal/bash commands</summary>
<br>

Query for existing interfaces on the device 
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc.*.query.1:query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "*ipc.1.query.1",
      "*ipc.1.interface_manager.1",
      "*dm.1.packages.1"
    ]
  },
  "status": 200
}

```

**Note**: A `*` in front of a name indicates that this is the default package for this interface.

Install key_vault.1 package in the address 134545408 [0x08050000] and sprinkler.1 in the address 134574080 [0x08057000]
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.*.packages.1:install" --mp "{\"source_type\":0,\"address\":134545408,\"package_name\":\"key_vault.1\"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}

az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.*.packages.1:install" --mp "{\"source_type\":0,\"address\":134574080,\"package_name\":\"sprinkler.1\"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Query for existing interfaces on the device. You should be able to see the newly installed key_vault and sprinkler interfaces.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc.*.query.1:query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "*ipc.1.query.1",
      "*ipc.1.interface_manager.1",
      "*dm.1.packages.1",
      "*key_vault.1.cipher.1",
      "*sprinkler.1.sprinkler.1"
    ]
  },
  "status": 200
}
```

Turn on the sprinkler, which will be modeled by turning on a LED on the STM Board
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "sprinkler.*.sprinkler.1:water_now" --mp "{\"zone\":0}"

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Turn off the sprinkler, which will be modeled by turning off a LED on the STM Board
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "sprinkler.*.sprinkler.1:stop" --mp "{\"zone\":0}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

We are now sending a message to the device and using the newly installed key_vault to encrypt the message "Welcome to Azure IoT!". The response will be the encrypted result of the message.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "key_vault.*.cipher.1:encrypt" --mp "{\"algorithm\":0, \"src\":\"Welcome to Azure IoT!\"}" 

// expected outcome
{
  "payload": {
    "dest": "0ZldfV1pbUhhNXhJyTkBEUhhwX2Uh"
  },
  "status": 200
}
```

We are now sending the result of the encrypted message back to the device to decrypt, and we should get our original message back.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "key_vault.*.cipher.1:decrypt" --mp "{\"src\":\"0ZldfV1pbUhhNXhJyTkBEUhhwX2Uh\"}" 

// expected outcome
{
  "payload": {
    "dest": "Welcome to Azure IoT!"
  },
  "status": 200
}
```

</details>
<br>

<details>
<summary>Click here for Powershell commands</summary>
<br>

The commands are different for Powershell because you have to use ` to escape any " double quotes

<br>

Query for existing interfaces on the device 
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc.*.query.1:query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "*ipc.1.query.1",
      "*ipc.1.interface_manager.1",
      "*dm.1.packages.1"
    ]
  },
  "status": 200
}

```

**Note**: A `*` in front of a name indicates that this is the default package for this interface.

Install key_vault.1 package in the address 134545408 [0x08050000] and sprinkler.1 in the address 134574080 [0x08057000]
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.*.packages.1:install" --mp "{\`"source_type\`":0,\`"address\`":134545408,\`"package_name\`":\`"key_vault.1\`"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}

az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.*.packages.1:install" --mp "{\`"source_type\`":0,\`"address\`":134574080,\`"package_name\`":\`"sprinkler.1\`"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Query for existing interfaces on the device. You should be able to see the newly installed key_vault and sprinkler interfaces.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc.*.query.1:query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "*ipc.1.query.1",
      "*ipc.1.interface_manager.1",
      "*dm.1.packages.1",
      "*key_vault.1.cipher.1",
      "*sprinkler.1.sprinkler.1"
    ]
  },
  "status": 200
}
```

Turn on the sprinkler, which will be modeled by turning on a LED on the STM Board
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "sprinkler.*.sprinkler.1:water_now" --mp "{\`"zone\`":0}"

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Turn off the sprinkler, which will be modeled by turning off a LED on the STM Board
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "sprinkler.*.sprinkler.1:stop" --mp "{\`"zone\`":0}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

We are now sending a message to the device and using the newly installed key_vault to encrypt the message "Welcome to Azure IoT!". The response will be the encrypted result of the message.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "key_vault.*.cipher.1:encrypt" --mp "{\`"algorithm\`":0, \`"src\`":\`"Welcome to Azure IoT!\`"}" 

// expected outcome
{
  "payload": {
    "dest": "0ZldfV1pbUhhNXhJyTkBEUhhwX2Uh"
  },
  "status": 200
}
```

We are now sending the result of the encrypted message back to the device to decrypt, and we should get our original message back.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "key_vault.*.cipher.1:decrypt" --mp "{\`"src\`":\`"0ZldfV1pbUhhNXhJyTkBEUhhwX2Uh\`"}" 

// expected outcome
{
  "payload": {
    "dest": "Welcome to Azure IoT!"
  },
  "status": 200
}
```

</details>
<br>

## From Blob example

Once you have your package ready, it is time to distribute it to the devices. 

If you do not have an Azure Storage account, you may create one following the steps in the [Create a storage account](https://docs.microsoft.com/en-us/azure/storage/common/storage-account-create?toc=%2Fazure%2Fstorage%2Fblobs%2Ftoc.json&tabs=azure-portal).

**Note**: To distribute a package, you need a Azure Blob storage that allows **HTTP** connection, witch means that the option **Secure transfer required** shall be **Disabled**.

<details>
<summary>Upload binaries to Azure Blob storage.</summary>
<br>

In the storage create a container for your packages:
  
  1. Navigate to your new storage account in the Azure portal.
  2. In the left menu for the storage account, scroll to the Data storage section, then select Blob containers.
  3. Select the `+ Container` button.
  4. Type a name `packages`.
  6. Select `Create` to create the container.

![AZ_BLOB_CREATE_CONTAINER](STMicroelectronics/STM32L4_L4+/media/azure_blob_create_container.png)

Upload your binary to the `packages` container

  1. Click in the new `packages` container to open it.
  2. Select the `Upload` button.
  3. In the left part of the Portal, find *sprinkler.1.bin* in the directory ".\build\samples\sprinkler.1".
  4. Select `Upload` to upload the file.

![AZ_BLOB_UPLOAD_FILE](STMicroelectronics/STM32L4_L4+/media/azure_blob_upload_file.png)

</details>
<br>

<details>
<summary>Create the URL with SAS for the package.</summary>
<br>

  1. Click in the file `sprinkler.1.bin` to open it in the Portal.
  2. Select `Generate SAS` button.
  3. Change `Expiry` date to give you some time, for example a week from the `Start` date.
  4. Change `Allowed protocols` to `HTTPS and HTTP`.
  5. Select `Generate SAS token and URL` to create the URL.

![AZ_BLOB_GENERATE_SAS](STMicroelectronics/STM32L4_L4+/media/azure_blob_generate_sas.png)

Copy the `Blob SAS URL` to your clipboard.

![AZ_BLOB_SAS_URL](STMicroelectronics/STM32L4_L4+/media/azure_blob_sas_url.png)

</details>
<br>

Install you package to a remote device running DCF using invoke-device-method:

<details>
<summary>Click here for Portal/bash commands</summary>
<br>

If you have sprinkler.1 installed, uninstall it before trying to install the new package from Blobs.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.*.packages.1:uninstall" --mp "{\"package_name\":\"sprinkler.1\"}" 

// expected outcome: Azure CLI
{
  "payload": {},
  "status": 200
}
```

Query for existing interfaces on the device to make sure that you don't have the sprinkler.1 interface anymore.

```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc.*.query.1:query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "*ipc.1.query.1",
      "*ipc.1.interface_manager.1",
      "*dm.1.packages.1",
      "*key_vault.1.cipher.1"
    ]
  },
  "status": 200
}

```

Install the sprinkler.1 from the Blob storage (**source_type:1**), use the copied **Blob SAS URL** as `package_name`. Because we are not providing any specific address, the DM will choose one for us.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.*.packages.1:install" --mp "{\"source_type\":1,\"package_name\":\"https://mystorage.blob.core.windows.net/packages/sprinkler.1.bin?sp=r&st=2021-05-17T22:11:04Z&se=2021-05-25T06:11:04Z&sv=2020-02-10&sr=b&sig=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Query for existing interfaces on the device. You should be able to see the newly installed key_vault and sprinkler interfaces.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc.*.query.1:query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "*ipc.1.query.1",
      "*ipc.1.interface_manager.1",
      "*dm.1.packages.1",
      "*key_vault.1.cipher.1",
      "*sprinkler.1.sprinkler.1"
    ]
  },
  "status": 200
}
```

</details>
<br>

<details>
<summary>Click here for Powershell commands</summary>
<br>

The commands are different for Powershell because you have to use ` to escape any " double quotes

<br>

If you have sprinkler.1 installed, uninstall it before trying to install the new package from Blobs.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.*.packages.1:uninstall" --mp "{\`"package_name\`":\`"sprinkler.1\`"}" 

// expected outcome: Azure CLI
{
  "payload": {},
  "status": 200
}
```

Query for existing interfaces on the device to make sure that you don't have the sprinkler.1 interface anymore.

```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc.*.query.1:query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "*ipc.1.query.1",
      "*ipc.1.interface_manager.1",
      "*dm.1.packages.1",
      "*key_vault.1.cipher.1"
    ]
  },
  "status": 200
}

```

Install the sprinkler.1 from the Blob storage (**source_type:1**) and use the copied **Blob SAS URL** as `package_name`. Because we are not providing any specific address, the DM will choose one for us.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.*.packages.1:install" --mp "{\`"source_type\`":1,\`"package_name\`":\`"https://mystorage.blob.core.windows.net/packages/sprinkler.1.bin?sp=r&st=2021-05-17T22:11:04Z&se=2021-05-25T06:11:04Z&sv=2020-02-10&sr=b&sig=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\`"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Query for existing interfaces on the device. You should be able to see the newly installed key_vault and sprinkler interfaces.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc.*.query.1:query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "*ipc.1.query.1",
      "*ipc.1.interface_manager.1",
      "*dm.1.packages.1",
      "*key_vault.1.cipher.1",
      "*sprinkler.1.sprinkler.1"
    ]
  },
  "status": 200
}
```

</details>
<br>

## Safe package update

One of the advantages of using DCF is the safe package update mechanism. It allows developers and administrators to test new packages on the devices before officially switching, It also supports rollback to the old package if necessary. The tutorial [How to safe update a DCF package](DCF_Safe_Update.md) will take you through this process using 2 versions of the key_vault package created in this tutorial. 

## Creating Your Own Package

It is possible to create your own DCF packages and install it onto the MCU's on-chip flash storage. Use this tutorial for [How to create a DCF package](STMicroelectronics/STM32L4_L4+/create_package.md).
