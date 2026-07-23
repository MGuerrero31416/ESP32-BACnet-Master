#include "sen54_sensor_service.h"

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"

#include "User_Settings.h"
#include "sen54.h"
#include "sen54_bacnet_config.h"

/* BACnet-stack headers */
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/bv.h"

static const char *TAG = "sen54_service";

/*
 * Ensure User_Settings contains all BACnet objects required
 * by the SEN54 service.
 */
_Static_assert(
    USER_AI_COUNT > USER_AI_SEN54_PM10,
    "USER_AI_COUNT must provide seven SEN54 Analog Inputs");

_Static_assert(
    USER_BV_COUNT > USER_BV_SEN54_FULL_RESET,
    "USER_BV_COUNT must provide the SEN54 reset Binary Value");

/*
 * Apply the same reliability state to all seven SEN54
 * BACnet Analog Inputs.
 */
static void sen54_set_reliability(
    BACNET_RELIABILITY reliability)
{
    static const user_ai_role_t sen54_ai_roles[] = {
        USER_AI_SEN54_TEMPERATURE,
        USER_AI_SEN54_HUMIDITY,
        USER_AI_SEN54_VOC_INDEX,
        USER_AI_SEN54_PM1_0,
        USER_AI_SEN54_PM2_5,
        USER_AI_SEN54_PM4_0,
        USER_AI_SEN54_PM10
    };

    const size_t role_count =
        sizeof(sen54_ai_roles) /
        sizeof(sen54_ai_roles[0]);

    for (size_t i = 0; i < role_count; i++) {
        Analog_Input_Reliability_Set(
            user_ai_instance(sen54_ai_roles[i]),
            reliability);
    }
}

/*
 * Publish one successful SEN54 measurement to the configured
 * BACnet Analog Inputs.
 */
static void sen54_publish_measurement(
    const sen54_data_t *data)
{
    Analog_Input_Present_Value_Set(
        user_ai_instance(USER_AI_SEN54_TEMPERATURE),
        data->temperature);

    Analog_Input_Present_Value_Set(
        user_ai_instance(USER_AI_SEN54_HUMIDITY),
        data->humidity);

    Analog_Input_Present_Value_Set(
        user_ai_instance(USER_AI_SEN54_VOC_INDEX),
        data->voc_index);

    Analog_Input_Present_Value_Set(
        user_ai_instance(USER_AI_SEN54_PM1_0),
        data->pm1_0);

    Analog_Input_Present_Value_Set(
        user_ai_instance(USER_AI_SEN54_PM2_5),
        data->pm2_5);

    Analog_Input_Present_Value_Set(
        user_ai_instance(USER_AI_SEN54_PM4_0),
        data->pm4_0);

    Analog_Input_Present_Value_Set(
        user_ai_instance(USER_AI_SEN54_PM10),
        data->pm10);
}

esp_err_t sen54_sensor_service_init(void)
{
    ESP_LOGI(TAG, "Initializing SEN54 service");

    /*
     * Register the Analog Value write callback before
     * synchronizing BACnet-controlled SEN54 settings.
     */
    sen54_bacnet_config_register_callback();

    /*
     * The current low-level SEN54 initialization function
     * returns void and starts continuous measurement.
     */
    sen54_init();

    esp_err_t err = sen54_bacnet_config_startup_sync();

    if (err != ESP_OK) {
        ESP_LOGW(
            TAG,
            "SEN54 BACnet startup sync failed: %s",
            esp_err_to_name(err));

        return err;
    }

    ESP_LOGI(TAG, "SEN54 service initialized");

    return ESP_OK;
}

sen54_sensor_cycle_result_t
sen54_sensor_service_cycle(void)
{
    const uint32_t reset_bv =
        user_bv_instance(USER_BV_SEN54_FULL_RESET);

    /*
     * Writing ACTIVE to the configured BV triggers a full
     * SEN54 reset. The BV automatically returns to INACTIVE.
     */
    if (Binary_Value_Present_Value(reset_bv) ==
        BINARY_ACTIVE) {

        ESP_LOGI(
            TAG,
            "BV%lu ACTIVE: sending SEN54 full reset",
            (unsigned long)reset_bv);

        esp_err_t err = sen54_full_reset();

        ESP_LOGI(
            TAG,
            "SEN54 full reset %s",
            err == ESP_OK ? "OK" : "FAILED");

        if (err == ESP_OK) {
            esp_err_t config_err =
                sen54_bacnet_config_reapply_saved();

            if (config_err != ESP_OK) {
                ESP_LOGW(
                    TAG,
                    "SEN54 configuration reapply failed: %s",
                    esp_err_to_name(config_err));
            }
        }

        Binary_Value_Present_Value_Set(
            reset_bv,
            BINARY_INACTIVE);

        /*
         * The common sensor task must apply the reset recovery
         * delay and skip the remainder of this acquisition cycle.
         */
        return SEN54_SENSOR_RESET_HANDLED;
    }

    sen54_data_t sensor_data = { 0 };

    if (!sen54_read(&sensor_data)) {
        sen54_set_reliability(
            RELIABILITY_UNRELIABLE_OTHER);

        ESP_LOGW(TAG, "SEN54 read failed");

        return SEN54_SENSOR_CYCLE_COMPLETE;
    }

    sen54_publish_measurement(&sensor_data);

    sen54_set_reliability(
        RELIABILITY_NO_FAULT_DETECTED);

    /*
     * This remains hidden when the normal ESP-IDF log level
     * is INFO.
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

    return SEN54_SENSOR_CYCLE_COMPLETE;
}