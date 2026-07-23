#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BACnet runtime events that can be forwarded to the existing
 * stack profiler in main.c.
 */
typedef enum {
    BACNET_APP_PROFILE_BIP_RX = 0,
    BACNET_APP_PROFILE_MSTP_RX,
    BACNET_APP_PROFILE_READ_PROPERTY,
    BACNET_APP_PROFILE_WRITE_PROPERTY,
    BACNET_APP_PROFILE_COV_NOTIFICATION
} bacnet_app_profile_event_t;

typedef void (*bacnet_app_profile_callback_t)(
    bacnet_app_profile_event_t event);

/*
 * The task handles remain stored in main.c for now because the existing
 * stack profiler refers to them.
 */
typedef struct {
    TaskHandle_t *bip_rx;
    TaskHandle_t *mstp_rx;
    TaskHandle_t *core;
    TaskHandle_t *cov;
} bacnet_app_task_handle_refs_t;

/*
 * Initialize BACnet transports, device, handlers and objects.
 * NVS must already be initialized before this function is called.
 */
esp_err_t bacnet_app_init(
    bacnet_app_profile_callback_t profile_callback);

/* Start the BACnet FreeRTOS tasks. */
esp_err_t bacnet_app_start(
    const bacnet_app_task_handle_refs_t *task_handles);

/* Called once per second by the application supervisor loop. */
void bacnet_app_maintenance_1s(void);

/* Send a BACnet I-Am on MS/TP when MS/TP is enabled. */
void bacnet_app_send_mstp_i_am(void);

/* Clear the existing MS/TP diagnostic counters. */
void bacnet_app_reset_mstp_diagnostics(void);

/* Used by the display link-alive logic. */
uint32_t bacnet_app_get_mstp_pdu_count(void);

/* Return the current Wi-Fi station connection state. */
bool bacnet_app_wifi_connected(void);

#ifdef __cplusplus
}
#endif