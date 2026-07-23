#ifndef BACNET_COORDINATOR_H
#define BACNET_COORDINATOR_H

#include <stdbool.h>

#include "bacnet_dispatcher_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BACNET_LINK_NONE = 0,
    BACNET_LINK_BIP,
    BACNET_LINK_MSTP
} bacnet_link_t;

void bacnet_coordinator_init(void);
void bacnet_coordinator_set_active_preference(bacnet_link_t link);
void bacnet_coordinator_set_link_ready(bacnet_link_t link, bool ready);
void bacnet_coordinator_select_active_link(void);
void bacnet_coordinator_activate_link(bacnet_link_t link);
void bacnet_coordinator_activate_link_name(const char *name);

/* Read-only API */
bacnet_link_t bacnet_get_active_link(void);
bool bacnet_should_send_iam(void);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_COORDINATOR_H */
