#include "analog_value.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "User_Settings.h"
#include <stdbool.h>
#include "app_storage.h"

/* bacnet-stack headers */
#include "bacnet/basic/object/av.h"

static const char *TAG = "bacnet_av";
#define NVS_NAMESPACE "bacnet"

static bool av_find_config_index(
    uint32_t instance,
    size_t *index)
{
    if (index == NULL) {
        return false;
    }

    for (size_t i = 0;
         i < USER_AV_COUNT;
         i++) {
        if (USER_AV_INSTANCES[i] == instance) {
            *index = i;
            return true;
        }
    }

    return false;
}


void bacnet_nvs_save_av_name(uint32_t instance, const char *name, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[65] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "analog_%lu_name", (unsigned long)instance);
    if (name && length > 0 && length < sizeof(buf)) {
        memcpy(buf, name, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AV%lu name: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AV%lu name: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for AV%lu name: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AV%lu name: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_av_desc(uint32_t instance, const char *desc, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[129] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "analog_%lu_desc", (unsigned long)instance);
    if (desc && length > 0 && length < sizeof(buf)) {
        memcpy(buf, desc, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AV%lu desc: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AV%lu desc: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for AV%lu desc: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AV%lu desc: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_av_units(uint32_t instance, uint16_t units) {
    nvs_handle_t nvs_handle;
    char key[32];
    esp_err_t err;
    snprintf(key, sizeof(key), "analog_%lu_unit", (unsigned long)instance);
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_u16(nvs_handle, key, units)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AV%lu units: %u", (unsigned long)instance, units);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AV%lu units: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_u16 failed for AV%lu units: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AV%lu units: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_av_pv(uint32_t instance, float value) {
    nvs_handle_t nvs_handle;
    char key[32];
    esp_err_t err;
    snprintf(key, sizeof(key), "analog_%lu_val", (unsigned long)instance);
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_blob(nvs_handle, key, &value, sizeof(value))) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved AV%lu value: %.2f", (unsigned long)instance, value);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for AV%lu: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_blob failed for AV%lu: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for AV%lu: %d", (unsigned long)instance, err);
    }
}

esp_err_t bacnet_nvs_load_av_pv(
    uint32_t instance,
    float *value,
    bool *found)
{
    if (value == NULL || found == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *found = false;

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    char key[32];
    snprintf(key, sizeof(key), "analog_%lu_val", (unsigned long)instance);

    size_t len = sizeof(*value);
    err = nvs_get_blob(nvs_handle, key, value, &len);

    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        *found = true;
        return ESP_OK;
    }

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }

    return err;
}

void bacnet_nvs_load_av(uint32_t instance) {
    nvs_handle_t nvs_handle;
    char key[32];
    static char av_names[USER_AV_COUNT][65];  /* Persistent storage for loaded names */
    static char av_descs[USER_AV_COUNT][129];  /* Persistent storage for loaded descriptions */
   size_t idx;

                if (!av_find_config_index(instance, &idx)) {
                    ESP_LOGE(
                        TAG,
                        "AV%lu is not configured",
                        (unsigned long)instance);
                    return;
                }
    uint16_t units = UNITS_PERCENT;
    float pv = 0.0f;
    size_t len;

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) {
        return;  /* NVS not initialized yet */
    }

    snprintf(key, sizeof(key), "analog_%lu_name", (unsigned long)instance);
    len = sizeof(av_names[idx]);
    if (nvs_get_str(nvs_handle, key, av_names[idx], &len) == ESP_OK) {
        Analog_Value_Name_Set(instance, av_names[idx]);
    }

    snprintf(key, sizeof(key), "analog_%lu_desc", (unsigned long)instance);
    len = sizeof(av_descs[idx]);
    if (nvs_get_str(nvs_handle, key, av_descs[idx], &len) == ESP_OK) {
        Analog_Value_Description_Set(instance, av_descs[idx]);
    }

    snprintf(key, sizeof(key), "analog_%lu_unit", (unsigned long)instance);
    if (nvs_get_u16(nvs_handle, key, &units) == ESP_OK) {
        Analog_Value_Units_Set(instance, units);
    }

    snprintf(key, sizeof(key), "analog_%lu_val", (unsigned long)instance);
    len = sizeof(pv);
    if (nvs_get_blob(nvs_handle, key, &pv, &len) == ESP_OK) {
        Analog_Value_Present_Value_Set(instance, pv, 16);
    }

    nvs_close(nvs_handle);
}

void bacnet_create_analog_values(void) {
    size_t i = 0;
    size_t num_instances = USER_AV_COUNT;

    for (i = 0; i < num_instances; i++) {
        uint32_t instance = USER_AV_INSTANCES[i];
        Analog_Value_Create(instance);
        Analog_Value_Name_Set(instance, USER_AV_NAMES[i]);
        Analog_Value_Description_Set(instance, USER_AV_DESCRIPTIONS[i]);
        Analog_Value_Units_Set(instance, USER_AV_UNITS[i]);
        Analog_Value_Present_Value_Set(instance, USER_AV_INITIAL_VALUES[i], 16);
        Analog_Value_COV_Increment_Set(instance, USER_AV_COV_INCREMENTS[i]);
        Analog_Value_Reliability_Set(instance, RELIABILITY_NO_FAULT_DETECTED);
        Analog_Value_Out_Of_Service_Set(instance, false);
        /* Load persisted values from NVS (if any) - unless override flag is set */
        if (!app_storage_override_enabled()) {
            bacnet_nvs_load_av(instance);
        }
    }

    ESP_LOGI(TAG, "Created %zu Analog Value objects", num_instances);
}
