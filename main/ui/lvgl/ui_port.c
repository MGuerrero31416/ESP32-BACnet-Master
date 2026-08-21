#include "ui_port.h"

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "ui_port";

#define UI_PORT_TASK_STACK 4096
#define UI_PORT_TASK_PRIO  5

static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;
static lv_indev_drv_t s_indev_drv;
static esp_timer_handle_t s_tick_timer;
static SemaphoreHandle_t s_mutex;
static ui_port_config_t s_cfg;
static bool s_ready;

static void flush_cb(
    lv_disp_drv_t *disp_drv,
    const lv_area_t *area,
    lv_color_t *color_p)
{
    if (s_cfg.flush != NULL) {
        s_cfg.flush(area, color_p);
    }

    lv_disp_flush_ready(disp_drv);
}

static void tick_timer_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(1);
}

static void touch_read_cb(
    lv_indev_drv_t *indev_drv,
    lv_indev_data_t *data)
{
    (void)indev_drv;

    int16_t x = 0;
    int16_t y = 0;

    if (s_cfg.touch != NULL && s_cfg.touch(&x, &y)) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
        return;
    }

    data->state = LV_INDEV_STATE_REL;
}

static void ui_port_task(void *arg)
{
    (void)arg;

    while (true) {
        if (ui_port_lock(portMAX_DELAY)) {
            lv_timer_handler();
            ui_port_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

bool ui_port_lock(uint32_t timeout_ms)
{
    if (s_mutex == NULL) {
        return false;
    }

    const TickType_t ticks = (timeout_ms == portMAX_DELAY)
        ? portMAX_DELAY
        : pdMS_TO_TICKS(timeout_ms);

    return xSemaphoreTakeRecursive(s_mutex, ticks) == pdTRUE;
}

void ui_port_unlock(void)
{
    if (s_mutex != NULL) {
        xSemaphoreGiveRecursive(s_mutex);
    }
}

bool ui_port_is_ready(void)
{
    return s_ready;
}

lv_coord_t ui_port_width(void)
{
    return s_cfg.hor_res;
}

lv_coord_t ui_port_height(void)
{
    return s_cfg.ver_res;
}

esp_err_t ui_port_init(const ui_port_config_t *cfg)
{
    if (cfg == NULL || cfg->flush == NULL || cfg->hor_res <= 0 || cfg->ver_res <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_ready) {
        return ESP_OK;
    }

    s_cfg = *cfg;
    if (s_cfg.draw_buf_lines == 0) {
        s_cfg.draw_buf_lines = 20;
    }

    s_mutex = xSemaphoreCreateRecursiveMutex();
    if (s_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL mutex");
        return ESP_ERR_NO_MEM;
    }

    lv_init();

    const size_t px_count = (size_t)s_cfg.hor_res * s_cfg.draw_buf_lines;
    lv_color_t *buf = heap_caps_malloc(
        px_count * sizeof(lv_color_t),
        MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate LVGL draw buffer");
        return ESP_ERR_NO_MEM;
    }

    lv_disp_draw_buf_init(&s_draw_buf, buf, NULL, px_count);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = s_cfg.hor_res;
    s_disp_drv.ver_res = s_cfg.ver_res;
    s_disp_drv.flush_cb = flush_cb;
    s_disp_drv.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&s_disp_drv);

    if (s_cfg.touch != NULL) {
        lv_indev_drv_init(&s_indev_drv);
        s_indev_drv.type = LV_INDEV_TYPE_POINTER;
        s_indev_drv.read_cb = touch_read_cb;
        lv_indev_drv_register(&s_indev_drv);
    }

    const esp_timer_create_args_t tick_timer_args = {
        .callback = &tick_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lvgl_tick",
        .skip_unhandled_events = true,
    };

    esp_err_t err = esp_timer_create(&tick_timer_args, &s_tick_timer);
    if (err == ESP_OK) {
        err = esp_timer_start_periodic(s_tick_timer, 1000);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LVGL tick timer failed: %s", esp_err_to_name(err));
        return err;
    }

    if (xTaskCreate(
            ui_port_task,
            "lvgl",
            UI_PORT_TASK_STACK,
            NULL,
            UI_PORT_TASK_PRIO,
            NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        return ESP_ERR_NO_MEM;
    }

    s_ready = true;
    return ESP_OK;
}
