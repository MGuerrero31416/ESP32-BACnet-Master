#include "sen54_bacnet_config.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "User_Settings.h"
#include "analog_value.h"
#include "app_storage.h"
#include "esp_log.h"
#include "sen54.h"

#include "bacnet/bacenum.h"
#include "bacnet/basic/object/av.h"

static const char *TAG = "sen54_cfg";

typedef struct {
    uint32_t fan_interval_seconds;
    int16_t raw_offset;
    int16_t raw_slope;
    uint16_t time_constant_seconds;
} sen54_raw_config_t;

static inline uint32_t sen54_av1_instance(void)
{
    return user_av_instance(USER_AV_SEN54_FAN_AUTO_CLEAN_INTERVAL);
}

static inline uint32_t sen54_av2_instance(void)
{
    return user_av_instance(USER_AV_SEN54_TEMP_COMP_OFFSET);
}

static inline uint32_t sen54_av3_instance(void)
{
    return user_av_instance(USER_AV_SEN54_TEMP_COMP_SLOPE);
}

static inline uint32_t sen54_av4_instance(void)
{
    return user_av_instance(USER_AV_SEN54_TEMP_COMP_TIME_CONSTANT);
}

static inline uint32_t ds18b20_av5_instance(void)
{
    return user_av_instance(USER_AV_DS18B20_TEMP_OFFSET);
}

static float raw_offset_to_c(int16_t raw)
{
    return (float)raw / 200.0f;
}

static float raw_slope_to_ratio(int16_t raw)
{
    return (float)raw / 10000.0f;
}

static bool parse_av1_fan_interval(float value, uint32_t *seconds)
{
    if (!isfinite(value) || value < 0.0f || value > 4294967295.0f) {
        return false;
    }

    uint32_t whole = (uint32_t)value;
    if ((float)whole != value) {
        return false;
    }

    *seconds = whole;
    return true;
}

static bool parse_av2_offset_raw(float value, int16_t *raw)
{
    if (!isfinite(value)) {
        return false;
    }

    float scaled = value * 200.0f;
    long rounded = lroundf(scaled);
    if (rounded < INT16_MIN || rounded > INT16_MAX) {
        return false;
    }

    *raw = (int16_t)rounded;
    return true;
}

static bool parse_av3_slope_raw(float value, int16_t *raw)
{
    if (!isfinite(value)) {
        return false;
    }

    float scaled = value * 10000.0f;
    long rounded = lroundf(scaled);
    if (rounded < INT16_MIN || rounded > INT16_MAX) {
        return false;
    }

    *raw = (int16_t)rounded;
    return true;
}

static bool parse_av4_time_constant(float value, uint16_t *seconds)
{
    if (!isfinite(value) || value < 0.0f || value > 65535.0f) {
        return false;
    }

    uint32_t whole = (uint32_t)value;
    if ((float)whole != value) {
        return false;
    }

    *seconds = (uint16_t)whole;
    return true;
}

static bool parse_av5_ds18b20_offset(float value)
{
    return isfinite(value);
}

static esp_err_t read_sensor_config(sen54_raw_config_t *config)
{
    esp_err_t err = sen54_get_fan_auto_cleaning_interval_seconds(
        &config->fan_interval_seconds);
    if (err != ESP_OK) {
        return err;
    }

    return sen54_get_temperature_offset_parameters_raw(
        &config->raw_offset,
        &config->raw_slope,
        &config->time_constant_seconds);
}

static void publish_raw_config_to_bacnet(const sen54_raw_config_t *config)
{
    (void)Analog_Value_Present_Value_Set(
        sen54_av1_instance(),
        (float)config->fan_interval_seconds,
        16);

    (void)Analog_Value_Present_Value_Set(
        sen54_av2_instance(),
        raw_offset_to_c(config->raw_offset),
        16);

    (void)Analog_Value_Present_Value_Set(
        sen54_av3_instance(),
        raw_slope_to_ratio(config->raw_slope),
        16);

    (void)Analog_Value_Present_Value_Set(
        sen54_av4_instance(),
        (float)config->time_constant_seconds,
        16);
}

static void persist_raw_config_to_nvs(const sen54_raw_config_t *config)
{
    bacnet_nvs_save_av_pv(
        sen54_av1_instance(),
        (float)config->fan_interval_seconds);

    bacnet_nvs_save_av_pv(
        sen54_av2_instance(),
        raw_offset_to_c(config->raw_offset));

    bacnet_nvs_save_av_pv(
        sen54_av3_instance(),
        raw_slope_to_ratio(config->raw_slope));

    bacnet_nvs_save_av_pv(
        sen54_av4_instance(),
        (float)config->time_constant_seconds);
}

static esp_err_t apply_and_verify_raw_config(sen54_raw_config_t *config)
{
    esp_err_t err = sen54_set_fan_auto_cleaning_interval_seconds(
        config->fan_interval_seconds);
    if (err != ESP_OK) {
        return err;
    }

    err = sen54_set_temperature_offset_parameters_raw(
        config->raw_offset,
        config->raw_slope,
        config->time_constant_seconds);
    if (err != ESP_OK) {
        return err;
    }

    return read_sensor_config(config);
}

static bool load_saved_config_if_complete(sen54_raw_config_t *config)
{
    bool found = false;
    float av_value = 0.0f;

    if (bacnet_nvs_load_av_pv(sen54_av1_instance(), &av_value, &found) != ESP_OK ||
        !found || !parse_av1_fan_interval(av_value, &config->fan_interval_seconds)) {
        return false;
    }

    if (bacnet_nvs_load_av_pv(sen54_av2_instance(), &av_value, &found) != ESP_OK ||
        !found || !parse_av2_offset_raw(av_value, &config->raw_offset)) {
        return false;
    }

    if (bacnet_nvs_load_av_pv(sen54_av3_instance(), &av_value, &found) != ESP_OK ||
        !found || !parse_av3_slope_raw(av_value, &config->raw_slope)) {
        return false;
    }

    if (bacnet_nvs_load_av_pv(sen54_av4_instance(), &av_value, &found) != ESP_OK ||
        !found || !parse_av4_time_constant(av_value, &config->time_constant_seconds)) {
        return false;
    }

    return true;
}

static bool sen54_av_write_request_callback(
    uint32_t object_instance,
    float requested_value,
    float *applied_value,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    if (object_instance == ds18b20_av5_instance()) {
        if (!parse_av5_ds18b20_offset(requested_value)) {
            if (error_class) {
                *error_class = ERROR_CLASS_PROPERTY;
            }
            if (error_code) {
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
            return false;
        }

        if (applied_value) {
            *applied_value = requested_value;
        }

        return true;
    }

    if (object_instance != sen54_av1_instance() &&
        object_instance != sen54_av2_instance() &&
        object_instance != sen54_av3_instance() &&
        object_instance != sen54_av4_instance()) {
        return true;
    }

    if (error_class) {
        *error_class = ERROR_CLASS_PROPERTY;
    }
    if (error_code) {
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
    }

    if (!Analog_Value_Valid_Instance(sen54_av1_instance()) ||
        !Analog_Value_Valid_Instance(sen54_av2_instance()) ||
        !Analog_Value_Valid_Instance(sen54_av3_instance()) ||
        !Analog_Value_Valid_Instance(sen54_av4_instance())) {
        if (error_class) {
            *error_class = ERROR_CLASS_OBJECT;
        }
        if (error_code) {
            *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        }
        return false;
    }

    if (object_instance == sen54_av1_instance()) {
        uint32_t requested_seconds = 0;
        if (!parse_av1_fan_interval(requested_value, &requested_seconds)) {
            return false;
        }

        if (sen54_set_fan_auto_cleaning_interval_seconds(requested_seconds) != ESP_OK) {
            if (error_class) {
                *error_class = ERROR_CLASS_DEVICE;
            }
            if (error_code) {
                *error_code = ERROR_CODE_OPERATIONAL_PROBLEM;
            }
            return false;
        }

        uint32_t verified_seconds = 0;
        if (sen54_get_fan_auto_cleaning_interval_seconds(&verified_seconds) != ESP_OK) {
            if (error_class) {
                *error_class = ERROR_CLASS_DEVICE;
            }
            if (error_code) {
                *error_code = ERROR_CODE_OPERATIONAL_PROBLEM;
            }
            return false;
        }

        (void)Analog_Value_Present_Value_Set(
            sen54_av1_instance(),
            (float)verified_seconds,
            16);

        bacnet_nvs_save_av_pv(
            sen54_av1_instance(),
            (float)verified_seconds);

        if (applied_value) {
            *applied_value = (float)verified_seconds;
        }

        return true;
    }

    sen54_raw_config_t current = {0};
    if (read_sensor_config(&current) != ESP_OK) {
        if (error_class) {
            *error_class = ERROR_CLASS_DEVICE;
        }
        if (error_code) {
            *error_code = ERROR_CODE_OPERATIONAL_PROBLEM;
        }
        return false;
    }

    if (object_instance == sen54_av2_instance()) {
        if (!parse_av2_offset_raw(requested_value, &current.raw_offset)) {
            return false;
        }
    } else if (object_instance == sen54_av3_instance()) {
        if (!parse_av3_slope_raw(requested_value, &current.raw_slope)) {
            return false;
        }
    } else {
        if (!parse_av4_time_constant(requested_value, &current.time_constant_seconds)) {
            return false;
        }
    }

    if (sen54_set_temperature_offset_parameters_raw(
            current.raw_offset,
            current.raw_slope,
            current.time_constant_seconds) != ESP_OK) {
        if (error_class) {
            *error_class = ERROR_CLASS_DEVICE;
        }
        if (error_code) {
            *error_code = ERROR_CODE_OPERATIONAL_PROBLEM;
        }
        return false;
    }

    if (sen54_get_temperature_offset_parameters_raw(
            &current.raw_offset,
            &current.raw_slope,
            &current.time_constant_seconds) != ESP_OK) {
        if (error_class) {
            *error_class = ERROR_CLASS_DEVICE;
        }
        if (error_code) {
            *error_code = ERROR_CODE_OPERATIONAL_PROBLEM;
        }
        return false;
    }

    (void)Analog_Value_Present_Value_Set(
        sen54_av2_instance(),
        raw_offset_to_c(current.raw_offset),
        16);
    (void)Analog_Value_Present_Value_Set(
        sen54_av3_instance(),
        raw_slope_to_ratio(current.raw_slope),
        16);
    (void)Analog_Value_Present_Value_Set(
        sen54_av4_instance(),
        (float)current.time_constant_seconds,
        16);

    bacnet_nvs_save_av_pv(
        sen54_av2_instance(),
        raw_offset_to_c(current.raw_offset));
    bacnet_nvs_save_av_pv(
        sen54_av3_instance(),
        raw_slope_to_ratio(current.raw_slope));
    bacnet_nvs_save_av_pv(
        sen54_av4_instance(),
        (float)current.time_constant_seconds);

    if (applied_value) {
        if (object_instance == sen54_av2_instance()) {
            *applied_value = raw_offset_to_c(current.raw_offset);
        } else if (object_instance == sen54_av3_instance()) {
            *applied_value = raw_slope_to_ratio(current.raw_slope);
        } else {
            *applied_value = (float)current.time_constant_seconds;
        }
    }

    return true;
}

void sen54_bacnet_config_register_callback(void)
{
    Analog_Value_Write_Present_Value_Request_Callback_Set(
        sen54_av_write_request_callback);
}

esp_err_t sen54_bacnet_config_startup_sync(void)
{
    sen54_raw_config_t sensor_values = {0};
    esp_err_t err = read_sensor_config(&sensor_values);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read SEN54 startup configuration");
        return err;
    }

    sen54_raw_config_t target = sensor_values;
    bool have_saved_values = false;

    if (!app_storage_override_enabled()) {
        have_saved_values = load_saved_config_if_complete(&target);
    }

    if (have_saved_values) {
        err = apply_and_verify_raw_config(&target);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to reapply saved SEN54 configuration");
            return err;
        }
    } else {
        target = sensor_values;
    }

    publish_raw_config_to_bacnet(&target);
    persist_raw_config_to_nvs(&target);

    ESP_LOGI(
        TAG,
        "SEN54 config synced: fan=%lu offset=%.3f slope=%.5f tc=%u",
        (unsigned long)target.fan_interval_seconds,
        raw_offset_to_c(target.raw_offset),
        raw_slope_to_ratio(target.raw_slope),
        (unsigned)target.time_constant_seconds);

    return ESP_OK;
}

esp_err_t sen54_bacnet_config_reapply_saved(void)
{
    sen54_raw_config_t target = {0};

    if (!load_saved_config_if_complete(&target)) {
        return sen54_bacnet_config_startup_sync();
    }

    esp_err_t err = apply_and_verify_raw_config(&target);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reapply saved SEN54 configuration");
        return err;
    }

    publish_raw_config_to_bacnet(&target);
    persist_raw_config_to_nvs(&target);

    return ESP_OK;
}
