# Heltec WiFi LoRa 32 V4 gateway

ESP32-S3 gateway profile for a Heltec WiFi LoRa 32 V4 board using its SX1262
radio and a 128x64 SSD1315 OLED. The gateway receives the project's fixed-size
LoRa sensor packets and presents the received count, VOC index, and PM2.5 on
the OLED while the BACnet services remain active.

## Select the profile

Select the `esp32s3` ESP-IDF target, then choose:

```text
Project hardware
\- Display profile
   \- LoRa 32 V4 SX1262 gateway receiver
```

This selects:

```text
CONFIG_USER_DISPLAY_LORA_GATEWAY
```

The profile does not use TFT_eSPI. It compiles the OLED UI, LoRa gateway,
SX1262 driver, and LoRa hardware abstraction layer.

## Hardware wiring

The pin assignments are defined in `main/platform/board_pins.h`.

### SX1262 radio and front-end

| Signal | GPIO |
|---|---:|
| NSS / CS | 8 |
| SCK | 9 |
| MOSI | 10 |
| MISO | 11 |
| RESET | 12 |
| BUSY | 13 |
| DIO1 | 14 |
| PA power | 7 |
| PA enable | 2 |
| PA TX enable | 46 |

The radio uses SPI2 at 8 MHz. The front-end is initialized for receive mode;
the GC1109 control pins are managed by the LoRa hardware abstraction layer.

### OLED

The OLED wiring is defined in `main/ui/profiles/display_lora_gateway.c`.

| Signal | GPIO or value |
|---|---:|
| Controller | SSD1315 |
| I2C address | 0x3C |
| SDA | 17 |
| SCL | 18 |
| RESET | 21 |
| VEXT | 36 |
| I2C speed | 400 kHz |

The OLED VEXT control is driven low to power the display. The UI uses a
128x64 landscape framebuffer.

## Runtime behavior

- SEN54 and DS18B20 acquisition is disabled for this profile.
- A LoRa gateway task receives and validates 26-byte protocol-version-1
  packets.
- The expected LoRa device ID and protocol values come from
  `main/User_Settings.c`.
- The OLED displays the accepted packet count, VOC index, and PM2.5 value.
- The profile-specific SEN54 defaults remain GPIO4/GPIO5, but no SEN54 task is
  compiled for this profile.

Before building, verify that the board revision and attached front-end use the
pin assignments above. Changing the radio or OLED wiring requires updating the
corresponding platform or display source files as well as this document.
