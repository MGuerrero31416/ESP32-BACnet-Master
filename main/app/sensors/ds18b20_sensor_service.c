#include "ds18b20_sensor_service.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "User_Settings.h"
#include "ds18b20.h"

/* BACnet-stack headers */
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/av.h"

/*
 * Number of recent DS18B20 measurements included in the
 * rolling average.
 *
 * This does not need to be a power of two because the buffer
 * index uses the modulo operator.
 */
#define DS18B20_AVERAGE_SAMPLE_COUNT 8U //

/*
 * Minimum corrected-temperature change required before AI8
 * is updated.
 *
 * Offset changes are published immediately regardless of this
 * deadband.
 */
#define DS18B20_UPDATE_DEADBAND_C 0.05f

static const char *TAG = "ds18b20_service";

typedef struct {
    float samples[DS18B20_AVERAGE_SAMPLE_COUNT];
    float sum;
    size_t next_sample;
    size_t sample_count;
} ds18b20_average_filter_t;

static ds18b20_average_filter_t ds18b20_filter;

/*
 * Last temperature actually published to the BACnet AI.
 * Small changes are compared against this value.
 */
static float last_published_temperature;

/*
 * Last offset used during a successful cycle.
 * Tracking it separately allows BACnet offset changes to take
 * effect immediately, even when the resulting temperature
 * change is smaller than the normal deadband.
 */
static float last_temperature_offset;

_Static_assert(
    DS18B20_AVERAGE_SAMPLE_COUNT > 0U,
    "DS18B20_AVERAGE_SAMPLE_COUNT must be greater than zero");

/*
 * Ensure User_Settings contains the BACnet objects required
 * by the DS18B20 service.
 */
_Static_assert(
    USER_AI_COUNT > USER_AI_DS18B20_TEMPERATURE,
    "USER_AI_COUNT must provide the DS18B20 Analog Input");

_Static_assert(
    USER_AV_COUNT > USER_AV_DS18B20_TEMP_OFFSET,
    "USER_AV_COUNT must provide the DS18B20 offset Analog Value");

/*
 * Add one valid measurement to the rolling average.
 *
 * During startup, the average uses only the samples collected
 * so far. The BACnet AI can therefore update immediately
 * without waiting for the full sample window.
 */
static float ds18b20_average_update(
    ds18b20_average_filter_t *filter,
    float new_sample)
{
    if (filter->sample_count <
        DS18B20_AVERAGE_SAMPLE_COUNT) {

        filter->samples[filter->next_sample] =
            new_sample;

        filter->sum += new_sample;
        filter->sample_count++;
    } else {
        filter->sum -=
            filter->samples[filter->next_sample];

        filter->samples[filter->next_sample] =
            new_sample;

        filter->sum += new_sample;
    }

    filter->next_sample =
        (filter->next_sample + 1U) %
        DS18B20_AVERAGE_SAMPLE_COUNT;

    return filter->sum /
        (float)filter->sample_count;
}

esp_err_t ds18b20_sensor_service_init(void)
{
    /*
     * The low-level DS18B20 driver performs lazy GPIO
     * initialization on its first read.
     */
    memset(
        &ds18b20_filter,
        0,
        sizeof(ds18b20_filter));

    /*
     * Force the first valid measurement and first valid offset
     * to be published.
     */
    last_published_temperature = NAN;
    last_temperature_offset = NAN;

    ESP_LOGI(
        TAG,
        "DS18B20 service initialized: AI%lu offset AV%lu "
        "average=%u deadband=%.3f C",
        (unsigned long)user_ai_instance(
            USER_AI_DS18B20_TEMPERATURE),
        (unsigned long)user_av_instance(
            USER_AV_DS18B20_TEMP_OFFSET),
        (unsigned int)DS18B20_AVERAGE_SAMPLE_COUNT,
        (double)DS18B20_UPDATE_DEADBAND_C);

    return ESP_OK;
}

void ds18b20_sensor_service_cycle(void)
{
    const uint32_t temperature_ai =
        user_ai_instance(
            USER_AI_DS18B20_TEMPERATURE);

    const uint32_t offset_av =
        user_av_instance(
            USER_AV_DS18B20_TEMP_OFFSET);

    float raw_temperature = 0.0f;

    if (!ds18b20_read_temperature(
            &raw_temperature)) {

        /*
         * Do not add a failed measurement to the moving
         * average. Retain the last valid AI value and mark
         * it unreliable.
         */
        Analog_Input_Reliability_Set(
            temperature_ai,
            RELIABILITY_UNRELIABLE_OTHER);

        ESP_LOGW(TAG, "DS18B20 read failed");

        return;
    }

    float filtered_temperature =
        ds18b20_average_update(
            &ds18b20_filter,
            raw_temperature);

    float temperature_offset =
        Analog_Value_Present_Value(offset_av);

    if (!isfinite(temperature_offset)) {
        temperature_offset = 0.0f;
    }

    /*
     * Apply the BACnet offset after averaging. Changes to the
     * offset therefore take effect immediately instead of
     * becoming part of the moving-average history.
     */
    float corrected_temperature =
        filtered_temperature +
        temperature_offset;

    /*
     * Publish when:
     *
     * 1. This is the first valid measurement.
     * 2. The BACnet offset has changed.
     * 3. The corrected temperature has moved by at least the
     *    configured deadband from the last published value.
     */
    bool publish_temperature =
        !isfinite(last_published_temperature) ||
        !isfinite(last_temperature_offset) ||
        temperature_offset != last_temperature_offset ||
        fabsf(
            corrected_temperature -
            last_published_temperature) >=
            DS18B20_UPDATE_DEADBAND_C;

    if (publish_temperature) {
        Analog_Input_Present_Value_Set(
            temperature_ai,
            corrected_temperature);

        last_published_temperature =
            corrected_temperature;
    }

    /*
     * Store the offset after the publication decision so that
     * an offset change is detected during the current cycle.
     */
    last_temperature_offset =
        temperature_offset;

    Analog_Input_Reliability_Set(
        temperature_ai,
        RELIABILITY_NO_FAULT_DETECTED);

    ESP_LOGD(
        TAG,
        "DS18B20: raw=%.3f C average=%.3f C "
        "offset=%.2f C corrected=%.3f C "
        "published=%s",
        raw_temperature,
        filtered_temperature,
        temperature_offset,
        corrected_temperature,
        publish_temperature ? "yes" : "no");
}