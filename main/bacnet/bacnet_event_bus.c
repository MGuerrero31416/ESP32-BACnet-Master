#include "bacnet_event_bus.h"

#include <string.h>
#include <stdio.h>

#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "bacnet_event_bus";
static volatile QueueHandle_t s_event_queue = NULL;  /* Volatile for multi-core safety */
static uint32_t s_event_queue_length = 0;
static volatile int s_event_queue_init_done = 0;

bool bacnet_event_bus_init(uint32_t queue_length)
{
    /* Single-initialization guard: If already created, return success.
       This prevents accidental double-initialization which could leak queue memory. */
    if (s_event_queue != NULL) {
        ESP_LOGI(TAG, "Event bus already initialized, skipping re-init (queue=%p)", 
            (void*)s_event_queue);
        return true;
    }

    if (queue_length == 0) {
        queue_length = 16;  /* Increased from 8 to handle burst traffic better */
    }

    ESP_LOGI(TAG, "Creating event queue: %u items × %zu bytes/item",
        queue_length, sizeof(bacnet_event_t));
    
    s_event_queue = xQueueCreate((UBaseType_t)queue_length, sizeof(bacnet_event_t));
    
    if (s_event_queue != NULL) {
        s_event_queue_length = queue_length;
        s_event_queue_init_done = 1;
        
        /* Verify queue was created properly */
        UBaseType_t items = uxQueueMessagesWaiting((QueueHandle_t)s_event_queue);
        ESP_LOGI(TAG, "Event bus initialized: handle=%p items_waiting=%u length=%u size_per_item=%zu",
            (void*)s_event_queue, items, queue_length, sizeof(bacnet_event_t));
        return true;
    } else {
        ESP_LOGE(TAG, "CRITICAL: Failed to create event queue!");
        s_event_queue_init_done = 0;
        return false;
    }
}

bool bacnet_event_bus_enqueue(const bacnet_event_t *event, TickType_t timeout)
{
    if ((s_event_queue == NULL) || (event == NULL)) {
        if (s_event_queue == NULL) {
            ESP_LOGW(TAG, "enqueue failed: queue not initialized");
        }
        return false;
    }

    /* Verify the event struct is self-contained with no uninitialized frame data */
    if ((event->type == BACNET_EVENT_RX_FRAME_BIP || event->type == BACNET_EVENT_RX_FRAME_MSTP) &&
        event->length > BACNET_EVENT_FRAME_MAX) {
        ESP_LOGW(TAG, "enqueue warning: event length %u exceeds frame size %u (clamped)",
            (unsigned)event->length, BACNET_EVENT_FRAME_MAX);
        return false;
    }

    BaseType_t result = xQueueSend(s_event_queue, event, timeout);
    if (result != pdTRUE) {
        ESP_LOGD(TAG, "enqueue failed: queue full or timeout (type=%u)", (unsigned)event->type);
        return false;
    }
    
    return true;
}

bool bacnet_event_bus_dequeue(bacnet_event_t *event, TickType_t timeout)
{
    /* HARD ASSERTION: Queue MUST be initialized before any task calls dequeue.
       If queue is NULL here, initialization was not called in app_main before task creation.
       This is a critical initialization order bug - halt and log it. */
    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "CRITICAL: Event queue NULL - not initialized before task start!");
        ESP_LOGE(TAG, "bacnet_event_bus_init() must be called in app_main BEFORE task creation");
        ESP_LOGE(TAG, "Halting bacnet_core_task to prevent crash in xQueueReceive");
        vTaskDelay(portMAX_DELAY);  /* Halt this task forever */
        return false;  /* Unreachable, but explicit */
    }

    if (event == NULL) {
        ESP_LOGE(TAG, "dequeue FAILED: event pointer is NULL");
        return false;
    }

    /* Log queue handle before attempting receive (for debugging) */
    QueueHandle_t q_handle = (QueueHandle_t)s_event_queue;
    ESP_LOGD(TAG, "dequeue: queue=%p event=%p timeout=%u", q_handle, event, (unsigned)timeout);

    /* xQueueReceive will fully overwrite the event buffer with data from the queue.
       FreeRTOS guarantees the entire structure will be copied.
       If xQueueReceive returns false (timeout), the buffer is not modified. */
    BaseType_t result = xQueueReceive(q_handle, event, timeout);
    if (result != pdTRUE) {
        if (timeout > pdMS_TO_TICKS(1)) {
            ESP_LOGD(TAG, "dequeue timeout (normal) timeout_ticks=%u", (unsigned)timeout);
        }
        return false;
    }
    
    return true;
}
