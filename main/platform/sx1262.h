#pragma once

#include "User_Settings.h"
#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>

#define SX1262_IRQ_TX_DONE 0x0001U
#define SX1262_IRQ_RX_DONE 0x0002U
#define SX1262_IRQ_CRC_ERROR 0x0040U
#define SX1262_IRQ_TIMEOUT 0x0200U
#define SX1262_IRQ_TX_EVENTS (SX1262_IRQ_TX_DONE | SX1262_IRQ_TIMEOUT)
#define SX1262_IRQ_RX_EVENTS (SX1262_IRQ_RX_DONE | SX1262_IRQ_CRC_ERROR | SX1262_IRQ_TIMEOUT)

typedef struct {
    uint32_t frequency_hz;
    uint8_t bandwidth;
    uint8_t spreading_factor;
    uint8_t coding_rate;
    uint8_t preamble_length;
    uint8_t tx_power_dbm;
    uint8_t packet_max_len;
} lora_radio_config_t;

esp_err_t sx1262_command(uint8_t command, const uint8_t *arguments, size_t argument_length);
esp_err_t sx1262_read(uint8_t command, const uint8_t *arguments, size_t argument_length,
                      uint8_t *response, size_t response_length);
esp_err_t sx1262_reset(void);
esp_err_t sx1262_configure(const lora_radio_config_t *config);
esp_err_t sx1262_set_irq_mask(uint16_t irq_mask);
esp_err_t sx1262_clear_irq(void);
esp_err_t sx1262_get_irq_status(uint16_t *irq_status);
esp_err_t sx1262_get_status(uint8_t *status);
esp_err_t sx1262_set_packet_length(uint8_t length);
esp_err_t sx1262_write_packet(const uint8_t *payload, uint8_t length);
esp_err_t sx1262_start_tx(uint32_t timeout_units);
esp_err_t sx1262_start_rx_continuous(void);
esp_err_t sx1262_get_rx_buffer_status(uint8_t *length, uint8_t *offset);
esp_err_t sx1262_get_packet_status(int16_t *rssi, int8_t *snr);
esp_err_t sx1262_read_buffer(uint8_t offset, uint8_t *payload, uint8_t length);
