#pragma once

#define USER_SETUP_INFO \
    "ESP32-WROOM-32 + HW657A ST7789 170x320"

/* TFT controller */
#define ST7789_DRIVER

/* Native portrait resolution */
#define TFT_WIDTH  170
#define TFT_HEIGHT 320

/* SPI wiring */
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO -1

#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4

/* Backlight */
#define TFT_BL 32
#define TFT_BACKLIGHT_ON HIGH

/* Working SPI speed from the original project */
#define SPI_FREQUENCY 20000000

/* Panel-specific settings */
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON

#define TFT_OFFSET_X 0
#define TFT_OFFSET_Y 0

/* Fonts */
#define LOAD_GLCD
/* #define LOAD_FONT2 */
/* #define LOAD_FONT4 */
/* Keep larger and free fonts disabled to reduce flash usage on 4 MB ESP32. */
/* #define LOAD_FONT6 */
/* #define LOAD_FONT7 */
/* #define LOAD_FONT8 */
/* #define LOAD_GFXFF */
/* Smooth-font rendering is not used by this profile UI. */
/* #define SMOOTH_FONT */

/* No touch controller */
#define TOUCH_CS -1