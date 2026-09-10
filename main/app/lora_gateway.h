#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LORA_GATEWAY_PROTOCOL_VERSION 1U
#define LORA_GATEWAY_PACKET_LEN 26U

#define LORA_GATEWAY_STATUS_OK 0U
#define LORA_GATEWAY_STATUS_WARNING 1U
#define LORA_GATEWAY_STATUS_ERROR 2U

typedef enum {
    LORA_PACKET_ACCEPTED = 0,
    LORA_PACKET_REJECT_CRC = 1,
    LORA_PACKET_REJECT_LENGTH = 2,
    LORA_PACKET_REJECT_VERSION = 3,
    LORA_PACKET_REJECT_DEVICE_ID = 4,
    LORA_PACKET_REJECT_SEQUENCE = 5,
    LORA_PACKET_REJECT_RANGE = 6,
    LORA_PACKET_REJECT_PARSE = 7
} lora_packet_reason_t;

typedef struct {
    uint32_t device_id;
    uint32_t sequence;
    float temperature_c;
    float humidity_pct;
    float pm2_5_ug_m3;
    float voc_index;
    uint8_t status;
} lora_gateway_packet_data_t;

bool validate_lora_packet(
    const uint8_t *packet,
    size_t packet_len,
    uint32_t expected_device_id,
    uint32_t expected_version,
    uint32_t last_accepted_sequence,
    lora_packet_reason_t *reason);

esp_err_t lora_gateway_start(TaskHandle_t *task_handle);

void lora_gateway_display_update(float voc, float pm2_5);
