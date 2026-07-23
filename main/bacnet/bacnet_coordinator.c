#include "bacnet_coordinator.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <string.h>

#include "bacnet/datalink/datalink.h"

typedef struct {
    bacnet_link_t active_link_preference;
    bool bip_ready;
    bool mstp_ready;
    SemaphoreHandle_t lock;
} bacnet_coordinator_state_t;

static bacnet_coordinator_state_t s_state = {
    .active_link_preference = BACNET_LINK_NONE,
    .bip_ready = false,
    .mstp_ready = false,
    .lock = NULL
};

static bacnet_link_t bacnet_link_from_name(const char *name)
{
    if (name == NULL) {
        return BACNET_LINK_NONE;
    }
    if (strcmp(name, "bip") == 0) {
        return BACNET_LINK_BIP;
    }
    if (strcmp(name, "mstp") == 0) {
        return BACNET_LINK_MSTP;
    }

    return BACNET_LINK_NONE;
}

static const char *bacnet_name_from_link(bacnet_link_t link)
{
    switch (link) {
        case BACNET_LINK_BIP:
            return "bip";
        case BACNET_LINK_MSTP:
            return "mstp";
        default:
            return NULL;
    }
}

void bacnet_coordinator_init(void)
{
    if (s_state.lock == NULL) {
        s_state.lock = xSemaphoreCreateMutex();
    }
    s_state.active_link_preference = BACNET_LINK_NONE;
    s_state.bip_ready = false;
    s_state.mstp_ready = false;
}

void bacnet_coordinator_set_active_preference(bacnet_link_t link)
{
    if (s_state.lock) {
        xSemaphoreTake(s_state.lock, portMAX_DELAY);
    }
    s_state.active_link_preference = link;
    if (s_state.lock) {
        xSemaphoreGive(s_state.lock);
    }
}

void bacnet_coordinator_set_link_ready(bacnet_link_t link, bool ready)
{
    if (s_state.lock) {
        xSemaphoreTake(s_state.lock, portMAX_DELAY);
    }

    if (link == BACNET_LINK_BIP) {
        s_state.bip_ready = ready;
    } else if (link == BACNET_LINK_MSTP) {
        s_state.mstp_ready = ready;
    }

    if (s_state.lock) {
        xSemaphoreGive(s_state.lock);
    }
}

void bacnet_coordinator_select_active_link(void)
{
    bacnet_link_t selected = BACNET_LINK_NONE;

    if (s_state.lock) {
        xSemaphoreTake(s_state.lock, portMAX_DELAY);
    }

    /* Policy: B/IP preferred over MS/TP. */
    if (s_state.bip_ready) {
        selected = BACNET_LINK_BIP;
    } else if (s_state.mstp_ready) {
        selected = BACNET_LINK_MSTP;
    }
    s_state.active_link_preference = selected;

    if (s_state.lock) {
        xSemaphoreGive(s_state.lock);
    }
}

void bacnet_coordinator_activate_link(bacnet_link_t link)
{
    const char *name = bacnet_name_from_link(link);
    if (name != NULL) {
        datalink_set((char *)name);
    }
}

void bacnet_coordinator_activate_link_name(const char *name)
{
    bacnet_link_t link = bacnet_link_from_name(name);
    if (link != BACNET_LINK_NONE) {
        bacnet_coordinator_activate_link(link);
    }
}

bacnet_link_t bacnet_get_active_link(void)
{
    bacnet_link_t link = BACNET_LINK_NONE;

    if (s_state.lock) {
        xSemaphoreTake(s_state.lock, portMAX_DELAY);
    }
    link = s_state.active_link_preference;
    if (s_state.lock) {
        xSemaphoreGive(s_state.lock);
    }

    return link;
}

bool bacnet_should_send_iam(void)
{
    bool should_send = true;

    if (s_state.lock) {
        xSemaphoreTake(s_state.lock, portMAX_DELAY);
    }
    /* Stub behavior for scaffolding only: always allow existing I-Am flow. */
    should_send = true;
    if (s_state.lock) {
        xSemaphoreGive(s_state.lock);
    }

    return should_send;
}
