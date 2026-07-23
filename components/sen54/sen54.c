#include "sen54.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sensirion/sen5x_i2c.h"
#include <string.h>

static const char *TAG = "SEN54";
static bool i2c_ready = false;

// SEN54 I2C commands (big-endian 16-bit)
// 0x0021: Start Measurement (enables fan and continuous measurements)
// 0x0104: Stop Measurement
// 0x03C4: Read Measured Values
// 0xD304: Device Reset (hardware reset, clears all learned state)
#define SEN54_CMD_START_MEASUREMENT  0x0021
#define SEN54_CMD_STOP_MEASUREMENT   0x0104
#define SEN54_CMD_READ_VALUES        0x03C4
// Device Reset (0xD304): forces a full hardware reset of the SEN54.
// All internal state — including the VOC/NOx algorithm learned baselines —
// is cleared. The sensor re-runs its start-up sequence (~1 s) and requires
// a new Start Measurement command (0x2103) before readings resume.
// Equivalent to a power-cycle. See SEN54 datasheet §3.2 "Device Reset".
#define SEN54_CMD_RESET              0xD304

// Read Measured Values returns 8 words × 3 bytes (2 data + 1 CRC) = 24 bytes
#define SEN54_READ_VALUES_LEN  24

static sen54_data_t current_data = {
    .pm1_0 = -1.0f, .pm2_5 = -1.0f, .pm4_0 = -1.0f, .pm10 = -1.0f,
    .humidity = -1.0f, .temperature = -1.0f, .voc_index = -1.0f, .nox_index = -1.0f
};
static SemaphoreHandle_t sen54_mutex = NULL;
static SemaphoreHandle_t sen54_i2c_mutex = NULL;

static esp_err_t sen54_ensure_i2c_mutex(void)
{
    if (sen54_i2c_mutex == NULL) {
        sen54_i2c_mutex = xSemaphoreCreateMutex();
        if (sen54_i2c_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create SEN54 I2C mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    return ESP_OK;
}

// ---------------------------------------------------------------------------
// CRC-8 (polynomial 0x31, init 0xFF) used by all Sensirion sensors
// ---------------------------------------------------------------------------
static uint8_t sen54_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
        }
    }
    return crc;
}

// ---------------------------------------------------------------------------
// Low-level I2C helpers
// ---------------------------------------------------------------------------
static esp_err_t sen54_write_cmd(uint16_t cmd)
{
    uint8_t buf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
    return sen54_i2c_bridge_write(
        SEN54_I2C_ADDR,
        buf,
        sizeof(buf));
}

static esp_err_t sen54_read_bytes(uint8_t *buf, size_t len)
{
    return sen54_i2c_bridge_read(
        SEN54_I2C_ADDR,
        buf,
        len);
}

esp_err_t sen54_i2c_bridge_write(
    uint8_t address,
    const uint8_t *data,
    size_t length)
{
    if (data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_write_to_device(
        SEN54_I2C_PORT,
        address,
        data,
        length,
        pdMS_TO_TICKS(100));
}

esp_err_t sen54_i2c_bridge_read(
    uint8_t address,
    uint8_t *data,
    size_t length)
{
    if (data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_read_from_device(
        SEN54_I2C_PORT,
        address,
        data,
        length,
        pdMS_TO_TICKS(100));
}

esp_err_t sen54_i2c_transaction_begin(void)
{
    esp_err_t err = sen54_ensure_i2c_mutex();
    if (err != ESP_OK) {
        return err;
    }

    if (xSemaphoreTake(sen54_i2c_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

void sen54_i2c_transaction_end(void)
{
    if (sen54_i2c_mutex != NULL) {
        xSemaphoreGive(sen54_i2c_mutex);
    }
}

// ---------------------------------------------------------------------------
// Parse a word (2 bytes) from a 3-byte CRC-word group, validating CRC.
// Returns false if CRC fails.
// ---------------------------------------------------------------------------
static bool sen54_parse_word(const uint8_t *p, uint16_t *out)
{
    if (sen54_crc8(p, 2) != p[2]) {
        return false;
    }
    *out = ((uint16_t)p[0] << 8) | p[1];
    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void sen54_init(void)
{
    if (sen54_ensure_i2c_mutex() != ESP_OK) {
        return;
    }

    i2c_config_t cfg = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = SEN54_I2C_SDA_PIN,
        .scl_io_num       = SEN54_I2C_SCL_PIN,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = SEN54_I2C_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(SEN54_I2C_PORT, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return;
    }

    if (!i2c_ready) {
        err = i2c_driver_install(SEN54_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
        if (err == ESP_ERR_INVALID_STATE) {
            /* Driver already installed on this port by another component */
            err = ESP_OK;
        }
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
            return;
        }
        i2c_ready = true;
    }

    // SEN54 needs up to 1 second after power-on before it will ACK on I2C
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Retry start measurement — cold power-up can take several seconds.
    err = ESP_FAIL;
    for (int attempt = 1; attempt <= 20; attempt++) {
        if (sen54_i2c_transaction_begin() == ESP_OK) {
            err = sen54_write_cmd(SEN54_CMD_START_MEASUREMENT);
            sen54_i2c_transaction_end();
        }

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "SEN54 initialized (attempt %d), measurement started", attempt);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        }
        ESP_LOGW(TAG, "Start measurement attempt %d/20 failed: %s — retrying in 500 ms",
                 attempt, esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SEN54 not responding at I2C addr 0x%02X (SDA=GPIO%d, SCL=GPIO%d). "
                 "Check wiring and pull-up resistors.",
                 SEN54_I2C_ADDR, SEN54_I2C_SDA_PIN, SEN54_I2C_SCL_PIN);

        /* Recovery: try a full device-reset sequence and then retry start. */
        ESP_LOGI(TAG, "Attempting SEN54 recovery reset and restart sequence");
        esp_err_t r = sen54_full_reset();
        if (r == ESP_OK) {
            ESP_LOGI(TAG, "SEN54 recovery reset sequence completed (full reset sent)");
        } else {
            ESP_LOGW(TAG, "SEN54 recovery reset failed (%s)", esp_err_to_name(r));
        }

        /* After reset attempt, try a few more start attempts in case sensor needs
         * extra time to become responsive. */
        for (int attempt = 1; attempt <= 3; attempt++) {
            esp_err_t err2 = ESP_FAIL;
            if (sen54_i2c_transaction_begin() == ESP_OK) {
                err2 = sen54_write_cmd(SEN54_CMD_START_MEASUREMENT);
                sen54_i2c_transaction_end();
            }
            if (err2 == ESP_OK) {
                ESP_LOGI(TAG, "SEN54 measurement started after recovery (attempt %d)", attempt);
                err = ESP_OK;
                break;
            }
            ESP_LOGW(TAG, "Recovery start attempt %d/3 failed: %s", attempt, esp_err_to_name(err2));
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

bool sen54_read(sen54_data_t *data)
{
    if (!data) {
        return false;
    }

    esp_err_t txn = sen54_i2c_transaction_begin();
    if (txn != ESP_OK) {
        return false;
    }

    // Issue the Read Measured Values command
    esp_err_t err = sen54_write_cmd(SEN54_CMD_READ_VALUES);
    if (err != ESP_OK) {
        // If startup was missed after a cold boot, restart measurement and retry once.
        if (sen54_write_cmd(SEN54_CMD_START_MEASUREMENT) == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(100));
            err = sen54_write_cmd(SEN54_CMD_READ_VALUES);
        }
        if (err != ESP_OK) {
            sen54_i2c_transaction_end();
            return false;
        }
    }

    // Sensor needs a short time to prepare the response
    vTaskDelay(pdMS_TO_TICKS(5));

    uint8_t buf[SEN54_READ_VALUES_LEN];
    err = sen54_read_bytes(buf, sizeof(buf));
    sen54_i2c_transaction_end();
    if (err != ESP_OK) {
        return false;
    }

    // Each word occupies 3 bytes: byte[0], byte[1], CRC
    uint16_t raw_pm1_0, raw_pm2_5, raw_pm4_0, raw_pm10;
    uint16_t raw_hum, raw_temp, raw_voc, raw_nox;

    if (!sen54_parse_word(&buf[0],  &raw_pm1_0) ||
        !sen54_parse_word(&buf[3],  &raw_pm2_5) ||
        !sen54_parse_word(&buf[6],  &raw_pm4_0) ||
        !sen54_parse_word(&buf[9],  &raw_pm10)  ||
        !sen54_parse_word(&buf[12], &raw_hum)   ||
        !sen54_parse_word(&buf[15], &raw_temp)  ||
        !sen54_parse_word(&buf[18], &raw_voc)   ||
        !sen54_parse_word(&buf[21], &raw_nox)) {
        ESP_LOGW(TAG, "CRC error in measurement response");
        return false;
    }

    // Apply scaling factors per SEN54 datasheet
    data->pm1_0        = (raw_pm1_0 == 0xFFFF) ? -1.0f : (float)raw_pm1_0 / 10.0f;
    data->pm2_5        = (raw_pm2_5 == 0xFFFF) ? -1.0f : (float)raw_pm2_5 / 10.0f;
    data->pm4_0        = (raw_pm4_0 == 0xFFFF) ? -1.0f : (float)raw_pm4_0 / 10.0f;
    data->pm10         = (raw_pm10  == 0xFFFF) ? -1.0f : (float)raw_pm10  / 10.0f;
    data->humidity     = ((int16_t)raw_hum  == 0x7FFF) ? -1.0f : (float)(int16_t)raw_hum  / 100.0f;
    data->temperature  = ((int16_t)raw_temp == 0x7FFF) ? -1.0f : (float)(int16_t)raw_temp / 200.0f;
    data->voc_index    = ((int16_t)raw_voc  == 0x7FFF) ? -1.0f : (float)(int16_t)raw_voc  / 10.0f;
    data->nox_index    = ((int16_t)raw_nox  == 0x7FFF) ? -1.0f : (float)(int16_t)raw_nox  / 10.0f;

    return true;
}

static void sen54_read_task(void *pvParameters)
{
    uint32_t interval_ms = (uint32_t)(uintptr_t)pvParameters;
    sen54_data_t data;

    for (;;) {
        if (sen54_read(&data)) {
            if (sen54_mutex && xSemaphoreTake(sen54_mutex, portMAX_DELAY) == pdTRUE) {
                memcpy(&current_data, &data, sizeof(sen54_data_t));
                xSemaphoreGive(sen54_mutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
}

void sen54_start_task(uint32_t interval_ms)
{
    if (interval_ms < 1000) {
        interval_ms = 1000;
    }
    if (!sen54_mutex) {
        sen54_mutex = xSemaphoreCreateMutex();
    }
    xTaskCreate(sen54_read_task, "sen54_task", 4096,
                (void *)(uintptr_t)interval_ms, 5, NULL);
}

// ---------------------------------------------------------------------------
// Thread-safe getters
// ---------------------------------------------------------------------------
#define SEN54_GETTER(field) \
    float value = -1.0f; \
    if (sen54_mutex && xSemaphoreTake(sen54_mutex, portMAX_DELAY) == pdTRUE) { \
        value = current_data.field; \
        xSemaphoreGive(sen54_mutex); \
    } else { \
        value = current_data.field; \
    } \
    return value;

float sen54_get_pm1_0(void)       { SEN54_GETTER(pm1_0) }
float sen54_get_pm2_5(void)       { SEN54_GETTER(pm2_5) }
float sen54_get_pm4_0(void)       { SEN54_GETTER(pm4_0) }
float sen54_get_pm10(void)        { SEN54_GETTER(pm10) }
float sen54_get_humidity(void)    { SEN54_GETTER(humidity) }
float sen54_get_temperature(void) { SEN54_GETTER(temperature) }
float sen54_get_voc_index(void)   { SEN54_GETTER(voc_index) }
float sen54_get_nox_index(void)   { SEN54_GETTER(nox_index) }

esp_err_t sen54_full_reset(void)
{
    // Send I2C command 0xD304 (Device Reset).
    // This clears all sensor state including VOC/NOx algorithm baselines.
    // The sensor needs ~1 s to complete its start-up sequence before it
    // will ACK further commands, after which measurement is restarted.
    esp_err_t ret = sen54_i2c_transaction_begin();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = sen54_write_cmd(SEN54_CMD_RESET);
    sen54_i2c_transaction_end();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SEN54 full reset command failed (%d)", ret);
        return ret;
    }
    ESP_LOGI(TAG, "SEN54 full reset (0xD304) sent");
    vTaskDelay(pdMS_TO_TICKS(1200));  /* datasheet: device ready after ~1 s */
    ret = sen54_i2c_transaction_begin();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = sen54_write_cmd(SEN54_CMD_START_MEASUREMENT);
    sen54_i2c_transaction_end();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SEN54 restart measurement after reset failed (%d)", ret);
    } else {
        ESP_LOGI(TAG, "SEN54 measurement restarted after full reset");
    }
    return ret;
}

esp_err_t sen54_get_fan_auto_cleaning_interval_seconds(uint32_t *seconds)
{
    if (seconds == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = sen54_i2c_transaction_begin();
    if (err != ESP_OK) {
        return err;
    }

    int16_t rc = sen5x_get_fan_auto_cleaning_interval(seconds);
    sen54_i2c_transaction_end();

    if (rc != 0) {
        ESP_LOGW(TAG, "sen5x_get_fan_auto_cleaning_interval failed: %d", rc);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t sen54_set_fan_auto_cleaning_interval_seconds(uint32_t seconds)
{
    esp_err_t err = sen54_i2c_transaction_begin();
    if (err != ESP_OK) {
        return err;
    }

    int16_t rc = sen5x_set_fan_auto_cleaning_interval(seconds);
    sen54_i2c_transaction_end();

    if (rc != 0) {
        ESP_LOGW(TAG, "sen5x_set_fan_auto_cleaning_interval failed: %d", rc);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t sen54_get_temperature_offset_parameters_raw(
    int16_t *raw_offset,
    int16_t *raw_slope,
    uint16_t *time_constant_seconds)
{
    if (raw_offset == NULL ||
        raw_slope == NULL ||
        time_constant_seconds == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = sen54_i2c_transaction_begin();
    if (err != ESP_OK) {
        return err;
    }

    int16_t rc = sen5x_get_temperature_offset_parameters(
        raw_offset,
        raw_slope,
        time_constant_seconds);
    sen54_i2c_transaction_end();

    if (rc != 0) {
        ESP_LOGW(TAG, "sen5x_get_temperature_offset_parameters failed: %d", rc);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t sen54_set_temperature_offset_parameters_raw(
    int16_t raw_offset,
    int16_t raw_slope,
    uint16_t time_constant_seconds)
{
    esp_err_t err = sen54_i2c_transaction_begin();
    if (err != ESP_OK) {
        return err;
    }

    int16_t rc = sen5x_set_temperature_offset_parameters(
        raw_offset,
        raw_slope,
        time_constant_seconds);
    sen54_i2c_transaction_end();

    if (rc != 0) {
        ESP_LOGW(TAG, "sen5x_set_temperature_offset_parameters failed: %d", rc);
        return ESP_FAIL;
    }

    return ESP_OK;
}

void sen54_get_data(sen54_data_t *data)
{
    if (!data) {
        return;
    }
    if (sen54_mutex && xSemaphoreTake(sen54_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(data, &current_data, sizeof(sen54_data_t));
        xSemaphoreGive(sen54_mutex);
    } else {
        memcpy(data, &current_data, sizeof(sen54_data_t));
    }
}
