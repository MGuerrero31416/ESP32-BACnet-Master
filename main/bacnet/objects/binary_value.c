#include "binary_value.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs_flash.h"
#include "app_storage.h"
#include "User_Settings.h"

/* bacnet-stack headers */
#include "bacnet/basic/object/bv.h"

static const char *TAG = "bacnet_bv";
#define NVS_NAMESPACE "bacnet"

void bacnet_nvs_save_bv_name(uint32_t instance, const char *name, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[65] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "binary_%lu_name", (unsigned long)instance);
    if (name && length > 0 && length < sizeof(buf)) {
        memcpy(buf, name, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BV%lu name: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BV%lu name: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for BV%lu name: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BV%lu name: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_bv_desc(uint32_t instance, const char *desc, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[129] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "binary_%lu_desc", (unsigned long)instance);
    if (desc && length > 0 && length < sizeof(buf)) {
        memcpy(buf, desc, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BV%lu desc: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BV%lu desc: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for BV%lu desc: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BV%lu desc: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_bv_pv(uint32_t instance, uint8_t value) {
    nvs_handle_t nvs_handle;
    char key[32];
    esp_err_t err;
    snprintf(key, sizeof(key), "binary_%lu_val", (unsigned long)instance);
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_u8(nvs_handle, key, value)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BV%lu value: %u", (unsigned long)instance, value);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BV%lu value: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_u8 failed for BV%lu value: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BV%lu value: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_load_bv(uint32_t instance) {
    nvs_handle_t nvs_handle;
    char key[32];
    static char bv_names[USER_BV_COUNT][65];  /* Persistent storage for loaded names */
    static char bv_descs[USER_BV_COUNT][129];  /* Persistent storage for loaded descriptions */
    uint8_t idx = (instance > 0 && instance <= 4) ? (instance - 1) : 0;
    uint8_t pv = BINARY_INACTIVE;
    size_t len;

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) {
        return;  /* NVS not initialized yet */
    }

    snprintf(key, sizeof(key), "binary_%lu_name", (unsigned long)instance);
    len = sizeof(bv_names[idx]);
    if (nvs_get_str(nvs_handle, key, bv_names[idx], &len) == ESP_OK) {
        Binary_Value_Name_Set(instance, bv_names[idx]);
    }

    snprintf(key, sizeof(key), "binary_%lu_desc", (unsigned long)instance);
    len = sizeof(bv_descs[idx]);
    if (nvs_get_str(nvs_handle, key, bv_descs[idx], &len) == ESP_OK) {
        Binary_Value_Description_Set(instance, bv_descs[idx]);
    }

    snprintf(key, sizeof(key), "binary_%lu_val", (unsigned long)instance);
    if (nvs_get_u8(nvs_handle, key, &pv) == ESP_OK) {
        Binary_Value_Present_Value_Set(instance, (BACNET_BINARY_PV)pv);
    }

    nvs_close(nvs_handle);
}

void bacnet_create_binary_values(void) {
    size_t i = 0;
    size_t num_instances = USER_BV_COUNT;

    for (i = 0; i < num_instances; i++) {
        uint32_t instance = USER_BV_INSTANCES[i];
        Binary_Value_Create(instance);
        Binary_Value_Name_Set(instance, USER_BV_NAMES[i]);
        Binary_Value_Description_Set(instance, USER_BV_DESCRIPTIONS[i]);
        Binary_Value_Active_Text_Set(instance, USER_BV_ACTIVE_TEXT[i]);
        Binary_Value_Inactive_Text_Set(instance, USER_BV_INACTIVE_TEXT[i]);
        Binary_Value_Present_Value_Set(instance, (BACNET_BINARY_PV)USER_BV_INITIAL_VALUES[i]);
        Binary_Value_Reliability_Set(instance, RELIABILITY_NO_FAULT_DETECTED);
        Binary_Value_Out_Of_Service_Set(instance, false);
        Binary_Value_Write_Enable(instance);
        /* Load persisted values from NVS (if any) - unless override flag is set */
        if (!app_storage_override_enabled()) {
            bacnet_nvs_load_bv(instance);
        }
    }

    ESP_LOGI(TAG, "Created %zu Binary Value objects", num_instances);
}
