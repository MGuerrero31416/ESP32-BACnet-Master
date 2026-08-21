#include "ui_manager.h"
#include "ui_theme.h"

typedef struct {
    const char *caption;
    const char *fmt;
    size_t offset;
} measurement_row_t;

#define MEASUREMENT_FIELD(field) offsetof(ui_model_t, field)

static const measurement_row_t s_rows[] = {
    { "Temp",  "%.1f", MEASUREMENT_FIELD(temperature) },
    { "%HR",   "%.1f", MEASUREMENT_FIELD(humidity) },
    { "PM2.5", "%.0f", MEASUREMENT_FIELD(pm25) },
    { "VOC",   "%.0f", MEASUREMENT_FIELD(voc) },
};

#define MEASUREMENT_ROW_COUNT (sizeof(s_rows) / sizeof(s_rows[0]))

static lv_obj_t *s_values[MEASUREMENT_ROW_COUNT];
static lv_obj_t *s_footer;

static lv_obj_t *create(void)
{
    lv_obj_t *screen = ui_theme_screen_create();
    lv_obj_t *content = ui_theme_content_create(screen);

    for (size_t i = 0; i < MEASUREMENT_ROW_COUNT; ++i) {
        lv_obj_t *row = ui_theme_row_create(content);

        ui_theme_label_create(row, s_rows[i].caption, UI_FONT_LABEL, UI_COLOR_LABEL);
        s_values[i] = ui_theme_label_create(row, "--", UI_FONT_VALUE, UI_COLOR_VALUE);
    }

    s_footer = ui_theme_label_create(content, "", UI_FONT_LABEL, UI_COLOR_INFO);

    return screen;
}

static void on_data(const ui_model_t *model)
{
    for (size_t i = 0; i < MEASUREMENT_ROW_COUNT; ++i) {
        const float value = *(const float *)((const char *)model + s_rows[i].offset);
        ui_theme_value_set(s_values[i], value, s_rows[i].fmt);
    }
}

const ui_screen_desc_t screen_measurements = {
    .id = "measurements",
    .in_carousel = true,
    .parent_id = NULL,
    .create = create,
    .on_data = on_data,
    .on_show = NULL,
};
