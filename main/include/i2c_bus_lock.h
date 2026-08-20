#pragma once

#include "driver/i2c_master.h"
#include "freertos/semphr.h"

// Forward declaration for build-time visibility in `main`.
// Implementation lives in components/i2c_bus_lock.
esp_err_t i2c_bus_lock(i2c_master_bus_handle_t bus, TickType_t ticks_to_wait);
void i2c_bus_unlock(i2c_master_bus_handle_t bus);
