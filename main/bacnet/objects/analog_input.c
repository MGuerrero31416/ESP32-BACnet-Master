#include "analog_input.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app_storage.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "User_Settings.h"

/* BACnet-stack headers */
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/ai.h"

static const char *TAG = "bacnet_ai";

#define NVS_NAMESPACE "bacnet"

/*
 * ESP-IDF NVS keys may contain a maximum of 15 characters,
 * excluding the terminating null character.
 */
#define AI_NVS_KEY_BUFFER_SIZE 16

#define AI_NAME_MAX_LENGTH 64
#define AI_DESCRIPTION_MAX_LENGTH 128

/*
 * The BACnet stack may retain pointers to names and descriptions.
 * These buffers must therefore remain valid after NVS loading.
 */
static char ai_names[USER_AI_COUNT][AI_NAME_MAX_LENGTH + 1];
static char ai_descriptions[USER_AI_COUNT][AI_DESCRIPTION_MAX_LENGTH + 1];


/*
 * Find the User_Settings array index corresponding to a BACnet
 * Analog Input instance.
 *
 * This supports non-consecutive object instances and avoids assuming
 * that array index always equals instance - 1.
 */
static bool ai_find_config_index(
    uint32_t instance,
    size_t *index)
{
    if (index == NULL) {
        return false;
    }

    for (size_t i = 0; i < USER_AI_COUNT; i++) {
        if (USER_AI_INSTANCES[i] == instance) {
            *index = i;
            return true;
        }
    }

    return false;
}


/*
 * Generate an NVS key and ensure that it fits within the ESP-IDF
 * 15-character key limit.
 */
static bool ai_make_nvs_key(
    char *key,
    size_t key_size,
    uint32_t instance,
    const char *suffix)
{
    if (key == NULL ||
        key_size == 0 ||
        suffix == NULL) {
        return false;
    }

    int written = snprintf(
        key,
        key_size,
        "ai_%lu_%s",
        (unsigned long)instance,
        suffix);

    if (written < 0 ||
        (size_t)written >= key_size) {
        ESP_LOGE(
            TAG,
            "NVS key too long for AI%lu property %s",
            (unsigned long)instance,
            suffix);

        return false;
    }

    return true;
}


/*
 * Save Object_Name.
 */
void bacnet_nvs_save_ai_name(
    uint32_t instance,
    const char *name,
    uint16_t length)
{
    if (name == NULL) {
        ESP_LOGE(
            TAG,
            "Cannot save NULL AI%lu name",
            (unsigned long)instance);
        return;
    }

    char key[AI_NVS_KEY_BUFFER_SIZE];
    char buffer[AI_NAME_MAX_LENGTH + 1] = {0};

    if (!ai_make_nvs_key(
            key,
            sizeof(key),
            instance,
            "name")) {
        return;
    }

    size_t copy_length = length;

    if (copy_length > AI_NAME_MAX_LENGTH) {
        copy_length = AI_NAME_MAX_LENGTH;
    }

    memcpy(buffer, name, copy_length);
    buffer[copy_length] = '\0';

    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open(
        NVS_NAMESPACE,
        NVS_READWRITE,
        &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "NVS open failed for AI%lu name: %s",
            (unsigned long)instance,
            esp_err_to_name(err));
        return;
    }

    err = nvs_set_str(
        nvs_handle,
        key,
        buffer);

    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    if (err == ESP_OK) {
        ESP_LOGI(
            TAG,
            "Saved AI%lu name: %s",
            (unsigned long)instance,
            buffer);
    } else {
        ESP_LOGE(
            TAG,
            "Failed to save AI%lu name: %s",
            (unsigned long)instance,
            esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}


/*
 * Save Description.
 */
void bacnet_nvs_save_ai_desc(
    uint32_t instance,
    const char *description,
    uint16_t length)
{
    if (description == NULL) {
        ESP_LOGE(
            TAG,
            "Cannot save NULL AI%lu description",
            (unsigned long)instance);
        return;
    }

    char key[AI_NVS_KEY_BUFFER_SIZE];
    char buffer[AI_DESCRIPTION_MAX_LENGTH + 1] = {0};

    if (!ai_make_nvs_key(
            key,
            sizeof(key),
            instance,
            "desc")) {
        return;
    }

    size_t copy_length = length;

    if (copy_length > AI_DESCRIPTION_MAX_LENGTH) {
        copy_length = AI_DESCRIPTION_MAX_LENGTH;
    }

    memcpy(buffer, description, copy_length);
    buffer[copy_length] = '\0';

    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open(
        NVS_NAMESPACE,
        NVS_READWRITE,
        &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "NVS open failed for AI%lu description: %s",
            (unsigned long)instance,
            esp_err_to_name(err));
        return;
    }

    err = nvs_set_str(
        nvs_handle,
        key,
        buffer);

    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    if (err == ESP_OK) {
        ESP_LOGI(
            TAG,
            "Saved AI%lu description: %s",
            (unsigned long)instance,
            buffer);
    } else {
        ESP_LOGE(
            TAG,
            "Failed to save AI%lu description: %s",
            (unsigned long)instance,
            esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}


/*
 * Save Present_Value as a float blob.
 */
void bacnet_nvs_save_ai_pv(
    uint32_t instance,
    float value)
{
    char key[AI_NVS_KEY_BUFFER_SIZE];

    if (!ai_make_nvs_key(
            key,
            sizeof(key),
            instance,
            "val")) {
        return;
    }

    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open(
        NVS_NAMESPACE,
        NVS_READWRITE,
        &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "NVS open failed for AI%lu value: %s",
            (unsigned long)instance,
            esp_err_to_name(err));
        return;
    }

    err = nvs_set_blob(
        nvs_handle,
        key,
        &value,
        sizeof(value));

    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    if (err == ESP_OK) {
        ESP_LOGI(
            TAG,
            "Saved AI%lu value: %.3f",
            (unsigned long)instance,
            value);
    } else {
        ESP_LOGE(
            TAG,
            "Failed to save AI%lu value: %s",
            (unsigned long)instance,
            esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}


/*
 * Save Units as a 16-bit unsigned integer.
 */
void bacnet_nvs_save_ai_units(
    uint32_t instance,
    uint16_t units)
{
    char key[AI_NVS_KEY_BUFFER_SIZE];

    if (!ai_make_nvs_key(
            key,
            sizeof(key),
            instance,
            "unit")) {
        return;
    }

    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open(
        NVS_NAMESPACE,
        NVS_READWRITE,
        &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "NVS open failed for AI%lu units: %s",
            (unsigned long)instance,
            esp_err_to_name(err));
        return;
    }

    err = nvs_set_u16(
        nvs_handle,
        key,
        units);

    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    if (err == ESP_OK) {
        ESP_LOGI(
            TAG,
            "Saved AI%lu units: %u",
            (unsigned long)instance,
            (unsigned)units);
    } else {
        ESP_LOGE(
            TAG,
            "Failed to save AI%lu units: %s",
            (unsigned long)instance,
            esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}


/*
 * Save COV_Increment as a float blob.
 */
void bacnet_nvs_save_ai_cov_increment(
    uint32_t instance,
    float cov_increment)
{
    if (!isfinite(cov_increment) ||
        cov_increment < 0.0f) {
        ESP_LOGE(
            TAG,
            "Invalid AI%lu COV increment: %.3f",
            (unsigned long)instance,
            cov_increment);
        return;
    }

    char key[AI_NVS_KEY_BUFFER_SIZE];

    if (!ai_make_nvs_key(
            key,
            sizeof(key),
            instance,
            "cov")) {
        return;
    }

    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open(
        NVS_NAMESPACE,
        NVS_READWRITE,
        &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "NVS open failed for AI%lu COV increment: %s",
            (unsigned long)instance,
            esp_err_to_name(err));
        return;
    }

    err = nvs_set_blob(
        nvs_handle,
        key,
        &cov_increment,
        sizeof(cov_increment));

    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }

    if (err == ESP_OK) {
        ESP_LOGI(
            TAG,
            "Saved AI%lu COV increment: %.3f",
            (unsigned long)instance,
            cov_increment);
    } else {
        ESP_LOGE(
            TAG,
            "Failed to save AI%lu COV increment: %s",
            (unsigned long)instance,
            esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}


/*
 * Restore all persisted AI properties.
 *
 * Defaults from User_Settings are applied before this function is
 * called. Any values found in NVS override those defaults.
 */
void bacnet_nvs_load_ai(uint32_t instance)
{
    size_t config_index;

    if (!ai_find_config_index(
            instance,
            &config_index)) {
        ESP_LOGE(
            TAG,
            "AI%lu is not present in USER_AI_INSTANCES",
            (unsigned long)instance);
        return;
    }

    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_open(
        NVS_NAMESPACE,
        NVS_READONLY,
        &nvs_handle);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return;
    }

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "NVS open failed while loading AI%lu: %s",
            (unsigned long)instance,
            esp_err_to_name(err));
        return;
    }

    char key[AI_NVS_KEY_BUFFER_SIZE];
    size_t length;

    /*
     * Restore Object_Name.
     */
    if (ai_make_nvs_key(
            key,
            sizeof(key),
            instance,
            "name")) {
        length = sizeof(ai_names[config_index]);

        err = nvs_get_str(
            nvs_handle,
            key,
            ai_names[config_index],
            &length);

        if (err == ESP_OK) {
            Analog_Input_Name_Set(
                instance,
                ai_names[config_index]);

            ESP_LOGI(
                TAG,
                "Restored AI%lu name: %s",
                (unsigned long)instance,
                ai_names[config_index]);
        }
    }

    /*
     * Restore Description.
     */
    if (ai_make_nvs_key(
            key,
            sizeof(key),
            instance,
            "desc")) {
        length = sizeof(ai_descriptions[config_index]);

        err = nvs_get_str(
            nvs_handle,
            key,
            ai_descriptions[config_index],
            &length);

        if (err == ESP_OK) {
            Analog_Input_Description_Set(
                instance,
                ai_descriptions[config_index]);

            ESP_LOGI(
                TAG,
                "Restored AI%lu description: %s",
                (unsigned long)instance,
                ai_descriptions[config_index]);
        }
    }

    /*
     * Restore Units.
     */
    if (ai_make_nvs_key(
            key,
            sizeof(key),
            instance,
            "unit")) {
        uint16_t units = 0;

        err = nvs_get_u16(
            nvs_handle,
            key,
            &units);

        if (err == ESP_OK) {
            Analog_Input_Units_Set(
                instance,
                (BACNET_ENGINEERING_UNITS)units);

            ESP_LOGI(
                TAG,
                "Restored AI%lu units: %u",
                (unsigned long)instance,
                (unsigned)units);
        }
    }

    /*
     * Restore COV_Increment.
     */
    if (ai_make_nvs_key(
            key,
            sizeof(key),
            instance,
            "cov")) {
        float cov_increment = 0.0f;

        length = sizeof(cov_increment);

        err = nvs_get_blob(
            nvs_handle,
            key,
            &cov_increment,
            &length);

        if (err == ESP_OK &&
            length == sizeof(cov_increment)) {
            if (isfinite(cov_increment) &&
                cov_increment >= 0.0f) {
                Analog_Input_COV_Increment_Set(
                    instance,
                    cov_increment);

                ESP_LOGI(
                    TAG,
                    "Restored AI%lu COV increment: %.3f",
                    (unsigned long)instance,
                    cov_increment);
            } else {
                ESP_LOGW(
                    TAG,
                    "Ignoring invalid saved AI%lu COV increment",
                    (unsigned long)instance);
            }
        }
    }

    /*
     * Restore Present_Value.
     */
    if (ai_make_nvs_key(
            key,
            sizeof(key),
            instance,
            "val")) {
        float present_value = 0.0f;

        length = sizeof(present_value);

        err = nvs_get_blob(
            nvs_handle,
            key,
            &present_value,
            &length);

        if (err == ESP_OK &&
            length == sizeof(present_value)) {
            Analog_Input_Present_Value_Set(
                instance,
                present_value);

            ESP_LOGI(
                TAG,
                "Restored AI%lu value: %.3f",
                (unsigned long)instance,
                present_value);
        }
    }

    nvs_close(nvs_handle);
}


/*
 * Create and initialize all configured Analog Input objects.
 */
void bacnet_create_analog_inputs(void)
{
    for (size_t i = 0; i < USER_AI_COUNT; i++) {
        uint32_t instance = USER_AI_INSTANCES[i];

        Analog_Input_Create(instance);

        /*
         * Apply compiled defaults first.
         */
        Analog_Input_Name_Set(
            instance,
            USER_AI_NAMES[i]);

        Analog_Input_Description_Set(
            instance,
            USER_AI_DESCRIPTIONS[i]);

        Analog_Input_Units_Set(
            instance,
            USER_AI_UNITS[i]);

        Analog_Input_Present_Value_Set(
            instance,
            USER_AI_INITIAL_VALUES[i]);

        Analog_Input_COV_Increment_Set(
            instance,
            USER_AI_COV_INCREMENTS[i]);

        Analog_Input_Reliability_Set(
            instance,
            RELIABILITY_NO_FAULT_DETECTED);

        Analog_Input_Out_Of_Service_Set(
            instance,
            false);

        /*
         * Override compiled defaults with persisted values unless the
         * application was configured to erase and reset NVS.
         */
            if (!app_storage_override_enabled()) {
                bacnet_nvs_load_ai(instance);
            }
    }

    ESP_LOGI(
        TAG,
        "Created %u Analog Input objects",
        (unsigned)USER_AI_COUNT);
}