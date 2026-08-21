#include "User_Settings.h"
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/device.h"
#include "esp_log.h"

#include <inttypes.h>
#include <string.h>

/* Private WiFi and Adafruit IO credentials are provided in User_Private_Settings.h */
#include "User_Private_Settings.h"
const bool USER_ENABLE_ADAFRUIT_IO = false; //  Enable Adafruit IO MQTT publishing service
const char USER_AIO_FEED_KEY[] = "sen54-01"; // Adafruit IO feed key for publishing SEN54 sensor data. This must exactly match the Feed Key shown in Adafruit IO


const bool USER_ENABLE_BACNET_IP = true;
const bool USER_WIFI_USE_STATIC_IP = false;
const char USER_WIFI_STATIC_IP_ADDR[] = "10.120.245.97";
const char USER_WIFI_STATIC_IP_GATEWAY[] = "10.120.245.254";
const char USER_WIFI_STATIC_IP_NETMASK[] = "255.255.255.0";
const char USER_WIFI_STATIC_DNS[] = "8.8.8.8";

/* BACnet device settings */
const char USER_BACNET_DEVICE_NAME[] = "ESP32_55533";
const uint32_t USER_BACNET_DEVICE_INSTANCE = 55533;
const int USER_OVERRIDE_NVS_ON_FLASH = 0; // 0 = use NVS on flash, 1 = override NVS on flash with settings in this file

/* BACnet device identity settings */
const char USER_BACNET_DEVICE_DESCRIPTION[] = "ESP32 BACnet Master";
const char USER_BACNET_MODEL_NAME[] = "ESP32-WROOM32-SEN54-ST7789";
const char USER_BACNET_VENDOR_NAME[] = "ESCAP FMS";
const uint16_t USER_BACNET_VENDOR_ID = 260;
const char USER_BACNET_LOCATION[] = "SEC-B Ground Floor FMS";
const char USER_BACNET_FIRMWARE_REVISION[] = "2.5a 2026_08_20";
// V2.5a added LVGL Lilygo T-Display-S3 Touch.
const char *USER_BACNET_APPLICATION_SOFTWARE_VERSION = USER_BACNET_FIRMWARE_REVISION;
const char USER_BACNET_SERIAL_NUMBER[] = "ESP32-55533-0001"; //CHANGE ME UNIQUE PER DEVICE

/* BACnet MS/TP settings */
const bool USER_ENABLE_BACNET_MSTP = false;
const uint8_t USER_MSTP_MAC_ADDRESS = 33;
const uint8_t USER_MSTP_MAX_INFO_FRAMES = 1;
const uint8_t USER_MSTP_MAX_MASTER = 34;
const uint32_t USER_MSTP_BAUD_RATE = 38400U;

/* BBMD foreign device registration */
const uint8_t USER_BBMD_IP_OCTET_1 = 192;
const uint8_t USER_BBMD_IP_OCTET_2 = 168;
const uint8_t USER_BBMD_IP_OCTET_3 = 1;
const uint8_t USER_BBMD_IP_OCTET_4 = 1;
const uint16_t USER_BBMD_PORT = 0xBAC0;
const uint16_t USER_BBMD_TTL_SECONDS = 600;

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
    1.0f,
    1.0f
};

const uint32_t USER_BV_INSTANCES[USER_BV_COUNT] = { 1, 2, 3, 4 };
const char *USER_BV_NAMES[USER_BV_COUNT] = {
    "SEN54 Full Reset",
    "SEN54 Measurement Enable",
    "SEN54 Start Fan Cleaning",
    "SEN54 Clear Status"
};
const char *USER_BV_DESCRIPTIONS[USER_BV_COUNT] = {
    "Send a full reset and reapply saved SEN54 configuration",
    "Keeps measurement enabled, INACTIVE stops measurement",
    "Start a manual SEN54 fan cleaning cycle",
    "Read-and-clear SEN54 sticky status flags"
};
const char *USER_BV_ACTIVE_TEXT[USER_BV_COUNT] = {
    "RESETTING",
    "ENABLED",
    "CLEANING",
    "CLEARING"
};
const char *USER_BV_INACTIVE_TEXT[USER_BV_COUNT] = {
    "IDLE",
    "DISABLED",
    "IDLE",
    "IDLE"
};
const uint8_t USER_BV_INITIAL_VALUES[USER_BV_COUNT] = {
    BINARY_INACTIVE,
    BINARY_ACTIVE,
    BINARY_INACTIVE,
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
    "SEN54 Fan Failure",
    "SEN54 Laser Error",
    "SEN54 VOC Sensor Error",
    "SEN54 RHT Sensor Error"
};
const char *USER_BI_DESCRIPTIONS[USER_BI_COUNT] = {
    "SEN54 fan error",
    "SEN54 laser error",
    "SEN54 VOC sensor (SGP) error",
    "ACTIVE RHT sensor (SHT) error "
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

#if USER_SETTINGS_PRINT_ENABLE
static const char *TAG_USER_SETTINGS = "user_settings";
#endif

void User_Settings_InitDeviceIdentity(void)
{
    (void)Device_Object_Name_ANSI_Init(
        USER_BACNET_DEVICE_NAME);
    (void)Device_Set_Description(
        USER_BACNET_DEVICE_DESCRIPTION,
        strlen(USER_BACNET_DEVICE_DESCRIPTION));
    (void)Device_Set_Model_Name(
        USER_BACNET_MODEL_NAME,
        strlen(USER_BACNET_MODEL_NAME));
    Device_Set_Vendor_Identifier(
        USER_BACNET_VENDOR_ID);
    (void)Device_Set_Location(
        USER_BACNET_LOCATION,
        strlen(USER_BACNET_LOCATION));
    (void)Device_Set_Firmware_Revision(
        USER_BACNET_FIRMWARE_REVISION,
        strlen(USER_BACNET_FIRMWARE_REVISION));
    (void)Device_Set_Application_Software_Version(
        USER_BACNET_APPLICATION_SOFTWARE_VERSION,
        strlen(USER_BACNET_APPLICATION_SOFTWARE_VERSION));
    (void)Device_Serial_Number_Set(
        USER_BACNET_SERIAL_NUMBER,
        strlen(USER_BACNET_SERIAL_NUMBER));
}

void User_Settings_Print(void)
{
#if USER_SETTINGS_PRINT_ENABLE
    ESP_LOGI(TAG_USER_SETTINGS, "========================================================");
    ESP_LOGI(TAG_USER_SETTINGS, "===================== User Settings ====================");

        /* Display hardware profile selected in menuconfig */
    #if defined(CONFIG_USER_DISPLAY_ST7796S)
        ESP_LOGI(TAG_USER_SETTINGS, "Display Hardware Profile: ST7796S 480x320 3.5in - colorful UI");
    #elif defined(CONFIG_USER_DISPLAY_ST7796S_TEST)
        ESP_LOGI(TAG_USER_SETTINGS, "Display Hardware Profile: ST7796S 480x320 - test UI");
    #elif defined(CONFIG_USER_DISPLAY_ST7789_TDISPLAY_S3)
        ESP_LOGI(TAG_USER_SETTINGS, "Display Hardware Profile: ST7789 170x320 - LilyGO T-Display-S3 Touch");
    #elif defined(CONFIG_USER_DISPLAY_LVGL_TDISPLAY_S3)
        ESP_LOGI(TAG_USER_SETTINGS, "Display Hardware Profile: LVGL T-Display-S3 - LilyGO LVGL UI");
    #elif defined(CONFIG_USER_DISPLAY_ST7789_GMT020)
        ESP_LOGI(TAG_USER_SETTINGS, "Display Hardware Profile: ST7789 240x320 - GMT020-02-7P");
    #elif defined(CONFIG_USER_DISPLAY_ST7789_HW657A)
        ESP_LOGI(TAG_USER_SETTINGS, "Display Hardware Profile: ST7789 170x320 - HW657A - Simple UI");
    #elif defined(CONFIG_USER_DISPLAY_NONE)
        ESP_LOGI(TAG_USER_SETTINGS, "Display Hardware Profile: None");
    #else
        ESP_LOGI(TAG_USER_SETTINGS, "Display Hardware Profile: (unknown)");
    #endif
    ESP_LOGI(TAG_USER_SETTINGS, "========================================================");
    ESP_LOGI(TAG_USER_SETTINGS, "[Wi-Fi / BACnet-IP]");
    ESP_LOGI(TAG_USER_SETTINGS, "USER_ENABLE_BACNET_IP: %s", USER_ENABLE_BACNET_IP ? "true" : "false");
    ESP_LOGI(TAG_USER_SETTINGS, "USER_WIFI_SSID: %s", USER_WIFI_SSID);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_WIFI_PASS: ****");
    ESP_LOGI(TAG_USER_SETTINGS, "USER_WIFI_USE_STATIC_IP: %s", USER_WIFI_USE_STATIC_IP ? "true" : "false");
    ESP_LOGI(TAG_USER_SETTINGS, "USER_WIFI_STATIC_IP_ADDR: %s", USER_WIFI_STATIC_IP_ADDR);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_WIFI_STATIC_IP_GATEWAY: %s", USER_WIFI_STATIC_IP_GATEWAY);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_WIFI_STATIC_IP_NETMASK: %s", USER_WIFI_STATIC_IP_NETMASK);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_WIFI_STATIC_DNS: %s", USER_WIFI_STATIC_DNS);
    ESP_LOGI(TAG_USER_SETTINGS, "========================================================");
    ESP_LOGI(TAG_USER_SETTINGS, "[Adafruit IO]");
    ESP_LOGI(TAG_USER_SETTINGS, "USER_ENABLE_ADAFRUIT_IO: %s", USER_ENABLE_ADAFRUIT_IO ? "true" : "false");
    ESP_LOGI(TAG_USER_SETTINGS, "USER_AIO_USERNAME: %s", USER_AIO_USERNAME);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_AIO_KEY: ****");
    ESP_LOGI(TAG_USER_SETTINGS, "USER_AIO_FEED_KEY: %s", USER_AIO_FEED_KEY);
    ESP_LOGI(TAG_USER_SETTINGS, "========================================================");
    ESP_LOGI(TAG_USER_SETTINGS, "[BACnet Device]");
    ESP_LOGI(TAG_USER_SETTINGS, "USER_BACNET_DEVICE_NAME: %s", USER_BACNET_DEVICE_NAME);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_BACNET_DEVICE_DESCRIPTION: %s", USER_BACNET_DEVICE_DESCRIPTION);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_BACNET_MODEL_NAME: %s", USER_BACNET_MODEL_NAME);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_BACNET_VENDOR_NAME: %s", USER_BACNET_VENDOR_NAME);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_BACNET_VENDOR_ID: %" PRIu16, USER_BACNET_VENDOR_ID);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_BACNET_LOCATION: %s", USER_BACNET_LOCATION);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_BACNET_FIRMWARE_REVISION: %s", USER_BACNET_FIRMWARE_REVISION);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_BACNET_APPLICATION_SOFTWARE_VERSION: %s", USER_BACNET_APPLICATION_SOFTWARE_VERSION);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_BACNET_SERIAL_NUMBER: %s", USER_BACNET_SERIAL_NUMBER);
    ESP_LOGI(
        TAG_USER_SETTINGS,
        "USER_BACNET_DEVICE_INSTANCE: %" PRIu32,
        USER_BACNET_DEVICE_INSTANCE);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_OVERRIDE_NVS_ON_FLASH: %d", USER_OVERRIDE_NVS_ON_FLASH);

    ESP_LOGI(TAG_USER_SETTINGS, "[BBMD]");
    ESP_LOGI(
        TAG_USER_SETTINGS,
        "USER_BBMD_IP: %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8,
        USER_BBMD_IP_OCTET_1,
        USER_BBMD_IP_OCTET_2,
        USER_BBMD_IP_OCTET_3,
        USER_BBMD_IP_OCTET_4);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_BBMD_PORT: %" PRIu16, USER_BBMD_PORT);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_BBMD_TTL_SECONDS: %" PRIu16, USER_BBMD_TTL_SECONDS);

    ESP_LOGI(TAG_USER_SETTINGS, "[BACnet MS/TP]");
    ESP_LOGI(TAG_USER_SETTINGS, "USER_ENABLE_BACNET_MSTP: %s", USER_ENABLE_BACNET_MSTP ? "true" : "false");
    ESP_LOGI(TAG_USER_SETTINGS, "USER_MSTP_MAC_ADDRESS: %" PRIu8, USER_MSTP_MAC_ADDRESS);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_MSTP_MAX_INFO_FRAMES: %" PRIu8, USER_MSTP_MAX_INFO_FRAMES);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_MSTP_MAX_MASTER: %" PRIu8, USER_MSTP_MAX_MASTER);
    ESP_LOGI(TAG_USER_SETTINGS, "USER_MSTP_BAUD_RATE: %" PRIu32, USER_MSTP_BAUD_RATE);

    ESP_LOGI(TAG_USER_SETTINGS, "========================================================");
    ESP_LOGI(TAG_USER_SETTINGS, "========================================================");
#endif
}
