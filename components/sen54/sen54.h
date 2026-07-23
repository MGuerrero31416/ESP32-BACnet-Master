#ifndef SEN54_H
#define SEN54_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// I2C configuration - adjust pins to match your hardware
#define SEN54_I2C_PORT    0          // I2C_NUM_0
#define SEN54_I2C_SDA_PIN 4
#define SEN54_I2C_SCL_PIN 5
#define SEN54_I2C_ADDR    0x69
#define SEN54_I2C_FREQ_HZ 100000

// Sensor data structure
// Values are -1.0f when the sensor reports them as not available
typedef struct {
    float pm1_0;        // PM1.0 concentration (μg/m³)
    float pm2_5;        // PM2.5 concentration (μg/m³)
    float pm4_0;        // PM4.0 concentration (μg/m³)
    float pm10;         // PM10 concentration (μg/m³)
    float humidity;     // Relative humidity (%RH)
    float temperature;  // Temperature (°C)
    float voc_index;    // VOC Index (1–500, dimensionless)
    float nox_index;    // NOx Index (1–500, dimensionless)
} sen54_data_t;

/**
 * @brief Initialize the SEN54 sensor over I2C and start continuous measurement.
 */
void sen54_init(void);

/**
 * @brief Read the latest measured values from the sensor.
 * @param data Pointer to sen54_data_t to fill.
 * @return true if the read succeeded and CRCs passed, false otherwise.
 */
bool sen54_read(sen54_data_t *data);

/**
 * @brief Start a background FreeRTOS task that reads the sensor periodically.
 * @param interval_ms Polling interval in milliseconds (minimum 1000).
 */
void sen54_start_task(uint32_t interval_ms);

/* Thread-safe getters for individual measurements */
float sen54_get_pm1_0(void);
float sen54_get_pm2_5(void);
float sen54_get_pm4_0(void);
float sen54_get_pm10(void);
float sen54_get_humidity(void);
float sen54_get_temperature(void);
float sen54_get_voc_index(void);
float sen54_get_nox_index(void);

/**
 * @brief Copy the latest sensor data (thread-safe).
 * @param data Pointer to sen54_data_t to fill.
 */
void sen54_get_data(sen54_data_t *data);

/**
 * @brief Send a full reset command (I2C 0xD304) to the SEN54 and restart measurement.
 *
 * Issues the Device Reset command defined in the SEN54 datasheet §3.2.
 * This resets all internal sensor state — including learned VOC/NOx algorithm
 * baselines — and is equivalent to a power-cycle. The function waits ~1.2 s
 * for the sensor to complete its start-up sequence, then sends Start
 * Measurement (0x0021) so that readings resume automatically.
 *
 * Triggered by BACnet BV1 (SEN54_Full_Reset) being written ACTIVE.
 *
 * @return ESP_OK on success, or an esp_err_t code on I2C failure.
 */
esp_err_t sen54_full_reset(void);

/* Official SEN5x configuration wrappers (thread-safe). */
esp_err_t sen54_get_fan_auto_cleaning_interval_seconds(uint32_t *seconds);
esp_err_t sen54_set_fan_auto_cleaning_interval_seconds(uint32_t seconds);
esp_err_t sen54_get_temperature_offset_parameters_raw(
    int16_t *raw_offset,
    int16_t *raw_slope,
    uint16_t *time_constant_seconds);
esp_err_t sen54_set_temperature_offset_parameters_raw(
    int16_t raw_offset,
    int16_t raw_slope,
    uint16_t time_constant_seconds);

/* Shared I2C transaction guard used by SEN54 and Sensirion HAL. */
esp_err_t sen54_i2c_transaction_begin(void);
void sen54_i2c_transaction_end(void);

/* I2C bridge consumed by Sensirion HAL. */
esp_err_t sen54_i2c_bridge_write(
    uint8_t address,
    const uint8_t *data,
    size_t length);
esp_err_t sen54_i2c_bridge_read(
    uint8_t address,
    uint8_t *data,
    size_t length);

#ifdef __cplusplus
}
#endif

#endif // SEN54_H
