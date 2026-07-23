# Display and Hardware Profiles Guide

## Project

`BACnet-ESP32-S3-SEN54-ST7796S-DS18B2`

This guide documents the compile-time display-profile system introduced on the
`multi` branch. It explains how the selection works and how to add displays with:

- a different UI but the same physical display;
- different GPIO assignments;
- a different TFT controller supported by TFT_eSPI;
- a completely different display driver;
- no display;
- future RS485 hardware profiles.

The central rule is:

> One selected profile must compile exactly one UI implementation and select
> exactly one matching hardware configuration.

---

## 1. Purpose of the profile system

Without profiles, the display controller, GPIO assignments, driver settings, and
UI layout are all tied to one implementation. Changing hardware then requires
manually editing source and library configuration files.

The profile system separates these concerns:

1. `Kconfig.projbuild` presents the available profiles in menuconfig.
2. `sdkconfig` stores the selected profile as a `CONFIG_...` symbol.
3. `main/CMakeLists.txt` compiles the selected UI implementation.
4. `components/TFT_eSPI/User_Setup.h` selects the matching TFT_eSPI hardware setup.
5. The selected hardware setup defines the controller, GPIOs, dimensions, SPI
   port, SPI speed, colour order, inversion, fonts, backlight, and touch settings.
6. All UI implementations expose the same public functions declared in
   `main/ui/display.h`.

The rest of the project continues calling the same display API and does not need
to know which display is selected.

---

## 2. Current project structure

The intended structure is:

```text
main/
├── CMakeLists.txt
├── Kconfig.projbuild
└── ui/
    ├── display.h
    ├── fonts/
    └── profiles/
        ├── display_st7796s_current.cpp
        ├── display_st7796s_test.cpp
        └── display_none.c

components/
└── TFT_eSPI/
    ├── User_Setup.h
    └── User_Setups/
        └── Setup_Project_ST7796S.h
```

### Responsibilities

#### `main/ui/display.h`

This is the stable application-facing interface.

The rest of the project should include only this header and call functions such
as:

```c
void display_init(void);

void display_update_values(
    float pm25,
    float temperature,
    float humidity,
    float voc,
    float temp_ds18b20);

void display_set_link_status(
    bool wifi_connected,
    bool mstp_connected);
```

Do not place controller names, GPIOs, TFT_eSPI configuration, or UI coordinates
in this header.

#### `main/ui/profiles/display_*.cpp`

Each file contains one complete UI implementation.

Examples:

```text
display_st7796s_current.cpp
display_st7796s_test.cpp
display_st7789_compact.cpp
display_ili9488_large.cpp
```

Only one implementation must be compiled in a build.

#### `components/TFT_eSPI/User_Setup.h`

This is now a selector rather than the place where all pins are directly defined.

It examines the selected `CONFIG_USER_DISPLAY_...` symbol and includes the
matching hardware setup.

#### `components/TFT_eSPI/User_Setups/Setup_Project_*.h`

Each file describes one physical display connection:

- TFT controller;
- native width and height;
- SPI or parallel interface;
- GPIO assignments;
- backlight GPIO and polarity;
- ESP32 SPI host selection;
- SPI frequency;
- RGB/BGR order;
- inversion;
- enabled fonts;
- touch controller settings.

---

## 3. How selection flows through the build

Selecting this menuconfig option:

```text
Project hardware
└── Display profile
    └── ST7796S 480x320 - test UI
```

writes this to `sdkconfig`:

```text
CONFIG_USER_DISPLAY_ST7796S_TEST=y
```

That same symbol is used in two places.

### UI selection

`main/CMakeLists.txt` selects:

```text
main/ui/profiles/display_st7796s_test.cpp
```

### Controller and GPIO selection

`components/TFT_eSPI/User_Setup.h` selects:

```text
components/TFT_eSPI/User_Setups/Setup_Project_ST7796S.h
```

Therefore, one menu selection controls both the UI source and the hardware
configuration.

```text
CONFIG_USER_DISPLAY_ST7796S_TEST
                  |
                  +--> display_st7796s_test.cpp
                  |
                  +--> Setup_Project_ST7796S.h
```

For the current duplicate test, both UIs intentionally select the same
ST7796S setup:

```text
CURRENT UI ─┐
            ├── Setup_Project_ST7796S.h
TEST UI ────┘
```

---

## 4. Current Kconfig display choice

`main/Kconfig.projbuild` should contain:

```kconfig
menu "Project hardware"

choice USER_DISPLAY_PROFILE
    prompt "Display profile"
    default USER_DISPLAY_ST7796S_CURRENT

    config USER_DISPLAY_ST7796S_CURRENT
        bool "ST7796S 480x320 - current UI"

    config USER_DISPLAY_ST7796S_TEST
        bool "ST7796S 480x320 - test UI"

    config USER_DISPLAY_NONE
        bool "No display"
endchoice

endmenu
```

A Kconfig `choice` is important because it ensures that only one display profile
can be selected.

ESP-IDF automatically exposes each selected item to C and CMake with the
`CONFIG_` prefix:

```text
USER_DISPLAY_ST7796S_TEST
```

becomes:

```text
CONFIG_USER_DISPLAY_ST7796S_TEST
```

---

## 5. Current CMake UI selection

The main source list contains all common application files but no fixed display
source.

The display source is appended according to the selected profile:

```cmake
if(CONFIG_USER_DISPLAY_ST7796S_CURRENT)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7796s_current.cpp"
    )

elseif(CONFIG_USER_DISPLAY_ST7796S_TEST)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7796s_test.cpp"
    )

elseif(CONFIG_USER_DISPLAY_NONE)
    list(APPEND APP_SRCS
        "ui/profiles/display_none.c"
    )

elseif(NOT CMAKE_BUILD_EARLY_EXPANSION)
    message(FATAL_ERROR "No display profile selected")
endif()
```

### Why `CMAKE_BUILD_EARLY_EXPANSION` is required

ESP-IDF processes component CMake files once during an early dependency scan.
During that first pass, Kconfig values may not yet be available.

This would fail incorrectly:

```cmake
else()
    message(FATAL_ERROR "No display profile selected")
endif()
```

Use:

```cmake
elseif(NOT CMAKE_BUILD_EARLY_EXPANSION)
    message(FATAL_ERROR "No display profile selected")
endif()
```

This suppresses the error only during the early scan while retaining the safety
check during normal configuration.

---

## 6. Current TFT_eSPI hardware selector

`components/TFT_eSPI/User_Setup.h` should remain small:

```cpp
#pragma once

#include "sdkconfig.h"

/*
 * Select the TFT_eSPI hardware configuration using the same
 * Kconfig option that selects the UI implementation.
 */

#if defined(CONFIG_USER_DISPLAY_ST7796S_CURRENT) || \
    defined(CONFIG_USER_DISPLAY_ST7796S_TEST)

#include "User_Setups/Setup_Project_ST7796S.h"

#else

#error "No TFT_eSPI hardware setup selected"

#endif
```

Do not put UI drawing code here.

Do not edit `User_Setup_Select.h` for each project build. The project-owned
`User_Setup.h` is the selector.

---

## 7. Current ST7796S hardware setup

`components/TFT_eSPI/User_Setups/Setup_Project_ST7796S.h` represents the current
physical display and wiring.

A suitable structure is:

```cpp
#pragma once

#define USER_SETUP_INFO "Project ST7796S 320x480 SPI"

/* Physical TFT controller */
#define ST7796_DRIVER

/* Native portrait dimensions */
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

/* SPI bus */
#define TFT_MOSI 10
#define TFT_SCLK 9
#define TFT_MISO -1

/* Display control */
#define TFT_CS   13
#define TFT_DC   12
#define TFT_RST  11

/* Backlight */
#define TFT_BL 14
#define TFT_BACKLIGHT_ON HIGH

/* ESP32-S3 SPI host used by the working configuration */
#define USE_HSPI_PORT

/* SPI speed */
#define SPI_FREQUENCY 26000000

/* Panel-specific options */
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_OFF

/* Fonts */
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

/* No SPI touch controller */
#define TOUCH_CS -1
```

The setup file describes the native panel. The UI implementation controls the
runtime orientation:

```cpp
tft.setRotation(1);
```

For example:

```text
Native panel:       320 x 480
Landscape runtime:  480 x 320
```

Do not swap `TFT_WIDTH` and `TFT_HEIGHT` merely because the UI is landscape.

---

## 8. Required rules inside every TFT_eSPI UI profile

Each UI implementation must include the public header and required driver
headers:

```cpp
#include "display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "esp_log.h"
```

Define a file-local logging tag after the includes:

```cpp
static const char *TAG = "display";
```

This is required before using:

```cpp
ESP_LOGI(TAG, "...");
```

Without the declaration, compilation fails with:

```text
error: 'TAG' was not declared in this scope
```

Using `static` is safe because only one UI profile is compiled and the symbol is
local to that source file.

### Backlight handling

Do not hardcode this inside the UI:

```cpp
#define DISP_BL 14
```

Do not use:

```cpp
pinMode(DISP_BL, OUTPUT);
digitalWrite(DISP_BL, HIGH);
```

Instead, obtain the GPIO and polarity from the selected hardware setup:

```cpp
#if defined(TFT_BL) && (TFT_BL >= 0)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    delay(10);
#endif
```

This allows another profile to use:

- another backlight GPIO;
- active-low backlight control;
- no controlled backlight.

### Diagnostic startup logging

After `tft.init()` and `tft.setRotation(...)`, log the selected UI and hardware:

```cpp
ESP_LOGI(TAG, "UI profile: ST7796S CURRENT");
ESP_LOGI(TAG, "TFT hardware: %s", USER_SETUP_INFO);

ESP_LOGI(
    TAG,
    "TFT GPIO: MOSI=%d SCLK=%d MISO=%d CS=%d DC=%d RST=%d BL=%d",
    TFT_MOSI,
    TFT_SCLK,
    TFT_MISO,
    TFT_CS,
    TFT_DC,
    TFT_RST,
    TFT_BL);

ESP_LOGI(
    TAG,
    "TFT native size: %dx%d; runtime size: %dx%d",
    TFT_WIDTH,
    TFT_HEIGHT,
    tft.width(),
    tft.height());
```

Use a different first line in each UI:

```cpp
ESP_LOGI(TAG, "UI profile: ST7796S TEST");
```

Expected output:

```text
UI profile: ST7796S TEST
TFT hardware: Project ST7796S 320x480 SPI
TFT GPIO: MOSI=10 SCLK=9 MISO=-1 CS=13 DC=12 RST=11 BL=14
TFT native size: 320x480; runtime size: 480x320
```

---

## 9. Selecting a profile in VS Code

1. Open the project in VS Code.
2. Press `Ctrl+Shift+P`.
3. Run:

   ```text
   ESP-IDF: SDK Configuration Editor (menuconfig)
   ```

4. Open:

   ```text
   Project hardware
   ```

5. Open:

   ```text
   Display profile
   ```

6. Select the desired profile.
7. Save the configuration.
8. Build the project.
9. Flash and monitor the ESP32-S3.

The selected profile can also be checked directly in `sdkconfig`.

Current UI:

```text
CONFIG_USER_DISPLAY_ST7796S_CURRENT=y
# CONFIG_USER_DISPLAY_ST7796S_TEST is not set
```

Test UI:

```text
# CONFIG_USER_DISPLAY_ST7796S_CURRENT is not set
CONFIG_USER_DISPLAY_ST7796S_TEST=y
```

---

## 10. When a full clean is required

A normal build is normally sufficient when changing only:

- UI coordinates;
- text;
- colours;
- fonts already enabled;
- draw functions;
- display values;
- startup log messages.

Run **Full Clean Project** after changing:

- `Kconfig.projbuild`;
- the selected display profile for the first time;
- `User_Setup.h`;
- a `Setup_Project_*.h` hardware file;
- controller type;
- width or height;
- GPIO assignments;
- SPI host;
- SPI frequency;
- colour order;
- inversion;
- enabled TFT_eSPI fonts;
- TFT_eSPI component configuration;
- source-file layout in CMake.

VS Code commands:

```text
ESP-IDF: Full Clean Project
ESP-IDF: Build Project
```

From an initialized ESP-IDF terminal:

```powershell
idf.py fullclean
idf.py reconfigure
idf.py build
```

---

# PART II — CREATING NEW PROFILES

## 11. Case A: same display and GPIOs, different UI

Use this when the physical hardware is unchanged but the screen layout differs.

Example:

```text
ST7796S current UI
ST7796S maintenance UI
ST7796S compact UI
```

### Step 1: duplicate the UI implementation

Copy:

```text
main/ui/profiles/display_st7796s_current.cpp
```

to:

```text
main/ui/profiles/display_st7796s_maintenance.cpp
```

Keep the same public functions.

Change the startup identification:

```cpp
ESP_LOGI(TAG, "UI profile: ST7796S MAINTENANCE");
```

### Step 2: add the Kconfig entry

Inside the existing `choice`:

```kconfig
config USER_DISPLAY_ST7796S_MAINTENANCE
    bool "ST7796S 480x320 - maintenance UI"
```

### Step 3: add the CMake source selection

```cmake
elseif(CONFIG_USER_DISPLAY_ST7796S_MAINTENANCE)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7796s_maintenance.cpp"
    )
```

### Step 4: map it to the existing hardware setup

Update `components/TFT_eSPI/User_Setup.h`:

```cpp
#if defined(CONFIG_USER_DISPLAY_ST7796S_CURRENT) || \
    defined(CONFIG_USER_DISPLAY_ST7796S_TEST) || \
    defined(CONFIG_USER_DISPLAY_ST7796S_MAINTENANCE)

#include "User_Setups/Setup_Project_ST7796S.h"
```

### Step 5: full clean and build

No new hardware setup file is required because the controller and GPIOs are
unchanged.

---

## 12. Case B: same controller, different GPIO wiring

Use this when the display still uses ST7796S but is connected to another board or
to different pins.

Do not reuse `Setup_Project_ST7796S.h`, because one setup file should describe
one known physical connection.

Example profile name:

```text
USER_DISPLAY_ST7796S_BOARD_B
```

### Step 1: create a new UI file

If the UI is also different:

```text
main/ui/profiles/display_st7796s_board_b.cpp
```

If the UI should be identical, duplicate the current implementation initially.
Later, common rendering code can be factored out if necessary.

### Step 2: create a new setup file

```text
components/TFT_eSPI/User_Setups/Setup_Project_ST7796S_Board_B.h
```

Example:

```cpp
#pragma once

#define USER_SETUP_INFO "Board B - ST7796S 320x480 SPI"

#define ST7796_DRIVER

#define TFT_WIDTH  320
#define TFT_HEIGHT 480

#define TFT_MOSI  <board_b_mosi>
#define TFT_SCLK  <board_b_sclk>
#define TFT_MISO  -1

#define TFT_CS    <board_b_cs>
#define TFT_DC    <board_b_dc>
#define TFT_RST   <board_b_rst>

#define TFT_BL <board_b_backlight>
#define TFT_BACKLIGHT_ON HIGH

#define USE_HSPI_PORT
#define SPI_FREQUENCY 10000000

#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_OFF

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF
#define SMOOTH_FONT

#define TOUCH_CS -1
```

Start an untested display at a conservative SPI speed such as 10 MHz. Increase it
only after the display is stable.

### Step 3: add the Kconfig entry

```kconfig
config USER_DISPLAY_ST7796S_BOARD_B
    bool "Board B - ST7796S 480x320"
```

### Step 4: add the CMake UI selection

```cmake
elseif(CONFIG_USER_DISPLAY_ST7796S_BOARD_B)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7796s_board_b.cpp"
    )
```

### Step 5: add the hardware selector branch

```cpp
#elif defined(CONFIG_USER_DISPLAY_ST7796S_BOARD_B)

#include "User_Setups/Setup_Project_ST7796S_Board_B.h"
```

### Step 6: check GPIO conflicts

Before building, compare the proposed GPIOs against all existing functions.

Current project allocations include approximately:

```text
GPIO4   SEN54 I2C SDA
GPIO5   SEN54 I2C SCL
GPIO9   TFT SCLK
GPIO10  TFT MOSI
GPIO11  TFT RST
GPIO12  TFT DC
GPIO13  TFT CS
GPIO14  TFT backlight
GPIO18  DS18B20
```

Also check:

- flash and PSRAM use;
- USB Serial/JTAG pins;
- boot strapping pins;
- UART console pins;
- future RS485 TX, RX, and DE pins;
- buttons and board LEDs;
- internal peripherals already wired on the development board.

### Step 7: full clean, build, and test

Verify the startup log before judging the screen output.

---

## 13. Case C: different TFT controller supported by TFT_eSPI

Example:

```text
ST7789 240x320
ILI9341 240x320
ILI9488 320x480
GC9A01 240x240
```

The controller cannot be identified reliably from screen size alone. Confirm the
controller from the module datasheet, manufacturer documentation, PCB markings,
flex-cable markings, or a known working example.

### Step 1: add a Kconfig entry

Example:

```kconfig
config USER_DISPLAY_ST7789_240X320
    bool "ST7789 240x320 - compact UI"
```

### Step 2: create a UI implementation

```text
main/ui/profiles/display_st7789_240x320.cpp
```

The UI will normally need different coordinates because the runtime resolution
differs.

It must still implement the same public API declared in `display.h`.

### Step 3: create the hardware setup

```text
components/TFT_eSPI/User_Setups/Setup_Project_ST7789_240x320.h
```

Example:

```cpp
#pragma once

#define USER_SETUP_INFO "Project ST7789 240x320 SPI"

#define ST7789_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MOSI <actual_gpio>
#define TFT_SCLK <actual_gpio>
#define TFT_MISO -1

#define TFT_CS   <actual_gpio>
#define TFT_DC   <actual_gpio>
#define TFT_RST  <actual_gpio>

#define TFT_BL <actual_gpio>
#define TFT_BACKLIGHT_ON HIGH

#define USE_HSPI_PORT

/* Begin conservatively */
#define SPI_FREQUENCY 10000000

/*
 * Determine these from the actual module.
 */
// #define TFT_RGB_ORDER TFT_BGR
// #define TFT_INVERSION_ON
// #define TFT_INVERSION_OFF

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF
#define SMOOTH_FONT

#define TOUCH_CS -1
```

Only one controller macro may be active in a setup file:

```cpp
#define ST7789_DRIVER
```

Do not also leave:

```cpp
#define ST7796_DRIVER
```

### Step 4: add the CMake branch

```cmake
elseif(CONFIG_USER_DISPLAY_ST7789_240X320)
    list(APPEND APP_SRCS
        "ui/profiles/display_st7789_240x320.cpp"
    )
```

### Step 5: add the TFT_eSPI selector branch

```cpp
#elif defined(CONFIG_USER_DISPLAY_ST7789_240X320)

#include "User_Setups/Setup_Project_ST7789_240x320.h"
```

### Step 6: full clean

A full clean is mandatory after changing the controller setup.

### Step 7: initial hardware test order

Test in this order:

1. Power and ground.
2. Backlight.
3. Correct selected profile in serial log.
4. Correct controller macro.
5. Correct MOSI, SCLK, CS, DC, and RST pins.
6. Low SPI frequency.
7. Basic full-screen fill.
8. Rotation.
9. Colour order.
10. Inversion.
11. Full application UI.
12. Increase SPI speed gradually.

---

## 14. Case D: a display that uses another driver library

Examples:

- ESP-IDF `esp_lcd`;
- Adafruit GFX;
- LovyanGFX;
- LVGL with another panel driver;
- a manufacturer-specific library.

The public project interface should remain:

```text
main/ui/display.h
```

The new implementation may be:

```text
main/ui/profiles/display_ili9488_esp_lcd.cpp
```

It still implements:

```c
display_init()
display_update_values(...)
display_set_link_status(...)
```

However, it should not include `TFT_eSPI.h`, and its GPIO/controller configuration
belongs in a driver-appropriate project file, not in TFT_eSPI's `User_Setup.h`.

### Dependency handling

At the present stage, `main/CMakeLists.txt` requires TFT_eSPI and Arduino
unconditionally. This is simple and safe while all real display profiles use
TFT_eSPI.

When introducing a completely different driver, review the component dependency
structure carefully. ESP-IDF performs an early dependency scan before all Kconfig
values are available, so dependency selection based only on late `CONFIG_...`
tests can be misleading.

A practical progression is:

1. Keep common dependencies unconditional while proving the new implementation.
2. Confirm that both driver implementations build independently.
3. Then isolate each driver into its own ESP-IDF component if conditional
   dependencies become necessary.

A robust long-term structure would be:

```text
components/
├── display_api/
├── display_st7796s_tft_espi/
└── display_new_esp_lcd/
```

Each display driver component can have its own dependencies. The selected profile
then links one component through a thin application wrapper.

Do not attempt this larger refactor before the basic profiles are working.

---

## 15. No-display profile

`display_none.c` should implement the same API and do nothing:

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

### Important current limitation

The current main component still requires TFT_eSPI unconditionally, and the
current `TFT_eSPI/User_Setup.h` produces an error when neither ST7796S profile is
selected.

Therefore, do not select `USER_DISPLAY_NONE` until one of these is implemented:

1. temporarily map `USER_DISPLAY_NONE` to a harmless valid TFT_eSPI setup so the
   unused TFT_eSPI component can still compile; or
2. restructure display drivers into separate components with driver-specific
   dependencies.

The simplest temporary workaround is:

```cpp
#elif defined(CONFIG_USER_DISPLAY_NONE)

/*
 * TFT_eSPI is still an unconditional dependency in the current build.
 * Include a valid setup only so the unused library can compile.
 */
#include "User_Setups/Setup_Project_ST7796S.h"
```

The application will compile `display_none.c`, so no TFT object will be created
or initialized.

---

# PART III — NAMING AND DESIGN RULES

## 16. Recommended naming convention

Kconfig symbol:

```text
USER_DISPLAY_<CONTROLLER>_<BOARD_OR_SIZE>_<UI>
```

Examples:

```text
USER_DISPLAY_ST7796S_CURRENT
USER_DISPLAY_ST7796S_TEST
USER_DISPLAY_ST7796S_BOARD_B
USER_DISPLAY_ST7789_240X320
USER_DISPLAY_GC9A01_ROUND
```

UI source:

```text
display_<controller>_<board_or_size>_<ui>.cpp
```

Examples:

```text
display_st7796s_current.cpp
display_st7796s_board_b.cpp
display_st7789_240x320.cpp
```

Hardware setup:

```text
Setup_Project_<Controller>_<BoardOrSize>.h
```

Examples:

```text
Setup_Project_ST7796S.h
Setup_Project_ST7796S_Board_B.h
Setup_Project_ST7789_240x320.h
```

The name should make it obvious whether two profiles share the same hardware
setup.

---

## 17. What belongs in the UI profile

Place these in `display_*.cpp`:

- rotation;
- screen layout;
- rectangles and panels;
- text labels;
- fonts used by specific screen elements;
- colours used by the UI;
- value formatting;
- screen refresh logic;
- link-status drawing;
- profile identification log;
- use of the public display API.

Do not place these in the UI profile:

- fixed MOSI/SCLK/CS/DC/RST pins;
- physical display controller selection;
- native panel dimensions;
- SPI host selection;
- SPI frequency;
- backlight GPIO number;
- backlight active polarity;
- RGB/BGR hardware setting;
- hardware inversion setting.

---

## 18. What belongs in the hardware setup

Place these in `Setup_Project_*.h`:

- controller macro;
- native width and height;
- interface type;
- MOSI, MISO, and SCLK;
- CS, DC, and RST;
- backlight GPIO;
- backlight active level;
- SPI host;
- SPI frequency;
- read frequency when used;
- touch frequency when used;
- RGB/BGR order;
- inversion;
- enabled TFT_eSPI fonts;
- touch CS or touch-related configuration.

---

## 19. Avoid independent selections that create invalid combinations

At this stage, do not offer separate independent menu choices for:

```text
UI
TFT controller
GPIO map
SPI host
```

That would allow invalid combinations.

Prefer one complete profile:

```text
Board B - ST7789 240x320 compact UI
```

which selects all required pieces together.

Independent choices can be introduced later only when compatibility rules are
clear and enforced by Kconfig dependencies.

---

# PART IV — TROUBLESHOOTING

## 20. Error: `No display profile selected`

Typical cause:

- Kconfig values were not available during ESP-IDF's early CMake scan; or
- `sdkconfig` has not been regenerated after adding `Kconfig.projbuild`.

Correct CMake guard:

```cmake
elseif(NOT CMAKE_BUILD_EARLY_EXPANSION)
    message(FATAL_ERROR "No display profile selected")
endif()
```

Then run a full clean and reopen menuconfig.

---

## 21. Error: `'TAG' was not declared in this scope`

Cause:

```cpp
ESP_LOGI(TAG, ...)
```

was added without declaring `TAG`.

Fix near the top of every profile using ESP logging:

```cpp
#include "esp_log.h"

static const char *TAG = "display";
```

---

## 22. Error: `No TFT_eSPI hardware setup selected`

Causes:

- a Kconfig profile exists but is missing from `User_Setup.h`;
- a profile name is misspelled;
- `USER_DISPLAY_NONE` is selected while TFT_eSPI is still built;
- stale `sdkconfig` or build files.

Check that the same symbol appears in all three places:

```text
Kconfig.projbuild
main/CMakeLists.txt
components/TFT_eSPI/User_Setup.h
```

Then run a full clean.

---

## 23. Multiple-definition linker errors for `display_init`

Cause:

Two UI implementation files are being compiled.

Check `main/CMakeLists.txt`.

Only one profile source should be appended to `APP_SRCS`.

Do not list all display profile files in the initial fixed `set(APP_SRCS ...)`
block.

---

## 24. Undefined reference to `display_init` or other display functions

Causes:

- no profile source was compiled;
- the implementation function signature differs from `display.h`;
- C++ implementation lacks `extern "C"` where required by the existing interface;
- selected profile source path is wrong.

Match the existing working function declarations exactly.

---

## 25. Display builds but remains white or black

Check in this order:

1. Correct profile selected.
2. Correct serial log profile name.
3. Display power voltage.
4. Ground.
5. Backlight GPIO and polarity.
6. Controller macro.
7. MOSI.
8. SCLK.
9. CS.
10. DC.
11. RST.
12. SPI host.
13. Low SPI frequency.
14. Rotation only after initialization.
15. Known basic fill-screen test.

A lit white screen often means the backlight has power but the controller did not
receive valid initialization or SPI commands.

---

## 26. Image appears but colours are wrong

Try only the setting supported by the specific panel:

```cpp
#define TFT_RGB_ORDER TFT_BGR
```

or omit it/use RGB as appropriate.

Do not treat colour order as a UI colour problem.

---

## 27. Image is inverted, washed out, or has incorrect black/white levels

Check:

```cpp
#define TFT_INVERSION_ON
```

versus:

```cpp
#define TFT_INVERSION_OFF
```

This is often panel-specific even when two modules use the same controller.

---

## 28. Image is rotated or mirrored

Use the UI implementation's:

```cpp
tft.setRotation(0);
tft.setRotation(1);
tft.setRotation(2);
tft.setRotation(3);
```

Do not change native `TFT_WIDTH` and `TFT_HEIGHT` merely to fix orientation.

If the module requires an unusual controller offset or mirror configuration,
consult its known working driver setup.

---

## 29. Display works at low speed but fails at higher speed

Possible causes:

- wiring length;
- poor ground;
- breadboard quality;
- level shifting;
- weak power supply;
- unsuitable SPI frequency;
- signal integrity;
- wrong SPI host configuration.

Keep the lower stable speed. Rendering speed is secondary to reliable operation.

---

## 30. Build still uses the previous display setup

Run:

```text
ESP-IDF: Full Clean Project
```

Then build again.

TFT_eSPI controller and GPIO settings are compile-time library configuration, so
incremental builds may retain stale objects after setup-header changes.

---

# PART V — TEST CHECKLIST

## 31. Checklist for every new profile

### Configuration

- [ ] Unique Kconfig symbol added.
- [ ] Human-readable menu label added.
- [ ] Exactly one choice can be selected.
- [ ] CMake branch added.
- [ ] Correct UI source path used.
- [ ] TFT_eSPI selector branch added when applicable.
- [ ] Correct hardware setup included.

### UI source

- [ ] Implements every function in `display.h`.
- [ ] Includes `esp_log.h` when using ESP logging.
- [ ] Declares `static const char *TAG`.
- [ ] Uses `TFT_BL`, not a hardcoded backlight GPIO.
- [ ] Uses `TFT_BACKLIGHT_ON`.
- [ ] Logs the UI profile name.
- [ ] Logs `USER_SETUP_INFO`.
- [ ] Uses the intended rotation.
- [ ] Has coordinates suitable for the runtime resolution.

### Hardware setup

- [ ] Exactly one controller macro enabled.
- [ ] Native dimensions correct.
- [ ] MOSI correct.
- [ ] SCLK correct.
- [ ] MISO correct or `-1`.
- [ ] CS correct.
- [ ] DC correct.
- [ ] RST correct or intentionally disabled.
- [ ] Backlight GPIO correct.
- [ ] Backlight polarity correct.
- [ ] SPI host correct.
- [ ] Conservative initial SPI frequency.
- [ ] RGB/BGR order verified.
- [ ] Inversion verified.
- [ ] Required fonts enabled.
- [ ] Touch configuration correct.
- [ ] No conflict with sensors, USB, flash/PSRAM, UART, RS485, or board peripherals.

### Build and test

- [ ] Full clean completed.
- [ ] Correct profile shown in `sdkconfig`.
- [ ] Build compiles only one UI implementation.
- [ ] Serial log shows expected UI profile.
- [ ] Serial log shows expected GPIOs.
- [ ] Backlight works.
- [ ] Basic drawing works.
- [ ] Orientation correct.
- [ ] Colours correct.
- [ ] Full application UI updates.
- [ ] Wi-Fi and MS/TP status drawing works.
- [ ] Sensor values update.
- [ ] BACnet behavior remains unchanged.

---

# PART VI — FUTURE RS485 PROFILES

## 32. Initial practical approach

When different RS485 boards are added, begin with a separate Kconfig choice:

```text
Project hardware
├── Display profile
└── RS485 profile
```

Suggested structure:

```text
main/platform/rs485/
├── rs485.h
└── profiles/
    ├── rs485_waveshare.cpp
    ├── rs485_max485.cpp
    └── rs485_none.c
```

The same architecture applies:

```text
Kconfig selection
       |
       +--> CMake source selection
       |
       +--> matching GPIO/transceiver configuration
```

Keep display and RS485 selections independent while testing.

---

## 33. Later complete board profiles

After several combinations are stable, a complete product/board profile may be
more practical:

```text
ESP32-S3 + ST7796S + Waveshare RS485
ESP32-S3 + ST7789 + MAX485
ESP32-S3 + no display + onboard RS485
```

A complete board profile should select:

- display implementation;
- display controller;
- display GPIOs;
- backlight;
- RS485 implementation;
- UART;
- RS485 TX/RX/DE GPIOs;
- sensors available on that board;
- optional buttons and LEDs.

Do not move BACnet object definitions into the hardware profile. BACnet objects
and application defaults should remain separate from physical pin assignments.

---

# PART VII — RECOMMENDED WORKFLOW

## 34. Safe development sequence

For each new physical display:

1. Create a feature branch.
2. Record the exact display controller and board/module identification.
3. Record the complete GPIO inventory.
4. Add the Kconfig entry.
5. Duplicate a working UI profile.
6. Add the CMake branch.
7. Create the new hardware setup.
8. Add the selector branch.
9. Start at a low SPI speed.
10. Add clear startup diagnostics.
11. Full clean.
12. Build.
13. Verify the selected profile in serial output.
14. Test backlight.
15. Test basic controller drawing.
16. Verify rotation, colour order, and inversion.
17. Adapt the UI layout.
18. Test sensor and BACnet operation.
19. Commit the working profile.
20. Update this document and the project README.

Do not combine controller bring-up, GPIO rewiring, a complete UI redesign, and
unrelated BACnet changes in one untested commit.

---

## 35. Summary

A display profile is a compile-time package containing:

```text
Menuconfig choice
+ one UI implementation
+ one controller/driver choice
+ one GPIO mapping
+ one native resolution
+ one backlight configuration
+ one SPI configuration
```

The project application uses only `display.h`.

`Kconfig.projbuild` selects the profile.

`main/CMakeLists.txt` selects the UI source.

`TFT_eSPI/User_Setup.h` selects the hardware setup.

`Setup_Project_*.h` defines the physical display.

This structure keeps one repository and one application while allowing multiple
displays, layouts, boards, and eventually RS485 interfaces without manually
rewriting the project before each build.
