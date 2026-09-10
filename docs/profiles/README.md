# Hardware and Display Profiles

The ESP-IDF processor target and hardware/display profile are selected separately.

Select the processor with:

```text
ESP-IDF: Set Espressif Device Target
```

Select the profile in:

```text
Project hardware
└── Display profile
```

## Available profiles

| Profile | Kconfig symbol | Intended target | Display | Runtime size | Interface | Touch |
|---|---|---|---|---:|---|---|
| [ST7796S colorful](st7796s-colorful.md) | `CONFIG_USER_DISPLAY_ST7796S` | ESP32-S3 | ST7796S | 480×320 | SPI | No |
| [ST7796S test](st7796s-test.md) | `CONFIG_USER_DISPLAY_ST7796S_TEST` | ESP32-S3 | ST7796S | 480×320 | SPI | No |
| [LilyGO T-Display-S3 LVGL](tdisplay-s3-lvgl.md) | `CONFIG_USER_DISPLAY_LVGL_TDISPLAY_S3` | ESP32-S3 | ST7789 | 320×170 | 8-bit parallel | CST816 |
| [GMT020-02-7P](gmt020-02-7p.md) | `CONFIG_USER_DISPLAY_ST7789_GMT020` | ESP32 | ST7789 | 320×240 | SPI | No |
| [HW657A](hw657a.md) | `CONFIG_USER_DISPLAY_ST7789_HW657A` | ESP32 | ST7789 | 320×170 | SPI | No |
| [Heltec WiFi LoRa 32 V4 gateway](lora-gateway.md) | `CONFIG_USER_DISPLAY_LORA_GATEWAY` | ESP32-S3 | SSD1315 OLED | 128×64 | I2C + SX1262 SPI | No |
| [No display](no-display.md) | `CONFIG_USER_DISPLAY_NONE` | ESP32 or ESP32-S3 | None | — | — | — |

The display profile selects:

- one UI implementation from `main/ui/profiles/`;
- the matching display hardware configuration (TFT_eSPI for TFT profiles);
- profile-specific SEN54, touch, and DS18B20 defaults.

After changing a profile or TFT setup, run a full clean build:

```powershell
idf.py fullclean
idf.py build
```

For instructions on adding or modifying profiles, see
[Display Hardware Profiles Guide](../DISPLAY_HARDWARE_PROFILES_GUIDE.md).
