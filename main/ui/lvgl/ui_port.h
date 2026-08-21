#ifndef UI_PORT_H
#define UI_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Board hook: push a rendered area to the panel. */
typedef void (*ui_port_flush_fn)(
    const lv_area_t *area,
    lv_color_t *pixels);

/* Board hook: report the pointer state already mapped to LVGL coordinates.
   Returns true while pressed. */
typedef bool (*ui_port_touch_fn)(int16_t *x, int16_t *y);

typedef struct {
    lv_coord_t hor_res;
    lv_coord_t ver_res;
    uint32_t draw_buf_lines;
    ui_port_flush_fn flush;
    ui_port_touch_fn touch;
} ui_port_config_t;

esp_err_t ui_port_init(const ui_port_config_t *cfg);
bool ui_port_is_ready(void);

lv_coord_t ui_port_width(void);
lv_coord_t ui_port_height(void);

/* Serialize access to LVGL against the internal LVGL task. */
bool ui_port_lock(uint32_t timeout_ms);
void ui_port_unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_PORT_H */
