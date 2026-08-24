#include "wifi_helper.h"

#include <string.h>

#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs.h"
#include "User_Settings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"

static const char *TAG = "wifi_helper";
static EventGroupHandle_t s_wifi_event_group;
static bool s_ip_logged;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_NVS_NAMESPACE "wifi_cfg"
#define WIFI_NVS_SSID_KEY "ssid"
#define WIFI_NVS_PASSWORD_KEY "password"

static bool s_reconfiguring;
static bool s_requested_connection_succeeded;

static void wifi_set_default_credentials(char *ssid, char *password)
{
    strncpy(ssid, USER_WIFI_SSID, 32);
    ssid[32] = '\0';
    strncpy(password, USER_WIFI_PASS, 64);
    password[64] = '\0';
}

#if defined(CONFIG_USER_DISPLAY_LVGL_TDISPLAY_S3) && \
    CONFIG_USER_DISPLAY_LVGL_TDISPLAY_S3
static void wifi_load_saved_credentials(char *ssid, char *password)
{
    char saved_ssid[33] = { 0 };
    char saved_password[65] = { 0 };
    size_t ssid_length = sizeof(saved_ssid);
    size_t password_length = sizeof(saved_password);
    nvs_handle_t nvs_handle;

    if (nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs_handle) != ESP_OK) {
        wifi_set_default_credentials(ssid, password);
        return;
    }

    const bool loaded =
        nvs_get_str(nvs_handle, WIFI_NVS_SSID_KEY, saved_ssid, &ssid_length) == ESP_OK &&
        saved_ssid[0] != '\0' &&
        nvs_get_str(
            nvs_handle,
            WIFI_NVS_PASSWORD_KEY,
            saved_password,
            &password_length) == ESP_OK;
    nvs_close(nvs_handle);

    if (!loaded) {
        wifi_set_default_credentials(ssid, password);
        return;
    }

    strncpy(ssid, saved_ssid, 32);
    ssid[32] = '\0';
    strncpy(password, saved_password, 64);
    password[64] = '\0';
}
#endif

static esp_err_t wifi_apply_credentials(const char *ssid, const char *password)
{
    wifi_config_t wifi_config = { 0 };

    strncpy(
        (char *)wifi_config.sta.ssid,
        ssid,
        sizeof(wifi_config.sta.ssid) - 1);
    strncpy(
        (char *)wifi_config.sta.password,
        password,
        sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

    return esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
}

static void sort_scan_results(wifi_scan_result_t *results, size_t result_count)
{
    for (size_t i = 1; i < result_count; ++i) {
        wifi_scan_result_t current = results[i];
        size_t insert_at = i;

        while (insert_at > 0 &&
               current.rssi > results[insert_at - 1].rssi) {
            results[insert_at] = results[insert_at - 1];
            --insert_at;
        }

        results[insert_at] = current;
    }
}

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_ip_logged = false;
        if (!s_reconfiguring) {
            esp_wifi_connect();
        }
        ESP_LOGW(TAG, "WiFi disconnected, retrying connection");
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        s_requested_connection_succeeded = true;
        if (event && !s_ip_logged) {
            ESP_LOGI(TAG, "WiFi connected with IP: " IPSTR, IP2STR(&event->ip_info.ip));
            ESP_LOGI(TAG, "WiFi connected to SSID: %s", USER_WIFI_SSID);
            s_ip_logged = true;
        }
        esp_wifi_set_ps(WIFI_PS_NONE);   // disable modem sleep – prevents ping timeouts
        /* Override DNS (applies to both DHCP and static IP modes) */
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif && USER_WIFI_STATIC_DNS[0] != '\0') {
            ip4_addr_t ip4 = {0};
            esp_netif_dns_info_t dns_info = {0};
            if (ip4addr_aton(USER_WIFI_STATIC_DNS, &ip4)) {
                dns_info.ip.u_addr.ip4.addr = ip4.addr;
                dns_info.ip.type = IPADDR_TYPE_V4;
                esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
                ESP_LOGI(TAG, "DNS set to %s", USER_WIFI_STATIC_DNS);
            }
        }
        if (s_wifi_event_group) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

/* Simple Wi-Fi setup (blocking until connected) */
void wifi_init_sta(void)
{
    /* Initialize networking stack (only call once in app_main context) */
    /* esp_netif_init() and esp_event_loop_create_default() should be called before this */
    s_wifi_event_group = xEventGroupCreate();
    s_ip_logged = false;

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    esp_event_handler_instance_t wifi_event_instance;
    esp_event_handler_instance_t ip_event_instance;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        &wifi_event_instance);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        &ip_event_instance);
    ESP_LOGI(TAG, "WiFi event hooks registered (WIFI_EVENT, IP_EVENT_STA_GOT_IP)");

    esp_netif_t *esp_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (USER_WIFI_USE_STATIC_IP && esp_netif) {
        esp_netif_dhcpc_stop(esp_netif);
        esp_netif_ip_info_t ip_info = {0};
        ip4_addr_t ip4 = {0};
        if (ip4addr_aton(USER_WIFI_STATIC_IP_ADDR, &ip4)) {
            ip_info.ip.addr = ip4.addr;
        }
        if (ip4addr_aton(USER_WIFI_STATIC_IP_GATEWAY, &ip4)) {
            ip_info.gw.addr = ip4.addr;
        }
        if (ip4addr_aton(USER_WIFI_STATIC_IP_NETMASK, &ip4)) {
            ip_info.netmask.addr = ip4.addr;
        }
        esp_netif_set_ip_info(esp_netif, &ip_info);
        ESP_LOGI(TAG, "Using static IP %s", USER_WIFI_STATIC_IP_ADDR);
    }

    char wifi_ssid[33] = { 0 };
    char wifi_password[65] = { 0 };
#if defined(CONFIG_USER_DISPLAY_LVGL_TDISPLAY_S3) && \
    CONFIG_USER_DISPLAY_LVGL_TDISPLAY_S3
    wifi_load_saved_credentials(wifi_ssid, wifi_password);
#else
    wifi_set_default_credentials(wifi_ssid, wifi_password);
#endif

    esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(wifi_apply_credentials(wifi_ssid, wifi_password));
    esp_wifi_start();
    esp_wifi_set_ps(WIFI_PS_NONE);   // disable modem sleep before connecting

    ESP_LOGI(TAG, "Connecting to Wi-Fi %s ...", wifi_ssid);
    esp_wifi_connect();

    EventBits_t bits = 0;
    if (s_wifi_event_group) {
        bits = xEventGroupWaitBits(s_wifi_event_group,
                                   WIFI_CONNECTED_BIT,
                                   pdFALSE,
                                   pdFALSE,
                                   pdMS_TO_TICKS(10000));
    }

    if ((bits & WIFI_CONNECTED_BIT) == 0) {
        ESP_LOGW(TAG, "WiFi connection timeout - proceeding anyway");
    }
}

int wifi_scan_get_results(
    wifi_scan_result_t *results,
    size_t max_results,
    size_t *result_count)
{
    if (results == NULL || result_count == NULL || max_results == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    *result_count = 0;

    wifi_scan_config_t scan_config = {
        .show_hidden = false,
    };
    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "WiFi scan failed: %s", esp_err_to_name(err));
        return err;
    }

    uint16_t ap_count = 0;
    err = esp_wifi_scan_get_ap_num(&ap_count);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Could not read WiFi scan count: %s", esp_err_to_name(err));
        return err;
    }

    if (ap_count == 0) {
        return ESP_OK;
    }

    wifi_ap_record_t ap_records[WIFI_SCAN_MAX_RESULTS];
    uint16_t records_to_read = ap_count;
    if (records_to_read > WIFI_SCAN_MAX_RESULTS) {
        records_to_read = WIFI_SCAN_MAX_RESULTS;
    }

    err = esp_wifi_scan_get_ap_records(&records_to_read, ap_records);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Could not read WiFi scan results: %s", esp_err_to_name(err));
        return err;
    }

    size_t count = 0;
    for (uint16_t i = 0; i < records_to_read && count < max_results; ++i) {
        if (ap_records[i].ssid[0] == '\0') {
            continue;
        }

        strncpy(
            results[count].ssid,
            (const char *)ap_records[i].ssid,
            sizeof(results[count].ssid) - 1);
        results[count].ssid[sizeof(results[count].ssid) - 1] = '\0';
        results[count].rssi = ap_records[i].rssi;
        ++count;
    }

    sort_scan_results(results, count);
    *result_count = count;
    return ESP_OK;
}

esp_err_t wifi_save_and_connect(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL || ssid[0] == '\0' ||
        strlen(ssid) > 32 || strlen(password) > 64) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Could not open WiFi credential storage: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, WIFI_NVS_SSID_KEY, ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(nvs_handle, WIFI_NVS_PASSWORD_KEY, password);
    }
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Could not save WiFi credentials: %s", esp_err_to_name(err));
        return err;
    }

    s_requested_connection_succeeded = false;
    s_reconfiguring = true;
    esp_wifi_disconnect();
    err = wifi_apply_credentials(ssid, password);
    if (err == ESP_OK) {
        err = esp_wifi_connect();
    }
    s_reconfiguring = false;

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Could not apply WiFi credentials: %s", esp_err_to_name(err));
    }

    return err;
}

bool wifi_requested_connection_succeeded(void)
{
    return s_requested_connection_succeeded;
}
