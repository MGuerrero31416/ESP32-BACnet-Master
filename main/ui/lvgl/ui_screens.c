#include "ui_manager.h"

/* Registration table: carousel order follows this array.
   Add a new screen by declaring its descriptor here. */

extern const ui_screen_desc_t screen_measurements;
extern const ui_screen_desc_t screen_air_quality;
extern const ui_screen_desc_t screen_placeholder;
extern const ui_screen_desc_t screen_settings;
extern const ui_screen_desc_t screen_wifi;
extern const ui_screen_desc_t screen_wifi_password;
extern const ui_screen_desc_t screen_sensor_settings;
extern const ui_screen_desc_t screen_system;

const ui_screen_desc_t *const g_ui_screens[] = {
    &screen_air_quality,
    &screen_measurements,
    &screen_placeholder,
    &screen_settings,
    &screen_wifi,
    &screen_wifi_password,
    &screen_sensor_settings,
    &screen_system,
};

const size_t g_ui_screen_count =
    sizeof(g_ui_screens) / sizeof(g_ui_screens[0]);
