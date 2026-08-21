#include <stdio.h>

#include "ui_manager.h"
#include "ui_theme.h"

/* EPA air-quality palette, same values as the GMT020 profile. */
#define AQ_COLOR_GOOD      lv_color_hex(0x00E400)
#define AQ_COLOR_MODERATE  lv_color_hex(0xFFFF00)
#define AQ_COLOR_SENSITIVE lv_color_hex(0xFF7E00)
#define AQ_COLOR_UNHEALTHY lv_color_hex(0xDC0000)
#define AQ_COLOR_VERY_UH   lv_color_hex(0x8F3F97)
#define AQ_COLOR_HAZARDOUS lv_color_hex(0x7E0023)
#define AQ_COLOR_TITLE_BAR lv_color_hex(0x000050)

typedef struct {
    lv_color_t bg;
    lv_color_t fg;
    const char *label;
} aq_info_t;

typedef struct {
    lv_obj_t *panel;
    lv_obj_t *value;
    lv_obj_t *unit;
    lv_obj_t *category;
} aq_panel_t;

static aq_panel_t s_pm25;
static aq_panel_t s_voc;

static aq_info_t pm25_info(float pm25)
{
    if (pm25 < 12.1f) {
        return (aq_info_t){ AQ_COLOR_GOOD, lv_color_black(), "Good" };
    }
    if (pm25 < 35.5f) {
        return (aq_info_t){ AQ_COLOR_MODERATE, lv_color_black(), "Moderate" };
    }
    if (pm25 < 55.5f) {
        return (aq_info_t){ AQ_COLOR_SENSITIVE, lv_color_black(), "Unhealt. 4 Sens." };
    }
    if (pm25 < 150.5f) {
        return (aq_info_t){ AQ_COLOR_UNHEALTHY, lv_color_white(), "Unhealthy" };
    }
    if (pm25 < 250.5f) {
        return (aq_info_t){ AQ_COLOR_VERY_UH, lv_color_white(), "Very Unhealthy" };
    }

    return (aq_info_t){ AQ_COLOR_HAZARDOUS, lv_color_white(), "Hazardous" };
}

static aq_info_t voc_info(float voc)
{
    if (voc < 100.0f) {
        return (aq_info_t){ AQ_COLOR_GOOD, lv_color_black(), "Good" };
    }
    if (voc < 250.0f) {
        return (aq_info_t){ AQ_COLOR_MODERATE, lv_color_black(), "Moderate" };
    }
    if (voc < 350.0f) {
        return (aq_info_t){ AQ_COLOR_SENSITIVE, lv_color_black(), "Polluted" };
    }
    if (voc < 400.0f) {
        return (aq_info_t){ AQ_COLOR_UNHEALTHY, lv_color_white(), "Very Polluted" };
    }

    return (aq_info_t){ AQ_COLOR_VERY_UH, lv_color_white(), "Severely Poll." };
}

static lv_obj_t *centered_label(
    lv_obj_t *parent,
    const char *text,
    const lv_font_t *font)
{
    lv_obj_t *label = ui_theme_label_create(parent, text, font, lv_color_black());

    lv_obj_set_width(label, LV_PCT(100));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_letter_space(label, 2, 0);
    lv_obj_set_style_text_line_space(label, 2, 0);

    return label;
}

static void panel_build(
    aq_panel_t *p,
    lv_obj_t *parent,
    const char *title,
    const char *unit)
{
    p->panel = lv_obj_create(parent);
    lv_obj_remove_style_all(p->panel);
    lv_obj_set_flex_grow(p->panel, 1);
    lv_obj_set_height(p->panel, LV_PCT(100));
    lv_obj_set_style_bg_opa(p->panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(p->panel, lv_color_white(), 0);
    lv_obj_set_style_border_width(p->panel, 2, 0);
    lv_obj_set_style_pad_all(p->panel, 0, 0);
    lv_obj_set_flex_flow(p->panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(p->panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *title_bar = lv_obj_create(p->panel);
    lv_obj_remove_style_all(title_bar);
    lv_obj_set_size(title_bar, LV_PCT(100), 26);
    lv_obj_set_style_bg_color(title_bar, AQ_COLOR_TITLE_BAR, 0);
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(title_bar, lv_color_white(), 0);
    lv_obj_set_style_border_width(title_bar, 1, 0);
    lv_obj_set_style_border_side(title_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *title_label =
        ui_theme_label_create(title_bar, title, UI_FONT_LABEL, lv_color_white());
    lv_obj_center(title_label);

    lv_obj_t *body = lv_obj_create(p->panel);
    lv_obj_remove_style_all(body);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_style_pad_hor(body, 2, 0);
    lv_obj_set_style_pad_row(body, 2, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        body,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    p->value = centered_label(body, "--", UI_FONT_HUGE);
    p->unit = centered_label(body, unit, UI_FONT_SMALL);
    p->category = centered_label(body, "", UI_FONT_SMALL);
}

static void panel_apply(aq_panel_t *p, float value, aq_info_t info)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", value);

    lv_obj_set_style_bg_color(p->panel, info.bg, 0);
    lv_label_set_text(p->value, buf);
    lv_label_set_text(p->category, info.label);

    lv_obj_set_style_text_color(p->value, info.fg, 0);
    lv_obj_set_style_text_color(p->unit, info.fg, 0);
    lv_obj_set_style_text_color(p->category, info.fg, 0);
}

static lv_obj_t *create(void)
{
    lv_obj_t *screen = ui_theme_screen_create();
    lv_obj_t *content = ui_theme_content_create(screen);

    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(content, 6, 0);

    panel_build(&s_pm25, content, "PM2.5", "ug/m3");
    panel_build(&s_voc, content, "VOC", "(1 - 500)");

    return screen;
}

static void on_data(const ui_model_t *model)
{
    panel_apply(&s_pm25, model->pm25, pm25_info(model->pm25));
    panel_apply(&s_voc, model->voc, voc_info(model->voc));
}

const ui_screen_desc_t screen_air_quality = {
    .id = "air_quality",
    .in_carousel = true,
    .parent_id = NULL,
    .create = create,
    .on_data = on_data,
    .on_show = NULL,
};
