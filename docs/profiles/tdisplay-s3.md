# LilyGO T-Display-S3 Touch

## Summary

ESP32-S3 profile for the LilyGO T-Display-S3 with an integrated 1.9-inch ST7789 display, CST816 capacitive touch, and LVGL UI.

## Selection

```text
ST7789 170x320 - LilyGO T-Display-S3 Touch
```

```c
CONFIG_USER_DISPLAY_ST7789_TDISPLAY_S3
```

This profile is available only when the ESP-IDF target is `esp32s3`.

## Display

| Setting | Value |
|---|---|
| Target | ESP32-S3 |
| Controller | ST7789 |
| Native resolution | 170×320 |
| Runtime orientation | 320×170 landscape |
| Interface | 8-bit parallel |
| Colour order | RGB |
| Inversion | On |
| Backlight | GPIO38 |
| UI framework | LVGL |

## Display wiring

| Signal | GPIO |
|---|---:|
| CS | 6 |
| DC | 7 |
| RST | 5 |
| WR | 8 |
| RD | 9 |
| D0 | 39 |
| D1 | 40 |
| D2 | 41 |
| D3 | 42 |
| D4 | 45 |
| D5 | 46 |
| D6 | 47 |
| D7 | 48 |
| Backlight | 38 |

## Touch configuration

| Setting | Value |
|---|---:|
| Controller | CST816 |
| I²C port | 0 |
| SDA | GPIO18 |
| SCL | GPIO17 |
| Address | `0x15` |
| RST | GPIO21 |
| INT | GPIO16 |
| Read method | Polling |

Touch coordinates are mapped from the native 170×320 portrait orientation to the 320×170 landscape UI.

## Sensor defaults

| Setting | Value |
|---|---:|
| SEN54 I²C port | 1 |
| SEN54 SDA | GPIO18 |
| SEN54 SCL | GPIO17 |
| DS18B20 | Disabled |

> **Current configuration warning:** the Kconfig defaults assign both the CST816 and SEN54 to physical GPIO18/17. Different I²C controller numbers do not make the same physical pins independent. Reassign the SEN54 to separate GPIOs before operating touch and SEN54 simultaneously.

## UI status

The profile currently provides:

- an LVGL measurement screen;
- touch navigation;
- additional placeholder/configuration screens;
- a Wi-Fi settings screen under development.

## Source files

```text
main/ui/profiles/display_st7789_tdisplay_s3.cpp
main/ui/hardware/touch_cst816.c
main/ui/hardware/touch_cst816.h
components/TFT_eSPI/User_Setups/Setup_Project_ST7789_TDISPLAY_S3.h
components/TFT_eSPI/User_Setups/Setup206_LilyGo_T_Display_S3.h
```
