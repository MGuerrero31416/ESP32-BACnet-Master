#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize WiFi in station mode and connect to the configured network.
 * Blocks until connected or timeout occurs.
 */
void wifi_init_sta(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_HELPER_H */
