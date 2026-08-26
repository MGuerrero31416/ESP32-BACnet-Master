#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

esp_err_t lora_hal_init(void);
esp_err_t lora_hal_transfer(const uint8_t *tx, uint8_t *rx, size_t length);
esp_err_t lora_hal_wait_ready(TickType_t timeout);
void lora_hal_set_reset(uint32_t level);
void lora_hal_fem_set_rx(void);
void lora_hal_fem_set_tx(void);
void lora_hal_clear_event(void);
bool lora_hal_wait_event(TickType_t timeout);
int lora_hal_dio1_level(void);
uint32_t lora_hal_dio1_isr_count(void);
