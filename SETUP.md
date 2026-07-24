# Setup Guide

This guide covers environment preparation, target selection, display-profile selection, configuration, building, flashing, and troubleshooting for **ESP32-BACnet-Master**.

The firmware supports:

* ESP32
* ESP32-S3
* BACnet/IP over Wi-Fi
* BACnet MS/TP over RS485
* Sensirion SEN54
* Optional DS18B20
* Selectable display/UI profiles
* Optional Adafruit IO publishing

See also:

* [`README.md`](README.md) — project overview and BACnet object model
* [`Profiles.md`](Profiles.md) — display profiles and wiring
* [`OBJECTS_CONFIGURATION.md`](OBJECTS_CONFIGURATION.md) — BACnet object configuration and NVS persistence

---

## 1. Required environment

Use:

* ESP-IDF `5.5.4`
* Arduino-ESP32 `3.3.10`
* Visual Studio Code
* ESP-IDF VS Code extension
* Git

Arduino-ESP32 and the other managed dependencies are resolved by the ESP-IDF Component Manager.

The repository also provides:

```text
tools/build_idf55.ps1
```

This wrapper requires ESP-IDF to be installed at:

```text
C:\esp\v5.5.4\esp-idf
```

When ESP-IDF is installed elsewhere, use the ESP-IDF VS Code extension or an initialized ESP-IDF 5.5.4 terminal instead.

---

## 2. Open the project

Clone the repository:

```powershell
git clone https://github.com/MGuerrero31416/ESP32-BACnet-Master.git
cd ESP32-BACnet-Master
```

Alternatively, copy the complete project folder from another computer.

Open the repository root in Visual Studio Code:

```powershell
code .
```

The opened folder must be the one containing:

```text
CMakeLists.txt
README.md
SETUP.md
main/
components/
tools/
```

Do not open only the `main` subdirectory.

---

## 3. Generated configuration files

ESP-IDF generates these local files:

```text
build/
sdkconfig
sdkconfig.old
```

They are not project source files.

The active target and Menuconfig selections are stored in:

```text
sdkconfig
```

Deleting only `build/` does not change the selected ESP-IDF target.

When copying the project between computers, the copied `sdkconfig` may still contain the target and settings from the previous computer.

Do not edit `sdkconfig` manually.

To change the processor target, use the ESP-IDF target-selection command described below.

---

## 4. Target-specific defaults

The project supports target-specific ESP-IDF defaults.

The recommended layout is:

```text
sdkconfig.defaults
sdkconfig.defaults.esp32
sdkconfig.defaults.esp32s3
```

### `sdkconfig.defaults`

This file should contain only settings shared by both processor targets:

```ini
# Common project defaults

CONFIG_FREERTOS_HZ=1000

# Memory optimization while retaining Wi-Fi and BACnet/IP support
CONFIG_COMPILER_OPTIMIZATION_SIZE=y
# CONFIG_COMPILER_OPTIMIZATION_DEBUG is not set
# CONFIG_ESP_WIFI_IRAM_OPT is not set
# CONFIG_LWIP_IPV6 is not set
# CONFIG_FMB_MDNS_INTEGRATION_ENABLE is not set

# TLS certificate bundle for secure MQTT connections
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y
CONFIG_MBEDTLS_DEFAULT_CERTIFICATE_BUNDLE=y
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL=y
```

### `sdkconfig.defaults.esp32`

Use this file for the ESP32-WROOM-32 HW657A target:

```ini
# ESP32-WROOM-32 target defaults
# 4 MB flash, no PSRAM, single application partition

CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="4MB"

# CONFIG_SPIRAM is not set

CONFIG_PARTITION_TABLE_SINGLE_APP=y
CONFIG_PARTITION_TABLE_FILENAME="partitions_singleapp.csv"
```

### `sdkconfig.defaults.esp32s3`

Use the separate file provided with this guide for the original ESP32-S3 hardware:

```text
sdkconfig.defaults.esp32s3
```

It configures:

* 16 MB flash
* External PSRAM
* 80 MHz PSRAM
* Large single-app partition table

Target-specific defaults are applied only when a base `sdkconfig.defaults` file exists.

Existing values in a generated `sdkconfig` take precedence over these default files.

---

## 5. Select the ESP-IDF target

The processor target and display profile are separate selections.

### Using VS Code

Press:

```text
Ctrl+Shift+P
```

Run:

```text
ESP-IDF: Set Espressif Device Target
```

Select one of:

```text
esp32
esp32s3
```

Typical target selection:

| Hardware                                      | ESP-IDF target |
| --------------------------------------------- | -------------- |
| ESP32-WROOM-32 HW657A board                   | `esp32`        |
| Original ESP32-S3 board with external ST7796S | `esp32s3`      |

Allow ESP-IDF to reconfigure the project after changing the target.

### Using an ESP-IDF terminal

For ESP32:

```powershell
idf.py set-target esp32
```

For ESP32-S3:

```powershell
idf.py set-target esp32s3
```

The `set-target` operation regenerates the project configuration and may preserve the previous configuration as `sdkconfig.old`.

---

## 6. Reset a copied or stale configuration

Use this procedure when:

* The project was copied from another computer.
* The wrong processor target remains selected.
* Menuconfig shows unexpected options.
* Old configuration values continue to override defaults.

Close Menuconfig and delete:

```powershell
Remove-Item -Recurse -Force .\build -ErrorAction SilentlyContinue
Remove-Item -Force .\sdkconfig -ErrorAction SilentlyContinue
Remove-Item -Force .\sdkconfig.old -ErrorAction SilentlyContinue
```

Then select the target again through:

```text
ESP-IDF: Set Espressif Device Target
```

Do not delete:

```text
sdkconfig.defaults
sdkconfig.defaults.esp32
sdkconfig.defaults.esp32s3
```

Those are tracked project defaults.

---

## 7. Create the private settings file

Copy:

```text
main/User_Private_Settings.example.h
```

to:

```text
main/User_Private_Settings.h
```

PowerShell:

```powershell
Copy-Item `
  .\main\User_Private_Settings.example.h `
  .\main\User_Private_Settings.h
```

Edit the private file:

```c
#ifndef USER_PRIVATE_SETTINGS_H
#define USER_PRIVATE_SETTINGS_H

const char USER_WIFI_SSID[] = "YOUR_WIFI_SSID";
const char USER_WIFI_PASS[] = "YOUR_WIFI_PASSWORD";

const char USER_AIO_USERNAME[] = "USER_AIO_USERNAME";
const char USER_AIO_KEY[] = "USER_AIO_KEY";

#endif
```

Enter the real Wi-Fi credentials.

Enter the Adafruit IO username and key only when Adafruit IO publishing will be enabled.

The private file is excluded from Git and must not be committed.

---

## 8. Select the display profile

Open the ESP-IDF SDK Configuration Editor:

```text
Ctrl+Shift+P
ESP-IDF: SDK Configuration Editor
```

Navigate to:

```text
Project hardware
└── Display profile
```

Available profiles:

```text
ST7796S 480x320 3.5in - colorful UI
ST7796S 480x320 - test UI
ST7789 170x320 - HW657A - Simple UI
No display
```

The display-profile options are visible independently of whether the selected target is `esp32` or `esp32s3`.

However, the GPIO mapping selected by a display profile must be valid for the physical processor board.

See [`Profiles.md`](Profiles.md) for details.

---

## 9. Current sensor defaults associated with profiles

The current `main/Kconfig.projbuild` also derives the SEN54 pins and DS18B20 build option from the selected display profile.

| Display profile     | SEN54 SDA | SEN54 SCL | DS18B20  |
| ------------------- | --------: | --------: | -------- |
| ST7796S colorful UI |     GPIO4 |     GPIO5 | Enabled  |
| ST7796S test UI     |     GPIO4 |     GPIO5 | Enabled  |
| ST7789 HW657A       |    GPIO13 |    GPIO14 | Disabled |
| No display          |     GPIO4 |     GPIO5 | Disabled |

These are compile-time defaults.

The selected values are exposed as:

```c
CONFIG_USER_SEN54_SDA_GPIO
CONFIG_USER_SEN54_SCL_GPIO
CONFIG_USER_SENSOR_DS18B20
```

The sensor configuration is therefore still partially coupled to the selected display profile in the current repository.

---

## 10. Display source selection

The selected Kconfig option determines which UI source file is compiled.

Current mapping:

| Kconfig symbol                      | UI source                                      |
| ----------------------------------- | ---------------------------------------------- |
| `CONFIG_USER_DISPLAY_ST7796S`       | `main/ui/profiles/display_st7796s_current.cpp` |
| `CONFIG_USER_DISPLAY_ST7796S_TEST`  | `main/ui/profiles/display_st7796s_test.cpp`    |
| `CONFIG_USER_DISPLAY_ST7789_HW657A` | `main/ui/profiles/display_st7789_hw657a.cpp`   |
| `CONFIG_USER_DISPLAY_NONE`          | `main/ui/profiles/display_none.c`              |

TFT_eSPI hardware selection is performed in:

```text
components/TFT_eSPI/User_Setup.h
```

The panel-specific configurations are:

```text
components/TFT_eSPI/User_Setups/Setup_Project_ST7796S.h
components/TFT_eSPI/User_Setups/Setup_Project_ST7789_HW657A.h
```

---

## 11. Configure public project settings

Public device configuration is centralized in:

```text
main/User_Settings.c
main/User_Settings.h
```

### BACnet transports

Review:

```c
USER_ENABLE_BACNET_IP
USER_ENABLE_BACNET_MSTP
```

At least one BACnet transport should normally be enabled.

### BACnet Device identity

Configure a unique identity for each physical device:

```c
USER_BACNET_DEVICE_NAME
USER_BACNET_DEVICE_INSTANCE
USER_BACNET_DEVICE_DESCRIPTION
USER_BACNET_MODEL_NAME
USER_BACNET_VENDOR_NAME
USER_BACNET_VENDOR_ID
USER_BACNET_LOCATION
USER_BACNET_FIRMWARE_REVISION
USER_BACNET_APPLICATION_SOFTWARE_VERSION
USER_BACNET_SERIAL_NUMBER
```

The following values must be unique where required by the BACnet network:

```c
USER_BACNET_DEVICE_INSTANCE
USER_BACNET_DEVICE_NAME
USER_BACNET_SERIAL_NUMBER
```

### BACnet/IP

Review:

```c
USER_WIFI_USE_STATIC_IP
USER_WIFI_STATIC_IP_ADDR
USER_WIFI_STATIC_IP_GATEWAY
USER_WIFI_STATIC_IP_NETMASK
USER_WIFI_STATIC_DNS
```

BBMD foreign-device registration is configured through:

```c
USER_BBMD_IP_OCTET_1
USER_BBMD_IP_OCTET_2
USER_BBMD_IP_OCTET_3
USER_BBMD_IP_OCTET_4
USER_BBMD_PORT
USER_BBMD_TTL_SECONDS
```

### BACnet MS/TP

Review:

```c
USER_MSTP_MAC_ADDRESS
USER_MSTP_MAX_INFO_FRAMES
USER_MSTP_MAX_MASTER
USER_MSTP_BAUD_RATE
```

The current RS485 implementation uses:

```text
UART: UART2
TX:   GPIO17
RX:   GPIO16
DE:   GPIO5
```

Verify that these pins do not conflict with the selected hardware.

### Adafruit IO

Review:

```c
USER_ENABLE_ADAFRUIT_IO
USER_AIO_FEED_KEY
USER_AIO_PUBLISH_INTERVAL_SECONDS
```

The feed key must exactly match the Adafruit IO feed key.

Real Adafruit IO credentials remain in:

```text
main/User_Private_Settings.h
```

---

## 12. BACnet object configuration

The default object model contains:

| Object type   |  Count |
| ------------- | -----: |
| Analog Value  |     16 |
| Binary Value  |      4 |
| Analog Input  |      8 |
| Binary Input  |      4 |
| Binary Output |      4 |
| **Total**     | **36** |

The object counts and logical roles are defined in:

```text
main/User_Settings.h
```

Instances, names, descriptions, units, default values, COV increments, and binary text are defined by parallel arrays in:

```text
main/User_Settings.c
```

Default logical roles include:

* AI1–AI7: SEN54 measurements
* AI8: DS18B20 temperature
* AV1–AV4: SEN54 configuration
* AV5: DS18B20 temperature offset
* BV1–BV4: SEN54 commands
* BI1–BI4: SEN54 diagnostic states

See [`OBJECTS_CONFIGURATION.md`](OBJECTS_CONFIGURATION.md) before changing object counts or parallel arrays.

---

## 13. Sensor-service layout

The common acquisition task is:

```text
main/app/sensor_service.c
```

Sensor-specific services are:

```text
main/app/sensors/sen54_sensor_service.c
main/app/sensors/ds18b20_sensor_service.c
```

The DS18B20 source is added to the build only when:

```c
CONFIG_USER_SENSOR_DS18B20=y
```

The SEN54 I²C pins are supplied by:

```c
CONFIG_USER_SEN54_SDA_GPIO
CONFIG_USER_SEN54_SCL_GPIO
```

---

## 14. Build the project

### VS Code

Press:

```text
Ctrl+Shift+P
```

Run:

```text
ESP-IDF: Build your project
```

### Repository PowerShell wrapper

When ESP-IDF 5.5.4 is installed at:

```text
C:\esp\v5.5.4\esp-idf
```

run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\build_idf55.ps1 build
```

The wrapper verifies:

* The expected ESP-IDF path exists.
* The activated `IDF_PATH` is correct.
* The active ESP-IDF version is 5.5.4.

### Initialized ESP-IDF shell

From an initialized ESP-IDF 5.5.4 terminal:

```powershell
idf.py build
```

Do not invoke CMake or Ninja directly.

---

## 15. Flash and monitor

### VS Code

Use:

```text
ESP-IDF: Flash your project
ESP-IDF: Monitor your device
```

### Repository wrapper

Replace `COMx` with the actual serial port:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\build_idf55.ps1 `
  -p COMx flash monitor
```

### Initialized ESP-IDF shell

```powershell
idf.py -p COMx flash monitor
```

Exit the serial monitor with:

```text
Ctrl+]
```

---

## 16. NVS persistence

NVS initialization and the project persistence policy are handled by:

```text
main/app/app_storage.c
```

Writable BACnet properties and supported sensor configuration values are restored from NVS during startup.

Normal operation:

```c
const int USER_OVERRIDE_NVS_ON_FLASH = 0;
```

To erase stored values and restore the compiled defaults on the next startup:

```c
const int USER_OVERRIDE_NVS_ON_FLASH = 1;
```

Procedure:

1. Set the value to `1`.
2. Build and flash.
3. Boot the device once.
4. Return the value to `0`.
5. Build and flash again.

Leaving the setting at `1` erases and restores NVS on every boot.

---

## 17. Runtime startup flow

The current startup sequence in `main/main.c` is:

1. Initialize NVS and apply the persistence policy.
2. Print public user settings.
3. Initialize the FreeRTOS stack profiler.
4. Initialize the BACnet runtime.
5. Initialize the selected display implementation.
6. Start BACnet/IP and/or MS/TP tasks.
7. Start the common sensor task.
8. Start Adafruit IO when enabled.
9. Enter the application supervisor loop.

---

## 18. Verify the startup log

After flashing, confirm the log reports:

* Correct ESP32 or ESP32-S3 target.
* Correct flash size.
* Expected PSRAM status.
* Selected display/UI profile.
* TFT hardware setup and GPIO mapping.
* SEN54 initialization.
* DS18B20 enabled or disabled as expected.
* BACnet Device instance.
* BACnet/IP enabled or disabled.
* BACnet MS/TP enabled or disabled.
* MS/TP MAC address and baud rate.
* Adafruit IO enabled or disabled.
* No repeated reboot or watchdog reset.

For the ESP32-S3 profile, confirm that PSRAM is detected during boot.

---

## 19. Troubleshooting

### Only some display profiles are visible

The profile choices should no longer be restricted by processor target.

Run:

```text
ESP-IDF: Reconfigure Project
```

Then reopen:

```text
ESP-IDF: SDK Configuration Editor
```

If necessary, remove `build/` and the generated `sdkconfig`, then select the target again.

### The wrong processor target is still active

Deleting only `build/` is not sufficient.

Run:

```text
ESP-IDF: Set Espressif Device Target
```

or:

```powershell
idf.py set-target esp32
```

or:

```powershell
idf.py set-target esp32s3
```

### Target defaults are not applied

Check that all three files are named exactly:

```text
sdkconfig.defaults
sdkconfig.defaults.esp32
sdkconfig.defaults.esp32s3
```

Target-specific default files are ignored when the base `sdkconfig.defaults` file does not exist.

Also remember that an existing generated `sdkconfig` takes precedence over the default files.

### Build wrapper rejects the environment

The wrapper requires:

```text
C:\esp\v5.5.4\esp-idf
```

Use the VS Code ESP-IDF extension or an initialized ESP-IDF 5.5.4 shell when the installation uses another path.

### `User_Private_Settings.h` is missing

Copy:

```text
main/User_Private_Settings.example.h
```

to:

```text
main/User_Private_Settings.h
```

Do not commit the private file.

### Display does not initialize

Check:

```text
main/Kconfig.projbuild
main/CMakeLists.txt
components/TFT_eSPI/User_Setup.h
components/TFT_eSPI/User_Setups/
main/ui/profiles/
```

Confirm that the selected display profile matches the connected panel and wiring.

### Display profile compiles on the wrong board but does not work

The Kconfig options are target-independent, but the physical pin mapping is not.

Verify every selected TFT GPIO against the actual processor board before powering the display.

### SEN54 does not respond

Check the sensor defaults selected by the display profile:

```text
ST7796S: SDA GPIO4, SCL GPIO5
HW657A: SDA GPIO13, SCL GPIO14
```

Also verify:

* 3.3 V supply
* Ground
* I²C pull-ups
* SEN54 cable orientation
* I²C address `0x69`

### DS18B20 is not compiled

The current Kconfig enables DS18B20 automatically for the two ST7796S profiles and disables it for HW657A and No Display.

Check:

```c
CONFIG_USER_SENSOR_DS18B20
```

### BACnet objects do not match the configuration

Check:

```text
main/User_Settings.h
main/User_Settings.c
main/bacnet/objects/
```

Ensure all parallel arrays have exactly the counts defined in `User_Settings.h`.

### Old BACnet values reappear after flashing

The values are probably being restored from NVS.

Use the one-boot reset procedure through:

```c
USER_OVERRIDE_NVS_ON_FLASH
```

Do not permanently leave it set to `1`.
