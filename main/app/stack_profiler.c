#include "stack_profiler.h"

#include <limits.h>
#include <stdint.h>

#include "esp_log.h"

static const char *TAG = "stack_profiler";

typedef struct {
    const char *task_name;
    TaskHandle_t *task_handle;

    /* ESP-IDF task stack sizes and high-water marks are in bytes. */
    uint32_t configured_stack_bytes;
    UBaseType_t last_hwm_bytes;
    UBaseType_t min_free_bytes_observed;
    UBaseType_t min_free_bytes_by_event[STACK_EVT_COUNT];
} stack_profile_task_t;

static stack_profile_task_t stack_profile_tasks[] = {
    {
        .task_name = "bacnet_rx",
        .task_handle = NULL,
        .configured_stack_bytes = 16384,
    },
    {
        .task_name = "bacnet_mstp_rx",
        .task_handle = NULL,
        .configured_stack_bytes = 12288,
    },
    {
        .task_name = "bacnet_core",
        .task_handle = NULL,
        .configured_stack_bytes = 20480,
    },
    {
        .task_name = "bacnet_cov",
        .task_handle = NULL,
        .configured_stack_bytes = 24576,
    },
    {
        .task_name = "sen54",
        .task_handle = NULL,
        .configured_stack_bytes = 4096,
    },
};

#define STACK_PROFILE_TASK_COUNT \
    ((int)(sizeof(stack_profile_tasks) / sizeof(stack_profile_tasks[0])))

static const char *stack_profile_event_name(
    stack_profile_event_t event)
{
    switch (event) {
        case STACK_EVT_NORMAL:
            return "normal";

        case STACK_EVT_BACNET_IP_RX:
            return "bacnet_ip_rx";

        case STACK_EVT_MSTP_RX:
            return "mstp_rx";

        case STACK_EVT_READ_PROPERTY:
            return "read_property";

        case STACK_EVT_WRITE_PROPERTY:
            return "write_property";

        case STACK_EVT_COV_NOTIFICATION:
            return "cov_notification";

        case STACK_EVT_DISPLAY_UPDATE:
            return "display_update";

        case STACK_EVT_SEN54_READ:
            return "sen54_read";

        default:
            return "unknown";
    }
}

static float stack_profile_used_percent(
    uint32_t configured_bytes,
    UBaseType_t min_free_bytes)
{
    if (configured_bytes == 0) {
        return 0.0f;
    }

    /*
     * Protect against an unexpected high-water value greater than
     * the configured size. Treat it as zero measured usage rather
     * than reporting 100%.
     */
    if (min_free_bytes >= configured_bytes) {
        return 0.0f;
    }

    uint32_t used_bytes =
        configured_bytes - (uint32_t)min_free_bytes;

    return (100.0f * (float)used_bytes) /
           (float)configured_bytes;
}

void stack_profiler_init(
    const stack_profiler_task_handle_refs_t *task_handles)
{
    if (task_handles == NULL) {
        ESP_LOGE(TAG, "Task handle references are NULL");
        return;
    }

    stack_profile_tasks[0].task_handle =
        task_handles->bacnet_rx;

    stack_profile_tasks[1].task_handle =
        task_handles->bacnet_mstp_rx;

    stack_profile_tasks[2].task_handle =
        task_handles->bacnet_core;

    stack_profile_tasks[3].task_handle =
        task_handles->bacnet_cov;

    stack_profile_tasks[4].task_handle =
        task_handles->sensor;

    for (int i = 0; i < STACK_PROFILE_TASK_COUNT; i++) {
        stack_profile_tasks[i].last_hwm_bytes = 0;
        stack_profile_tasks[i].min_free_bytes_observed =
            UINT_MAX;

        for (int event = 0;
             event < STACK_EVT_COUNT;
             event++) {
            stack_profile_tasks[i]
                .min_free_bytes_by_event[event] =
                UINT_MAX;
        }
    }
}



void stack_profiler_log_report(void)
{
    ESP_LOGI(TAG, "==== STACK PROFILE REPORT ====");

    for (int i = 0; i < STACK_PROFILE_TASK_COUNT; i++) {
        stack_profile_task_t *entry =
            &stack_profile_tasks[i];

        if (entry->task_handle == NULL) {
            ESP_LOGW(
                TAG,
                "stack task=%s has no handle reference",
                entry->task_name);
            continue;
        }

        TaskHandle_t handle = *entry->task_handle;

        if (handle == NULL) {
            ESP_LOGW(
                TAG,
                "stack task=%s not running",
                entry->task_name);
            continue;
        }

        UBaseType_t hwm_now_bytes =
            uxTaskGetStackHighWaterMark(handle);

        if (hwm_now_bytes <
            entry->min_free_bytes_observed) {
            entry->min_free_bytes_observed =
                hwm_now_bytes;
        }

        float used_pct = stack_profile_used_percent(
            entry->configured_stack_bytes,
            entry->min_free_bytes_observed);

        ESP_LOGI(
            TAG,
            "stack task=%s configured=%lu bytes "
            "hwm_now=%lu bytes "
            "min_free_observed=%lu bytes "
            "used=%.1f%%",
            entry->task_name,
            (unsigned long)entry->configured_stack_bytes,
            (unsigned long)hwm_now_bytes,
            (unsigned long)
                entry->min_free_bytes_observed,
            used_pct);
    }

    for (int event_index = 0;
         event_index < STACK_EVT_COUNT;
         event_index++) {
        stack_profile_event_t event =
            (stack_profile_event_t)event_index;

        for (int i = 0;
             i < STACK_PROFILE_TASK_COUNT;
             i++) {
            stack_profile_task_t *entry =
                &stack_profile_tasks[i];

            UBaseType_t min_event_bytes =
                entry->min_free_bytes_by_event[
                    event_index];

            if (min_event_bytes == UINT_MAX) {
                continue;
            }

            float used_pct =
                stack_profile_used_percent(
                    entry->configured_stack_bytes,
                    min_event_bytes);

            ESP_LOGI(
                TAG,
                "stack_event event=%s task=%s "
                "min_free=%lu bytes used=%.1f%%",
                stack_profile_event_name(event),
                entry->task_name,
                (unsigned long)min_event_bytes,
                used_pct);
        }
    }
}

void stack_profiler_sample(
    stack_profile_event_t event)
{
    for (int i = 0; i < STACK_PROFILE_TASK_COUNT; i++) {
        if (stack_profile_tasks[i].task_handle == NULL) {
            continue;
        }

        TaskHandle_t handle =
            *stack_profile_tasks[i].task_handle;

        if (handle == NULL) {
            continue;
        }

        UBaseType_t hwm_bytes =
            uxTaskGetStackHighWaterMark(handle);

        stack_profile_tasks[i].last_hwm_bytes =
            hwm_bytes;

        if (hwm_bytes <
            stack_profile_tasks[i].min_free_bytes_observed) {
            stack_profile_tasks[i].min_free_bytes_observed =
                hwm_bytes;
        }

        if (event >= STACK_EVT_NORMAL &&
            event < STACK_EVT_COUNT &&
            hwm_bytes <
                stack_profile_tasks[i]
                    .min_free_bytes_by_event[event]) {
            stack_profile_tasks[i]
                .min_free_bytes_by_event[event] =
                hwm_bytes;
        }
    }
}

void stack_profiler_bacnet_callback(
    bacnet_app_profile_event_t event)
{
    switch (event) {
        case BACNET_APP_PROFILE_BIP_RX:
            stack_profiler_sample(
                STACK_EVT_BACNET_IP_RX);
            break;

        case BACNET_APP_PROFILE_MSTP_RX:
            stack_profiler_sample(
                STACK_EVT_MSTP_RX);
            break;

        case BACNET_APP_PROFILE_READ_PROPERTY:
            stack_profiler_sample(
                STACK_EVT_READ_PROPERTY);
            break;

        case BACNET_APP_PROFILE_WRITE_PROPERTY:
            stack_profiler_sample(
                STACK_EVT_WRITE_PROPERTY);
            break;

        case BACNET_APP_PROFILE_COV_NOTIFICATION:
            stack_profiler_sample(
                STACK_EVT_COV_NOTIFICATION);
            break;

        default:
            break;
    }
}