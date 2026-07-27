#include "display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

#include <stdio.h>

#include "esp_log.h"
#include "User_Settings.h"

#include "board_tdisplay_s3.h"

static TFT_eSPI tft;
static const char *TAG = "display";

/* 170x320 panel in landscape orientation */
#define DISP_WIDTH  320
#define DISP_HEIGHT 170

#define DISP_GLOBAL_X_OFFSET 8
#define DISP_TEXT_X          (41 + DISP_GLOBAL_X_OFFSET)
#define DISP_TEXT_Y          20
#define DISP_ROW_SPACING     25

#define VALUE_X      (115 + DISP_GLOBAL_X_OFFSET)
#define VALUE_Y      20
#define VALUE_WIDTH  135
#define VALUE_HEIGHT 24
#define ROW_SPACING  25

#define FOOTER_Y      126
#define FOOTER_X      DISP_TEXT_X
#define FOOTER_WIDTH  DISP_WIDTH
#define FOOTER_HEIGHT 16

static void draw_footer(void)
{
    char footer_text[40];

    snprintf(
        footer_text,
        sizeof(footer_text),
        "ID:%lu MAC:%u",
        (unsigned long)USER_BACNET_DEVICE_INSTANCE,
        (unsigned int)USER_MSTP_MAC_ADDRESS);

    tft.fillRect(
        0,
        FOOTER_Y,
        FOOTER_WIDTH,
        FOOTER_HEIGHT,
        TFT_BLACK);

    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(FOOTER_X, FOOTER_Y);
    tft.print(footer_text);
}

extern "C" void display_init(void)
{
    initArduino();

        /*
        * GPIO15 supplies power to the LCD and onboard peripherals.
        * It must be enabled before initializing TFT_eSPI.
        */
        pinMode(BOARD_LCD_POWER_EN_PIN, OUTPUT);
        digitalWrite(
            BOARD_LCD_POWER_EN_PIN,
            BOARD_LCD_POWER_ON_LEVEL);
        delay(20);

    #if defined(TFT_BL) && (TFT_BL >= 0)
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
        delay(10);
    #endif

        tft.init();

    #if defined(TFT_BL) && (TFT_BL >= 0)
        if (ledcAttach(TFT_BL, 20000, 8)) {
            ledcWrite(TFT_BL, 64); /* Approximately 25% */
        } else {
            ESP_LOGW(TAG, "Failed to initialize backlight PWM");
        }
    #endif

        tft.setRotation(1);
        tft.fillScreen(TFT_BLACK);

    const char *labels[4] = {
        "Temp",
        "%RH",
        "PM2.5",
        "VOC"
    };

    for (int i = 0; i < 4; i++) {
        tft.setCursor(
            DISP_TEXT_X,
            DISP_TEXT_Y + i * DISP_ROW_SPACING);

        tft.print(labels[i]);
    }

    draw_footer();

    ESP_LOGI(TAG, "UI profile: LilyGO T-Display-S3 Touch");
    ESP_LOGI(TAG, "TFT hardware: %s", USER_SETUP_INFO);

        ESP_LOGI(
            TAG,
            "TFT I80 GPIO: D0=%d D1=%d D2=%d D3=%d "
            "D4=%d D5=%d D6=%d D7=%d",
            TFT_D0,
            TFT_D1,
            TFT_D2,
            TFT_D3,
            TFT_D4,
            TFT_D5,
            TFT_D6,
            TFT_D7);

        ESP_LOGI(
            TAG,
            "TFT control GPIO: CS=%d DC=%d RST=%d "
            "WR=%d RD=%d BL=%d PWR=%d",
            TFT_CS,
            TFT_DC,
            TFT_RST,
            TFT_WR,
            TFT_RD,
            TFT_BL,
            BOARD_LCD_POWER_EN_PIN);

    ESP_LOGI(
        TAG,
        "TFT native size: %dx%d; runtime size: %dx%d",
        TFT_WIDTH,
        TFT_HEIGHT,
        tft.width(),
        tft.height());
}

extern "C" void display_set_link_status(
    bool wifi_connected,
    bool mstp_connected)
{
    /*
     * The original compact UI has no link indicators.
     * Keep this API function so it remains compatible
     * with app_supervisor.c.
     */
    (void)wifi_connected;
    (void)mstp_connected;
}

extern "C" void display_update_values(
    float pm25,
    float temperature,
    float humidity,
    float voc,
    float temp_ds18b20)
{
    /*
     * This compact four-row UI does not display the
     * optional DS18B20 value.
     */
    (void)temp_ds18b20;

    const float values[4] = {
        temperature,
        humidity,
        pm25,
        voc
    };

    const char *formats[4] = {
        "%.1f",
        "%.1f",
        "%.0f",
        "%.0f"
    };

    char buffer[16];

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);

    for (int i = 0; i < 4; i++) {
        int y = VALUE_Y + i * ROW_SPACING;

        tft.fillRect(
            VALUE_X,
            y,
            VALUE_WIDTH,
            VALUE_HEIGHT,
            TFT_BLACK);

        snprintf(
            buffer,
            sizeof(buffer),
            formats[i],
            values[i]);

        tft.setCursor(VALUE_X, y);
        tft.print(buffer);
    }
}