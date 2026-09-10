//Standard C library
#include <stddef.h>

//ESP-IDF error handling and logging
#include "esp_err.h"
#include "esp_log.h"

// FreeRTOS task types
// Required for TaskHandle_t variables used to track the BACnet and sensor tasks
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Application services
#include "app_storage.h"         /* Initialize NVS and persistence policy */
#include "app_supervisor.h"      /* Run periodic display/status maintenance */
#include "bacnet_app.h"          /* Initialize and start the BACnet runtime */
#include "display.h"             /* Initialize the physical display */
#include "lora_gateway.h"        /* Start LoRa receiver/gateway task */
#if !defined(CONFIG_USER_DISPLAY_LORA_GATEWAY) || \
    !CONFIG_USER_DISPLAY_LORA_GATEWAY
#include "sensor_service.h"      /* Start SEN54 and DS18B20 measurements */
#endif
#include "stack_profiler.h"      /* Monitor FreeRTOS task stack usage */
#include "adafruit_io_service.h" /* Publish sensor data to Adafruit IO */

#include "User_Settings.h"

static const char *TAG = "bacnet";

static TaskHandle_t bacnet_rx_task_handle = NULL;
static TaskHandle_t bacnet_mstp_rx_task_handle = NULL;
static TaskHandle_t bacnet_core_task_handle = NULL;
static TaskHandle_t bacnet_cov_task_handle = NULL;
#if !defined(CONFIG_USER_DISPLAY_LORA_GATEWAY) || \
    !CONFIG_USER_DISPLAY_LORA_GATEWAY
static TaskHandle_t sen54_task_handle = NULL;
#endif
static TaskHandle_t lora_gateway_task_handle = NULL;
static TaskHandle_t adafruit_io_task_handle = NULL;

void app_main(void)
{
    // Bus recovery re-reserves pins it already owns, which the IDF gpio
    // reservation tracker can't tell apart from a real conflict; these two
    // tags only ever log that false positive at W, so keep errors visible
    // and drop the noise.
    esp_log_level_set("gpio_reserve", ESP_LOG_ERROR);
    esp_log_level_set("i2c.common", ESP_LOG_ERROR);

    ESP_ERROR_CHECK(app_storage_init());
    User_Settings_Print();

    const stack_profiler_task_handle_refs_t profiler_task_handles = {
        .bacnet_rx = &bacnet_rx_task_handle,
        .bacnet_mstp_rx = &bacnet_mstp_rx_task_handle,
        .bacnet_core = &bacnet_core_task_handle,
        .bacnet_cov = &bacnet_cov_task_handle,
    #if !defined(CONFIG_USER_DISPLAY_LORA_GATEWAY) || \
        !CONFIG_USER_DISPLAY_LORA_GATEWAY
        .sensor = &sen54_task_handle,
    #else
        .sensor = NULL,
    #endif
    };

    stack_profiler_init(&profiler_task_handles);

    ESP_ERROR_CHECK(
        bacnet_app_init(
            stack_profiler_bacnet_callback));

    ESP_LOGI(TAG, "Initializing display");
    display_init();
  

    /* Remaining task startup and supervisor loop */

    bacnet_app_task_handle_refs_t task_handles = {  // Provide task-handle references to the BACnet runtime
        .bip_rx = &bacnet_rx_task_handle,           // Receive task for BACnet/IP
        .mstp_rx = &bacnet_mstp_rx_task_handle,     // Receive task for BACnet MS/TP
        .core = &bacnet_core_task_handle,           // Core BACnet task for processing messages and events
        .cov = &bacnet_cov_task_handle,             // Change-of-Value (COV) notification task for BACnet
    };

    ESP_ERROR_CHECK(
        bacnet_app_start(&task_handles));           // Start BACnet runtime tasks

#if defined(CONFIG_USER_DISPLAY_LORA_GATEWAY) && \
    CONFIG_USER_DISPLAY_LORA_GATEWAY
    ESP_ERROR_CHECK(
        lora_gateway_start(&lora_gateway_task_handle));
#else
    ESP_ERROR_CHECK(
        sensor_service_start(&sen54_task_handle));  // Start SEN54 and DS18B20 sensor acquisition task
#endif

    if (USER_ENABLE_ADAFRUIT_IO) {
        ESP_LOGI(
            TAG,
            "Adafruit IO publishing enabled");

        ESP_ERROR_CHECK(
            adafruit_io_service_start(
                &adafruit_io_task_handle));
    } else {
        ESP_LOGI(
            TAG,
            "Adafruit IO publishing disabled");
    }

    app_supervisor_run();
}