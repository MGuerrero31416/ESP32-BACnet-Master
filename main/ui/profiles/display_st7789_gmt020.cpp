#include "display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

#include <stdio.h>

#include "esp_log.h"
#include "User_Settings.h"

static TFT_eSPI tft;
static const char *TAG = "display";

/* 240x320 panel in landscape orientation */
#define DISP_WIDTH  320
#define DISP_HEIGHT 240

#define DISP_GLOBAL_X_OFFSET 8

#define DISP_TEXT_X          (41 + DISP_GLOBAL_X_OFFSET)
#define DISP_TEXT_Y          30
#define DISP_ROW_SPACING     36

#define VALUE_X              (115 + DISP_GLOBAL_X_OFFSET)
#define VALUE_Y              30
#define VALUE_WIDTH          135
#define VALUE_HEIGHT         28
#define ROW_SPACING          36

#define FOOTER_Y             205
#define FOOTER_X             DISP_TEXT_X
#define FOOTER_WIDTH         DISP_WIDTH
#define FOOTER_HEIGHT        18

#ifdef TFT_BL
#define DISPLAY_TFT_BL TFT_BL
#else
#define DISPLAY_TFT_BL -1
#endif

#if defined(TFT_BL) && defined(TFT_BACKLIGHT_ON) && (TFT_BL >= 0)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    delay(10);
#endif

#if defined(TFT_BL) && defined(TFT_BACKLIGHT_ON) && (TFT_BL >= 0)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    delay(10);
#endif

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

#if defined(TFT_BL) && (TFT_BL >= 0)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    delay(10);
#endif

    tft.init();

    #if defined(TFT_BL) && (TFT_BL >= 0)
        if (ledcAttach(TFT_BL, 20000, 8)) {
            ledcWrite(TFT_BL, 125);  // 25% brightness: 64 out of 255
        } else {
            ESP_LOGW(TAG, "Failed to initialize backlight PWM");
        }
    #endif

    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);

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

    ESP_LOGI(TAG, "UI profile: ST7789 GMT020");
    ESP_LOGI(TAG, "TFT hardware: %s", USER_SETUP_INFO);

    ESP_LOGI(
        TAG,
        "TFT GPIO: MOSI=%d SCLK=%d MISO=%d "
        "CS=%d DC=%d RST=%d BL=%d",
        TFT_MOSI,
        TFT_SCLK,
        TFT_MISO,
        TFT_CS,
        TFT_DC,
        TFT_RST,
        DISPLAY_TFT_BL);

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