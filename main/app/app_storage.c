#include "app_storage.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "User_Settings.h"

static const char *TAG = "app_storage";

/*
 * Private application state.
 *
 * BACnet object modules must access this through
 * app_storage_override_enabled().
 */
static bool s_override_nvs_on_flash = false;


bool app_storage_override_enabled(void)
{
    return s_override_nvs_on_flash;
}


esp_err_t app_storage_init(void)
{
    esp_err_t ret = nvs_flash_init();

    s_override_nvs_on_flash =
        (USER_OVERRIDE_NVS_ON_FLASH != 0);

    /*
     * Explicitly erase NVS when compiled defaults must override
     * all previously saved values.
     */
    if (s_override_nvs_on_flash) {
        ESP_LOGI(
            TAG,
            "Override flag set - erasing NVS to reset to defaults");

        ret = nvs_flash_erase();

        if (ret != ESP_OK) {
            ESP_LOGE(
                TAG,
                "Failed to erase NVS: %s",
                esp_err_to_name(ret));

            return ret;
        }

        ret = nvs_flash_init();

        if (ret != ESP_OK) {
            ESP_LOGE(
                TAG,
                "Failed to reinitialize NVS after erase: %s",
                esp_err_to_name(ret));

            return ret;
        }

        ESP_LOGI(
            TAG,
            "NVS reinitialized successfully");

        return ESP_OK;
    }

    /*
     * Recover automatically when the existing NVS partition
     * is incompatible or has no free pages.
     */
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(
            TAG,
            "NVS requires erase and reinitialization");

        ret = nvs_flash_erase();

        if (ret != ESP_OK) {
            ESP_LOGE(
                TAG,
                "Failed to erase NVS: %s",
                esp_err_to_name(ret));

            return ret;
        }

        ret = nvs_flash_init();

        if (ret != ESP_OK) {
            ESP_LOGE(
                TAG,
                "Failed to initialize NVS after erase: %s",
                esp_err_to_name(ret));

            return ret;
        }

        ESP_LOGI(
            TAG,
            "NVS initialized after erase");

        return ESP_OK;
    }

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "NVS initialization failed: %s",
            esp_err_to_name(ret));

        return ret;
    }

    ESP_LOGI(
        TAG,
        "NVS initialized from existing data");

    return ESP_OK;
}