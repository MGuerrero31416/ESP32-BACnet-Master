#include "wifi_helper.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
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

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_ip_logged = false;
        esp_wifi_connect();
        ESP_LOGW(TAG, "WiFi disconnected, retrying connection");
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
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

    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid, USER_WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, USER_WIFI_PASS, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_set_ps(WIFI_PS_NONE);   // disable modem sleep before connecting

    ESP_LOGI(TAG, "Connecting to Wi-Fi %s ...", USER_WIFI_SSID);
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
