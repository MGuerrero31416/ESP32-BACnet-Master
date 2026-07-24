#include "sen54_sensor_service.h"

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "User_Settings.h"
#include "sen54.h"
#include "sen54_bacnet_config.h"

/* BACnet-stack headers */
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bv.h"

static const char *TAG = "sen54_service";

/* SEN5x device-status flag bit indexes (Sensirion reference driver). */
#define SEN54_STATUS_FAN_ERROR_BIT   4U
#define SEN54_STATUS_LASER_ERROR_BIT 5U
#define SEN54_STATUS_SHT_ERROR_BIT   6U
#define SEN54_STATUS_SGP_ERROR_BIT   7U

/*
 * Ensure User_Settings contains all BACnet objects required
 * by the SEN54 service.
 */
_Static_assert(
    USER_AI_COUNT > USER_AI_SEN54_PM10,
    "USER_AI_COUNT must provide seven SEN54 Analog Inputs");

_Static_assert(
    USER_BV_COUNT > USER_BV_SEN54_CLEAR_STATUS,
    "USER_BV_COUNT must provide four SEN54 Binary Values");

_Static_assert(
    USER_BI_COUNT > USER_BI_SEN54_RHT_SENSOR_ERROR,
    "USER_BI_COUNT must provide four SEN54 Binary Inputs");

static uint32_t sen54_read_fail_count = 0;
static uint32_t sen54_status_fail_count = 0;
static bool measurement_state_known = false;
static bool measurement_enabled = true;

static bool sen54_status_flag_is_set(
    uint32_t status,
    uint32_t bit_index)
{
    return (status & (1UL << bit_index)) != 0UL;
}

static void sen54_update_diagnostic_binary_inputs(
    uint32_t status)
{
    Binary_Input_Present_Value_Set(
        user_bi_instance(USER_BI_SEN54_FAN_FAILURE),
        sen54_status_flag_is_set(status, SEN54_STATUS_FAN_ERROR_BIT) ?
            BINARY_ACTIVE :
            BINARY_INACTIVE);

    Binary_Input_Present_Value_Set(
        user_bi_instance(USER_BI_SEN54_LASER_ERROR),
        sen54_status_flag_is_set(status, SEN54_STATUS_LASER_ERROR_BIT) ?
            BINARY_ACTIVE :
            BINARY_INACTIVE);

    Binary_Input_Present_Value_Set(
        user_bi_instance(USER_BI_SEN54_VOC_SENSOR_ERROR),
        sen54_status_flag_is_set(status, SEN54_STATUS_SGP_ERROR_BIT) ?
            BINARY_ACTIVE :
            BINARY_INACTIVE);

    Binary_Input_Present_Value_Set(
        user_bi_instance(USER_BI_SEN54_RHT_SENSOR_ERROR),
        sen54_status_flag_is_set(status, SEN54_STATUS_SHT_ERROR_BIT) ?
            BINARY_ACTIVE :
            BINARY_INACTIVE);
}

static void sen54_poll_and_publish_status(void)
{
    uint32_t status = 0;
    esp_err_t status_err = sen54_read_device_status(&status);

    if (status_err != ESP_OK) {
        sen54_status_fail_count++;

        if (sen54_status_fail_count == 1 ||
            (sen54_status_fail_count % 12U) == 0U) {
            ESP_LOGW(
                TAG,
                "SEN54 status read failed (%lu): %s",
                (unsigned long)sen54_status_fail_count,
                esp_err_to_name(status_err));
        }

        return;
    }

    if (sen54_status_fail_count > 0U) {
        ESP_LOGI(TAG, "SEN54 status read recovered");
        sen54_status_fail_count = 0U;
    }

    sen54_update_diagnostic_binary_inputs(status);
}

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
    const uint32_t measurement_bv =
        user_bv_instance(USER_BV_SEN54_MEASUREMENT_ENABLE);
    const uint32_t fan_clean_bv =
        user_bv_instance(USER_BV_SEN54_START_FAN_CLEANING);
    const uint32_t clear_status_bv =
        user_bv_instance(USER_BV_SEN54_CLEAR_STATUS);

    if (!measurement_state_known) {
        measurement_enabled = true;
        measurement_state_known = true;
    }

    bool measurement_requested =
        Binary_Value_Present_Value(measurement_bv) ==
        BINARY_ACTIVE;

    if (measurement_requested != measurement_enabled) {
        esp_err_t err =
            sen54_set_measurement_enabled(measurement_requested);

        if (err == ESP_OK) {
            measurement_enabled = measurement_requested;

            ESP_LOGI(
                TAG,
                "SEN54 measurement %s via BV%lu",
                measurement_enabled ? "enabled" : "disabled",
                (unsigned long)measurement_bv);
        } else {
            ESP_LOGW(
                TAG,
                "SEN54 measurement %s failed: %s",
                measurement_requested ? "enable" : "disable",
                esp_err_to_name(err));
        }
    }

    /*
     * Writing ACTIVE to the configured BV triggers a full
     * SEN54 reset/recovery sequence. The BV automatically
     * returns to INACTIVE.
     */
    if (Binary_Value_Present_Value(reset_bv) ==
        BINARY_ACTIVE) {

        ESP_LOGI(
            TAG,
            "BV%lu ACTIVE: sending SEN54 full reset",
            (unsigned long)reset_bv);

        measurement_enabled = false;

        esp_err_t reset_err = sen54_full_reset();
        if (reset_err != ESP_OK) {
            ESP_LOGW(
                TAG,
                "SEN54 full reset failed: %s",
                esp_err_to_name(reset_err));
        }

        esp_err_t config_err = ESP_FAIL;
        if (reset_err == ESP_OK) {
            config_err = sen54_bacnet_config_reapply_saved();

            if (config_err != ESP_OK) {
                ESP_LOGW(
                    TAG,
                    "SEN54 configuration reapply failed: %s",
                    esp_err_to_name(config_err));
            }
        }

        esp_err_t restart_err = ESP_FAIL;
        if (reset_err == ESP_OK && config_err == ESP_OK) {
            restart_err = sen54_start_measurement();

            if (restart_err != ESP_OK) {
                ESP_LOGW(
                    TAG,
                    "SEN54 measurement restart failed: %s",
                    esp_err_to_name(restart_err));
            } else {
                /* Allow start-measurement processing to complete. */
                vTaskDelay(pdMS_TO_TICKS(100));
                measurement_enabled = true;
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

    if (Binary_Value_Present_Value(fan_clean_bv) ==
        BINARY_ACTIVE) {

        ESP_LOGI(
            TAG,
            "BV%lu ACTIVE: starting SEN54 fan cleaning",
            (unsigned long)fan_clean_bv);

        esp_err_t err = sen54_start_fan_cleaning();

        if (err != ESP_OK) {
            ESP_LOGW(
                TAG,
                "SEN54 fan cleaning start failed: %s",
                esp_err_to_name(err));
        }

        Binary_Value_Present_Value_Set(
            fan_clean_bv,
            BINARY_INACTIVE);
    }

    if (Binary_Value_Present_Value(clear_status_bv) ==
        BINARY_ACTIVE) {

        uint32_t cleared_status = 0;

        ESP_LOGI(
            TAG,
            "BV%lu ACTIVE: clearing SEN54 device status flags",
            (unsigned long)clear_status_bv);

        esp_err_t err =
            sen54_read_and_clear_device_status(
                &cleared_status);

        if (err != ESP_OK) {
            ESP_LOGW(
                TAG,
                "SEN54 clear status failed: %s",
                esp_err_to_name(err));
        }

        Binary_Value_Present_Value_Set(
            clear_status_bv,
            BINARY_INACTIVE);
    }

    sen54_poll_and_publish_status();

    if (!measurement_enabled) {
        return SEN54_SENSOR_CYCLE_COMPLETE;
    }

    sen54_data_t sensor_data = { 0 };

    if (!sen54_read(&sensor_data)) {
        sen54_set_reliability(
            RELIABILITY_UNRELIABLE_OTHER);

        sen54_read_fail_count++;

        if (sen54_read_fail_count == 1 ||
            (sen54_read_fail_count % 12U) == 0U) {
            ESP_LOGW(
                TAG,
                "SEN54 read failed (%lu)",
                (unsigned long)sen54_read_fail_count);
        }

        return SEN54_SENSOR_CYCLE_COMPLETE;
    }

    if (sen54_read_fail_count > 0U) {
        ESP_LOGI(TAG, "SEN54 measurement read recovered");
        sen54_read_fail_count = 0U;
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