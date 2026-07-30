# ST7796S Colorful UI

## Summary

ESP32-S3 profile for a 3.5-inch ST7796S display with the main colorful environmental-monitoring UI.

## Selection

```text
ST7796S 480x320 3.5in - colorful UI
```

```c
CONFIG_USER_DISPLAY_ST7796S
```

## Display

| Setting | Value |
|---|---|
| Intended target | ESP32-S3 |
| Controller | ST7796S |
| Native resolution | 320×480 |
| Runtime orientation | 480×320 landscape |
| Interface | SPI |
| SPI frequency | 26 MHz |
| Colour order | BGR |
| Inversion | Off |
| Touch | Not configured |

## Display wiring

| Signal | GPIO |
|---|---:|
| MOSI | 10 |
| SCLK | 9 |
| MISO | Not used |
| CS | 13 |
| DC | 12 |
| RST | 11 |
| Backlight | 14 |

## Sensor defaults

| Setting | Value |
|---|---:|
| SEN54 I²C port | 0 |
| SEN54 SDA | GPIO4 |
| SEN54 SCL | GPIO5 |
| DS18B20 | Enabled |

## UI

The colorful UI includes:

- SEN54 temperature, humidity, PM2.5, and VOC;
- optional DS18B20 comparison temperature;
- SEN54 temperature-compensation values;
- BACnet Device instance and IP address;
- Wi-Fi and BACnet MS/TP status;
- colour-coded environmental categories.

## Source files

```text
main/ui/profiles/display_st7796s.cpp
components/TFT_eSPI/User_Setups/Setup_Project_ST7796S.h
```

The [ST7796S test profile](st7796s-test.md) uses the same display hardware setup.

## Images

![ST7796S colorful UI](../images/Profile1_UI.jpg)

![ST7796S display](../images/ST7796S_480x320_3.5in.jpg)
