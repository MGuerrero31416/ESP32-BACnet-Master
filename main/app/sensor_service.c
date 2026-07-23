#include "sensor_service.h"

#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sensors/sen54_sensor_service.h"

#if defined(CONFIG_USER_SENSOR_DS18B20) && \
    CONFIG_USER_SENSOR_DS18B20

#include "sensors/ds18b20_sensor_service.h"

#endif

#define SENSOR_ACQUISITION_INTERVAL_MS 10000U
#define SEN54_STARTUP_DELAY_MS          5000U
#define SEN54_RESET_RECOVERY_DELAY_MS  10000U

static const char *TAG = "sensor_service";

/*
 * Common sensor acquisition task.
 *
 * Sensor-specific initialization, acquisition, filtering,
 * BACnet updates and reliability handling are implemented
 * in the individual sensor service modules.
 */
static void sensor_service_task(void *parameter)
{
    (void)parameter;

    ESP_LOGI(TAG, "Sensor service task started");

    /*
     * Initialize the SEN54 service.
     *
     * This includes:
     * - BACnet configuration callback registration;
     * - SEN54 hardware initialization;
     * - startup synchronization of saved settings.
     */
    esp_err_t err = sen54_sensor_service_init();

    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "SEN54 service initialization failed: %s",
            esp_err_to_name(err));
    }

#if defined(CONFIG_USER_SENSOR_DS18B20) && \
    CONFIG_USER_SENSOR_DS18B20

    /*
     * Initialize the DS18B20 application service and
     * moving-average state.
     */
    err = ds18b20_sensor_service_init();

    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "DS18B20 service initialization failed: %s",
            esp_err_to_name(err));
    }

#else

    ESP_LOGI(
        TAG,
        "DS18B20 disabled by hardware profile");

#endif

    /*
     * Preserve the original startup delay so the SEN54 fan
     * and measurement chamber can stabilize before the first
     * measurement.
     */
    vTaskDelay(
        pdMS_TO_TICKS(
            SEN54_STARTUP_DELAY_MS));

    ESP_LOGI(TAG, "Sensor acquisition started");

    for (;;) {
        /*
         * Handle the SEN54 reset command or perform one
         * normal SEN54 acquisition cycle.
         */
        sen54_sensor_cycle_result_t sen54_result =
            sen54_sensor_service_cycle();

        /*
         * Preserve the original reset behavior:
         *
         * - wait for SEN54 recovery;
         * - skip DS18B20 acquisition during this cycle;
         * - restart the loop afterward.
         */
        if (sen54_result ==
            SEN54_SENSOR_RESET_HANDLED) {

            vTaskDelay(
                pdMS_TO_TICKS(
                    SEN54_RESET_RECOVERY_DELAY_MS));

            continue;
        }

#if defined(CONFIG_USER_SENSOR_DS18B20) && \
    CONFIG_USER_SENSOR_DS18B20

        /*
         * Read, average and calibrate the DS18B20 value,
         * then update its BACnet Analog Input.
         */
        ds18b20_sensor_service_cycle();

#endif

        /*
         * Wait before the next normal acquisition cycle.
         */
        vTaskDelay(
            pdMS_TO_TICKS(
                SENSOR_ACQUISITION_INTERVAL_MS));
    }
}

esp_err_t sensor_service_start(
    TaskHandle_t *task_handle)
{
    if (task_handle == NULL) {
        ESP_LOGE(
            TAG,
            "Sensor task-handle reference is NULL");

        return ESP_ERR_INVALID_ARG;
    }

    BaseType_t result = xTaskCreate(
        sensor_service_task,
        "sensors",
        4096,
        NULL,
        3,
        task_handle);

    if (result != pdPASS) {
        *task_handle = NULL;

        ESP_LOGE(
            TAG,
            "Failed to create sensor task");

        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}