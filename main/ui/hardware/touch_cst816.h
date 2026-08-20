#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool pressed;
    uint16_t raw_x; /* portrait raw (0..169) */
    uint16_t raw_y; /* portrait raw (0..319) */
    uint16_t x;     /* mapped landscape (0..319) */
    uint16_t y;     /* mapped landscape (0..169) */
} touch_cst816_point_t;

esp_err_t touch_cst816_init(void);
esp_err_t touch_cst816_read(touch_cst816_point_t *pt);

#ifdef __cplusplus
}
#endif