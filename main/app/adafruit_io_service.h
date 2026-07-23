#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the Adafruit IO MQTT publishing service.
 *
 * The service publishes the current SEN54:
 *
 * - Temperature
 * - Relative humidity
 * - PM2.5
 * - VOC Index
 *
 * Measurements are published every 10 seconds to the configured
 * Adafruit IO feed.
 *
 * @param task_handle Receives the publishing task handle.
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if task_handle is NULL
 *     - ESP_ERR_INVALID_STATE if the credentials are invalid
 *     - ESP_ERR_NO_MEM if resources cannot be created
 *     - ESP_FAIL if MQTT initialization fails
 */
esp_err_t adafruit_io_service_start(TaskHandle_t *task_handle);

/**
 * @brief Check whether MQTT is connected to Adafruit IO.
 *
 * @return true when connected; otherwise false.
 */
bool adafruit_io_service_is_connected(void);

#ifdef __cplusplus
}
#endif