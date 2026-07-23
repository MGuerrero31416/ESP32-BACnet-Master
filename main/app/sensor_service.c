#include "sensor_service.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "User_Settings.h"
#include "ds18b20.h"
#include "sen54_bacnet_config.h"
#include "sen54.h"

/* BACnet-stack headers */
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/bv.h"

#define SENSOR_ACQUISITION_INTERVAL_MS 10000U
#define SEN54_RESET_RECOVERY_DELAY_MS  10000U

static const char *TAG = "sensor_service";
/*
 * DS18B20 moving-average filter.
 *
 * Four samples provide light smoothing without making the displayed
 * temperature respond too slowly.
 */
#define DS18B20_AVERAGE_SAMPLE_COUNT 4U //for light filtering. Change it to 8U only if the value still appears too active;

typedef struct {
    float samples[DS18B20_AVERAGE_SAMPLE_COUNT];
    float sum;
    size_t next_sample;
    size_t sample_count;
} ds18b20_average_filter_t;

/*
 * Add one valid DS18B20 measurement to the rolling average.
 *
 * During startup, the average uses only the samples collected so far,
 * so AI8 can be updated immediately without waiting for the buffer
 * to become completely full.
 */
static float ds18b20_average_update(
    ds18b20_average_filter_t *filter,
    float new_sample)
{
    if (filter->sample_count < DS18B20_AVERAGE_SAMPLE_COUNT) {
        filter->samples[filter->next_sample] = new_sample;
        filter->sum += new_sample;
        filter->sample_count++;
    } else {
        filter->sum -= filter->samples[filter->next_sample];
        filter->samples[filter->next_sample] = new_sample;
        filter->sum += new_sample;
    }

    filter->next_sample =
        (filter->next_sample + 1U) % DS18B20_AVERAGE_SAMPLE_COUNT;

    return filter->sum / (float)filter->sample_count;
}

/*
 * Logical positions in USER_AI_INSTANCES[].
 *
 * These values are array indexes, not BACnet object instance numbers.
 * The actual BACnet instances are read from USER_AI_INSTANCES[].
 */
typedef enum {
    SENSOR_AI_SEN54_TEMPERATURE = 0,
    SENSOR_AI_SEN54_HUMIDITY,
    SENSOR_AI_SEN54_VOC_INDEX,
    SENSOR_AI_SEN54_PM1_0,
    SENSOR_AI_SEN54_PM2_5,
    SENSOR_AI_SEN54_PM4_0,
    SENSOR_AI_SEN54_PM10,
    SENSOR_AI_DS18B20_TEMPERATURE
} sensor_ai_role_t;


/*
 * Logical positions in USER_BV_INSTANCES[].
 */
typedef enum {
    SENSOR_BV_SEN54_FULL_RESET = 0
} sensor_bv_role_t;


/*
 * Ensure User_Settings contains enough configured objects for the
 * sensor mappings used by this service.
 */
_Static_assert(
    USER_AI_COUNT > SENSOR_AI_DS18B20_TEMPERATURE,
    "USER_AI_COUNT must provide eight sensor Analog Inputs");

_Static_assert(
    USER_BV_COUNT > SENSOR_BV_SEN54_FULL_RESET,
    "USER_BV_COUNT must provide the SEN54 reset Binary Value");


/*
 * Return the configured BACnet Analog Input instance associated with
 * a logical sensor role.
 */
static uint32_t sensor_ai_instance(
    sensor_ai_role_t role)
{
    return USER_AI_INSTANCES[(size_t)role];
}


/*
 * Return the configured BACnet Binary Value instance associated with
 * a logical sensor-control role.
 */
static uint32_t sensor_bv_instance(
    sensor_bv_role_t role)
{
    return USER_BV_INSTANCES[(size_t)role];
}


/*
 * Apply the same reliability state to all seven SEN54 Analog Inputs.
 */
static void sensor_service_set_sen54_reliability(
    BACNET_RELIABILITY reliability)
{
    static const sensor_ai_role_t sen54_roles[] = {
        SENSOR_AI_SEN54_TEMPERATURE,
        SENSOR_AI_SEN54_HUMIDITY,
        SENSOR_AI_SEN54_VOC_INDEX,
        SENSOR_AI_SEN54_PM1_0,
        SENSOR_AI_SEN54_PM2_5,
        SENSOR_AI_SEN54_PM4_0,
        SENSOR_AI_SEN54_PM10
    };

    const size_t role_count =
        sizeof(sen54_roles) /
        sizeof(sen54_roles[0]);

    for (size_t i = 0; i < role_count; i++) {
        Analog_Input_Reliability_Set(
            sensor_ai_instance(sen54_roles[i]),
            reliability);
    }
}


/*
 * Sensor acquisition task.
 *
 * BACnet mapping is controlled by the object instance arrays in
 * User_Settings.c:
 *
 * USER_AI_INSTANCES[0] = SEN54 temperature
 * USER_AI_INSTANCES[1] = SEN54 relative humidity
 * USER_AI_INSTANCES[2] = SEN54 VOC index
 * USER_AI_INSTANCES[3] = SEN54 PM1.0
 * USER_AI_INSTANCES[4] = SEN54 PM2.5
 * USER_AI_INSTANCES[5] = SEN54 PM4.0
 * USER_AI_INSTANCES[6] = SEN54 PM10
 * USER_AI_INSTANCES[7] = DS18B20 temperature
 *
 * USER_BV_INSTANCES[0] = SEN54 full-reset command
 */
static void sensor_service_task(void *parameter)
{
    (void)parameter;

    float raw_ds18b20_temperature = 0.0f;
    ds18b20_average_filter_t ds18b20_filter = { 0 };
    sen54_data_t sensor_data;

    const uint32_t sen54_reset_bv =
        sensor_bv_instance(
            SENSOR_BV_SEN54_FULL_RESET);

    const uint32_t ds18b20_ai =
        sensor_ai_instance(
            SENSOR_AI_DS18B20_TEMPERATURE);

    const uint32_t ds18b20_offset_av =
        user_av_instance(
            USER_AV_DS18B20_TEMP_OFFSET);

    ESP_LOGI(TAG, "Sensor service task started");
    sen54_bacnet_config_register_callback();
    ESP_LOGI(TAG, "Initializing SEN54");

    sen54_init();

    if (sen54_bacnet_config_startup_sync() != ESP_OK) {
        ESP_LOGW(TAG, "SEN54 BACnet startup sync failed");
    }

    /*
     * Allow the SEN54 fan and measurement chamber to stabilize
     * before requesting the first measurement.
     */
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(TAG, "Sensor acquisition started");

    for (;;) {
        /*
         * Writing ACTIVE to the configured reset BV triggers a full
         * SEN54 reset. The BV returns automatically to INACTIVE.
         */
        if (Binary_Value_Present_Value(
                sen54_reset_bv) == BINARY_ACTIVE) {
            ESP_LOGI(
                TAG,
                "BV%lu ACTIVE: sending SEN54 full reset",
                (unsigned long)sen54_reset_bv);

            esp_err_t err = sen54_full_reset();

            ESP_LOGI(
                TAG,
                "SEN54 full reset %s",
                err == ESP_OK ? "OK" : "FAILED");

            if (err == ESP_OK &&
                sen54_bacnet_config_reapply_saved() != ESP_OK) {
                ESP_LOGW(
                    TAG,
                    "SEN54 configuration reapply after reset failed");
            }

            Binary_Value_Present_Value_Set(
                sen54_reset_bv,
                BINARY_INACTIVE);

            /*
             * Give the sensor time to recover before the next
             * normal measurement.
             */
            vTaskDelay(pdMS_TO_TICKS(SEN54_RESET_RECOVERY_DELAY_MS)); // Wait before the next normal acquisition cycle.
            continue;
        }

        /*
         * Read the SEN54 and update its configured BACnet AIs.
         */
        if (sen54_read(&sensor_data)) {
            Analog_Input_Present_Value_Set(
                sensor_ai_instance(
                    SENSOR_AI_SEN54_TEMPERATURE),
                sensor_data.temperature);

            Analog_Input_Present_Value_Set(
                sensor_ai_instance(
                    SENSOR_AI_SEN54_HUMIDITY),
                sensor_data.humidity);

            Analog_Input_Present_Value_Set(
                sensor_ai_instance(
                    SENSOR_AI_SEN54_VOC_INDEX),
                sensor_data.voc_index);

            Analog_Input_Present_Value_Set(
                sensor_ai_instance(
                    SENSOR_AI_SEN54_PM1_0),
                sensor_data.pm1_0);

            Analog_Input_Present_Value_Set(
                sensor_ai_instance(
                    SENSOR_AI_SEN54_PM2_5),
                sensor_data.pm2_5);

            Analog_Input_Present_Value_Set(
                sensor_ai_instance(
                    SENSOR_AI_SEN54_PM4_0),
                sensor_data.pm4_0);

            Analog_Input_Present_Value_Set(
                sensor_ai_instance(
                    SENSOR_AI_SEN54_PM10),
                sensor_data.pm10);

            sensor_service_set_sen54_reliability(
                RELIABILITY_NO_FAULT_DETECTED);

            /*
             * Debug-level measurement log. It remains hidden when
             * the normal ESP-IDF log level is INFO.
             */
            ESP_LOGD(
                TAG,
                "SEN54: T=%.2f C RH=%.2f %% VOC=%.1f "
                "PM1=%.1f PM2.5=%.1f PM4=%.1f PM10=%.1f",
                sensor_data.temperature,
                sensor_data.humidity,
                sensor_data.voc_index,
                sensor_data.pm1_0,
                sensor_data.pm2_5,
                sensor_data.pm4_0,
                sensor_data.pm10);
        } else {
            sensor_service_set_sen54_reliability(
                RELIABILITY_UNRELIABLE_OTHER);

            ESP_LOGW(TAG, "SEN54 read failed");
        }

        /*
        * Read the DS18B20, smooth the raw measurement, apply the
        * user-configurable calibration offset, and update BACnet AI8.
        */
        if (ds18b20_read_temperature(
                &raw_ds18b20_temperature)) {
            float filtered_ds18b20_temperature =
                ds18b20_average_update(
                    &ds18b20_filter,
                    raw_ds18b20_temperature);

            float ds18b20_offset =
                Analog_Value_Present_Value(ds18b20_offset_av);

            if (!isfinite(ds18b20_offset)) {
                ds18b20_offset = 0.0f;
            }

            /*
            * Apply the offset after averaging so that BACnet changes to the
            * offset take effect immediately instead of being slowly averaged.
            */
            float corrected_ds_temperature =
                filtered_ds18b20_temperature + ds18b20_offset;

            Analog_Input_Present_Value_Set(
                ds18b20_ai,
                corrected_ds_temperature);

            Analog_Input_Reliability_Set(
                ds18b20_ai,
                RELIABILITY_NO_FAULT_DETECTED);

            ESP_LOGD(
                TAG,
                "DS18B20: raw=%.3f C average=%.3f C "
                "offset=%.2f C corrected=%.3f C",
                raw_ds18b20_temperature,
                filtered_ds18b20_temperature,
                ds18b20_offset,
                corrected_ds_temperature);
        } else {
            /*
            * Do not add a failed reading to the moving average.
            * Retain the last valid AI value and mark it unreliable.
            */
            Analog_Input_Reliability_Set(
                ds18b20_ai,
                RELIABILITY_UNRELIABLE_OTHER);

            ESP_LOGW(TAG, "DS18B20 read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_ACQUISITION_INTERVAL_MS)); // Wait before the next normal acquisition cycle.
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