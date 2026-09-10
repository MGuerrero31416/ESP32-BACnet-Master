#include "sx1262.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora_hal.h"

#include <string.h>

#define TAG "sx1262"
#define SX1262_BUFFER_MAX (LORA_PACKET_MAX_LEN + 8U)

static lora_radio_config_t sx1262_config;

esp_err_t sx1262_command(uint8_t command, const uint8_t *arguments, size_t argument_length)
{
    uint8_t buffer[SX1262_BUFFER_MAX] = {command};
    ESP_RETURN_ON_FALSE(argument_length <= sizeof(buffer) - 1, ESP_ERR_INVALID_SIZE, TAG, "command too long");
    if (argument_length > 0) {
        memcpy(&buffer[1], arguments, argument_length);
    }
    ESP_RETURN_ON_ERROR(lora_hal_wait_ready(pdMS_TO_TICKS(100)), TAG, "radio busy");
    return lora_hal_transfer(buffer, NULL, argument_length + 1);
}

esp_err_t sx1262_read(uint8_t command, const uint8_t *arguments, size_t argument_length,
                      uint8_t *response, size_t response_length)
{
    uint8_t buffer[SX1262_BUFFER_MAX] = {0};
    ESP_RETURN_ON_FALSE(argument_length + response_length + 2 <= sizeof(buffer), ESP_ERR_INVALID_SIZE, TAG, "read too long");
    buffer[0] = command;
    if (argument_length > 0) {
        memcpy(&buffer[1], arguments, argument_length);
    }
    ESP_RETURN_ON_ERROR(lora_hal_wait_ready(pdMS_TO_TICKS(100)), TAG, "radio busy");
    ESP_RETURN_ON_ERROR(lora_hal_transfer(buffer, buffer, argument_length + response_length + 2), TAG, "SPI read failed");
    memcpy(response, &buffer[argument_length + 2], response_length);
    return ESP_OK;
}

esp_err_t sx1262_reset(void)
{
    lora_hal_set_reset(0);
    vTaskDelay(pdMS_TO_TICKS(2));
    lora_hal_set_reset(1);
    return lora_hal_wait_ready(pdMS_TO_TICKS(100));
}

static esp_err_t sx1262_calibrate_image(void)
{
    uint8_t calibration[] = {0xE1, 0xE9};
    return sx1262_command(0x98, calibration, sizeof(calibration));
}

esp_err_t sx1262_set_packet_length(uint8_t length)
{
    uint8_t packet[] = {0x00, sx1262_config.preamble_length, 0x00, length, 0x01, 0x00};
    return sx1262_command(0x8C, packet, sizeof(packet));
}

esp_err_t sx1262_write_packet(const uint8_t *payload, uint8_t length)
{
    uint8_t command[LORA_PACKET_MAX_LEN + 2U] = {0x0E, 0x00};
    ESP_RETURN_ON_FALSE(length <= LORA_PACKET_MAX_LEN, ESP_ERR_INVALID_SIZE, TAG, "payload too long");
    memcpy(&command[2], payload, length);
    return sx1262_command(command[0], &command[1], length + 1U);
}

esp_err_t sx1262_start_tx(uint32_t timeout_units)
{
    uint8_t tx_timeout[] = {
        (uint8_t)(timeout_units >> 16),
        (uint8_t)(timeout_units >> 8),
        (uint8_t)timeout_units,
    };
    return sx1262_command(0x83, tx_timeout, sizeof(tx_timeout));
}

esp_err_t sx1262_start_rx_continuous(void)
{
    uint8_t rx_timeout[] = {0xFF, 0xFF, 0xFF};
    return sx1262_command(0x82, rx_timeout, sizeof(rx_timeout));
}

esp_err_t sx1262_set_irq_mask(uint16_t irq_mask)
{
    uint8_t irq_params[] = {
        (uint8_t)(irq_mask >> 8), (uint8_t)irq_mask,
        (uint8_t)(irq_mask >> 8), (uint8_t)irq_mask,
        0x00, 0x00,
        0x00, 0x00,
    };
    return sx1262_command(0x08, irq_params, sizeof(irq_params));
}

esp_err_t sx1262_get_irq_status(uint16_t *irq_status)
{
    uint8_t response[2];
    esp_err_t result = sx1262_read(0x12, NULL, 0, response, sizeof(response));
    if (result == ESP_OK) {
        *irq_status = ((uint16_t)response[0] << 8) | response[1];
    }
    return result;
}

esp_err_t sx1262_clear_irq(void)
{
    uint8_t irq[] = {0xFF, 0xFF};
    return sx1262_command(0x02, irq, sizeof(irq));
}

esp_err_t sx1262_get_status(uint8_t *status)
{
    uint8_t command[] = {0xC0, 0x00};
    uint8_t response[sizeof(command)] = {0};
    esp_err_t result = sx1262_read(0xC0, command + 1, 0, response, sizeof(response));
    if (result == ESP_OK) {
        *status = response[1];
    }
    return result;
}

esp_err_t sx1262_get_rx_buffer_status(uint8_t *length, uint8_t *offset)
{
    uint8_t status[2];
    ESP_RETURN_ON_ERROR(sx1262_read(0x13, NULL, 0, status, sizeof(status)), TAG, "buffer status failed");
    *length = status[0];
    *offset = status[1];
    return ESP_OK;
}

esp_err_t sx1262_get_packet_status(int16_t *rssi, int8_t *snr)
{
    uint8_t packet_status[3];
    ESP_RETURN_ON_ERROR(sx1262_read(0x14, NULL, 0, packet_status, sizeof(packet_status)), TAG, "packet status failed");
    *rssi = -(int16_t)(packet_status[0] / 2);
    *snr = (int8_t)packet_status[1] / 4;
    return ESP_OK;
}

esp_err_t sx1262_read_buffer(uint8_t offset, uint8_t *payload, uint8_t length)
{
    uint8_t read_args[] = {offset};
    return sx1262_read(0x1E, read_args, sizeof(read_args), payload, length);
}

esp_err_t sx1262_configure(const lora_radio_config_t *config)
{
    sx1262_config = *config;

    uint8_t standby[] = {0x00};
    uint8_t packet_type[] = {0x01};
    uint8_t regulator[] = {0x01};
    uint8_t tcxo[] = {0x07, 0x00, 0x01, 0x40};
    uint8_t calibration[] = {0x7F};
    uint8_t rf_switch[] = {0x01};
    uint8_t pa_config[] = {0x04, 0x07, 0x00, 0x01};
    uint32_t frequency = (uint32_t)(((uint64_t)config->frequency_hz << 25) / 32000000ULL);
    uint8_t rf_frequency[] = {(uint8_t)(frequency >> 24), (uint8_t)(frequency >> 16), (uint8_t)(frequency >> 8), (uint8_t)frequency};
    uint8_t buffer_base[] = {0x00, 0x00};
    uint8_t packet_params[] = {0x00, config->preamble_length, 0x00, config->packet_max_len, 0x01, 0x00};
    uint8_t modulation[] = {config->spreading_factor, config->bandwidth, config->coding_rate, 0x00};
    uint8_t tx_params[] = {config->tx_power_dbm, 0x04};

    ESP_RETURN_ON_ERROR(sx1262_reset(), TAG, "radio reset failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x80, standby, sizeof(standby)), TAG, "standby failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x96, regulator, sizeof(regulator)), TAG, "regulator setup failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x97, tcxo, sizeof(tcxo)), TAG, "TCXO setup failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x89, calibration, sizeof(calibration)), TAG, "radio calibration failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x9D, rf_switch, sizeof(rf_switch)), TAG, "RF switch setup failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x95, pa_config, sizeof(pa_config)), TAG, "PA setup failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x8A, packet_type, sizeof(packet_type)), TAG, "packet type failed");
    ESP_RETURN_ON_ERROR(sx1262_calibrate_image(), TAG, "image calibration failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x86, rf_frequency, sizeof(rf_frequency)), TAG, "frequency failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x8F, buffer_base, sizeof(buffer_base)), TAG, "buffer setup failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x8B, modulation, sizeof(modulation)), TAG, "modulation setup failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x8C, packet_params, sizeof(packet_params)), TAG, "packet setup failed");
    ESP_RETURN_ON_ERROR(sx1262_command(0x8E, tx_params, sizeof(tx_params)), TAG, "TX power setup failed");
    ESP_RETURN_ON_ERROR(sx1262_set_irq_mask(SX1262_IRQ_TX_EVENTS), TAG, "IRQ setup failed");

    uint8_t status;
    ESP_RETURN_ON_ERROR(sx1262_get_status(&status), TAG, "radio status read failed");
    ESP_LOGI(TAG, "SX1262 status after init: 0x%02x", status);
    return ESP_OK;
}
