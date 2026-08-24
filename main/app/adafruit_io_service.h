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
 * - PM2.5
 * - VOC Index
 * - Temperature
 * - Relative humidity
 *
 * Measurements are published to separate Adafruit IO feeds derived from
 * USER_AIO_FEED_KEY.
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