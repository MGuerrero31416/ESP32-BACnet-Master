#include "adafruit_io_service.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "mqtt_client.h"

#include "User_Settings.h"

/* BACnet Analog Input access */
#include "bacnet/basic/object/ai.h"

/*
 * These variables will be declared in User_Settings.h and defined
 * through User_Private_Settings.h / User_Settings.c in the next step.
 *
 * Do not place the actual username or AIO key in this source file.
 */
//extern const char USER_AIO_USERNAME[];
//extern const char USER_AIO_KEY[];
// provided in User_Settings.h

static const char *TAG = "adafruit_io";

/*
 * Adafruit IO MQTT configuration.
 *
 * The Adafruit IO feed displayed as "SEN54-01" uses the feed key
 * "sen54-01".
 */
#define AIO_BROKER_URI "mqtts://io.adafruit.com:8883"


#define AIO_PUBLISH_INTERVAL_MS 60000U
#define AIO_SENSOR_STARTUP_DELAY_MS 8000U

#define AIO_PUBLISH_TASK_STACK_SIZE 4096U
#define AIO_PUBLISH_TASK_PRIORITY 3U

#define AIO_CONNECTED_BIT BIT0

#define AIO_TOPIC_BUFFER_SIZE 160U
#define AIO_PAYLOAD_BUFFER_SIZE 96U

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static EventGroupHandle_t s_mqtt_event_group = NULL;
static TaskHandle_t s_publish_task_handle = NULL;

static bool s_service_started = false;

static char s_feed_topic[AIO_TOPIC_BUFFER_SIZE];
static char s_errors_topic[AIO_TOPIC_BUFFER_SIZE];
static char s_throttle_topic[AIO_TOPIC_BUFFER_SIZE];

/**
 * @brief Check whether a private string setting is usable.
 */
static bool adafruit_io_setting_is_valid(const char *value)
{
    if (value == NULL || value[0] == '\0') {
        return false;
    }

    /*
     * Reject placeholder values from
     * User_Private_Settings.example.h.
     */
    if (strstr(value, "YOUR_") != NULL) {
        return false;
    }

    return true;
}

/**
 * @brief Build one Adafruit IO MQTT topic.
 */
static bool adafruit_io_build_topic(
    char *destination,
    size_t destination_size,
    const char *format,
    const char *username,
    const char *suffix)
{
    const int result = snprintf(
        destination,
        destination_size,
        format,
        username,
        suffix);

    return result >= 0 &&
           (size_t)result < destination_size;
}

/**
 * @brief Build all MQTT topics used by this service.
 */
static bool adafruit_io_build_topics(void)
{
    if (!adafruit_io_build_topic(
            s_feed_topic,
            sizeof(s_feed_topic),
            "%s/feeds/%s",
            USER_AIO_USERNAME,
            USER_AIO_FEED_KEY)) {
        return false;
    }

    if (!adafruit_io_build_topic(
            s_errors_topic,
            sizeof(s_errors_topic),
            "%s/%s",
            USER_AIO_USERNAME,
            "errors")) {
        return false;
    }

    if (!adafruit_io_build_topic(
            s_throttle_topic,
            sizeof(s_throttle_topic),
            "%s/%s",
            USER_AIO_USERNAME,
            "throttle")) {
        return false;
    }

    return true;
}

/**
 * @brief Subscribe to Adafruit IO diagnostic topics.
 */
static void adafruit_io_subscribe_diagnostics(
    esp_mqtt_client_handle_t client)
{
    int message_id = esp_mqtt_client_subscribe(
        client,
        s_errors_topic,
        0);

    if (message_id < 0) {
        ESP_LOGW(
            TAG,
            "Failed to subscribe to %s",
            s_errors_topic);
    }

    message_id = esp_mqtt_client_subscribe(
        client,
        s_throttle_topic,
        0);

    if (message_id < 0) {
        ESP_LOGW(
            TAG,
            "Failed to subscribe to %s",
            s_throttle_topic);
    }
}

/**
 * @brief Handle MQTT events.
 */
static void adafruit_io_mqtt_event_handler(
    void *handler_argument,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    (void)handler_argument;
    (void)event_base;

    esp_mqtt_event_handle_t event =
        (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(
                TAG,
                "Connecting to Adafruit IO");
            break;

        case MQTT_EVENT_CONNECTED:
            xEventGroupSetBits(
                s_mqtt_event_group,
                AIO_CONNECTED_BIT);

            ESP_LOGI(
                TAG,
                "Connected to Adafruit IO");

            ESP_LOGI(
                TAG,
                "Publishing to: %s",
                s_feed_topic);

            adafruit_io_subscribe_diagnostics(
                event->client);
            break;

        case MQTT_EVENT_DISCONNECTED:
            xEventGroupClearBits(
                s_mqtt_event_group,
                AIO_CONNECTED_BIT);

            ESP_LOGW(
                TAG,
                "Disconnected from Adafruit IO");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(
                TAG,
                "Subscription acknowledged, message ID=%d",
                event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(
                TAG,
                "Publish acknowledged, message ID=%d",
                event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            /*
             * MQTT event topic and data buffers are not guaranteed
             * to be null terminated. Use bounded string lengths.
             */
            ESP_LOGW(
                TAG,
                "Adafruit IO message: topic=%.*s data=%.*s",
                event->topic_len,
                event->topic != NULL ? event->topic : "",
                event->data_len,
                event->data != NULL ? event->data : "");
            break;

        case MQTT_EVENT_ERROR:
            xEventGroupClearBits(
                s_mqtt_event_group,
                AIO_CONNECTED_BIT);

            ESP_LOGE(
                TAG,
                "MQTT error event");
            break;

        default:
            break;
    }
}

/**
 * @brief Read the current SEN54 BACnet AI values and publish them.
 *
 * Payload format:
 *
 * v=1,t=24.37,h=56.20,p=8.40,o=112.00
 *
 * v = payload format version
 * t = SEN54 temperature
 * h = SEN54 relative humidity
 * p = SEN54 PM2.5
 * o = SEN54 VOC Index
 */
static void adafruit_io_publish_measurements(void)
{
    const uint32_t temperature_instance =
        user_ai_instance(
            USER_AI_SEN54_TEMPERATURE);

    const uint32_t humidity_instance =
        user_ai_instance(
            USER_AI_SEN54_HUMIDITY);

    const uint32_t pm25_instance =
        user_ai_instance(
            USER_AI_SEN54_PM2_5);

    const uint32_t voc_instance =
        user_ai_instance(
            USER_AI_SEN54_VOC_INDEX);

    const float temperature =
        Analog_Input_Present_Value(
            temperature_instance);

    const float humidity =
        Analog_Input_Present_Value(
            humidity_instance);

    const float pm25 =
        Analog_Input_Present_Value(
            pm25_instance);

    const float voc =
        Analog_Input_Present_Value(
            voc_instance);

    if (!isfinite(temperature) ||
        !isfinite(humidity) ||
        !isfinite(pm25) ||
        !isfinite(voc)) {
        ESP_LOGW(
            TAG,
            "Invalid sensor value; publish skipped");
        return;
    }

    char payload[AIO_PAYLOAD_BUFFER_SIZE];

    const int payload_length = snprintf(
        payload,
        sizeof(payload),
        "v=1,t=%.2f,h=%.2f,p=%.2f,o=%.2f",
        temperature,
        humidity,
        pm25,
        voc);

    if (payload_length < 0 ||
        (size_t)payload_length >= sizeof(payload)) {
        ESP_LOGE(
            TAG,
            "Telemetry payload buffer is too small");
        return;
    }

    /*
     * QoS 1 requests acknowledgement from Adafruit IO.
     * Retained publishing is disabled.
     */
    const int message_id = esp_mqtt_client_publish(
        s_mqtt_client,
        s_feed_topic,
        payload,
        payload_length,
        1,
        0);

    if (message_id < 0) {
        ESP_LOGW(
            TAG,
            "Failed to queue telemetry publish");
        return;
    }

    ESP_LOGI(
        TAG,
        "Published: %s",
        payload);
}

/**
 * @brief Periodic Adafruit IO publishing task.
 */
static void adafruit_io_publish_task(void *parameter)
{
    (void)parameter;

    ESP_LOGI(
        TAG,
        "Publishing task started");

    /*
     * Allow sensor_service to complete SEN54 startup and obtain
     * valid measurements before the first publish.
     */
    vTaskDelay(
        pdMS_TO_TICKS(
            AIO_SENSOR_STARTUP_DELAY_MS));

    for (;;) {
        const EventBits_t event_bits =
            xEventGroupGetBits(
                s_mqtt_event_group);

        if ((event_bits & AIO_CONNECTED_BIT) != 0U) {
            adafruit_io_publish_measurements();
        } else {
            ESP_LOGD(
                TAG,
                "Publish skipped: MQTT not connected");
        }

        vTaskDelay(
            pdMS_TO_TICKS(
                AIO_PUBLISH_INTERVAL_MS));
    }
}

bool adafruit_io_service_is_connected(void)
{
    if (s_mqtt_event_group == NULL) {
        return false;
    }

    const EventBits_t event_bits =
        xEventGroupGetBits(
            s_mqtt_event_group);

    return
        (event_bits & AIO_CONNECTED_BIT) != 0U;
}

esp_err_t adafruit_io_service_start(
    TaskHandle_t *task_handle)
{
    if (task_handle == NULL) {
        ESP_LOGE(
            TAG,
            "Task-handle reference is NULL");

        return ESP_ERR_INVALID_ARG;
    }

    if (s_service_started) {
        *task_handle = s_publish_task_handle;
        return ESP_OK;
    }

    *task_handle = NULL;

    if (!adafruit_io_setting_is_valid(
            USER_AIO_USERNAME)) {
        ESP_LOGE(
            TAG,
            "Adafruit IO username is not configured");

        return ESP_ERR_INVALID_STATE;
    }

    if (!adafruit_io_setting_is_valid(
            USER_AIO_KEY)) {
        ESP_LOGE(
            TAG,
            "Adafruit IO key is not configured");

        return ESP_ERR_INVALID_STATE;
    }

    if (!adafruit_io_build_topics()) {
        ESP_LOGE(
            TAG,
            "Failed to build MQTT topics");

        return ESP_ERR_INVALID_SIZE;
    }

    s_mqtt_event_group =
        xEventGroupCreate();

    if (s_mqtt_event_group == NULL) {
        ESP_LOGE(
            TAG,
            "Failed to create MQTT event group");

        return ESP_ERR_NO_MEM;
    }

    /*
     * When client_id is omitted, ESP-MQTT generates a client ID
     * from the ESP32 chip identity. Each physical ESP32 will
     * therefore use a different default MQTT client ID.
     */
    const esp_mqtt_client_config_t mqtt_configuration = {
        .broker = {
            .address = {
                .uri = AIO_BROKER_URI,
            },
            .verification = {
                .crt_bundle_attach =
                    esp_crt_bundle_attach,
            },
        },
        .credentials = {
            .username = USER_AIO_USERNAME,
            .authentication = {
                .password = USER_AIO_KEY,
            },
        },
    };

    s_mqtt_client =
        esp_mqtt_client_init(
            &mqtt_configuration);

    if (s_mqtt_client == NULL) {
        ESP_LOGE(
            TAG,
            "Failed to initialize MQTT client");

        vEventGroupDelete(
            s_mqtt_event_group);

        s_mqtt_event_group = NULL;

        return ESP_FAIL;
    }

    esp_err_t error =
        esp_mqtt_client_register_event(
            s_mqtt_client,
            ESP_EVENT_ANY_ID,
            adafruit_io_mqtt_event_handler,
            NULL);

    if (error != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to register MQTT event handler: %s",
            esp_err_to_name(error));

        esp_mqtt_client_destroy(
            s_mqtt_client);

        s_mqtt_client = NULL;

        vEventGroupDelete(
            s_mqtt_event_group);

        s_mqtt_event_group = NULL;

        return error;
    }

    error =
        esp_mqtt_client_start(
            s_mqtt_client);

    if (error != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to start MQTT client: %s",
            esp_err_to_name(error));

        esp_mqtt_client_destroy(
            s_mqtt_client);

        s_mqtt_client = NULL;

        vEventGroupDelete(
            s_mqtt_event_group);

        s_mqtt_event_group = NULL;

        return error;
    }

    const BaseType_t task_result =
        xTaskCreate(
            adafruit_io_publish_task,
            "aio_publish",
            AIO_PUBLISH_TASK_STACK_SIZE,
            NULL,
            AIO_PUBLISH_TASK_PRIORITY,
            &s_publish_task_handle);

    if (task_result != pdPASS) {
        ESP_LOGE(
            TAG,
            "Failed to create publishing task");

        esp_mqtt_client_stop(
            s_mqtt_client);

        esp_mqtt_client_destroy(
            s_mqtt_client);

        s_mqtt_client = NULL;

        vEventGroupDelete(
            s_mqtt_event_group);

        s_mqtt_event_group = NULL;

        return ESP_ERR_NO_MEM;
    }

    s_service_started = true;
    *task_handle = s_publish_task_handle;

    ESP_LOGI(
        TAG,
        "Adafruit IO service started");

    return ESP_OK;
}