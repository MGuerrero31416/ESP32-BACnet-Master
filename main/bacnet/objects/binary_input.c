#include "binary_input.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "User_Settings.h"
#include "app_storage.h"

/* bacnet-stack headers */
#include "bacnet/basic/object/bi.h"

static const char *TAG = "bacnet_bi";
#define NVS_NAMESPACE "bacnet"

void bacnet_nvs_save_bi_name(uint32_t instance, const char *name, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[65] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "bi_%lu_name", (unsigned long)instance);
    if (name && length > 0 && length < sizeof(buf)) {
        memcpy(buf, name, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BI%lu name: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BI%lu name: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for BI%lu name: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BI%lu name: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_bi_desc(uint32_t instance, const char *desc, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[129] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "bi_%lu_desc", (unsigned long)instance);
    if (desc && length > 0 && length < sizeof(buf)) {
        memcpy(buf, desc, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BI%lu desc: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BI%lu desc: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for BI%lu desc: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BI%lu desc: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_bi_pv(uint32_t instance, uint8_t value) {
    nvs_handle_t nvs_handle;
    char key[32];
    esp_err_t err;
    snprintf(key, sizeof(key), "bi_%lu_val", (unsigned long)instance);
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_u8(nvs_handle, key, value)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BI%lu value: %u", (unsigned long)instance, value);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BI%lu value: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_u8 failed for BI%lu value: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BI%lu value: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_load_bi(uint32_t instance) {
    nvs_handle_t nvs_handle;
    char key[32];
    static char bi_names[USER_BI_COUNT][65];  /* Persistent storage for loaded names */
    static char bi_descs[USER_BI_COUNT][129];  /* Persistent storage for loaded descriptions */
    uint8_t idx = (instance > 0 && instance <= 4) ? (instance - 1) : 0;
    uint8_t pv = BINARY_INACTIVE;
    size_t len;

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) {
        return;  /* NVS not initialized yet */
    }

    snprintf(key, sizeof(key), "bi_%lu_name", (unsigned long)instance);
    len = sizeof(bi_names[idx]);
    if (nvs_get_str(nvs_handle, key, bi_names[idx], &len) == ESP_OK) {
        Binary_Input_Name_Set(instance, bi_names[idx]);
    }

    snprintf(key, sizeof(key), "bi_%lu_desc", (unsigned long)instance);
    len = sizeof(bi_descs[idx]);
    if (nvs_get_str(nvs_handle, key, bi_descs[idx], &len) == ESP_OK) {
        Binary_Input_Description_Set(instance, bi_descs[idx]);
    }

    snprintf(key, sizeof(key), "bi_%lu_val", (unsigned long)instance);
    if (nvs_get_u8(nvs_handle, key, &pv) == ESP_OK) {
        Binary_Input_Present_Value_Set(instance, (BACNET_BINARY_PV)pv);
    }

    nvs_close(nvs_handle);
}

void bacnet_create_binary_inputs(void) {
    size_t i = 0;
    size_t num_instances = USER_BI_COUNT;

    for (i = 0; i < num_instances; i++) {
        uint32_t instance = USER_BI_INSTANCES[i];
        Binary_Input_Create(instance);
        Binary_Input_Name_Set(instance, USER_BI_NAMES[i]);
        Binary_Input_Description_Set(instance, USER_BI_DESCRIPTIONS[i]);
        Binary_Input_Active_Text_Set(instance, USER_BI_ACTIVE_TEXT[i]);
        Binary_Input_Inactive_Text_Set(instance, USER_BI_INACTIVE_TEXT[i]);
        Binary_Input_Present_Value_Set(instance, (BACNET_BINARY_PV)USER_BI_INITIAL_VALUES[i]);
        Binary_Input_Reliability_Set(instance, RELIABILITY_NO_FAULT_DETECTED);
        Binary_Input_Out_Of_Service_Set(instance, false);
        /* Load persisted values from NVS (if any) - unless override flag is set */
        if (!app_storage_override_enabled()) {
            bacnet_nvs_load_bi(instance);
        }
    }

    ESP_LOGI(TAG, "Created %zu Binary Input objects", num_instances);
}
