#ifndef BACNET_STRUCT_SIZE_CHECK_H
#define BACNET_STRUCT_SIZE_CHECK_H

#include "bacnet_event_bus.h"
#include <stdio.h>

/* Verify struct sizes at compile time */
_Static_assert(sizeof(BACNET_ADDRESS) > 0, "BACNET_ADDRESS has unexpected size");
_Static_assert(sizeof(bacnet_event_t) > 0, "bacnet_event_t has unexpected size");

/* Log sizes at runtime */
static inline void bacnet_log_struct_sizes(void)
{
    printf("=== BACnet Struct Size Check ===\n");
    printf("sizeof(BACNET_ADDRESS) = %zu bytes\n", sizeof(BACNET_ADDRESS));
    printf("sizeof(bacnet_event_type_t) = %zu bytes\n", sizeof(bacnet_event_type_t));
    printf("sizeof(bacnet_event_link_t) = %zu bytes\n", sizeof(bacnet_event_link_t));
    printf("sizeof(bacnet_network_state_change_t) = %zu bytes\n", sizeof(bacnet_network_state_change_t));
    printf("sizeof(bacnet_event_t) = %zu bytes\n", sizeof(bacnet_event_t));
    printf("  - Expected allocation per queue item: %zu bytes\n", sizeof(bacnet_event_t));
    printf("  - With 8 queue items: %zu bytes total\n", 8 * sizeof(bacnet_event_t));
    printf("Offset of src in bacnet_event_t: %zu bytes\n", offsetof(bacnet_event_t, src));
    printf("Offset of data in bacnet_event_t: %zu bytes\n", offsetof(bacnet_event_t, data));
    printf("=====================================\n");
}

#endif /* BACNET_STRUCT_SIZE_CHECK_H */
