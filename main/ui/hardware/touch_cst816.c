#include "touch_cst816.h"

#include "sdkconfig.h"

#if CONFIG_USER_DISPLAY_ST7789_TDISPLAY_S3 && CONFIG_USER_TOUCH_CST816

#include <stddef.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
//#include "esp_timer.h" // Uncomment if you want to log touch coordinates every 250ms when pressed
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "touch_cst816"

#define CST816_REG_FINGER_NUM 0x02
#define CST816_REG_XH         0x03
#define CST816_REG_XL         0x04
#define CST816_REG_YH         0x05
#define CST816_REG_YL         0x06
#define CST816_REG_CHIP_ID    0xA7

#define TOUCH_PORTRAIT_W  170U
#define TOUCH_PORTRAIT_H  320U
#define TOUCH_LANDSCAPE_W 320U
#define TOUCH_LANDSCAPE_H 170U

static bool s_inited = false;
// static int64_t s_last_log_us = 0; // Uncomment if you want to log touch coordinates every 250ms when pressed

static inline uint16_t clamp_u16(uint16_t v, uint16_t maxv)
{
    return (v > maxv) ? maxv : v;
}

static esp_err_t cst816_read_regs(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_write_read_device(
        CONFIG_USER_TOUCH_CST816_I2C_PORT,
        CONFIG_USER_TOUCH_CST816_ADDR,
        &reg,
        1,
        buf,
        len,
        pdMS_TO_TICKS(50));
}

esp_err_t touch_cst816_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }

    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_USER_TOUCH_CST816_SDA_GPIO,
        .scl_io_num = CONFIG_USER_TOUCH_CST816_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = 400000,
        },
        .clk_flags = 0,
    };

    ESP_ERROR_CHECK(i2c_param_config(CONFIG_USER_TOUCH_CST816_I2C_PORT, &cfg));

    esp_err_t ret = i2c_driver_install(
        CONFIG_USER_TOUCH_CST816_I2C_PORT,
        I2C_MODE_MASTER,
        0,
        0,
        0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }

    gpio_config_t rst_cfg = {
        .pin_bit_mask = (1ULL << CONFIG_USER_TOUCH_CST816_RST_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&rst_cfg));

    gpio_config_t int_cfg = {
        .pin_bit_mask = (1ULL << CONFIG_USER_TOUCH_CST816_INT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&int_cfg));

    gpio_set_level(CONFIG_USER_TOUCH_CST816_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CONFIG_USER_TOUCH_CST816_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t chip_id = 0;
    ret = cst816_read_regs(CST816_REG_CHIP_ID, &chip_id, 1);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "probe failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG,
             "ready i2c=%d sda=%d scl=%d addr=0x%02X rst=%d int=%d chip=0x%02X",
             CONFIG_USER_TOUCH_CST816_I2C_PORT,
             CONFIG_USER_TOUCH_CST816_SDA_GPIO,
             CONFIG_USER_TOUCH_CST816_SCL_GPIO,
             CONFIG_USER_TOUCH_CST816_ADDR,
             CONFIG_USER_TOUCH_CST816_RST_GPIO,
             CONFIG_USER_TOUCH_CST816_INT_GPIO,
             chip_id);

    s_inited = true;
    return ESP_OK;
}

esp_err_t touch_cst816_read(touch_cst816_point_t *pt)
{
    if (pt == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_inited) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t b[5] = {0};
    esp_err_t ret = cst816_read_regs(CST816_REG_FINGER_NUM, b, sizeof(b));
    if (ret != ESP_OK) {
        return ret;
    }

    const uint8_t fingers = (uint8_t)(b[0] & 0x0F);
    uint16_t raw_x = (uint16_t)(((uint16_t)(b[1] & 0x0F) << 8) | b[2]);
    uint16_t raw_y = (uint16_t)(((uint16_t)(b[3] & 0x0F) << 8) | b[4]);

    raw_x = clamp_u16(raw_x, (uint16_t)(TOUCH_PORTRAIT_W - 1U));
    raw_y = clamp_u16(raw_y, (uint16_t)(TOUCH_PORTRAIT_H - 1U));

    /* Portrait 170x320 -> Landscape 320x170 (clockwise) */
    const uint16_t map_x = raw_y;
    const uint16_t map_y = (uint16_t)((TOUCH_PORTRAIT_W - 1U) - raw_x);

    pt->pressed = (fingers > 0U);
    pt->raw_x = raw_x;
    pt->raw_y = raw_y;
    pt->x = clamp_u16(map_x, (uint16_t)(TOUCH_LANDSCAPE_W - 1U));
    pt->y = clamp_u16(map_y, (uint16_t)(TOUCH_LANDSCAPE_H - 1U));

    /* LOGGING: Uncomment the following block to log touch coordinates every 250ms when pressed
    if (pt->pressed) {
        const int64_t now = esp_timer_get_time();
        if ((now - s_last_log_us) >= 250000) {
            s_last_log_us = now;
            ESP_LOGI(TAG,
                     "raw=(%u,%u) landscape=(%u,%u)",
                     (unsigned int)pt->raw_x,
                     (unsigned int)pt->raw_y,
                     (unsigned int)pt->x,
                     (unsigned int)pt->y);
        }
    }*/

    return ESP_OK;
}

#else

esp_err_t touch_cst816_init(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t touch_cst816_read(touch_cst816_point_t *pt)
{
    (void)pt;
    return ESP_ERR_NOT_SUPPORTED;
}

#endif
