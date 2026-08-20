/* LVGL T-Display S3 profile (copy of ST7789 LVGL profile adapted for LVGL port)
   Implements functions in main/ui/display.h */
#include "display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "lvgl.h"

#include <stdio.h>

#include "esp_timer.h"
#include "esp_log.h"
#include "User_Settings.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "board_tdisplay_s3.h"
#include "ui/hardware/touch_cst816.h"

static TFT_eSPI tft;
static const char *TAG = "display_lvgl";

/* 170x320 panel in landscape orientation */
#define DISP_WIDTH  320
#define DISP_HEIGHT 170

static lv_disp_draw_buf_t s_lvgl_draw_buf;
static lv_color_t s_lvgl_buf[DISP_WIDTH * 20];
static esp_timer_handle_t s_lvgl_tick_timer;
static SemaphoreHandle_t s_lvgl_mutex;
static bool s_lvgl_inited;

typedef enum {
    UI_SCREEN_MEASUREMENTS = 0,
    UI_SCREEN_PLACEHOLDER,
    UI_SCREEN_PLACEHOLDER_2,
    UI_SCREEN_WIFI_SETTINGS,
} ui_screen_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *value_labels[4];
    lv_obj_t *footer_label;
    lv_obj_t *right_arrow;
} measurements_screen_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *title_label;
    lv_obj_t *body_label;
    lv_obj_t *left_arrow;
    lv_obj_t *right_arrow;
} placeholder_screen_t;

static measurements_screen_t s_measurements_screen = {};
static placeholder_screen_t s_placeholder_screen = {};
static placeholder_screen_t s_placeholder_screen_2 = {};
static placeholder_screen_t s_wifi_settings_screen = {};
static ui_screen_t s_active_screen = UI_SCREEN_MEASUREMENTS;
static float s_latest_pm25;
static float s_latest_temperature;
static float s_latest_humidity;
static float s_latest_voc;

static const lv_coord_t UI_SCREEN_MARGIN_X = 14;
static const lv_coord_t UI_NAV_ARROW_INSET = 8;

static void nav_button_event_cb(lv_event_t *e);

static void add_click_nav_target(lv_obj_t *obj, ui_screen_t next_screen)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(obj, 10);
    lv_obj_add_event_cb(obj, nav_button_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)next_screen);
}

static void set_screen_bg(lv_obj_t *screen)
{
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

static void set_screen_border(lv_obj_t *screen)
{
    lv_obj_set_style_border_width(screen, 2, 0);
    lv_obj_set_style_border_color(screen, lv_color_hex(0xFF00FF), 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
}

static lv_obj_t *create_text_label(
    lv_obj_t *parent,
    const char *text,
    lv_coord_t x,
    lv_coord_t y,
    const lv_font_t *font,
    lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_pos(label, x, y);
    return label;
}

static lv_obj_t *create_nav_arrow(lv_obj_t *parent, const char *text, lv_align_t align)
{
    lv_obj_t *arrow = lv_label_create(parent);
    lv_label_set_text(arrow, text);
    lv_obj_set_style_text_font(arrow, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(arrow, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_bg_opa(arrow, LV_OPA_TRANSP, 0);
    lv_obj_align(arrow, align, 0, 0);

    if (align == LV_ALIGN_LEFT_MID) {
        lv_obj_set_x(arrow, UI_NAV_ARROW_INSET);
    } else if (align == LV_ALIGN_RIGHT_MID) {
        lv_obj_set_x(arrow, -UI_NAV_ARROW_INSET);
    }

    return arrow;
}

static void update_measurement_value(
    lv_obj_t *label,
    float value,
    const char *format)
{
    if (label == NULL) {
        return;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), format, value);
    lv_label_set_text(label, buf);
}

static void show_screen(ui_screen_t screen_id)
{
    if (screen_id == UI_SCREEN_MEASUREMENTS && s_measurements_screen.screen != NULL) {
        lv_scr_load(s_measurements_screen.screen);
        s_active_screen = UI_SCREEN_MEASUREMENTS;
        return;
    }

    if (screen_id == UI_SCREEN_PLACEHOLDER && s_placeholder_screen.screen != NULL) {
        lv_scr_load(s_placeholder_screen.screen);
        s_active_screen = UI_SCREEN_PLACEHOLDER;
        return;
    }

    if (screen_id == UI_SCREEN_PLACEHOLDER_2 && s_placeholder_screen_2.screen != NULL) {
        lv_scr_load(s_placeholder_screen_2.screen);
        s_active_screen = UI_SCREEN_PLACEHOLDER_2;
        return;
    }

    if (screen_id == UI_SCREEN_WIFI_SETTINGS && s_wifi_settings_screen.screen != NULL) {
        lv_scr_load(s_wifi_settings_screen.screen);
        s_active_screen = UI_SCREEN_WIFI_SETTINGS;
    }
}

static void nav_button_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    ui_screen_t next_screen = (ui_screen_t)(uintptr_t)lv_event_get_user_data(e);
    show_screen(next_screen);
}

static lv_obj_t *create_nav_arrow_button(
    lv_obj_t *parent,
    const char *text,
    lv_align_t align,
    ui_screen_t next_screen)
{
    lv_obj_t *arrow = create_nav_arrow(parent, text, align);
    add_click_nav_target(arrow, next_screen);
    return arrow;
}

static void apply_measurement_screen_values(void)
{
    update_measurement_value(s_measurements_screen.value_labels[0], s_latest_temperature, "%.1f");
    update_measurement_value(s_measurements_screen.value_labels[1], s_latest_humidity, "%.1f");
    update_measurement_value(s_measurements_screen.value_labels[2], s_latest_pm25, "%.0f");
    update_measurement_value(s_measurements_screen.value_labels[3], s_latest_voc, "%.0f");
}

static void create_measurements_screen(void)
{
    static const char *labels[4] = {
        "Temp",
        "%HR",
        "PM2.5",
        "VOC",
    };

    const lv_coord_t disp_text_x = UI_SCREEN_MARGIN_X + 6;
    const lv_coord_t disp_text_y = 20;
    const lv_coord_t value_x = 123;
    const lv_coord_t value_y = 18;
    const lv_coord_t footer_x = disp_text_x;
    const lv_coord_t footer_y = 126;
    const lv_coord_t row_spacing = 25;

    lv_obj_t *screen = lv_obj_create(NULL);
    s_measurements_screen.screen = screen;

    set_screen_bg(screen);
    set_screen_border(screen);

    for (int i = 0; i < 4; ++i) {
        create_text_label(
            screen,
            labels[i],
            disp_text_x,
            disp_text_y + (i * row_spacing),
            &lv_font_montserrat_20,
            lv_color_hex(0xFFFF00));

        s_measurements_screen.value_labels[i] = create_text_label(
            screen,
            "--",
            value_x,
            value_y + (i * row_spacing),
            &lv_font_montserrat_24,
            lv_color_hex(0xFFFFFF));
    }

    s_measurements_screen.footer_label = create_text_label(
        screen,
        "",
        footer_x,
        footer_y,
        &lv_font_montserrat_20,
        lv_color_hex(0x3A7DFF));

    s_measurements_screen.right_arrow = create_nav_arrow_button(
        screen,
        ">",
        LV_ALIGN_RIGHT_MID,
        UI_SCREEN_PLACEHOLDER);

    apply_measurement_screen_values();
}

static void create_placeholder_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    s_placeholder_screen.screen = screen;

    set_screen_bg(screen);
    set_screen_border(screen);

    s_placeholder_screen.title_label = lv_label_create(screen);
    lv_label_set_text(s_placeholder_screen.title_label, "Placeholder 1");
    lv_obj_set_pos(s_placeholder_screen.title_label, 94, 34);
    lv_obj_set_style_text_font(s_placeholder_screen.title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_placeholder_screen.title_label, lv_color_hex(0xFFFFFF), 0);

    s_placeholder_screen.body_label = lv_label_create(screen);
    lv_label_set_text(s_placeholder_screen.body_label, "Second screen");
    lv_obj_set_pos(s_placeholder_screen.body_label, 44, 74);
    lv_obj_set_style_text_font(s_placeholder_screen.body_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_placeholder_screen.body_label, lv_color_hex(0xBDBDBD), 0);

    s_placeholder_screen.left_arrow = create_nav_arrow_button(
        screen,
        "<",
        LV_ALIGN_LEFT_MID,
        UI_SCREEN_MEASUREMENTS);
    s_placeholder_screen.right_arrow = create_nav_arrow_button(
        screen,
        ">",
        LV_ALIGN_RIGHT_MID,
        UI_SCREEN_PLACEHOLDER_2);
}

static void create_placeholder_screen_2(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    s_placeholder_screen_2.screen = screen;

    set_screen_bg(screen);

    s_placeholder_screen_2.title_label = lv_label_create(screen);
    lv_label_set_text(s_placeholder_screen_2.title_label, "Settings");
    lv_obj_set_pos(s_placeholder_screen_2.title_label, 105, 16);
    lv_obj_set_style_text_font(s_placeholder_screen_2.title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_placeholder_screen_2.title_label, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *wifi_tile = lv_obj_create(screen);
    lv_obj_set_size(wifi_tile, 138, 92);
    lv_obj_center(wifi_tile);
    lv_obj_set_style_bg_color(wifi_tile, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_bg_opa(wifi_tile, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(wifi_tile, lv_color_hex(0x3A7DFF), 0);
    lv_obj_set_style_border_width(wifi_tile, 2, 0);
    lv_obj_set_style_radius(wifi_tile, 14, 0);
    lv_obj_set_style_pad_all(wifi_tile, 10, 0);
    lv_obj_clear_flag(wifi_tile, LV_OBJ_FLAG_SCROLLABLE);

    s_placeholder_screen_2.body_label = lv_label_create(wifi_tile);
    lv_label_set_text(s_placeholder_screen_2.body_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(s_placeholder_screen_2.body_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(s_placeholder_screen_2.body_label, lv_color_hex(0x00FFFF), 0);
    lv_obj_align(s_placeholder_screen_2.body_label, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t *wifi_label = lv_label_create(wifi_tile);
    lv_label_set_text(wifi_label, "Wi-Fi");
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(wifi_label, LV_ALIGN_BOTTOM_MID, 0, -6);

    add_click_nav_target(wifi_tile, UI_SCREEN_WIFI_SETTINGS);

    s_placeholder_screen_2.left_arrow = create_nav_arrow_button(
        screen,
        "<",
        LV_ALIGN_LEFT_MID,
        UI_SCREEN_PLACEHOLDER);
    s_placeholder_screen_2.right_arrow = create_nav_arrow(screen, ">", LV_ALIGN_RIGHT_MID);
}

static void create_wifi_settings_screen(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    s_wifi_settings_screen.screen = screen;

    set_screen_bg(screen);

    s_wifi_settings_screen.title_label = lv_label_create(screen);
    lv_label_set_text(s_wifi_settings_screen.title_label, "Wi-Fi Settings");
    lv_obj_set_pos(s_wifi_settings_screen.title_label, 72, 34);
    lv_obj_set_style_text_font(s_wifi_settings_screen.title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_wifi_settings_screen.title_label, lv_color_hex(0xFFFFFF), 0);

    s_wifi_settings_screen.body_label = lv_label_create(screen);
    lv_label_set_text(s_wifi_settings_screen.body_label, "Placeholder");
    lv_obj_set_pos(s_wifi_settings_screen.body_label, 108, 74);
    lv_obj_set_style_text_font(s_wifi_settings_screen.body_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_wifi_settings_screen.body_label, lv_color_hex(0xBDBDBD), 0);

    s_wifi_settings_screen.left_arrow = create_nav_arrow_button(
        screen,
        "<",
        LV_ALIGN_LEFT_MID,
        UI_SCREEN_PLACEHOLDER_2);
    s_wifi_settings_screen.right_arrow = create_nav_arrow(screen, ">", LV_ALIGN_RIGHT_MID);
}

static void lvgl_flush_cb(
    lv_disp_drv_t *disp_drv,
    const lv_area_t *area,
    lv_color_t *color_p)
{
    const uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    const uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp_drv);
}

static void lvgl_tick_timer_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(1);
}

static void lvgl_touch_read_cb(
    lv_indev_drv_t *indev_drv,
    lv_indev_data_t *data)
{
    (void)indev_drv;

    /* Diagnostic: log every time LVGL calls the touch read callback */
    ESP_LOGI("LVGL_TOUCH", "lvgl_touch_read_cb called");

    touch_cst816_point_t pt;
    esp_err_t ret = touch_cst816_read(&pt);
    if (ret == ESP_OK) {
        /* Debug log raw values to verify callback calls and raw coordinates */
        ESP_LOGI("LVGL_TOUCH", "Raw X=%u Y=%u TouchPoints=%d", (unsigned int)pt.raw_x, (unsigned int)pt.raw_y, pt.pressed ? 1 : 0);

        /* Use touch_points (pt.pressed) to set LVGL state */
        if (pt.pressed) {
            /* Rotate 90 degrees clockwise to match runtime display size (320x170)
             * CST816 raw: portrait coordinates (raw_x: 0..169, raw_y: 0..319)
             * Mapping: lv_x = raw_y (0..319 -> 0..319),
             *          lv_y = (portrait_width - 1) - raw_x (0..169 -> 0..169)
             */
            int lv_x = (int)pt.raw_y;
            int lv_y = (int)(TOUCH_PORTRAIT_W - 1) - (int)pt.raw_x;

            if (lv_x < 0) lv_x = 0;
            if (lv_x > DISP_WIDTH - 1) lv_x = DISP_WIDTH - 1;
            if (lv_y < 0) lv_y = 0;
            if (lv_y > DISP_HEIGHT - 1) lv_y = DISP_HEIGHT - 1;

            data->state = LV_INDEV_STATE_PR;
            data->point.x = lv_x;
            data->point.y = lv_y;
            return;
        }
    }

    data->state = LV_INDEV_STATE_REL;
}

static void lvgl_task(void *arg)
{
    (void)arg;

    while (true) {
        if (s_lvgl_mutex != NULL && xSemaphoreTake(s_lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            lv_timer_handler();
            xSemaphoreGive(s_lvgl_mutex);
        } else {
            lv_timer_handler();
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void lvgl_init(void)
{
    if (s_lvgl_inited) {
        return;
    }

    lv_init();
    if (s_lvgl_mutex == NULL) {
        s_lvgl_mutex = xSemaphoreCreateMutex();
    }

    lv_disp_draw_buf_init(
        &s_lvgl_draw_buf,
        s_lvgl_buf,
        NULL,
        DISP_WIDTH * 20);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISP_WIDTH;
    disp_drv.ver_res = DISP_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &s_lvgl_draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    const esp_timer_create_args_t tick_timer_args = {
        .callback = &lvgl_tick_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lvgl_tick",
        .skip_unhandled_events = true,
    };

    if (esp_timer_create(&tick_timer_args, &s_lvgl_tick_timer) == ESP_OK) {
        if (esp_timer_start_periodic(s_lvgl_tick_timer, 1000) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start LVGL tick timer");
        }
    } else {
        ESP_LOGE(TAG, "Failed to create LVGL tick timer");
    }

    create_measurements_screen();
    create_placeholder_screen();
    create_placeholder_screen_2();
    create_wifi_settings_screen();
    show_screen(UI_SCREEN_MEASUREMENTS);

    if (xTaskCreate(
            lvgl_task,
            "lvgl",
            4096,
            NULL,
            5,
            NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        return;
    }

    s_lvgl_inited = true;
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

    esp_err_t touch_ret = touch_cst816_init();
    if (touch_ret != ESP_OK) {
        ESP_LOGW(TAG, "CST816 init failed: %s", esp_err_to_name(touch_ret));
    }

    if (!s_lvgl_inited) {
        lvgl_init();
    }

    ESP_LOGI(TAG, "UI profile: LilyGO T-Display-S3 LVGL measurements screen (LVGL port)");
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
    (void)wifi_connected;
    (void)mstp_connected;

    /* Kept for API compatibility with app supervisor. */
}

extern "C" void display_update_values(
    float pm25,
    float temperature,
    float humidity,
    float voc,
    float temp_ds18b20)
{
    (void)temp_ds18b20;

    if (!s_lvgl_inited || s_lvgl_mutex == NULL) {
        s_latest_pm25 = pm25;
        s_latest_temperature = temperature;
        s_latest_humidity = humidity;
        s_latest_voc = voc;
        return;
    }

    if (xSemaphoreTake(s_lvgl_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return;
    }

    s_latest_pm25 = pm25;
    s_latest_temperature = temperature;
    s_latest_humidity = humidity;
    s_latest_voc = voc;
    apply_measurement_screen_values();
    xSemaphoreGive(s_lvgl_mutex);
}
