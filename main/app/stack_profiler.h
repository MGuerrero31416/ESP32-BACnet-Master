#pragma once

#include "bacnet_app.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STACK_EVT_NORMAL = 0,
    STACK_EVT_BACNET_IP_RX,
    STACK_EVT_MSTP_RX,
    STACK_EVT_READ_PROPERTY,
    STACK_EVT_WRITE_PROPERTY,
    STACK_EVT_COV_NOTIFICATION,
    STACK_EVT_DISPLAY_UPDATE,
    STACK_EVT_SEN54_READ,
    STACK_EVT_COUNT
} stack_profile_event_t;

typedef struct {
    TaskHandle_t *bacnet_rx;
    TaskHandle_t *bacnet_mstp_rx;
    TaskHandle_t *bacnet_core;
    TaskHandle_t *bacnet_cov;
    TaskHandle_t *sensor;
} stack_profiler_task_handle_refs_t;

void stack_profiler_init(
    const stack_profiler_task_handle_refs_t *task_handles);

void stack_profiler_sample(stack_profile_event_t event);

void stack_profiler_log_report(void);

/*
 * Adapter passed directly to bacnet_app_init().
 */
void stack_profiler_bacnet_callback(
    bacnet_app_profile_event_t event);

#ifdef __cplusplus
}
#endif