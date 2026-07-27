#pragma once

/* Interface selection */
#define BOARD_LCD_INTERFACE_SPI 0
#define BOARD_LCD_INTERFACE_I80 1

/* Display */
#define BOARD_DISPLAY_ENABLED     1
#define BOARD_DISPLAY_TYPE_ST7789 1
#define BOARD_LCD_INTERFACE       BOARD_LCD_INTERFACE_I80
#define BOARD_LCD_WIDTH           170
#define BOARD_LCD_HEIGHT          320

/* LCD I80 data bus */
#define BOARD_LCD_PIN_NUM_D0 39
#define BOARD_LCD_PIN_NUM_D1 40
#define BOARD_LCD_PIN_NUM_D2 41
#define BOARD_LCD_PIN_NUM_D3 42
#define BOARD_LCD_PIN_NUM_D4 45
#define BOARD_LCD_PIN_NUM_D5 46
#define BOARD_LCD_PIN_NUM_D6 47
#define BOARD_LCD_PIN_NUM_D7 48

/* LCD control */
#define BOARD_LCD_PIN_NUM_WR  8
#define BOARD_LCD_PIN_NUM_RD  9
#define BOARD_LCD_PIN_NUM_CS  6
#define BOARD_LCD_PIN_NUM_DC  7
#define BOARD_LCD_PIN_NUM_RST 5

/* Power and backlight */
#define BOARD_LCD_POWER_EN_PIN       15
#define BOARD_LCD_POWER_ON_LEVEL     1
#define BOARD_LCD_PIN_NUM_BK_LIGHT   38
#define BOARD_LCD_BK_LIGHT_ON_LEVEL  1

/* CST816 touch */
#define BOARD_TOUCH_ENABLED     1
#define BOARD_TOUCH_I2C_PORT    0
#define BOARD_TOUCH_I2C_SDA_PIN 18
#define BOARD_TOUCH_I2C_SCL_PIN 17
#define BOARD_TOUCH_I2C_ADDR    0x15
#define BOARD_TOUCH_PIN_NUM_INT 16
#define BOARD_TOUCH_PIN_NUM_RST 21

/* Buttons */
#define BOARD_BUTTON_BOOT 0
#define BOARD_BUTTON_USER 14

/* Battery ADC */
#define BOARD_BAT_ADC_PIN 4

/* Native esp_lcd values, retained for possible future use */
#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define LCD_H_RES          170
#define LCD_V_RES          320
#define LCD_GAP_X          0
#define LCD_GAP_Y          35