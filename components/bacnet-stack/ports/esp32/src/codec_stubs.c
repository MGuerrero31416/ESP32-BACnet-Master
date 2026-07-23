/* Stub implementations for optional BACnet codec decoders */
#include <stdint.h>
#include <string.h>
#include "bacnet/bacdef.h"
#include "bacnet/lighting.h"
#include "bacnet/weeklyschedule.h"
#include "bacnet/calendar_entry.h"
#include "bacnet/special_event.h"
#include "bacnet/hostnport.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/bacdest.h"
#include "bacnet/bacaction.h"
#include "bacnet/shed_level.h"
#include "bacnet/access_rule.h"
#include "bacnet/channel_value.h"
#include "bacnet/timer_value.h"
#include "bacnet/baclog.h"
#include "bacnet/secure_connect.h"

/* Lighting command decoder stub */
int lighting_command_decode(
    const uint8_t *apdu, unsigned int apdu_len, BACNET_LIGHTING_COMMAND *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;  /* 0 bytes decoded */
}

/* XY color decoder stub */
int xy_color_decode(
    const uint8_t *apdu, uint32_t apdu_len, BACNET_XY_COLOR *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Color command decoder stub */
int color_command_decode(
    const uint8_t *apdu,
    uint16_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_COLOR_COMMAND *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Weekly schedule decoder stub */
int bacnet_weeklyschedule_decode(
    const uint8_t *apdu, int apdu_len, BACNET_WEEKLY_SCHEDULE *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Calendar entry decoder stub */
int bacnet_calendar_entry_decode(
    const uint8_t *apdu, uint32_t apdu_len, BACNET_CALENDAR_ENTRY *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Special event decoder stub */
int bacnet_special_event_decode(
    const uint8_t *apdu, int apdu_len, BACNET_SPECIAL_EVENT *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Host N Port decoder stub */
int host_n_port_decode(
    const uint8_t *apdu,
    uint32_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_HOST_N_PORT *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Destination decoder stub */
int bacnet_destination_decode(
    const uint8_t *apdu, int apdu_len, BACNET_DESTINATION *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* BDT entry decoder stub */
int bacnet_bdt_entry_decode(
    const uint8_t *apdu,
    uint32_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_BDT_ENTRY *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* FDT entry decoder stub */
int bacnet_fdt_entry_decode(
    const uint8_t *apdu,
    uint32_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_FDT_ENTRY *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Action command decoder stub */
int bacnet_action_command_decode(
    const uint8_t *apdu, size_t apdu_len, BACNET_ACTION_LIST *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Shed level decoder stub */
int bacnet_shed_level_decode(
    const uint8_t *apdu, size_t apdu_len, BACNET_SHED_LEVEL *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Access rule decoder stub */
int bacnet_access_rule_decode(
    const uint8_t *apdu, size_t apdu_len, BACNET_ACCESS_RULE *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Channel value decoder stub */
int bacnet_channel_value_decode(
    const uint8_t *apdu, size_t apdu_len, BACNET_CHANNEL_VALUE *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Timer value decoder stub */
int bacnet_timer_value_decode(
    const uint8_t *apdu, size_t apdu_len, BACNET_TIMER_STATE_CHANGE_VALUE *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Recipient decoder stub */
int bacnet_recipient_decode(
    const uint8_t *apdu, int apdu_len, BACNET_RECIPIENT *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* Log record decoder stub */
int bacnet_log_record_decode(
    const uint8_t *apdu, size_t apdu_len, BACNET_LOG_RECORD *value)
{
    (void)apdu;
    (void)apdu_len;
    (void)value;
    return 0;
}

/* SC Failed Connection Request decoder stub */
int bacapp_decode_SCFailedConnectionRequest(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_SC_FAILED_CONNECTION_REQUEST *value)
{
    (void)apdu;
    (void)apdu_size;
    (void)value;
    return 0;
}

/* SC Hub Function Connection decoder stub */
int bacapp_decode_SCHubFunctionConnection(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value)
{
    (void)apdu;
    (void)apdu_size;
    (void)value;
    return 0;
}

/* SC Direct Connection decoder stub */
int bacapp_decode_SCDirectConnection(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_SC_DIRECT_CONNECTION_STATUS *value)
{
    (void)apdu;
    (void)apdu_size;
    (void)value;
    return 0;
}

/* SC Hub Connection decoder stub */
int bacapp_decode_SCHubConnection(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_SC_HUB_CONNECTION_STATUS *value)
{
    (void)apdu;
    (void)apdu_size;
    (void)value;
    return 0;
}

/* Encoder stubs for optional BACnet datatypes */
int lighting_command_encode(uint8_t *apdu, const BACNET_LIGHTING_COMMAND *data)
{
    (void)apdu;
    (void)data;
    return 0;
}

int xy_color_encode(uint8_t *apdu, const BACNET_XY_COLOR *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int color_command_encode(uint8_t *apdu, const BACNET_COLOR_COMMAND *address)
{
    (void)apdu;
    (void)address;
    return 0;
}

int bacnet_weeklyschedule_encode(
    uint8_t *apdu, const BACNET_WEEKLY_SCHEDULE *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int bacnet_calendar_entry_encode(
    uint8_t *apdu, const BACNET_CALENDAR_ENTRY *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int bacnet_special_event_encode(
    uint8_t *apdu, const BACNET_SPECIAL_EVENT *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int host_n_port_encode(uint8_t *apdu, const BACNET_HOST_N_PORT *address)
{
    (void)apdu;
    (void)address;
    return 0;
}

int bacnet_destination_encode(
    uint8_t *apdu, const BACNET_DESTINATION *destination)
{
    (void)apdu;
    (void)destination;
    return 0;
}

int bacnet_bdt_entry_encode(uint8_t *apdu, const BACNET_BDT_ENTRY *entry)
{
    (void)apdu;
    (void)entry;
    return 0;
}

int bacnet_fdt_entry_encode(uint8_t *apdu, const BACNET_FDT_ENTRY *entry)
{
    (void)apdu;
    (void)entry;
    return 0;
}

int bacnet_action_command_encode(
    uint8_t *apdu, const BACNET_ACTION_LIST *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int bacnet_shed_level_encode(uint8_t *apdu, const BACNET_SHED_LEVEL *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int bacapp_encode_access_rule(uint8_t *apdu, const BACNET_ACCESS_RULE *rule)
{
    (void)apdu;
    (void)rule;
    return 0;
}

int bacnet_channel_value_type_encode(
    uint8_t *apdu, const BACNET_CHANNEL_VALUE *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int bacnet_timer_value_type_encode(
    uint8_t *apdu, const BACNET_TIMER_STATE_CHANGE_VALUE *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int bacnet_recipient_encode(uint8_t *apdu, const BACNET_RECIPIENT *recipient)
{
    (void)apdu;
    (void)recipient;
    return 0;
}

int bacnet_timer_value_no_value_encode(uint8_t *apdu)
{
    (void)apdu;
    return 0;
}

int bacnet_log_record_value_encode(
    uint8_t *apdu, const BACNET_LOG_RECORD *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int bacapp_encode_SCFailedConnectionRequest(
    uint8_t *apdu, const BACNET_SC_FAILED_CONNECTION_REQUEST *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int bacapp_encode_SCHubFunctionConnection(
    uint8_t *apdu, const BACNET_SC_HUB_FUNCTION_CONNECTION_STATUS *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int bacapp_encode_SCDirectConnection(
    uint8_t *apdu, const BACNET_SC_DIRECT_CONNECTION_STATUS *value)
{
    (void)apdu;
    (void)value;
    return 0;
}

int bacapp_encode_SCHubConnection(
    uint8_t *apdu, const BACNET_SC_HUB_CONNECTION_STATUS *value)
{
    (void)apdu;
    (void)value;
    return 0;
}
