#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* WiFi settings */
extern const bool USER_ENABLE_BACNET_IP;
extern const char USER_WIFI_SSID[];
extern const char USER_WIFI_PASS[];
extern const bool USER_WIFI_USE_STATIC_IP;
extern const char USER_WIFI_STATIC_IP_ADDR[];
extern const char USER_WIFI_STATIC_IP_GATEWAY[];
extern const char USER_WIFI_STATIC_IP_NETMASK[];
extern const char USER_WIFI_STATIC_DNS[];

/* Adafruit IO settings */
extern const bool USER_ENABLE_ADAFRUIT_IO;
extern const char USER_AIO_USERNAME[];
extern const char USER_AIO_KEY[];
extern const char USER_AIO_FEED_KEY[];
extern const uint32_t USER_AIO_PUBLISH_INTERVAL_SECONDS; // Adafruit IO publishing interval in seconds. 

/* BACnet device settings */
extern const char USER_BACNET_DEVICE_NAME[];
extern const uint32_t USER_BACNET_DEVICE_INSTANCE;
extern const int USER_OVERRIDE_NVS_ON_FLASH;

/* BBMD foreign device registration */
extern const uint8_t USER_BBMD_IP_OCTET_1;
extern const uint8_t USER_BBMD_IP_OCTET_2;
extern const uint8_t USER_BBMD_IP_OCTET_3;
extern const uint8_t USER_BBMD_IP_OCTET_4;
extern const uint16_t USER_BBMD_PORT;
extern const uint16_t USER_BBMD_TTL_SECONDS;

/* BACnet MS/TP settings */
extern const bool USER_ENABLE_BACNET_MSTP;
extern const uint8_t USER_MSTP_MAC_ADDRESS;
extern const uint8_t USER_MSTP_MAX_INFO_FRAMES;
extern const uint8_t USER_MSTP_MAX_MASTER;
extern const uint32_t USER_MSTP_BAUD_RATE;

/* BACnet object defaults */
#define USER_AV_COUNT 16
#define USER_BV_COUNT 4
#define USER_AI_COUNT 8
#define USER_BI_COUNT 4
#define USER_BO_COUNT 4

/*
 * Logical positions in the USER_AI_* parallel arrays.
 *
 * These are array indexes, not BACnet object instance numbers.
 * The actual instance numbers come from USER_AI_INSTANCES[].
 */
typedef enum {
    USER_AI_SEN54_TEMPERATURE = 0,
    USER_AI_SEN54_HUMIDITY,
    USER_AI_SEN54_VOC_INDEX,
    USER_AI_SEN54_PM1_0,
    USER_AI_SEN54_PM2_5,
    USER_AI_SEN54_PM4_0,
    USER_AI_SEN54_PM10,
    USER_AI_DS18B20_TEMPERATURE
} user_ai_role_t;

/*
 * Logical positions in the USER_BV_* parallel arrays.
 */
typedef enum {
    USER_BV_SEN54_FULL_RESET = 0
} user_bv_role_t;

/*
 * Logical positions in the USER_AV_* parallel arrays.
 */
typedef enum {
    USER_AV_SEN54_FAN_AUTO_CLEAN_INTERVAL = 0,
    USER_AV_SEN54_TEMP_COMP_OFFSET,
    USER_AV_SEN54_TEMP_COMP_SLOPE,
    USER_AV_SEN54_TEMP_COMP_TIME_CONSTANT,
    USER_AV_DS18B20_TEMP_OFFSET
} user_av_role_t;


extern const uint32_t USER_AV_INSTANCES[USER_AV_COUNT];
extern const char *USER_AV_NAMES[USER_AV_COUNT];
extern const char *USER_AV_DESCRIPTIONS[USER_AV_COUNT];
extern const uint16_t USER_AV_UNITS[USER_AV_COUNT];
extern const float USER_AV_INITIAL_VALUES[USER_AV_COUNT];
extern const float USER_AV_COV_INCREMENTS[USER_AV_COUNT];

extern const uint32_t USER_BV_INSTANCES[USER_BV_COUNT];
extern const char *USER_BV_NAMES[USER_BV_COUNT];
extern const char *USER_BV_DESCRIPTIONS[USER_BV_COUNT];
extern const char *USER_BV_ACTIVE_TEXT[USER_BV_COUNT];
extern const char *USER_BV_INACTIVE_TEXT[USER_BV_COUNT];
extern const uint8_t USER_BV_INITIAL_VALUES[USER_BV_COUNT];

extern const uint32_t USER_AI_INSTANCES[USER_AI_COUNT];
extern const char *USER_AI_NAMES[USER_AI_COUNT];
extern const char *USER_AI_DESCRIPTIONS[USER_AI_COUNT];
extern const uint16_t USER_AI_UNITS[USER_AI_COUNT];
extern const float USER_AI_INITIAL_VALUES[USER_AI_COUNT];
extern const float USER_AI_COV_INCREMENTS[USER_AI_COUNT];

extern const uint32_t USER_BI_INSTANCES[USER_BI_COUNT];
extern const char *USER_BI_NAMES[USER_BI_COUNT];
extern const char *USER_BI_DESCRIPTIONS[USER_BI_COUNT];
extern const char *USER_BI_ACTIVE_TEXT[USER_BI_COUNT];
extern const char *USER_BI_INACTIVE_TEXT[USER_BI_COUNT];
extern const uint8_t USER_BI_INITIAL_VALUES[USER_BI_COUNT];

extern const uint32_t USER_BO_INSTANCES[USER_BO_COUNT];
extern const char *USER_BO_NAMES[USER_BO_COUNT];
extern const char *USER_BO_DESCRIPTIONS[USER_BO_COUNT];
extern const char *USER_BO_ACTIVE_TEXT[USER_BO_COUNT];
extern const char *USER_BO_INACTIVE_TEXT[USER_BO_COUNT];
extern const uint8_t USER_BO_INITIAL_VALUES[USER_BO_COUNT];

static inline uint32_t user_ai_instance(
    user_ai_role_t role)
{
    return USER_AI_INSTANCES[(size_t)role];
}

static inline uint32_t user_bv_instance(
    user_bv_role_t role)
{
    return USER_BV_INSTANCES[(size_t)role];
}

static inline uint32_t user_av_instance(
    user_av_role_t role)
{
    return USER_AV_INSTANCES[(size_t)role];
}

#endif /* USER_SETTINGS_H */
