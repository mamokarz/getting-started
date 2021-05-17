# DCF Invoke Device Method Using Azure CLI

## Prerequisites 
Please make sure you have completed the Getting Started Guide for the STM32 board and flashed the binary to your device before continuing this demo. [Instructions for STM32 Getting Started Guide](https://github.com/mamokarz/getting-started/blob/master/README.md)

## Open up an instance of Azure CLI 
You can use the Azure Portal or an instance on Powershell/bash/other local environments

Device Manager current support install code from 4 different sources.

0. **In memory**: Is the code that was brought to the memory using external interfaces (like JTag).
1. **From Blobs**: DM will download the code from Blobs using the provided URL.
2. **From CLI** (*not supported yet*): DM will copy the code from CLI to the memory.

## Commands to Run 

### In Memory example

Install an **In Memory** package assume that the binary is already in the MCU memory when the install command arrives. The main goal of this installation is to debug package's code. 
If properly configured, all debug tools applicable to a code built in the OS can be equally applied to a package using the DCF in memory installation.

The current example produces the key_vault_v1i1 and sprinkler_v1i1 packages. The CMake will create the key_vault_v1i1.elf and key_vault_v1i1.bin for key_vault_v1i1 package in ".\build\packages\key_vault_v1i1" directory, and sprinkler_v1i1.elf and sprinkler_v1i1.bin for sprinkler_v1i1 package in ".\build\packages\sprinkler_v1i1" directory. 

For now, both packages was created to run in a specific memory address, key_vault_v1i1 shall be installed in the address 0x08050000, and sprinkler_v1i1 in the address 0x08057000.

## Upload Binary to MCU Flash

Here are two ways to upload the binary to the MCU flash. Both examples use key_vault_v1i1, you can do the same for the sprinkler_v1i1 replacing the package name and the target address from 0x08050000 by 0x08057000.  

<details>
<summary>GDB Commands</summary>
<br>

One of the ways to upload the binary file is to use GDB commands.

```
restore build/packages/key_vault_v1i1/key_vault_v1i1.bin binary 0x08050000
```
and, if you are using GDB to debug your code, you can add the symbols as well. 
```
add-symbol-file build/packages/key_vault_v1i1/key_vault_v1i1.elf 0x08050080
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
- Click the `Binary File` tab and locate the `build/packages/key_vault_v1i1/key_vault_v1i1.bin` file. 

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

## Test the New Package

Now that you have the package in the memory, you can make it available to the device using DCF commands via invoke_device_method.

<details>
<summary>Click here for Portal/bash commands</summary>
<br>

Query for existing interfaces on the device 
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc_query.1.query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "ipc_query.1",
      "dm.1"
    ]
  },
  "status": 200
}

```


Install key_vault_v1i1 package in the address 134545408 [0x08050000] and sprinkler_v1i1 in the address 134574080 [0x08057000]
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.1.install" --mp "{\"source_type\":0,\"address\":134545408,\"package_name\":\"key_vault_v1i1\"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}

az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.1.install" --mp "{\"source_type\":0,\"address\":134574080,\"package_name\":\"sprinkler_v1i1\"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Query for existing interfaces on the device. You should be able to see the newly installed key_vault and sprinkler interfaces.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc_query.1.query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "ipc_query.1",
      "dm.1",
      "key_vault.1",
      "sprinkler.1"
    ]
  },
  "status": 200
}
```

Turn on the sprinkler, which will be modeled by turning on a LED on the STM Board
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "sprinkler.1.water_now" --mp "{\`"area\`":0}"

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Turn off the sprinkler, which will be modeled by turning off a LED on the STM Board
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "sprinkler.1.stop" --mp "{\`"area\`":0}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

We are now sending a message to the device and using the newly installed key_vault to encrypt the message "Welcome to Azure IoT!". The response will be the encrypted result of the message.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "key_vault.1.encrypt" --mp "{\"context\":0, \"src\":\"Welcome to Azure IoT!\"}" 

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
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "key_vault.1.decrypt" --mp "{\"src\":\"0ZldfV1pbUhhNXhJyTkBEUhhwX2Uh\"}" 

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
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc_query.1.query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "ipc_query.1",
      "dm.1"
    ]
  },
  "status": 200
}

```


Install key_vault_v1i1 package in the address 134545408 [0x08050000] and sprinkler_v1i1 in the address 134574080 [0x08057000]
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.1.install" --mp "{\`"source_type\`":0,\`"address\`":134545408,\`"package_name\`":\`"key_vault_v1i1\`"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}

az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.1.install" --mp "{\`"source_type\`":0,\`"address\`":134574080,\`"package_name\`":\`"sprinkler_v1i1\`"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Query for existing interfaces on the device. You should be able to see the newly installed key_vault and sprinkler interfaces.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc_query.1.query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "ipc_query.1",
      "dm.1",
      "key_vault.1",
      "sprinkler.1"
    ]
  },
  "status": 200
}
```

Turn on the sprinkler, which will be modeled by turning on a LED on the STM Board
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "sprinkler.1.water_now" --mp "{\`"area\`":0}"

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Turn off the sprinkler, which will be modeled by turning off a LED on the STM Board
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "sprinkler.1.stop" --mp "{\`"area\`":0}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

We are now sending a message to the device and using the newly installed key_vault to encrypt the message "Welcome to Azure IoT!". The response will be the encrypted result of the message.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "key_vault.1.encrypt" --mp "{\`"context\`":0, \`"src\`":\`"Welcome to Azure IoT!\`"}" 

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
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "key_vault.1.decrypt" --mp "{\`"src\`":\`"0ZldfV1pbUhhNXhJyTkBEUhhwX2Uh\`"}" 

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

### From Blob example

Once you have your package ready, it is time to distribute it to the devices. 

If you do not have a Azure Storage account, you may create one following the steps in the [Create a storage account](https://docs.microsoft.com/en-us/azure/storage/common/storage-account-create?toc=%2Fazure%2Fstorage%2Fblobs%2Ftoc.json&tabs=azure-portal).

**Note**: To distribute a package, you need a Azure Blob storage that allows **HTTP** connection, witch means that the option **Secure transfer required** shall be **Disabled**.

In the storage create a container for your packages:
  
  1. Navigate to your new storage account in the Azure portal.
  2. In the left menu for the storage account, scroll to the Data storage section, then select Blob containers.
  3. Select the `+ Container` button.
  4. Type a name `packages`.
  6. Select `Create` to create the container.

![AZ_BLOB_CREATE_CONTAINER](STMicroelectronics/STM32L4_L4+/media/azure_blob_create_container.PNG)

Upload your binary to the `packages` container

  1. Click in the new `packages` container to open it.
  2. Select the `Upload` button.
  3. In the left part of the Portal, find *sprinkler_v1i1.bin* in the directory ".\build\packages\sprinkler_v1i1".
  4. Select `Upload` to upload the file.

![AZ_BLOB_UPLOAD_FILE](STMicroelectronics/STM32L4_L4+/media/azure_blob_upload_file.PNG)

Create the URL with SAS for the package.

  1. Click in the file `sprinkler_v1i1.bin` to open it in the Portal.
  2. Select `Generate SAS` button.
  3. Change `Expiry` date to give you some time, for example a week from the `Start` date.
  4. Change `Allowed protocols` to `HTTPS and HTTP`.
  5. Select `Generate SAS token and URL` to create the URL.

![AZ_BLOB_GENERATE_SAS](STMicroelectronics/STM32L4_L4+/media/azure_blob_generate_sas.PNG)

Copy the `Blob SAS URL` to your clipboard.

![AZ_BLOB_SAS_URL](STMicroelectronics/STM32L4_L4+/media/azure_blob_sas_url.PNG)

Install you package to a remote device running DCF using invoke-device-method:

<details>
<summary>Click here for Portal/bash commands</summary>
<br>

Query for existing interfaces on the device, if you have sprinkler.1 installed, uninstall it before try to install the new package from Blobs.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc_query.1.query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "ipc_query.1",
      "dm.1"
    ]
  },
  "status": 200
}

```

Install the sprinkler_v1i1 in the address 134574080 [0x08057000] from the Blob storage (**source_type:1**), use the copied **Blob SAS URL** as `package_name`.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.1.install" --mp "{\"source_type\":1,\"address\":134574080,\"package_name\":\"https://mystorage.blob.core.windows.net/packages/sprinkler_v1i1.bin?sp=r&st=2021-05-17T22:11:04Z&se=2021-05-25T06:11:04Z&sv=2020-02-10&sr=b&sig=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Query for existing interfaces on the device. You should be able to see the newly installed key_vault and sprinkler interfaces.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc_query.1.query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "ipc_query.1",
      "dm.1",
      "sprinkler.1"
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

Query for existing interfaces on the device, if you have sprinkler.1 installed, uninstall it before try to install the new package from Blobs.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc_query.1.query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "ipc_query.1",
      "dm.1"
    ]
  },
  "status": 200
}

```

Install the sprinkler_v1i1 in the address 134574080 [0x08057000] from the Blob storage (**source_type:1**), use the copied **Blob SAS URL** as `package_name`.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "dm.1.install" --mp "{\`"source_type\`":1,\`"address\`":134574080,\`"package_name\`":\`"https://mystorage.blob.core.windows.net/packages/sprinkler_v1i1.bin?sp=r&st=2021-05-17T22:11:04Z&se=2021-05-25T06:11:04Z&sv=2020-02-10&sr=b&sig=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\`"}" 

// expected outcome
{
  "payload": {},
  "status": 200
}
```

Query for existing interfaces on the device. You should be able to see the newly installed key_vault and sprinkler interfaces.
```
az iot hub invoke-device-method -n [name-of-iothub] -d [name-of-device] --mn "ipc_query.1.query" --mp "{}"

// expected outcome
{
  "payload": {
    "continuation_token": 655615,
    "result": [
      "ipc_query.1",
      "dm.1",
      "sprinkler.1"
    ]
  },
  "status": 200
}
```

</details>
<br>


## Creating Your Own Package

It is possible to create your own DCF packages and install it onto the MCU's on-chip flash storage. Use this tutorial for [How to create a DCF package](STMicroelectronics/STM32L4_L4+/create_package.md).