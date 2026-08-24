#include "screen_wifi_password.h"

#include <stdio.h>
#include <string.h>

#include "ui_manager.h"
#include "ui_model.h"
#include "ui_theme.h"

#include "wifi_helper.h"

static char s_selected_ssid[33];
static lv_obj_t *s_selected_ssid_label = NULL;
static lv_obj_t *s_password_textarea = NULL;
static lv_obj_t *s_keyboard = NULL;
static lv_obj_t *s_return_button = NULL;
static bool s_waiting_for_connection;

void wifi_password_select_ssid(const char *ssid)
{
    if (ssid == NULL) {
        s_selected_ssid[0] = '\0';
        return;
    }

    strncpy(s_selected_ssid, ssid, sizeof(s_selected_ssid) - 1);
    s_selected_ssid[sizeof(s_selected_ssid) - 1] = '\0';
}

static void password_ready_event_cb(lv_event_t *e)
{
    (void)e;

    const esp_err_t err = wifi_save_and_connect(
        s_selected_ssid,
        lv_textarea_get_text(s_password_textarea));

    s_waiting_for_connection = (err == ESP_OK);
    lv_label_set_text(
        s_selected_ssid_label,
        (err == ESP_OK) ? "Connecting..." : "Could not save network");
}

static void show_connection_success(void)
{
    char message[64];
    snprintf(
        message,
        sizeof(message),
        "Successfully connected to\n%s",
        s_selected_ssid);
    lv_label_set_text(s_selected_ssid_label, message);
    lv_label_set_long_mode(s_selected_ssid_label, LV_LABEL_LONG_WRAP);
    lv_obj_add_flag(s_password_textarea, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_return_button, LV_OBJ_FLAG_HIDDEN);
    s_waiting_for_connection = false;
}

static void connection_update(const ui_model_t *model)
{
    (void)model;

    if (s_waiting_for_connection && wifi_requested_connection_succeeded()) {
        show_connection_success();
    }
}

static void show_selected_network(void)
{
    lv_label_set_text(
        s_selected_ssid_label,
        (s_selected_ssid[0] != '\0') ? s_selected_ssid : "No network selected");
    lv_label_set_long_mode(s_selected_ssid_label, LV_LABEL_LONG_DOT);
    lv_textarea_set_text(s_password_textarea, "");
    lv_obj_clear_flag(s_password_textarea, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_return_button, LV_OBJ_FLAG_HIDDEN);
    s_waiting_for_connection = false;
}

static lv_obj_t *create(void)
{
    lv_obj_t *screen = ui_theme_screen_create();
    lv_obj_t *content = ui_theme_content_create(screen);
    lv_obj_set_style_pad_ver(content, 2, 0);
    lv_obj_set_style_pad_row(content, 2, 0);

    s_selected_ssid_label = ui_theme_label_create(
        content, "", UI_FONT_SMALL, UI_COLOR_ACCENT);
    lv_obj_set_width(s_selected_ssid_label, 250);
    lv_label_set_long_mode(s_selected_ssid_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s_selected_ssid_label, LV_TEXT_ALIGN_CENTER, 0);

    s_password_textarea = lv_textarea_create(content);
    lv_obj_set_size(s_password_textarea, 250, 28);
    lv_textarea_set_one_line(s_password_textarea, true);
    lv_textarea_set_max_length(s_password_textarea, 64);
    lv_textarea_set_placeholder_text(s_password_textarea, "Password");
    lv_obj_add_event_cb(
        s_password_textarea,
        password_ready_event_cb,
        LV_EVENT_READY,
        NULL);

    s_keyboard = lv_keyboard_create(content);
    lv_obj_set_size(s_keyboard, 250, 102);
    lv_keyboard_set_mode(s_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(s_keyboard, s_password_textarea);

    s_return_button = lv_btn_create(content);
    lv_obj_set_size(s_return_button, 150, 34);
    lv_obj_add_flag(s_return_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *return_label = ui_theme_label_create(
        s_return_button, "Return to Main", UI_FONT_SMALL, UI_COLOR_VALUE);
    lv_obj_center(return_label);
    ui_manager_bind_nav(s_return_button, "air_quality");

    return screen;
}

const ui_screen_desc_t screen_wifi_password = {
    .id = "wifi_password",
    .in_carousel = false,
    .parent_id = "wifi",
    .create = create,
    .on_data = connection_update,
    .on_show = show_selected_network,
};