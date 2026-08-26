#include "display.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "u8g2.h"

#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#define TAG "display_lora_gateway"
#define OLED_ADDRESS 0x3C
#define OLED_I2C_FREQUENCY_HZ 400000
#define OLED_SDA GPIO_NUM_17
#define OLED_SCL GPIO_NUM_18
#define OLED_RESET GPIO_NUM_21
#define OLED_VEXT GPIO_NUM_36

static i2c_master_bus_handle_t display_bus;
static i2c_master_dev_handle_t display_device;
static u8g2_t display;
static bool display_ready;
static uint32_t received_count;

static uint8_t u8g2_i2c_callback(
    u8x8_t *u8x8,
    uint8_t message,
    uint8_t argument,
    void *data)
{
    static uint8_t buffer[132];
    static size_t buffer_length;
    (void)u8x8;

    switch (message) {
    case U8X8_MSG_BYTE_INIT: {
        const i2c_device_config_t device_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = OLED_ADDRESS,
            .scl_speed_hz = OLED_I2C_FREQUENCY_HZ,
        };
        return i2c_master_bus_add_device(
                   display_bus,
                   &device_config,
                   &display_device) == ESP_OK;
    }
    case U8X8_MSG_BYTE_START_TRANSFER:
        buffer_length = 0;
        break;
    case U8X8_MSG_BYTE_SEND:
        if (buffer_length + argument > sizeof(buffer)) {
            return 0;
        }
        for (size_t index = 0; index < argument; index++) {
            buffer[buffer_length++] = ((const uint8_t *)data)[index];
        }
        break;
    case U8X8_MSG_BYTE_END_TRANSFER:
        return i2c_master_transmit(display_device, buffer, buffer_length, -1) == ESP_OK;
    default:
        break;
    }
    return 1;
}

static uint8_t u8g2_gpio_delay_callback(
    u8x8_t *u8x8,
    uint8_t message,
    uint8_t argument,
    void *data)
{
    (void)u8x8;
    (void)data;

    switch (message) {
    case U8X8_MSG_DELAY_MILLI:
        vTaskDelay(pdMS_TO_TICKS(argument));
        break;
    case U8X8_MSG_DELAY_10MICRO:
        esp_rom_delay_us(argument * 10);
        break;
    case U8X8_MSG_DELAY_100NANO:
        __asm__ __volatile__("nop");
        break;
    case U8X8_MSG_DELAY_I2C:
        esp_rom_delay_us(argument);
        break;
    case U8X8_MSG_GPIO_RESET:
        gpio_set_level(OLED_RESET, argument);
        break;
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        break;
    default:
        break;
    }
    return 1;
}

static void display_send_buffer(void)
{
    u8g2_SendBuffer(&display);
}

static void display_draw_values(float voc, float pm25)
{
    char line[24];

    u8g2_ClearBuffer(&display);

    // --- Line 1: Header ---
    u8g2_SetFont(&display, u8g2_font_6x13B_tr); // Set to BOLD for header
    u8g2_DrawStr(&display, 0, 13, "RECEIVER");

    // Draw a visual separator line directly under the header cell (at Y = 15)
    u8g2_DrawLine(&display, 0, 15, 127, 15);

    // --- Lines 2-4: Data Items ---
    u8g2_SetFont(&display, u8g2_font_6x13_tr);  // Switch back to regular font

    // Line 2 (Data Line 1)
    (void)snprintf(line, sizeof(line), "Received: %" PRIu32, received_count);
    u8g2_DrawStr(&display, 0, 31, line);

    // Line 3 (Data Line 2)
    (void)snprintf(line, sizeof(line), "VOC: %.0f", (double)voc);
    u8g2_DrawStr(&display, 0, 47, line);

    // Line 4 (Data Line 3)
    (void)snprintf(line, sizeof(line), "PM2.5: %.0f", (double)pm25);
    u8g2_DrawStr(&display, 0, 63, line);

    display_send_buffer();
}


void display_init(void)
{
    const gpio_config_t output = {
        .pin_bit_mask = (1ULL << OLED_VEXT) | (1ULL << OLED_RESET),
        .mode = GPIO_MODE_OUTPUT,
    };
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = OLED_SDA,
        .scl_io_num = OLED_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    if (gpio_config(&output) != ESP_OK) {
        ESP_LOGE(TAG, "OLED GPIO setup failed; continuing without display");
        return;
    }
    gpio_set_level(OLED_VEXT, 0);
    gpio_set_level(OLED_RESET, 1);

    if (i2c_new_master_bus(&bus_config, &display_bus) != ESP_OK) {
        ESP_LOGE(TAG, "OLED I2C setup failed; continuing without display");
        return;
    }

    u8g2_Setup_ssd1315_i2c_128x64_noname_f(
        &display,
        U8G2_R0,
        u8g2_i2c_callback,
        u8g2_gpio_delay_callback);
    u8g2_SetI2CAddress(&display, OLED_ADDRESS * 2);
    u8g2_InitDisplay(&display);
    u8g2_SetPowerSave(&display, 0);
    u8g2_SetContrast(&display, 140);
    display_ready = true;
    display_draw_values(0.0f, 0.0f);
}

void display_update_values(
    float pm25,
    float temperature,
    float humidity,
    float voc,
    float temp_ds18b20)
{
    (void)pm25;
    (void)temperature;
    (void)humidity;
    (void)voc;
    (void)temp_ds18b20;
}

void display_set_link_status(bool wifi_connected, bool mstp_connected)
{
    (void)wifi_connected;
    (void)mstp_connected;
}

void lora_gateway_display_update(float voc, float pm25)
{
    if (!display_ready) {
        return;
    }

    received_count++;
    display_draw_values(voc, pm25);
}
