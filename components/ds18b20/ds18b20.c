#include "ds18b20.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"

static const char *TAG = "bacnet";

#define DS18B20_GPIO GPIO_NUM_18
#define DS18B20_CMD_SKIP_ROM 0xCC
#define DS18B20_CMD_CONVERT_T 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

static bool ds18b20_gpio_ready = false;
static portMUX_TYPE ds18b20_spinlock = portMUX_INITIALIZER_UNLOCKED;

static void ds18b20_delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

static esp_err_t ds18b20_init_gpio(void)
{
    if (ds18b20_gpio_ready) {
        return ESP_OK;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << DS18B20_GPIO,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err == ESP_OK) {
        gpio_set_level(DS18B20_GPIO, 1);
        ds18b20_gpio_ready = true;
    }

    return err;
}

static bool ds18b20_reset(void)
{
    portENTER_CRITICAL(&ds18b20_spinlock);

    gpio_set_level(DS18B20_GPIO, 0);
    ds18b20_delay_us(480);
    gpio_set_level(DS18B20_GPIO, 1);
    ds18b20_delay_us(70);

    bool presence = (gpio_get_level(DS18B20_GPIO) == 0);

    ds18b20_delay_us(410);
    portEXIT_CRITICAL(&ds18b20_spinlock);

    return presence;
}

static void ds18b20_write_bit(uint8_t bit)
{
    portENTER_CRITICAL(&ds18b20_spinlock);

    gpio_set_level(DS18B20_GPIO, 0);

    if (bit) {
        ds18b20_delay_us(6);
        gpio_set_level(DS18B20_GPIO, 1);
        ds18b20_delay_us(64);
    } else {
        ds18b20_delay_us(60);
        gpio_set_level(DS18B20_GPIO, 1);
        ds18b20_delay_us(10);
    }

    portEXIT_CRITICAL(&ds18b20_spinlock);
}

static uint8_t ds18b20_read_bit(void)
{
    uint8_t bit;

    portENTER_CRITICAL(&ds18b20_spinlock);

    gpio_set_level(DS18B20_GPIO, 0);
    ds18b20_delay_us(6);
    gpio_set_level(DS18B20_GPIO, 1);
    ds18b20_delay_us(9);

    bit = (gpio_get_level(DS18B20_GPIO) != 0) ? 1U : 0U;

    ds18b20_delay_us(55);
    portEXIT_CRITICAL(&ds18b20_spinlock);

    return bit;
}

static void ds18b20_write_byte(uint8_t value)
{
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(value & 0x01U);
        value >>= 1;
    }
}

static uint8_t ds18b20_read_byte(void)
{
    uint8_t value = 0;

    for (int i = 0; i < 8; i++) {
        value |= (uint8_t)(ds18b20_read_bit() << i);
    }

    return value;
}

static uint8_t ds18b20_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;

    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];

        for (int bit = 0; bit < 8; bit++) {
            uint8_t mix = (crc ^ byte) & 0x01U;
            crc >>= 1;

            if (mix) {
                crc ^= 0x8CU;
            }

            byte >>= 1;
        }
    }

    return crc;
}

bool ds18b20_read_temperature(float *temperature_c)
{
    uint8_t scratchpad[9] = { 0 };

    if (!temperature_c) {
        return false;
    }

    if (ds18b20_init_gpio() != ESP_OK) {
        return false;
    }

    for (int attempt = 1; attempt <= 3; attempt++) {
        memset(scratchpad, 0, sizeof(scratchpad));

        if (!ds18b20_reset()) {
            ESP_LOGW(
                TAG,
                "DS18B20 not present on GPIO%d (attempt %d/3)",
                (int)DS18B20_GPIO,
                attempt);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        ds18b20_write_byte(DS18B20_CMD_SKIP_ROM);
        ds18b20_write_byte(DS18B20_CMD_CONVERT_T);

        vTaskDelay(pdMS_TO_TICKS(750));

        if (!ds18b20_reset()) {
            ESP_LOGW(
                TAG,
                "DS18B20 presence pulse missing after conversion (attempt %d/3)",
                attempt);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        ds18b20_write_byte(DS18B20_CMD_SKIP_ROM);
        ds18b20_write_byte(DS18B20_CMD_READ_SCRATCHPAD);

        for (size_t i = 0; i < sizeof(scratchpad); i++) {
            scratchpad[i] = ds18b20_read_byte();
        }

        if (ds18b20_crc8(scratchpad, 8) != scratchpad[8]) {
            ESP_LOGW(
                TAG,
                "DS18B20 scratchpad CRC failed (attempt %d/3)",
                attempt);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        int16_t raw =
            (int16_t)(((uint16_t)scratchpad[1] << 8) | scratchpad[0]);

        *temperature_c = (float)raw / 16.0f;
        return true;
    }

    return false;
}
