#include "touch_cst816.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "CST816";

static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t dev_handle = NULL;
static i2c_master_dev_handle_t wake_dev_handle = NULL; // Special handle for wake-up
static bool s_inited = false;
static TickType_t s_next_idle_probe_tick = 0;

#define CST816_IDLE_PROBE_MS 30

static inline uint16_t clamp_u16(uint16_t v, uint16_t maxv) {
    return (v > maxv) ? maxv : v;
}

esp_err_t touch_cst816_init(void)
{
    if (s_inited) return ESP_OK;

    esp_err_t ret;
    int port = CONFIG_USER_TOUCH_CST816_I2C_PORT;

    // 1. Get or create the I2C bus
    ret = i2c_master_get_bus_handle(port, &bus_handle);
    if (ret == ESP_ERR_INVALID_STATE || ret == ESP_ERR_NOT_FOUND) {
        i2c_master_bus_config_t bus_config = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = port,
            .sda_io_num = CONFIG_USER_TOUCH_CST816_SDA_GPIO,
            .scl_io_num = CONFIG_USER_TOUCH_CST816_SCL_GPIO,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        ret = i2c_new_master_bus(&bus_config, &bus_handle);
        if (ret != ESP_OK) return ret;
    } else if (ret != ESP_OK) return ret;

    // 2. Add normal CST816 device (100kHz, ACK check enabled)
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_USER_TOUCH_CST816_ADDR,
        .scl_speed_hz = 100000, 
    };
    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) return ret;

    // 3. Add WAKE-UP device (100kHz, ACK check DISABLED)
    // This is the magic bullet: it forces the driver to send the STOP condition 
    // even if the sleeping chip NACKs, which wakes the chip up.
    i2c_device_config_t wake_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_USER_TOUCH_CST816_ADDR,
        .scl_speed_hz = 100000,
        .flags.disable_ack_check = true, 
    };
    ret = i2c_master_bus_add_device(bus_handle, &wake_dev_cfg, &wake_dev_handle);
    if (ret != ESP_OK) return ret;

    // 4. Configure RST and INT pins
    gpio_config_t rst_cfg = {
        .pin_bit_mask = (1ULL << CONFIG_USER_TOUCH_CST816_RST_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&rst_cfg);

    gpio_config_t int_cfg = {
        .pin_bit_mask = (1ULL << CONFIG_USER_TOUCH_CST816_INT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&int_cfg);

    // 5. Hardware Reset Sequence
    gpio_set_level(CONFIG_USER_TOUCH_CST816_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CONFIG_USER_TOUCH_CST816_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // 6. Read Chip ID to verify communication
    uint8_t reg = 0xA7;
    uint8_t chip_id = 0;
    ret = i2c_master_transmit_receive(dev_handle, &reg, 1, &chip_id, 1, 100);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ready chip=0x%02X", chip_id);
        s_inited = true;
        return ESP_OK;
    }
    return ret;
}

esp_err_t touch_cst816_read(touch_cst816_point_t *pt)
{
    if (!s_inited || dev_handle == NULL || pt == NULL) return ESP_ERR_INVALID_STATE;

    pt->pressed = false;

    const bool int_active = (gpio_get_level(CONFIG_USER_TOUCH_CST816_INT_GPIO) == 0);
    const TickType_t now = xTaskGetTickCount();

    if (!int_active && (int32_t)(now - s_next_idle_probe_tick) < 0) {
        return ESP_OK;
    }

    s_next_idle_probe_tick = now + pdMS_TO_TICKS(CST816_IDLE_PROBE_MS);

    // WAKE-UP PING: Send a command using the special handle that ignores NACKs.
    // This forces the I2C bus to send a STOP condition, waking up the sleeping CST816.
    uint8_t wake_reg = 0x01;
    i2c_master_transmit(wake_dev_handle, &wake_reg, 1, 50);
    
    // Give the chip a few milliseconds to fully wake up from the STOP condition
    vTaskDelay(pdMS_TO_TICKS(5)); 

    // Now read the actual touch data using the normal handle (which expects ACKs)
    uint8_t reg = 0x02;
    uint8_t rx_buf[5] = {0};

    esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg, 1, rx_buf, 5, 100);

    
    if (ret != ESP_OK) {
        return ret;
    }

    // Parse the data
    const uint8_t fingers = rx_buf[0] & 0x0F;
    uint16_t raw_x = ((rx_buf[1] & 0x0F) << 8) | rx_buf[2];
    uint16_t raw_y = ((rx_buf[3] & 0x0F) << 8) | rx_buf[4];

    raw_x = clamp_u16(raw_x, 169);
    raw_y = clamp_u16(raw_y, 319);

    // Rotate to landscape
    const uint16_t map_x = raw_y;
    const uint16_t map_y = (170 - 1) - raw_x;

    pt->pressed = (fingers > 0);
    pt->raw_x = raw_x;
    pt->raw_y = raw_y;
    pt->x = clamp_u16(map_x, 319);
    pt->y = clamp_u16(map_y, 169);

    return ESP_OK;
}