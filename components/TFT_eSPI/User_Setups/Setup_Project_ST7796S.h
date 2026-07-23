#pragma once

/*
 * Project display hardware profile:
 * ESP32-S3 + ST7796S 320x480 SPI TFT
 */

#define USER_SETUP_INFO "Project ST7796S 320x480 SPI"

// -------------------------------------------------------
// Display controller
// -------------------------------------------------------

#define ST7796_DRIVER

// Native portrait panel dimensions.
// The UI later rotates this to 480x320 landscape.
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// -------------------------------------------------------
// SPI GPIO mapping
// -------------------------------------------------------

#define TFT_MOSI 10
#define TFT_SCLK 9
#define TFT_MISO -1

#define TFT_CS   13
#define TFT_DC   12
#define TFT_RST  11

// -------------------------------------------------------
// Backlight
// -------------------------------------------------------

#define TFT_BL 14
#define TFT_BACKLIGHT_ON HIGH

// -------------------------------------------------------
// ESP32-S3 SPI controller
// -------------------------------------------------------

#define USE_HSPI_PORT

// -------------------------------------------------------
// SPI speed
// -------------------------------------------------------

#define SPI_FREQUENCY 26000000

// -------------------------------------------------------
// Panel-specific colour and inversion settings
// -------------------------------------------------------

#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_OFF

// -------------------------------------------------------
// Fonts
// -------------------------------------------------------

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// -------------------------------------------------------
// Touch controller
// -------------------------------------------------------

#define TOUCH_CS -1