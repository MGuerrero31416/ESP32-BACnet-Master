#include "ui_manager.h"

#include <stdint.h>
#include <string.h>

#include "esp_log.h"

#include "ui_theme.h"

static const char *TAG = "ui_manager";

#define UI_MANAGER_MAX_SCREENS 16

static lv_obj_t *s_instances[UI_MANAGER_MAX_SCREENS];
static int s_active = -1;

static int index_of(const char *id)
{
    if (id == NULL) {
        return -1;
    }

    for (size_t i = 0; i < g_ui_screen_count; ++i) {
        if (strcmp(g_ui_screens[i]->id, id) == 0) {
            return (int)i;
        }
    }

    return -1;
}

/* Nearest carousel entry in the given direction, or -1 at the edge. */
static int carousel_neighbour(int index, int step)
{
    for (int i = index + step; i >= 0 && i < (int)g_ui_screen_count; i += step) {
        if (g_ui_screens[i]->in_carousel) {
            return i;
        }
    }

    return -1;
}

static void show_index(int index);

static void nav_event_cb(lv_event_t *e)
{
    const int target = (int)(intptr_t)lv_event_get_user_data(e);
    show_index(target);
}

static void bind_nav_index(lv_obj_t *obj, int target)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(obj, 10);
    lv_obj_add_event_cb(
        obj,
        nav_event_cb,
        LV_EVENT_CLICKED,
        (void *)(intptr_t)target);
}

static void add_arrow(lv_obj_t *screen, const char *text, lv_align_t align, int target)
{
    lv_obj_t *arrow = ui_theme_label_create(screen, text, UI_FONT_ICON, UI_COLOR_ACCENT);

    lv_obj_align(arrow, align, 0, 0);
    lv_obj_set_x(
        arrow,
        (align == LV_ALIGN_LEFT_MID) ? UI_NAV_ARROW_INSET : -UI_NAV_ARROW_INSET);

    bind_nav_index(arrow, target);
}

static void add_nav_arrows(int index)
{
    const ui_screen_desc_t *desc = g_ui_screens[index];
    lv_obj_t *screen = s_instances[index];

    if (desc->in_carousel) {
        const int prev = carousel_neighbour(index, -1);
        const int next = carousel_neighbour(index, +1);

        if (prev >= 0) {
            add_arrow(screen, "<", LV_ALIGN_LEFT_MID, prev);
        }
        if (next >= 0) {
            add_arrow(screen, ">", LV_ALIGN_RIGHT_MID, next);
        }
        return;
    }

    const int parent = index_of(desc->parent_id);
    if (parent >= 0) {
        add_arrow(screen, "<", LV_ALIGN_LEFT_MID, parent);
    }
}

static lv_obj_t *ensure_created(int index)
{
    if (s_instances[index] != NULL) {
        return s_instances[index];
    }

    const ui_screen_desc_t *desc = g_ui_screens[index];
    lv_obj_t *screen = desc->create();
    if (screen == NULL) {
        ESP_LOGE(TAG, "Screen '%s' failed to build", desc->id);
        return NULL;
    }

    s_instances[index] = screen;
    add_nav_arrows(index);

    if (desc->on_data != NULL) {
        desc->on_data(ui_model_get());
    }

    return screen;
}

static void show_index(int index)
{
    if (index < 0 || index >= (int)g_ui_screen_count) {
        return;
    }

    lv_obj_t *screen = ensure_created(index);
    if (screen == NULL) {
        return;
    }

    lv_scr_load(screen);
    s_active = index;

    const ui_screen_desc_t *desc = g_ui_screens[index];
    if (desc->on_show != NULL) {
        desc->on_show();
    }
    if (desc->on_data != NULL) {
        desc->on_data(ui_model_get());
    }
}

void ui_manager_bind_nav(lv_obj_t *obj, const char *target_id)
{
    const int target = index_of(target_id);
    if (target < 0) {
        ESP_LOGE(TAG, "Unknown navigation target '%s'", target_id ? target_id : "(null)");
        return;
    }

    bind_nav_index(obj, target);
}

void ui_manager_show(const char *id)
{
    show_index(index_of(id));
}

void ui_manager_notify_data(const ui_model_t *model)
{
    if (s_active < 0) {
        return;
    }

    const ui_screen_desc_t *desc = g_ui_screens[s_active];
    if (desc->on_data != NULL) {
        desc->on_data(model);
    }
}

void ui_manager_init(void)
{
    if (g_ui_screen_count == 0) {
        ESP_LOGE(TAG, "No screens registered");
        return;
    }

    if (g_ui_screen_count > UI_MANAGER_MAX_SCREENS) {
        ESP_LOGE(TAG, "Too many screens registered");
        return;
    }

    show_index(0);
}
