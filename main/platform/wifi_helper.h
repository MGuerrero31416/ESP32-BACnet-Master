#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_SCAN_MAX_RESULTS 16

typedef struct {
	char ssid[33];
	int8_t rssi;
} wifi_scan_result_t;

/**
 * Initialize WiFi in station mode and connect to the configured network.
 * Blocks until connected or timeout occurs.
 */
void wifi_init_sta(void);

/* Scan nearby access points and return visible SSIDs in descending RSSI order. */
int wifi_scan_get_results(
	wifi_scan_result_t *results,
	size_t max_results,
	size_t *result_count);

/* Persist credentials and request a reconnect using the selected network. */
esp_err_t wifi_save_and_connect(const char *ssid, const char *password);

/* True after the most recently requested credentials receive an IP address. */
bool wifi_requested_connection_succeeded(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_HELPER_H */
