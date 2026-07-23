#include "User_Settings.h"
#include "bacnet/bacenum.h"

/* Private WiFi and Adafruit IO credentials are provided in User_Private_Settings.h */
#include "User_Private_Settings.h"
const bool USER_ENABLE_ADAFRUIT_IO = true; //  Enable Adafruit IO MQTT publishing service
const char USER_AIO_FEED_KEY[] = "sen54-01"; // Adafruit IO feed key for publishing SEN54 sensor data. This must exactly match the Feed Key shown in Adafruit IO


const bool USER_ENABLE_BACNET_IP = true;
const bool USER_WIFI_USE_STATIC_IP = false;
const char USER_WIFI_STATIC_IP_ADDR[] = "10.120.245.97";
const char USER_WIFI_STATIC_IP_GATEWAY[] = "10.120.245.254";
const char USER_WIFI_STATIC_IP_NETMASK[] = "255.255.255.0";
const char USER_WIFI_STATIC_DNS[] = "8.8.8.8";

/* BACnet device settings */
const char USER_BACNET_DEVICE_NAME[] = "ESP32-S3_55502";
const uint32_t USER_BACNET_DEVICE_INSTANCE = 55502;
const int USER_OVERRIDE_NVS_ON_FLASH = 0; // 0 = use NVS on flash, 1 = override NVS on flash with settings in this file

/* BBMD foreign device registration */
const uint8_t USER_BBMD_IP_OCTET_1 = 192;
const uint8_t USER_BBMD_IP_OCTET_2 = 168;
const uint8_t USER_BBMD_IP_OCTET_3 = 1;
const uint8_t USER_BBMD_IP_OCTET_4 = 1;
const uint16_t USER_BBMD_PORT = 0xBAC0;
const uint16_t USER_BBMD_TTL_SECONDS = 600;

/* BACnet MS/TP settings */
const bool USER_ENABLE_BACNET_MSTP = false;
const uint8_t USER_MSTP_MAC_ADDRESS = 21;
const uint8_t USER_MSTP_MAX_INFO_FRAMES = 80;
const uint8_t USER_MSTP_MAX_MASTER = 127;
const uint32_t USER_MSTP_BAUD_RATE = 38400U;

/* BACnet object defaults */
const uint32_t USER_AV_INSTANCES[USER_AV_COUNT] = {
    1, 2, 3, 4, 5, 6, 7, 8,
    9, 10, 11, 12, 13, 14, 15, 16
};
const char *USER_AV_NAMES[USER_AV_COUNT] = {
    "SEN54 Fan Auto Clean Interval",
    "SEN54 Temp Compensation Offset",
    "SEN54 Temp Compensation Slope",
    "SEN54 Temp Compensation Time Constant",
    "DS18B20 Temp Offset",
    "AV6",
    "AV7",
    "AV8",
    "AV9",
    "AV10",
    "AV11",
    "AV12",
    "AV13",
    "AV14",
    "AV15",
    "AV16"
};
const char *USER_AV_DESCRIPTIONS[USER_AV_COUNT] = {
    "SEN54 automatic fan cleaning interval (seconds, 0 disables)",
    "SEN54 temperature compensation offset (deg C)",
    "SEN54 normalized temperature compensation slope",
    "SEN54 temperature compensation time constant (seconds)",
    "DS18B20 temperature compensation offset",
    "Analog Value 6",
    "Analog Value 7",
    "Analog Value 8",
    "Analog Value 9",
    "Analog Value 10",
    "Analog Value 11",
    "Analog Value 12",
    "Analog Value 13",
    "Analog Value 14",
    "Analog Value 15",
    "Analog Value 16"
};
const uint16_t USER_AV_UNITS[USER_AV_COUNT] = {
    UNITS_SECONDS,
    UNITS_DEGREES_CELSIUS,
    UNITS_NO_UNITS,
    UNITS_SECONDS,
    UNITS_DEGREES_CELSIUS,
    UNITS_NO_UNITS,
    UNITS_NO_UNITS,
    UNITS_NO_UNITS,
    UNITS_NO_UNITS,
    UNITS_NO_UNITS,
    UNITS_NO_UNITS,
    UNITS_NO_UNITS,
    UNITS_NO_UNITS,
    UNITS_NO_UNITS,
    UNITS_NO_UNITS
};
const float USER_AV_INITIAL_VALUES[USER_AV_COUNT] = {
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f
};
const float USER_AV_COV_INCREMENTS[USER_AV_COUNT] = {
    1.0f,
    0.005f,
    0.0001f,
    1.0f,
    0.1f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f
};

const uint32_t USER_BV_INSTANCES[USER_BV_COUNT] = { 1, 2, 3, 4 };
const char *USER_BV_NAMES[USER_BV_COUNT] = {
    "SEN54_Full_Reset",
    "BV2",
    "BV3",
    "BV4"
};
const char *USER_BV_DESCRIPTIONS[USER_BV_COUNT] = {
    "Write ACTIVE to send I2C reset (0xD304) to SEN54",
    "Binary Value 2",
    "Binary Value 3",
    "Binary Value 4"
};
const char *USER_BV_ACTIVE_TEXT[USER_BV_COUNT] = {
    "RESETTING",
    "ACTIVE",
    "ACTIVE",
    "ACTIVE"
};
const char *USER_BV_INACTIVE_TEXT[USER_BV_COUNT] = {
    "IDLE",
    "INACTIVE",
    "INACTIVE",
    "INACTIVE"
};
const uint8_t USER_BV_INITIAL_VALUES[USER_BV_COUNT] = {
    BINARY_INACTIVE,
    BINARY_INACTIVE,
    BINARY_ACTIVE,
    BINARY_INACTIVE
};

const uint32_t USER_AI_INSTANCES[USER_AI_COUNT] = { 1, 2, 3, 4, 5, 6, 7, 8 };
const char *USER_AI_NAMES[USER_AI_COUNT] = {
    "SEN54 Temp",
    "SEN54 RH",
    "SEN54 VOC",
    "SEN54 PM1.0",
    "SEN54 PM2.5",
    "SEN54 PM4.0",
    "SEN54 PM10",
    "DS18B20 Temp"
};
const char *USER_AI_DESCRIPTIONS[USER_AI_COUNT] = {
    "SEN54 Temperature",
    "SEN54 Relative Humidity",
    "SEN54 VOC Index",
    "SEN54 PM1.0",
    "SEN54 PM2.5",
    "SEN54 PM4.0",
    "SEN54 PM10",
    "DS18B20 Temp sensor"
};
const uint16_t USER_AI_UNITS[USER_AI_COUNT] = {
    UNITS_DEGREES_CELSIUS,
    UNITS_PERCENT_RELATIVE_HUMIDITY,
    UNITS_NO_UNITS,
    UNITS_MICROGRAMS_PER_CUBIC_METER,
    UNITS_MICROGRAMS_PER_CUBIC_METER,
    UNITS_MICROGRAMS_PER_CUBIC_METER,
    UNITS_MICROGRAMS_PER_CUBIC_METER,
    UNITS_DEGREES_CELSIUS
};
const float USER_AI_INITIAL_VALUES[USER_AI_COUNT] = {
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f
};
const float USER_AI_COV_INCREMENTS[USER_AI_COUNT] = {
    0.1f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    0.1f
};

const uint32_t USER_BI_INSTANCES[USER_BI_COUNT] = { 1, 2, 3, 4 };
const char *USER_BI_NAMES[USER_BI_COUNT] = {
    "BI1",
    "BI2",
    "BI3",
    "BI4"
};
const char *USER_BI_DESCRIPTIONS[USER_BI_COUNT] = {
    "Binary Input 1",
    "Binary Input 2",
    "Binary Input 3",
    "Binary Input 4"
};
const char *USER_BI_ACTIVE_TEXT[USER_BI_COUNT] = {
    "ACTIVE",
    "ACTIVE",
    "ACTIVE",
    "ACTIVE"
};
const char *USER_BI_INACTIVE_TEXT[USER_BI_COUNT] = {
    "INACTIVE",
    "INACTIVE",
    "INACTIVE",
    "INACTIVE"
};
const uint8_t USER_BI_INITIAL_VALUES[USER_BI_COUNT] = {
    BINARY_INACTIVE,
    BINARY_INACTIVE,
    BINARY_INACTIVE,
    BINARY_INACTIVE
};

const uint32_t USER_BO_INSTANCES[USER_BO_COUNT] = { 1, 2, 3, 4 };
const char *USER_BO_NAMES[USER_BO_COUNT] = {
    "BO1",
    "BO2",
    "BO3",
    "BO4"
};
const char *USER_BO_DESCRIPTIONS[USER_BO_COUNT] = {
    "Binary Output 1",
    "Binary Output 2",
    "Binary Output 3",
    "Binary Output 4"
};
const char *USER_BO_ACTIVE_TEXT[USER_BO_COUNT] = {
    "ON",
    "ON",
    "ON",
    "ON"
};
const char *USER_BO_INACTIVE_TEXT[USER_BO_COUNT] = {
    "OFF",
    "OFF",
    "OFF",
    "OFF"
};
const uint8_t USER_BO_INITIAL_VALUES[USER_BO_COUNT] = {
    BINARY_INACTIVE,
    BINARY_INACTIVE,
    BINARY_INACTIVE,
    BINARY_INACTIVE
};
