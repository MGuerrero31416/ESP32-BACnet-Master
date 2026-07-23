/* Minimal stubs for optional BACnet features */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "bacnet/bacdef.h"
#include "bacnet/datetime.h"
#include "bacnet/wp.h"
#include "bacnet/apdu.h"
#include "bacnet/basic/sys/mstimer.h"

/* datetime_local - used by device.c Update_Current_Time() 
   Returns: bool, sets date and time pointers, UTC offset and daylight saving */
bool datetime_local(
    BACNET_DATE *date,
    BACNET_TIME *time,
    int16_t *utc_offset_minutes,
    bool *is_dst)
{
    if (date) {
        date->year = 2024;
        date->month = 1;
        date->day = 1;
    }
    if (time) {
        time->hour = 0;
        time->min = 0;
        time->sec = 0;
        time->hundredths = 0;
    }
    if (utc_offset_minutes) {
        *utc_offset_minutes = 0;
    }
    if (is_dst) {
        *is_dst = false;
    }
    return true;
}

/* datetime_init - minimal stub */
void datetime_init(void)
{
    /* No-op for minimal build */
}

/* Network_Port stubs - optional network port object */
uint32_t Network_Port_Index_To_Instance(unsigned index)
{
    (void)index;
    return 0;  /* No network port objects */
}

/* handler_unrecognized_service - handles unrecognized services */
void handler_unrecognized_service(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    (void)service_request;
    (void)service_len;
    (void)src;
    (void)service_data;
    /* Silently ignore unrecognized services */
}

void debug_fprintf(void *stream, const char *format, ...)
{
    (void)stream;
    if (!format) {
        return;
    }
    if (strncmp(format, "WP:", 3) != 0 &&
        strncmp(format, "AV Write", 8) != 0) {
        return;
    }
    char buffer[160];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ESP_LOGW("bacnet_wp", "%s", buffer);
}

void debug_printf(const char *format, ...)
{
    if (!format) {
        return;
    }
    if (strncmp(format, "WP:", 3) != 0 &&
        strncmp(format, "AV Write", 8) != 0) {
        return;
    }
    char buffer[160];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ESP_LOGW("bacnet_wp", "%s", buffer);
}

void debug_printf_disabled(const char *format, ...)
{
    (void)format;
    /* No-op for minimal build */
}

/* mstimer_now - returns millisecond tick count */
unsigned long mstimer_now(void)
{
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}
