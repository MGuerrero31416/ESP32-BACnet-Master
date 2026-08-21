#ifndef UI_THEME_H
#define UI_THEME_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_COLOR_BG      lv_color_hex(0x000000)
#define UI_COLOR_LABEL   lv_color_hex(0xFFFF00)
#define UI_COLOR_VALUE   lv_color_hex(0xFFFFFF)
#define UI_COLOR_MUTED   lv_color_hex(0xBDBDBD)
#define UI_COLOR_ACCENT  lv_color_hex(0x00FFFF)
#define UI_COLOR_INFO    lv_color_hex(0x3A7DFF)
#define UI_COLOR_CARD_BG lv_color_hex(0x1E1E1E)

#define UI_FONT_TITLE (&lv_font_montserrat_20)
#define UI_FONT_LABEL (&lv_font_montserrat_20)
#define UI_FONT_VALUE (&lv_font_montserrat_24)
#define UI_FONT_HUGE  (&lv_font_montserrat_48)
#define UI_FONT_SMALL (&lv_font_montserrat_14)
#define UI_FONT_ICON  (&lv_font_montserrat_24)

/* Band reserved on both screen edges for the navigation arrows. */
#define UI_NAV_ZONE_W      30
#define UI_NAV_ARROW_INSET 8

lv_obj_t *ui_theme_screen_create(void);

/* Transparent vertical flex container inset from the navigation zones. */
lv_obj_t *ui_theme_content_create(lv_obj_t *screen);

/* Transparent horizontal flex row: first child left, last child right. */
lv_obj_t *ui_theme_row_create(lv_obj_t *parent);

lv_obj_t *ui_theme_label_create(
    lv_obj_t *parent,
    const char *text,
    const lv_font_t *font,
    lv_color_t color);

lv_obj_t *ui_theme_title_create(lv_obj_t *parent, const char *text);
lv_obj_t *ui_theme_card_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h);

void ui_theme_value_set(lv_obj_t *label, float value, const char *fmt);

#ifdef __cplusplus
}
#endif

#endif /* UI_THEME_H */
