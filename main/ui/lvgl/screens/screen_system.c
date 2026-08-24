#include "ui_manager.h"
#include "ui_theme.h"

#include <stdio.h>

#include "display.h"

#define SYSTEM_DEFAULT_BRIGHTNESS 25

static lv_obj_t *s_brightness_value = NULL;

static void brightness_value_set(uint8_t percent)
{
    if (s_brightness_value == NULL) {
        return;
    }

    char text[8];
    snprintf(text, sizeof(text), "%u%%", (unsigned)percent);
    lv_label_set_text(s_brightness_value, text);
}

static void brightness_slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    const uint8_t percent = (uint8_t)lv_slider_get_value(slider);

    display_set_brightness(percent);
    brightness_value_set(percent);
}

static lv_obj_t *create(void)
{
    lv_obj_t *screen = ui_theme_screen_create();
    lv_obj_t *content = ui_theme_content_create(screen);

    ui_theme_title_create(content, "System");

    lv_obj_t *brightness_row = ui_theme_row_create(content);
    ui_theme_label_create(
        brightness_row,
        "Brightness",
        UI_FONT_SMALL,
        UI_COLOR_MUTED);

    s_brightness_value = ui_theme_label_create(
        brightness_row,
        "",
        UI_FONT_SMALL,
        UI_COLOR_VALUE);
    brightness_value_set(SYSTEM_DEFAULT_BRIGHTNESS);

    lv_obj_t *brightness_slider = lv_slider_create(content);
    lv_obj_set_width(brightness_slider, 220);
    lv_slider_set_range(brightness_slider, 5, 100);
    lv_slider_set_value(
        brightness_slider,
        SYSTEM_DEFAULT_BRIGHTNESS,
        LV_ANIM_OFF);
    lv_obj_add_event_cb(
        brightness_slider,
        brightness_slider_event_cb,
        LV_EVENT_VALUE_CHANGED,
        NULL);
    lv_obj_set_style_bg_color(
        brightness_slider,
        UI_COLOR_CARD_BG,
        LV_PART_MAIN);
    lv_obj_set_style_bg_color(
        brightness_slider,
        UI_COLOR_ACCENT,
        LV_PART_INDICATOR);

    display_set_brightness(SYSTEM_DEFAULT_BRIGHTNESS);

    return screen;
}

const ui_screen_desc_t screen_system = {
    .id = "system",
    .in_carousel = false,
    .parent_id = "settings",
    .create = create,
    .on_data = NULL,
    .on_show = NULL,
};