# Display Hardware Profiles Guide

## 1. Overview

A display profile is a compile-time combination of:

* one UI implementation;
* one display controller and GPIO configuration;
* one resolution and SPI configuration;
* optional backlight and touch settings.

The application uses only the common interface in:

```text
main/ui/display.h
```

The selected profile determines the UI and physical display setup.

## 2. Relevant files

| File                                                | Purpose                                                    |
| --------------------------------------------------- | ---------------------------------------------------------- |
| `main/Kconfig.projbuild`                            | Menuconfig profile selection and profile-specific defaults |
| `main/CMakeLists.txt`                               | Compiles exactly one UI implementation                     |
| `main/ui/display.h`                                 | Common application-facing display API                      |
| `main/ui/profiles/display_*.cpp`                    | UI layout and drawing implementation                       |
| `components/TFT_eSPI/User_Setup.h`                  | Selects the matching TFT_eSPI setup                        |
| `components/TFT_eSPI/User_Setups/Setup_Project_*.h` | Controller, dimensions, pins and SPI settings              |

Selection flow:

```text
CONFIG_USER_DISPLAY_<PROFILE>
        |
        +--> main/ui/profiles/display_<profile>.cpp
        |
        +--> Setup_Project_<Profile>.h
```

Exactly one display profile must be selected.

## 3. Adding a new TFT_eSPI profile

### Step 1: Record the hardware

Confirm:

```text
Controller
Native width and height
MOSI
MISO, if used
SCLK
CS
DC
RST
Backlight pin and polarity
SPI frequency
RGB or BGR order
Display inversion
Touch controller, if present
```

Check the proposed GPIOs against:

* boot-strapping pins;
* flash and PSRAM;
* UART and USB;
* SEN54 and other sensors;
* RS485;
* buttons, LEDs and onboard peripherals.

Avoid externally driven boot-strapping pins when practical.

### Step 2: Add the Kconfig profile

Edit:

```text
main/Kconfig.projbuild
```

Add the profile inside `choice USER_DISPLAY_PROFILE`:

```kconfig
config USER_DISPLAY_ST7789_EXAMPLE
    bool "ST7789 240x320 - Example board"
```

Add profile-specific sensor or touch defaults in the same file when required.

For example:

```kconfig
config USER_SEN54_SDA_GPIO
    int
    default 13 if USER_DISPLAY_ST7789_EXAMPLE
    default 4
```

### Step 3: Create the TFT_eSPI setup

Create:

```text
components/TFT_eSPI/User_Setups/Setup_Project_ST7789_Example.h
```

Example:

```cpp
#pragma once

#define USER_SETUP_INFO "ST7789 Example 240x320"

#define ST7789_DRIVER

/* Native panel dimensions */
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MOSI 33
#define TFT_SCLK 32
#define TFT_MISO -1

#define TFT_CS   27
#define TFT_DC   26
#define TFT_RST  25

#define SPI_FREQUENCY 20000000

#define TFT_RGB_ORDER TFT_BGR
/* Define only the setting required by the panel. */
// #define TFT_INVERSION_ON
// #define TFT_INVERSION_OFF

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF
#define SMOOTH_FONT
```

Only one controller macro may be enabled.

Use native panel dimensions here. Rotation belongs in the UI implementation.

For an uncontrolled or permanently powered backlight, omit `TFT_BL`. When a backlight GPIO exists:

```cpp
#define TFT_BL 14
#define TFT_BACKLIGHT_ON HIGH
```

### Step 4: Create the UI implementation

Create or copy:

```text
main/ui/profiles/display_st7789_example.cpp
```

It must implement every function declared in:

```text
main/ui/display.h
```

Keep UI-specific items here:

* rotation;
* coordinates;
* colours;
* labels;
* fonts;
* value formatting;
* screens and controls;
* display refresh logic.

Do not hardcode display GPIOs or controller settings in the UI file.

For a TFT_eSPI implementation:

```cpp
#include "display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "esp_log.h"

static const char *TAG = "display";
static TFT_eSPI tft;

extern "C" void display_init(void)
{
#if defined(TFT_BL) && defined(TFT_BACKLIGHT_ON) && (TFT_BL >= 0)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif

    tft.init();
    tft.setRotation(1);

    ESP_LOGI(TAG, "UI profile: ST7789 EXAMPLE");
    ESP_LOGI(TAG, "TFT hardware: %s", USER_SETUP_INFO);
    ESP_LOGI(
        TAG,
        "TFT native size: %dx%d; runtime size: %dx%d",
        TFT_WIDTH,
        TFT_HEIGHT,
        tft.width(),
        tft.height());
}
```

### Step 5: Add the CMake selection

Edit:

```text
main/CMakeLists.txt
```

Add:

```cmake
elseif(CONFIG_USER_DISPLAY_ST7789_EXAMPLE)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7789_example.cpp"
    )
```

Only one display implementation may be appended to `APP_SRCS`.

Keep the final guard:

```cmake
elseif(NOT CMAKE_BUILD_EARLY_EXPANSION)
    message(FATAL_ERROR "No display profile selected")
endif()
```

### Step 6: Add the TFT_eSPI selector

Edit:

```text
components/TFT_eSPI/User_Setup.h
```

Add:

```cpp
#elif defined(CONFIG_USER_DISPLAY_ST7789_EXAMPLE)

#include "User_Setups/Setup_Project_ST7789_Example.h"
```

The same Kconfig symbol must appear in:

```text
main/Kconfig.projbuild
main/CMakeLists.txt
components/TFT_eSPI/User_Setup.h
```

### Step 7: Select and build

In VS Code:

```text
ESP-IDF: Set Espressif Device Target
ESP-IDF: SDK Configuration Editor
Project hardware
Display profile
```

Select the new profile and save.

After changing a profile, controller, GPIO mapping, setup header or CMake source selection, run:

```powershell
idf.py fullclean
idf.py build
idf.py flash monitor
```

## 4. Same hardware, different UI

When only the layout changes:

1. Create another `display_*.cpp`.
2. Add another Kconfig entry.
3. Add its CMake branch.
4. Map it to the existing `Setup_Project_*.h`.

Do not create another hardware setup when the controller and wiring are identical.

Example:

```cpp
#if defined(CONFIG_USER_DISPLAY_ST7796S) || \
    defined(CONFIG_USER_DISPLAY_ST7796S_TEST)

#include "User_Setups/Setup_Project_ST7796S.h"
```

## 5. Different hardware, same UI

If the controller or GPIO wiring changes, create a separate hardware setup.

The UI may initially be copied from a compatible profile, but keep the hardware definitions separate:

```text
display_st7789_board_a.cpp
Setup_Project_ST7789_Board_A.h

display_st7789_board_b.cpp
Setup_Project_ST7789_Board_B.h
```

This prevents changes for one board from breaking another.

## 6. No-display profile

`display_none.c` must implement the same API as the real display profiles but perform no operations.

Example:

```c
#include "display.h"

void display_init(void)
{
}

void display_update_values(
    float pm25,
    float temperature,
    float humidity,
    float voc,
    float temp_ds18b20)
{
    (void)pm25;
    (void)temperature;
    (void)humidity;
    (void)voc;
    (void)temp_ds18b20;
}

void display_set_link_status(
    bool wifi_connected,
    bool mstp_connected)
{
    (void)wifi_connected;
    (void)mstp_connected;
}
```

## 7. Initial hardware test

Test a new profile in this order:

1. Power and ground.
2. ESP32 booting and UART flashing.
3. Correct profile shown in `sdkconfig`.
4. Correct profile shown in the serial log.
5. Backlight.
6. Controller and GPIO definitions.
7. Basic `fillScreen()` test.
8. Rotation.
9. Colour order.
10. Inversion.
11. Full UI.
12. Sensor and BACnet operation.
13. Higher SPI speed, if required.

Begin with a conservative SPI speed such as:

```cpp
#define SPI_FREQUENCY 10000000
```

Increase it only after the display is stable.

## 8. Troubleshooting

### No display profile selected

Check that the Kconfig symbol exists and run:

```powershell
idf.py fullclean
idf.py reconfigure
```

### No TFT_eSPI hardware setup selected

The profile is missing from `components/TFT_eSPI/User_Setup.h`, or the symbol is misspelled.

### Multiple definitions of `display_init`

More than one UI source is being compiled. Check `main/CMakeLists.txt`.

### Undefined reference to display functions

The selected UI file is missing, has incorrect function signatures, or was not added to CMake.

### White or black display

Check:

```text
Power
Ground
Backlight
Selected profile
Controller macro
MOSI
SCLK
CS
DC
RST
SPI frequency
```

### Incorrect colours

Check:

```cpp
#define TFT_RGB_ORDER TFT_BGR
```

### Washed-out or inverted display

Test the panel-specific setting:

```cpp
#define TFT_INVERSION_ON
```

or:

```cpp
#define TFT_INVERSION_OFF
```

Do not define both.

### Old GPIO or controller configuration still used

Run a full clean. TFT_eSPI configuration is compiled into the library.

### Display works but sensor data is missing

Check:

* profile-specific SEN54 SDA and SCL defaults;
* I²C port selection;
* GPIO conflicts;
* whether the sensor service is started;
* sensor initialization errors in the serial log.

## 9. Final checklist

Before committing a profile, verify:

* [ ] Unique Kconfig symbol.
* [ ] Correct CMake branch.
* [ ] Correct `User_Setup.h` branch.
* [ ] Separate hardware setup where required.
* [ ] Exactly one controller macro.
* [ ] Correct native dimensions.
* [ ] Correct GPIO mapping.
* [ ] No strapping-pin or peripheral conflict.
* [ ] Appropriate SPI frequency.
* [ ] Correct colour order and inversion.
* [ ] UI implements the complete `display.h` API.
* [ ] Full clean build succeeds.
* [ ] UART flashing works with the display connected.
* [ ] Display updates correctly.
* [ ] Sensors still update.
* [ ] BACnet operation remains unchanged.
* [ ] Working changes committed on a feature branch.
