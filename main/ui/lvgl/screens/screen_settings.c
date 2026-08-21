#include "ui_manager.h"
#include "ui_theme.h"

static lv_obj_t *create(void)
{
    lv_obj_t *screen = ui_theme_screen_create();
    lv_obj_t *content = ui_theme_content_create(screen);

    ui_theme_title_create(content, "Settings");

    lv_obj_t *wifi_tile = ui_theme_card_create(content, 138, 92);
    lv_obj_t *icon = ui_theme_label_create(
        wifi_tile, LV_SYMBOL_WIFI, UI_FONT_ICON, UI_COLOR_ACCENT);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t *caption = ui_theme_label_create(
        wifi_tile, "Wi-Fi", UI_FONT_LABEL, UI_COLOR_VALUE);
    lv_obj_align(caption, LV_ALIGN_BOTTOM_MID, 0, -6);

    ui_manager_bind_nav(wifi_tile, "wifi");

    return screen;
}

const ui_screen_desc_t screen_settings = {
    .id = "settings",
    .in_carousel = true,
    .parent_id = NULL,
    .create = create,
    .on_data = NULL,
    .on_show = NULL,
};
