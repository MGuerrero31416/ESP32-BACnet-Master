#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sensor_service_start(TaskHandle_t *task_handle);

#ifdef __cplusplus
}
#endif