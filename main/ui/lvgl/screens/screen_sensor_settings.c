#include "ui_manager.h"
#include "ui_theme.h"

static lv_obj_t *create(void)
{
    lv_obj_t *screen = ui_theme_screen_create();
    lv_obj_t *content = ui_theme_content_create(screen);

    ui_theme_title_create(content, "Sensor Settings");
    ui_theme_label_create(content, "Placeholder", UI_FONT_LABEL, UI_COLOR_MUTED);

    return screen;
}

const ui_screen_desc_t screen_sensor_settings = {
    .id = "sensor_settings",
    .in_carousel = false,
    .parent_id = "settings",
    .create = create,
    .on_data = NULL,
    .on_show = NULL,
};