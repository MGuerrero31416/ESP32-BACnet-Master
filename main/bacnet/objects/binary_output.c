#include "binary_output.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "app_storage.h"

/* bacnet-stack headers */
#include "bacnet/basic/object/bo.h"

/* FreeRTOS for GPIO sync task */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "User_Settings.h"

static const char *TAG = "bacnet_bo";
#define NVS_NAMESPACE "bacnet"

void bacnet_nvs_save_bo_name(uint32_t instance, const char *name, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[65] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "bo_%lu_name", (unsigned long)instance);
    if (name && length > 0 && length < sizeof(buf)) {
        memcpy(buf, name, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BO%lu name: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BO%lu name: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for BO%lu name: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BO%lu name: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_bo_desc(uint32_t instance, const char *desc, uint16_t length) {
    nvs_handle_t nvs_handle;
    char key[32];
    char buf[129] = {0};
    esp_err_t err;
    snprintf(key, sizeof(key), "bo_%lu_desc", (unsigned long)instance);
    if (desc && length > 0 && length < sizeof(buf)) {
        memcpy(buf, desc, length);
        buf[length] = 0;
    }
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_str(nvs_handle, key, buf)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BO%lu desc: %s", (unsigned long)instance, buf);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BO%lu desc: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_str failed for BO%lu desc: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BO%lu desc: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_save_bo_pv(uint32_t instance, uint8_t value) {
    nvs_handle_t nvs_handle;
    char key[32];
    esp_err_t err;
    snprintf(key, sizeof(key), "bo_%lu_val", (unsigned long)instance);
    if ((err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
        if ((err = nvs_set_u8(nvs_handle, key, value)) == ESP_OK) {
            if ((err = nvs_commit(nvs_handle)) == ESP_OK) {
                ESP_LOGI(TAG, "Saved BO%lu value: %u", (unsigned long)instance, value);
            } else {
                ESP_LOGE(TAG, "NVS commit failed for BO%lu value: %d", (unsigned long)instance, err);
            }
        } else {
            ESP_LOGE(TAG, "NVS set_u8 failed for BO%lu value: %d", (unsigned long)instance, err);
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "NVS open failed for BO%lu value: %d", (unsigned long)instance, err);
    }
}

void bacnet_nvs_load_bo(uint32_t instance) {
    nvs_handle_t nvs_handle;
    char key[32];
    static char bo_names[USER_BO_COUNT][65];  /* Persistent storage for loaded names */
    static char bo_descs[USER_BO_COUNT][129];  /* Persistent storage for loaded descriptions */
    uint8_t idx = (instance > 0 && instance <= 4) ? (instance - 1) : 0;
    uint8_t pv = BINARY_INACTIVE;
    size_t len;

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) {
        return;  /* NVS not initialized yet */
    }

    snprintf(key, sizeof(key), "bo_%lu_name", (unsigned long)instance);
    len = sizeof(bo_names[idx]);
    if (nvs_get_str(nvs_handle, key, bo_names[idx], &len) == ESP_OK) {
        Binary_Output_Name_Set(instance, bo_names[idx]);
    }

    snprintf(key, sizeof(key), "bo_%lu_desc", (unsigned long)instance);
    len = sizeof(bo_descs[idx]);
    if (nvs_get_str(nvs_handle, key, bo_descs[idx], &len) == ESP_OK) {
        Binary_Output_Description_Set(instance, bo_descs[idx]);
    }

    snprintf(key, sizeof(key), "bo_%lu_val", (unsigned long)instance);
    if (nvs_get_u8(nvs_handle, key, &pv) == ESP_OK) {
        Binary_Output_Present_Value_Set(instance, (BACNET_BINARY_PV)pv, 16);
    }

    nvs_close(nvs_handle);
}

void bacnet_create_binary_outputs(void) {
    size_t i = 0;
    size_t num_instances = USER_BO_COUNT;

    for (i = 0; i < num_instances; i++) {
        uint32_t instance = USER_BO_INSTANCES[i];
        Binary_Output_Create(instance);
        Binary_Output_Name_Set(instance, USER_BO_NAMES[i]);
        Binary_Output_Description_Set(instance, USER_BO_DESCRIPTIONS[i]);
        Binary_Output_Active_Text_Set(instance, USER_BO_ACTIVE_TEXT[i]);
        Binary_Output_Inactive_Text_Set(instance, USER_BO_INACTIVE_TEXT[i]);
        Binary_Output_Present_Value_Set(instance, (BACNET_BINARY_PV)USER_BO_INITIAL_VALUES[i], 16);
        Binary_Output_Out_Of_Service_Set(instance, false);
        /* Load persisted values from NVS (if any) - unless override flag is set */
        if (!app_storage_override_enabled()) {
            bacnet_nvs_load_bo(instance);
        }
    }

    ESP_LOGI(TAG, "Created %zu Binary Output objects", num_instances);
}

/* No-op: BO1 no longer drives a physical GPIO */
void bacnet_bo1_gpio_update(uint8_t state)
{
    (void)state;
}

/* Task to monitor BO1 for value changes (no physical GPIO action) */
static uint8_t last_bo1_state = BINARY_INACTIVE;

static void bo1_gpio_sync_task(void *pvParameters)
{
    (void)pvParameters;
    while (1) {
        last_bo1_state = Binary_Output_Present_Value(USER_BO_INSTANCES[0]);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void bacnet_create_binary_outputs_with_gpio_sync(void) {
    bacnet_create_binary_outputs();
    
    /* Start GPIO sync task */
    if (xTaskCreate(bo1_gpio_sync_task, "bo1_gpio_sync", 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create BO1 GPIO sync task");
    }
}
