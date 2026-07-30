#pragma once

#define USER_SETUP_INFO \
    "GMT020-02-7P ST7789 240x320"

/* TFT controller */
#define ST7789_DRIVER

/* Native portrait resolution */
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

/* SPI wiring */
#define TFT_SCLK 32
#define TFT_MOSI 33
#define TFT_RST  25
#define TFT_DC   26
#define TFT_CS   27

#define TFT_MISO -1

/*
 * No software-controlled backlight GPIO was provided.
 *
 * Defining TFT_BL as -1 is necessary because the current
 * simple UI prints TFT_BL in its startup diagnostics.
 */
//#define TFT_BL -1

/* SPI frequencies from the working configuration */
#define SPI_FREQUENCY      40000000
#define SPI_READ_FREQUENCY 20000000

/* Panel colour order */
#define TFT_RGB_ORDER TFT_BGR

/* Fonts */
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

/* No touch controller */
//#define TOUCH_CS -1