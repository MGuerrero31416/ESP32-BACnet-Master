#pragma once

#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sen54_i2c_bridge_write(
    uint8_t address,
    const uint8_t *data,
    size_t length);

esp_err_t sen54_i2c_bridge_read(
    uint8_t address,
    uint8_t *data,
    size_t length);

esp_err_t sen54_i2c_transaction_begin(void);
void sen54_i2c_transaction_end(void);

#ifdef __cplusplus
}
#endif
