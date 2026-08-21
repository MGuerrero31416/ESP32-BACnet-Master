/* LilyGO T-Display-S3 board bring-up for the LVGL UI.
   Screen content lives in main/ui/lvgl; this file only owns the panel and touch. */
#include "display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"

#include "User_Settings.h"
#include "board_tdisplay_s3.h"
#include "ui/hardware/touch_cst816.h"
#include "ui/lvgl/ui_manager.h"
#include "ui/lvgl/ui_model.h"
#include "ui/lvgl/ui_port.h"

static TFT_eSPI tft;
static const char *TAG = "display_lvgl";

/* 170x320 panel in landscape orientation */
#define DISP_WIDTH  320
#define DISP_HEIGHT 170

static void panel_flush(const lv_area_t *area, lv_color_t *pixels)
{
    const uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    const uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)pixels, w * h, true);
    tft.endWrite();
}

static bool panel_touch(int16_t *x, int16_t *y)
{
    touch_cst816_point_t pt;

    if (touch_cst816_read(&pt) != ESP_OK || !pt.pressed) {
        return false;
    }

    /* CST816 reports portrait coordinates (raw_x: 0..169, raw_y: 0..319);
       rotate 90 deg CW to match the landscape panel. */
    int lv_x = (int)pt.raw_y;
    int lv_y = (DISP_HEIGHT - 1) - (int)pt.raw_x;

    if (lv_x < 0) lv_x = 0;
    if (lv_x > DISP_WIDTH - 1) lv_x = DISP_WIDTH - 1;
    if (lv_y < 0) lv_y = 0;
    if (lv_y > DISP_HEIGHT - 1) lv_y = DISP_HEIGHT - 1;

    *x = (int16_t)lv_x;
    *y = (int16_t)lv_y;
    return true;
}

extern "C" void display_init(void)
{
    initArduino();

    /* GPIO15 supplies power to the LCD and onboard peripherals;
       it must be enabled before initializing TFT_eSPI. */
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
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

#if defined(TFT_BL) && (TFT_BL >= 0)
    if (ledcAttach(TFT_BL, 20000, 8)) {
        ledcWrite(TFT_BL, 64); /* Approximately 25% */
    } else {
        ESP_LOGW(TAG, "Failed to initialize backlight PWM");
    }
#endif

    const bool touch_ok = (touch_cst816_init() == ESP_OK);
    if (!touch_ok) {
        ESP_LOGW(TAG, "CST816 init failed; UI runs without touch");
    }

    ui_port_config_t cfg = {};
    cfg.hor_res = DISP_WIDTH;
    cfg.ver_res = DISP_HEIGHT;
    cfg.draw_buf_lines = 20;
    cfg.flush = panel_flush;
    cfg.touch = touch_ok ? panel_touch : NULL;

    const esp_err_t err = ui_port_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LVGL port init failed: %s", esp_err_to_name(err));
        return;
    }

    if (ui_port_lock(portMAX_DELAY)) {
        ui_manager_init();
        ui_port_unlock();
    }

    ESP_LOGI(TAG, "UI profile: LilyGO T-Display-S3 LVGL (screen registry)");
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
    ui_model_set_link(wifi_connected, mstp_connected);
}

extern "C" void display_update_values(
    float pm25,
    float temperature,
    float humidity,
    float voc,
    float temp_ds18b20)
{
    ui_model_set_values(pm25, temperature, humidity, voc, temp_ds18b20);
}
