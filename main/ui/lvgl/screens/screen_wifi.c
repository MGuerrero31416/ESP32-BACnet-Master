#include "ui_manager.h"
#include "ui_theme.h"
#include "screen_wifi_password.h"

#include <stdio.h>

#include "wifi_helper.h"

static lv_obj_t *s_network_list = NULL;

static void network_event_cb(lv_event_t *e)
{
    lv_obj_t *network = lv_event_get_target(e);
    lv_obj_t *ssid = lv_obj_get_child(network, 0);

    if (ssid == NULL) {
        return;
    }

    wifi_password_select_ssid(lv_label_get_text(ssid));
    ui_manager_show("wifi_password");
}

static void add_message(const char *message)
{
    lv_obj_t *label = ui_theme_label_create(
        s_network_list, message, UI_FONT_SMALL, UI_COLOR_MUTED);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
}

static void scan_networks(void)
{
    if (s_network_list == NULL) {
        return;
    }

    lv_obj_clean(s_network_list);

    wifi_scan_result_t networks[WIFI_SCAN_MAX_RESULTS];
    size_t network_count = 0;
    const int err = wifi_scan_get_results(
        networks,
        WIFI_SCAN_MAX_RESULTS,
        &network_count);
    if (err != 0) {
        add_message("Scan unavailable");
        return;
    }

    if (network_count == 0) {
        add_message("No networks found");
        return;
    }

    for (size_t i = 0; i < network_count; ++i) {
        lv_obj_t *network = ui_theme_card_create(
            s_network_list,
            LV_PCT(100),
            26);
        lv_obj_set_style_radius(network, 4, 0);
        lv_obj_set_style_pad_hor(network, 8, 0);
        lv_obj_set_style_pad_ver(network, 2, 0);
        lv_obj_add_flag(network, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(network, network_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *ssid = ui_theme_label_create(
            network,
            networks[i].ssid,
            UI_FONT_SMALL,
            UI_COLOR_VALUE);
        lv_obj_align(ssid, LV_ALIGN_LEFT_MID, 0, 0);

        char rssi[12];
        snprintf(rssi, sizeof(rssi), "%d dBm", networks[i].rssi);
        lv_obj_t *signal = ui_theme_label_create(
            network,
            rssi,
            UI_FONT_SMALL,
            UI_COLOR_ACCENT);
        lv_obj_align(signal, LV_ALIGN_RIGHT_MID, 0, 0);
    }
}

static lv_obj_t *create(void)
{
    lv_obj_t *screen = ui_theme_screen_create();
    lv_obj_t *content = ui_theme_content_create(screen);

    ui_theme_title_create(content, "Wi-Fi Settings");

    s_network_list = lv_obj_create(content);
    lv_obj_set_size(s_network_list, 220, 112);
    lv_obj_set_style_bg_opa(s_network_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_network_list, 0, 0);
    lv_obj_set_style_pad_all(s_network_list, 0, 0);
    lv_obj_set_style_pad_row(s_network_list, 4, 0);
    lv_obj_set_flex_flow(s_network_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        s_network_list,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(s_network_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_network_list, LV_SCROLLBAR_MODE_AUTO);

    return screen;
}

const ui_screen_desc_t screen_wifi = {
    .id = "wifi",
    .in_carousel = false,
    .parent_id = "settings",
    .create = create,
    .on_data = NULL,
    .on_show = scan_networks,
};
