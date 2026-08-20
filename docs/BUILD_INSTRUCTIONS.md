# ESP32 BACnet Device - Build Instructions

This project uses ESP-IDF 6.0.2 and Arduino-ESP32 3.3.11. The complete
environment and configuration procedure is documented in
[`SETUP.md`](../SETUP.md).

## Prerequisites

- ESP-IDF 6.0.2 installed at `C:\esp\v6.0.2\esp-idf`, or an initialized
  ESP-IDF 6.0.2 terminal
- Visual Studio Code with the ESP-IDF extension
- Wi-Fi credentials in `main/User_Private_Settings.h`

Create the private settings file from
`main/User_Private_Settings.example.h`; do not commit the private file.

## Build Steps

From the repository root, use the supplied wrapper when ESP-IDF is installed
at the required path:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\build_idf60.ps1 build
```

From an initialized ESP-IDF 6.0.2 terminal, the equivalent command is:

```powershell
idf.py build
```

Select the ESP-IDF target and display profile before building. After changing
the target, display profile, TFT setup, or GPIO mapping, run:

```powershell
idf.py fullclean
idf.py build
```

## Flash and Monitor

Replace `COMx` with the device serial port:

```powershell
idf.py -p COMx flash monitor
```

Do not flash a device until the selected target, display profile, and wiring
have been checked.

## BACnet Object Model

The default configuration exposes 36 objects:

- 16 Analog Values
- 4 Binary Values
- 8 Analog Inputs
- 4 Binary Inputs
- 4 Binary Outputs

BACnet device identity, object instances, and metadata are configured in
`main/User_Settings.c` and `main/User_Settings.h`. The active device instance
and name are configuration values and should not be assumed to be fixed.

For BACnet verification, use tools such as YABE, BACnet Scanner, or Visual
Test Shell. See [`OBJECTS_CONFIGURATION.md`](../OBJECTS_CONFIGURATION.md) for
the object roles and persistence behavior.
