#ifndef BACNET_EVENT_BUS_H
#define BACNET_EVENT_BUS_H

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "bacnet/bacaddr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BACNET_EVENT_FRAME_MAX 600U

typedef enum {
    BACNET_EVENT_LINK_NONE = 0,
    BACNET_EVENT_LINK_BIP,
    BACNET_EVENT_LINK_MSTP
} bacnet_event_link_t;

typedef enum {
    BACNET_EVENT_RX_FRAME_BIP = 0,
    BACNET_EVENT_RX_FRAME_MSTP,
    BACNET_EVENT_NETWORK_STATE_CHANGE
} bacnet_event_type_t;

typedef struct {
    uint32_t flags;
} bacnet_network_state_change_t;

typedef struct {
    bacnet_event_type_t type;
    bacnet_event_link_t link_id;
    uint16_t length;
    uint64_t timestamp_us;
    BACNET_ADDRESS src;
    union {
        uint8_t frame[BACNET_EVENT_FRAME_MAX];
        bacnet_network_state_change_t network;
    } data;
} bacnet_event_t;

bool bacnet_event_bus_init(uint32_t queue_length);
bool bacnet_event_bus_enqueue(const bacnet_event_t *event, TickType_t timeout);
bool bacnet_event_bus_dequeue(bacnet_event_t *event, TickType_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_EVENT_BUS_H */
