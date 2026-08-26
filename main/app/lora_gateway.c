/*
 * LoRa packet format (little-endian, packed, 26 bytes):
 *   0: VERSION (uint8_t)
 *   1-4: DEVICE_ID (uint32_t)
 *   5-8: SEQUENCE (uint32_t)
 *   9-12: TEMPERATURE (float IEEE-754)
 *   13-16: HUMIDITY (float IEEE-754)
 *   17-20: PM2_5 (float IEEE-754)
 *   21-24: VOC (float IEEE-754)
 *   25: STATUS (uint8_t)
 *
 * Validation rules:
 *   - CRC must pass before radio receive callback is accepted;
 *   - length must be exactly 26 bytes;
 *   - version must match the supported gateway protocol version;
 *   - device id must match the configured gateway sensor ID;
 *   - sequence must be strictly newer than the last accepted packet;
 *   - temperature, humidity, PM2.5, and VOC values must be in sane ranges.
 */
#include "lora_gateway.h"

#include "User_Settings.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "platform/lora_hal.h"
#include "platform/sx1262.h"

#include "bacnet/basic/object/ai.h"

#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define TAG "lora_gateway"

static const lora_radio_config_t g_lora_radio = {
    .frequency_hz = 923000000UL,
    .bandwidth = 0x04,
    .spreading_factor = 10,
    .coding_rate = 0x01,
    .preamble_length = 12,
    .tx_power_dbm = 22,
    .packet_max_len = LORA_PACKET_MAX_LEN,
};

static uint32_t g_reject_counter[8] = {0U};

static bool lora_range_valid(float value, float min, float max)
{
    return isfinite(value) && value >= min && value <= max;
}

static bool lora_sequence_is_newer(uint32_t sequence, uint32_t last_accepted_sequence)
{
    if (sequence == last_accepted_sequence) {
        return false;
    }

    if (sequence > last_accepted_sequence) {
        return (sequence - last_accepted_sequence) < 0x80000000u;
    }

    return (last_accepted_sequence - sequence) > 0x80000000u;
}

bool validate_lora_packet(
    const uint8_t *packet,
    size_t packet_len,
    uint32_t expected_device_id,
    uint32_t expected_version,
    uint32_t last_accepted_sequence,
    lora_packet_reason_t *reason)
{
    if (reason == NULL) {
        return false;
    }

    *reason = LORA_PACKET_ACCEPTED;

    if (packet == NULL || packet_len == 0U) {
        *reason = LORA_PACKET_REJECT_PARSE;
        g_reject_counter[LORA_PACKET_REJECT_PARSE]++;
        ESP_LOGW(TAG, "reject parse: null packet or empty payload");
        return false;
    }

    if (packet_len != LORA_GATEWAY_PACKET_LEN) {
        *reason = LORA_PACKET_REJECT_LENGTH;
        g_reject_counter[LORA_PACKET_REJECT_LENGTH]++;
        ESP_LOGW(TAG, "reject length=%zu expected=%u", packet_len, LORA_GATEWAY_PACKET_LEN);
        return false;
    }

    if (packet[0] != (uint8_t)expected_version) {
        *reason = LORA_PACKET_REJECT_VERSION;
        g_reject_counter[LORA_PACKET_REJECT_VERSION]++;
        ESP_LOGW(TAG, "reject version=%u expected=%u", packet[0], expected_version);
        return false;
    }

    uint32_t device_id = 0U;
    memcpy(&device_id, &packet[1], sizeof(device_id));
    if (device_id != expected_device_id) {
        *reason = LORA_PACKET_REJECT_DEVICE_ID;
        g_reject_counter[LORA_PACKET_REJECT_DEVICE_ID]++;
        ESP_LOGW(TAG, "reject device_id=%" PRIu32 " expected=%" PRIu32, device_id, expected_device_id);
        return false;
    }

    float temperature = 0.0f;
    float humidity = 0.0f;
    float pm2_5 = 0.0f;
    float voc = 0.0f;
    memcpy(&temperature, &packet[9], sizeof(temperature));
    memcpy(&humidity, &packet[13], sizeof(humidity));
    memcpy(&pm2_5, &packet[17], sizeof(pm2_5));
    memcpy(&voc, &packet[21], sizeof(voc));

    if (!lora_range_valid(temperature, -60.0f, 150.0f) ||
        !lora_range_valid(humidity, 0.0f, 100.0f) ||
        !lora_range_valid(pm2_5, 0.0f, 1000.0f) ||
        !lora_range_valid(voc, 0.0f, 5000.0f)) {
        *reason = LORA_PACKET_REJECT_RANGE;
        g_reject_counter[LORA_PACKET_REJECT_RANGE]++;
        ESP_LOGW(TAG,
                 "reject range T=%.2f RH=%.2f PM2.5=%.2f VOC=%.2f",
                 temperature,
                 humidity,
                 pm2_5,
                 voc);
        return false;
    }

    uint32_t sequence = 0U;
    memcpy(&sequence, &packet[5], sizeof(sequence));
    const bool boot_session = (packet[25] & 0x01U) != 0U;
    if (boot_session) {
        return true;
    }

    if (!lora_sequence_is_newer(sequence, last_accepted_sequence)) {
        *reason = LORA_PACKET_REJECT_SEQUENCE;
        g_reject_counter[LORA_PACKET_REJECT_SEQUENCE]++;
        ESP_LOGW(TAG, "reject seq=%" PRIu32 " last_accepted=%" PRIu32, sequence, last_accepted_sequence);
        return false;
    }

    return true;
}

static void lora_gateway_publish_valid_packet(const lora_gateway_packet_data_t *packet)
{
    Analog_Input_Present_Value_Set(user_ai_instance(USER_AI_SEN54_TEMPERATURE), packet->temperature_c);
    Analog_Input_Present_Value_Set(user_ai_instance(USER_AI_SEN54_HUMIDITY), packet->humidity_pct);
    Analog_Input_Present_Value_Set(user_ai_instance(USER_AI_SEN54_PM2_5), packet->pm2_5_ug_m3);
    Analog_Input_Present_Value_Set(user_ai_instance(USER_AI_SEN54_VOC_INDEX), packet->voc_index);
    Analog_Input_Present_Value_Set(user_ai_instance(USER_AI_LORA_STATUS), (float)packet->status);

    ESP_LOGI(TAG,
             "accepted Lora packet seq=%" PRIu32 " temp=%.2f RH=%.2f PM2.5=%.2f VOC=%.2f status=%u",
             packet->sequence,
             packet->temperature_c,
             packet->humidity_pct,
             packet->pm2_5_ug_m3,
             packet->voc_index,
             packet->status);
}

static void lora_gateway_task(void *argument)
{
    (void)argument;
    ESP_LOGI(TAG, "LoRa gateway task started");

    const uint32_t expected_device_id = USER_LORA_EXPECTED_DEVICE_ID;
    const uint32_t expected_version = USER_LORA_SUPPORTED_VERSION;

    uint8_t data[LORA_GATEWAY_PACKET_LEN] = {0};
    uint32_t last_accepted_sequence = 0U;
    uint32_t sequence = 0U;

    if (sx1262_configure(&g_lora_radio) != ESP_OK) {
        ESP_LOGE(TAG, "SX1262 configure failed");
        vTaskDelete(NULL);
    }

    for (;;) {
        lora_hal_fem_set_rx();
        lora_hal_clear_event();
        sx1262_set_irq_mask(SX1262_IRQ_RX_EVENTS);
        sx1262_clear_irq();
        sx1262_start_rx_continuous();

        if (!lora_hal_wait_event(portMAX_DELAY)) {
            continue;
        }

        uint16_t irq_status = 0U;
        uint8_t length = 0U;
        uint8_t offset = 0U;
        if (sx1262_get_irq_status(&irq_status) != ESP_OK || sx1262_clear_irq() != ESP_OK) {
            ESP_LOGW(TAG, "radio IRQ read failed");
            continue;
        }

        if ((irq_status & SX1262_IRQ_CRC_ERROR) != 0U) {
            g_reject_counter[LORA_PACKET_REJECT_CRC]++;
            ESP_LOGW(TAG, "reject CRC");
            continue;
        }

        if ((irq_status & SX1262_IRQ_RX_DONE) == 0U) {
            continue;
        }

        if (sx1262_get_rx_buffer_status(&length, &offset) != ESP_OK) {
            g_reject_counter[LORA_PACKET_REJECT_PARSE]++;
            ESP_LOGW(TAG, "reject radio buffer status");
            continue;
        }

        if (length != LORA_GATEWAY_PACKET_LEN) {
            g_reject_counter[LORA_PACKET_REJECT_LENGTH]++;
            ESP_LOGW(TAG, "reject length=%u expected=%u", length, LORA_GATEWAY_PACKET_LEN);
            continue;
        }

        memset(data, 0, sizeof(data));
        if (sx1262_read_buffer(offset, data, length) != ESP_OK) {
            g_reject_counter[LORA_PACKET_REJECT_PARSE]++;
            ESP_LOGW(TAG, "reject packet read");
            continue;
        }

        lora_packet_reason_t reason = LORA_PACKET_ACCEPTED;
        if (!validate_lora_packet(data, length, expected_device_id, expected_version, last_accepted_sequence, &reason)) {
            continue;
        }

        uint32_t packet_device_id = 0U;
        memcpy(&packet_device_id, &data[1], sizeof(packet_device_id));
        memcpy(&sequence, &data[5], sizeof(sequence));
        const bool boot_session = (data[25] & 0x01U) != 0U;
        if (boot_session) {
            ESP_LOGI(TAG, "accepted new LoRa session: device=%" PRIu32 " seq=%" PRIu32,
                     packet_device_id,
                     sequence);
        }
        last_accepted_sequence = sequence;

        lora_gateway_packet_data_t packet = {0};
        memcpy(&packet.device_id, &data[1], sizeof(packet.device_id));
        memcpy(&packet.sequence, &data[5], sizeof(packet.sequence));
        memcpy(&packet.temperature_c, &data[9], sizeof(packet.temperature_c));
        memcpy(&packet.humidity_pct, &data[13], sizeof(packet.humidity_pct));
        memcpy(&packet.pm2_5_ug_m3, &data[17], sizeof(packet.pm2_5_ug_m3));
        memcpy(&packet.voc_index, &data[21], sizeof(packet.voc_index));
        packet.status = data[25];

        lora_gateway_publish_valid_packet(&packet);
        lora_gateway_display_update(packet.voc_index, packet.pm2_5_ug_m3);
    }
}

esp_err_t lora_gateway_start(TaskHandle_t *task_handle)
{
    if (task_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(lora_hal_init(), TAG, "LoRa HAL init failed");

    BaseType_t result = xTaskCreate(lora_gateway_task, "lora_gateway", 4096, NULL, 4, task_handle);
    if (result != pdPASS) {
        *task_handle = NULL;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
