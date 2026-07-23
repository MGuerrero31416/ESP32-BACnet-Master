# Setup Guide

This project is currently documented for the active build and source layout in this repository.

## Required Environment

Use only the following toolchain combination:

- ESP-IDF `5.5.4`
- `IDF_PATH = C:\esp\v5.5.4\esp-idf`
- target `esp32s3`
- Arduino-ESP32 `3.3.10`

The repository includes a build wrapper that checks the environment before building.

## Project Preparation

1. Clone or copy the repository.
2. Open the repository root in VS Code.
3. Ensure your ESP-IDF environment is the required `5.5.4` installation.
4. Create the private Wi-Fi header:
   - Copy `main/User_Private_Settings.example.h`
   - Save it as `main/User_Private_Settings.h`
5. Fill in `USER_WIFI_SSID` and `USER_WIFI_PASS` in `main/User_Private_Settings.h`.
6. Review tracked defaults in `main/User_Settings.c`.

## What to Configure

### Private Wi-Fi Settings

Private Wi-Fi credentials are intentionally separated from tracked source.

- Template: `main/User_Private_Settings.example.h`
- Private file: `main/User_Private_Settings.h`

Only the private header should contain the real SSID and password.

### BACnet and Device Defaults

Adjust `main/User_Settings.c` and `main/User_Settings.h` for:

- BACnet/IP enable flag
- BACnet MS/TP enable flag
- device name and device instance
- BBMD foreign device registration settings
- static IP behavior
- object counts
- object instance arrays
- default object names, descriptions, units, values, and COV increments

### Current Code Layout

The current code is split across:

- `main/main.c`
- `main/app/`
- `main/bacnet/`
- `main/platform/`
- `main/ui/`

## Build

Build only with the repository script:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\build_idf55.ps1 build
```

Do not use direct `idf.py`, CMake, or Ninja commands for the documented build flow in this repository.

## Display Stack

The active display stack uses `TFT_eSPI`.

- UI implementation: `main/ui/display.cpp`
- TFT configuration: `components/TFT_eSPI/User_Setup.h`

Current ST7796S SPI pin mapping:

- MOSI: GPIO10
- SCLK: GPIO9
- CS: GPIO13
- DC: GPIO12
- RST: GPIO11
- Backlight: GPIO14

## Sensor and Object Layout

This firmware exposes 36 BACnet objects:

- 16 AV
- 4 BV
- 8 AI
- 4 BI
- 4 BO

Default sensor mapping for the configured Analog Inputs is:

- `AI1`: SEN54 temperature
- `AI2`: SEN54 relative humidity
- `AI3`: SEN54 VOC index
- `AI4`: SEN54 PM1.0
- `AI5`: SEN54 PM2.5
- `AI6`: SEN54 PM4.0
- `AI7`: SEN54 PM10
- `AI8`: DS18B20 temperature

The sensor acquisition logic lives in `main/app/sensor_service.c`.

## NVS Persistence

NVS is initialized by `main/app/app_storage.c`.

BACnet object modules under `main/bacnet/objects/` load and save persisted object data. Existing NVS data is normally preserved across boots.

To force compiled defaults to overwrite persisted state on the next boot, set:

```c
USER_OVERRIDE_NVS_ON_FLASH = 1;
```

in `main/User_Settings.c`, then rebuild and reboot once. Return it to `0` for normal persistence afterward.

## Main Runtime Flow

The current startup sequence in `main/main.c` is:

1. initialize NVS and persistence policy
2. initialize BACnet runtime
3. initialize the display
4. start BACnet tasks
5. start the sensor service
6. enter the application supervisor loop

## Troubleshooting

### Build script rejects the environment

Verify that the active shell is using ESP-IDF `5.5.4` from `C:\esp\v5.5.4\esp-idf`.

### Wi-Fi does not connect

Check:

- `main/User_Private_Settings.h` for SSID and password
- `USER_WIFI_USE_STATIC_IP` and the static IP fields in `main/User_Settings.c`
- `main/platform/wifi_helper.c` for station startup behavior

### Display does not behave as expected

Check:

- `main/ui/display.cpp`
- `components/TFT_eSPI/User_Setup.h`

### BACnet objects do not match expectations

Check:

- counts in `main/User_Settings.h`
- instance arrays and defaults in `main/User_Settings.c`
- runtime object modules in `main/bacnet/objects/`
