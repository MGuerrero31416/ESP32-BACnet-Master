#include "display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <stdio.h>
#include <string.h>

#if __has_include("Arial_Bold_26.h")
#include "Arial_Bold_26.h"
#define DISPLAY_HAS_ARIAL_BOLD_26 1
#else
#include "Arial_Bold_36.h"
#define DISPLAY_HAS_ARIAL_BOLD_26 0
#endif

#include "Arial_Bold_44.h"
#include "Arial_Regular_16.h"
#include "User_Settings.h"
#include "esp_log.h"

static TFT_eSPI tft;
static const char *TAG = "display";

/* 240x320 panel in landscape orientation. */
#define DISP_WIDTH  320
#define DISP_HEIGHT 240

/* Header: left-aligned BACnet ID/MAC and right-side Wi-Fi pilot. */
#define HEADER_X               0
#define HEADER_Y               0
#define HEADER_WIDTH           DISP_WIDTH
#define HEADER_HEIGHT          36
#define HEADER_TEXT_X          8
#define HEADER_WIFI_AREA_X     238
#define HEADER_WIFI_LED_X      306
#define HEADER_WIFI_LED_Y      (HEADER_HEIGHT / 2)
#define HEADER_WIFI_LED_RADIUS 6

/* Footer: compact temperature and humidity line. */
#define FOOTER_HEIGHT 34
#define FOOTER_Y      (DISP_HEIGHT - FOOTER_HEIGHT)

/* Two-panel layout between the header and footer. */
#define PANEL_MARGIN   6
#define PANEL_GAP      6
#define PANEL_Y        (HEADER_HEIGHT + PANEL_MARGIN)
#define PANEL_HEIGHT   (FOOTER_Y - PANEL_Y - PANEL_MARGIN)
#define PANEL_WIDTH    151
#define PANEL_LEFT_X   PANEL_MARGIN
#define PANEL_RIGHT_X  (PANEL_LEFT_X + PANEL_WIDTH + PANEL_GAP)
#define PANEL_TITLE_H  32

/* RGB565 compile-time colour constants. */
#define MAKE_C565(r, g, b)                                             \
    ((uint16_t)((((uint16_t)(r) & 0xF8u) << 8) |                      \
                (((uint16_t)(g) & 0xFCu) << 3) |                      \
                ((uint16_t)(b) >> 3)))

#define COL_AQI_GOOD      MAKE_C565(  0, 228,   0) /* Green  */
#define COL_AQI_MODERATE  MAKE_C565(255, 255,   0) /* Yellow */
#define COL_AQI_SENS      MAKE_C565(255, 126,   0) /* Orange */
#define COL_AQI_UNHEALTHY MAKE_C565(220,   0,   0) /* Red    */
#define COL_AQI_VERY_UH   MAKE_C565(143,  63, 151) /* Purple */
#define COL_AQI_HAZARDOUS MAKE_C565(126,   0,  35) /* Maroon */
#define COL_AQI_NAVY      MAKE_C565(  0,   0,  80) /* Title  */

#ifdef TFT_BL
#define DISPLAY_TFT_BL TFT_BL
#else
#define DISPLAY_TFT_BL -1
#endif

struct AqiInfo {
    uint16_t bg_color;
    uint16_t fg_color;
    const char *label;
};

struct PanelState {
    uint16_t bg_color;
    char value_text[16];
    char category_text[32];
    bool initialized;
};

/* Panel indices: 0 = PM2.5, 1 = VOC. */
static PanelState s_panels[2] = {};

static char s_footer_text[64] = {};
static bool s_footer_initialized = false;

static int s_wifi_connected = -1;
static int s_last_wifi_drawn = -2;

static int16_t s_value_font_height = 0;
static int16_t s_label_font_height = 0;

static AqiInfo pm25_aqi_info(float pm25)
{
    if (pm25 < 12.1f) {
        return {COL_AQI_GOOD, TFT_BLACK, "Good"};
    }
    if (pm25 < 35.5f) {
        return {COL_AQI_MODERATE, TFT_BLACK, "Moderate"};
    }
    if (pm25 < 55.5f) {
        return {COL_AQI_SENS, TFT_BLACK, "Unhealt. 4 Sens."};
    }
    if (pm25 < 150.5f) {
        return {COL_AQI_UNHEALTHY, TFT_WHITE, "Unhealthy"};
    }
    if (pm25 < 250.5f) {
        return {COL_AQI_VERY_UH, TFT_WHITE, "Very Unhealthy"};
    }

    return {COL_AQI_HAZARDOUS, TFT_WHITE, "Hazardous"};
}

static AqiInfo voc_aqi_info(float voc)
{
    if (voc < 100.0f) {
        return {COL_AQI_GOOD, TFT_BLACK, "Good"};
    }
    if (voc < 250.0f) {
        return {COL_AQI_MODERATE, TFT_BLACK, "Moderate"};
    }
    if (voc < 350.0f) {
        return {COL_AQI_SENS, TFT_BLACK, "Polluted"};
    }
    if (voc < 400.0f) {
        return {COL_AQI_UNHEALTHY, TFT_WHITE, "Very Polluted"};
    }

    return {COL_AQI_VERY_UH, TFT_WHITE, "Severely Poll."};
}

static void draw_wifi_indicator(bool force_redraw)
{
    if (!force_redraw &&
        s_last_wifi_drawn == s_wifi_connected) {
        return;
    }

    /*
     * Clear only the right side of the header. Leave the cyan
     * separator line at the bottom of the header untouched.
     */
    tft.fillRect(
        HEADER_WIFI_AREA_X,
        HEADER_Y,
        HEADER_WIDTH - HEADER_WIFI_AREA_X,
        HEADER_HEIGHT - 1,
        TFT_BLUE);

    tft.loadFont(Arial_Regular_16);
    tft.setTextColor(TFT_WHITE, TFT_BLUE, true);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(
        "WiFi",
        HEADER_WIFI_LED_X - HEADER_WIFI_LED_RADIUS - 8,
        HEADER_WIFI_LED_Y);
    tft.unloadFont();

    tft.fillCircle(
        HEADER_WIFI_LED_X,
        HEADER_WIFI_LED_Y,
        HEADER_WIFI_LED_RADIUS,
        s_wifi_connected > 0 ? TFT_GREEN : TFT_RED);

    s_last_wifi_drawn = s_wifi_connected;
}

static void draw_header(void)
{
    char header_text[48];

    snprintf(
        header_text,
        sizeof(header_text),
        "ID:%lu   MAC:%u",
        (unsigned long)USER_BACNET_DEVICE_INSTANCE,
        (unsigned int)USER_MSTP_MAC_ADDRESS);

    tft.fillRect(
        HEADER_X,
        HEADER_Y,
        HEADER_WIDTH,
        HEADER_HEIGHT,
        TFT_BLUE);

    tft.drawFastHLine(
        HEADER_X,
        HEADER_HEIGHT - 1,
        HEADER_WIDTH,
        TFT_CYAN);

    tft.loadFont(Arial_Regular_16);
    tft.setTextColor(TFT_WHITE, TFT_BLUE, true);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(
        header_text,
        HEADER_TEXT_X,
        HEADER_Y + HEADER_HEIGHT / 2);
    tft.unloadFont();

    draw_wifi_indicator(true);
}

static void draw_footer(float temperature, float humidity)
{
    char footer_text[64];

    snprintf(
        footer_text,
        sizeof(footer_text),
        "Temp: %.1f C   RH: %.1f %%",
        temperature,
        humidity);

    if (s_footer_initialized &&
        strcmp(s_footer_text, footer_text) == 0) {
        return;
    }

    tft.fillRect(
        0,
        FOOTER_Y,
        DISP_WIDTH,
        FOOTER_HEIGHT,
        TFT_BLACK);

    tft.drawFastHLine(
        0,
        FOOTER_Y,
        DISP_WIDTH,
        TFT_CYAN);

    tft.loadFont(Arial_Regular_16);
    tft.setTextColor(TFT_WHITE, TFT_BLACK, true);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(
        footer_text,
        DISP_WIDTH / 2,
        FOOTER_Y + FOOTER_HEIGHT / 2);
    tft.unloadFont();

    snprintf(
        s_footer_text,
        sizeof(s_footer_text),
        "%s",
        footer_text);

    s_footer_initialized = true;
}


static void draw_compact_bold_title(
    int16_t x,
    int16_t y,
    int16_t width,
    const char *title)
{
#if DISPLAY_HAS_ARIAL_BOLD_26
    /*
     * Preferred path: native anti-aliased 26-pixel smooth font.
     * This is used automatically when Arial_Bold_26.h exists.
     */
    tft.loadFont(Arial_Bold_26);
    tft.setTextColor(TFT_WHITE, COL_AQI_NAVY, true);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(
        title,
        x + width / 2,
        y + PANEL_TITLE_H / 2);
    tft.unloadFont();
#else
    /*
     * Build-safe fallback: the project already contains
     * Arial_Bold_36.h. Render it to a sprite and reduce it
     * proportionally to approximately 26 pixels high.
     */
    TFT_eSprite title_sprite(&tft);
    title_sprite.setColorDepth(16);

    const int16_t source_width = width;
    const int16_t source_height = 46;

    if (title_sprite.createSprite(
            source_width,
            source_height) == nullptr) {
        /*
         * Last-resort fallback if the sprite cannot be allocated.
         * Draw the existing 16-pixel smooth font twice with a
         * one-pixel offset to simulate a heavier weight.
         */
        tft.loadFont(Arial_Regular_16);
        tft.setTextColor(TFT_WHITE, COL_AQI_NAVY, true);
        tft.setTextDatum(MC_DATUM);

        const int16_t centre_x = x + width / 2;
        const int16_t centre_y = y + PANEL_TITLE_H / 2;

        tft.drawString(title, centre_x, centre_y);
        tft.drawString(title, centre_x + 1, centre_y);

        tft.unloadFont();
        return;
    }

    title_sprite.fillSprite(COL_AQI_NAVY);
    title_sprite.loadFont(Arial_Bold_36);
    title_sprite.setTextColor(
        TFT_WHITE,
        COL_AQI_NAVY,
        true);
    title_sprite.setTextDatum(MC_DATUM);

    const int16_t source_text_width =
        title_sprite.textWidth(title);
    const int16_t source_font_height =
        title_sprite.fontHeight();

    title_sprite.drawString(
        title,
        source_width / 2,
        source_height / 2);

    const int16_t target_text_height = 26;

    int16_t target_text_width =
        (source_text_width * target_text_height) /
        source_font_height;

    if (target_text_width > width - 4) {
        target_text_width = width - 4;
    }

    const int16_t target_x =
        x + (width - target_text_width) / 2;

    const int16_t target_y =
        y + (PANEL_TITLE_H - target_text_height) / 2;

    const int16_t source_left =
        (source_width - source_text_width) / 2;

    const int16_t source_top =
        (source_height - source_font_height) / 2;

    for (int16_t dy = 0;
         dy < target_text_height;
         ++dy) {
        const int16_t sy =
            source_top +
            (dy * source_font_height) /
                target_text_height;

        for (int16_t dx = 0;
             dx < target_text_width;
             ++dx) {
            const int16_t sx =
                source_left +
                (dx * source_text_width) /
                    target_text_width;

            const uint16_t pixel =
                title_sprite.readPixel(sx, sy);

            tft.drawPixel(
                target_x + dx,
                target_y + dy,
                pixel);
        }
    }

    title_sprite.unloadFont();
    title_sprite.deleteSprite();
#endif
}

static void draw_panel(
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height,
    const char *title,
    const char *value_text,
    const char *unit_text,
    const char *category_text,
    uint16_t bg_color,
    uint16_t fg_color,
    int panel_index)
{
    PanelState *state = &s_panels[panel_index];

    const bool full_redraw =
        !state->initialized ||
        state->bg_color != bg_color;

    const bool content_changed =
        full_redraw ||
        strcmp(state->value_text, value_text) != 0 ||
        strcmp(state->category_text, category_text) != 0;

    if (!content_changed) {
        return;
    }

    const int16_t middle_x = x + width / 2;
    const int16_t content_y = y + 2 + PANEL_TITLE_H + 1;
    const int16_t content_height =
        height - 2 - PANEL_TITLE_H - 1 - 2;

    if (full_redraw) {
        /* Coloured panel background and two-pixel white border. */
        tft.fillRect(x, y, width, height, bg_color);
        tft.drawRect(x, y, width, height, TFT_WHITE);
        tft.drawRect(x + 1, y + 1, width - 2, height - 2, TFT_WHITE);

        /* Navy title bar. */
        tft.fillRect(
            x + 2,
            y + 2,
            width - 4,
            PANEL_TITLE_H,
            COL_AQI_NAVY);

        tft.drawFastHLine(
            x + 2,
            y + 2 + PANEL_TITLE_H,
            width - 4,
            TFT_WHITE);

        draw_compact_bold_title(
            x + 2,
            y + 2,
            width - 4,
            title);
    }

    /*
     * Clear only the content area. The title and border remain unchanged
     * unless the category colour changes.
     */
    tft.fillRect(
        x + 2,
        content_y,
        width - 4,
        content_height,
        bg_color);

    const int16_t value_to_unit_gap = 8;
    const int16_t unit_to_category_gap = 6;

    const int16_t block_height =
        s_value_font_height +
        value_to_unit_gap +
        s_label_font_height +
        unit_to_category_gap +
        s_label_font_height;

    const int16_t block_top =
        content_y + (content_height - block_height) / 2;

    /* Main numeric value. */
    tft.loadFont(Arial_Bold_44);
    tft.setTextColor(fg_color, bg_color, true);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(value_text, middle_x, block_top);
    tft.unloadFont();

    /* Unit and air-quality category. */
    tft.loadFont(Arial_Regular_16);
    tft.setTextColor(fg_color, bg_color, true);
    tft.setTextDatum(TC_DATUM);

    const int16_t unit_y =
        block_top +
        s_value_font_height +
        value_to_unit_gap;

    const int16_t category_y =
        unit_y +
        s_label_font_height +
        unit_to_category_gap;

    tft.drawString(unit_text, middle_x, unit_y);
    tft.drawString(category_text, middle_x, category_y);
    tft.unloadFont();

    state->bg_color = bg_color;

    snprintf(
        state->value_text,
        sizeof(state->value_text),
        "%s",
        value_text);

    snprintf(
        state->category_text,
        sizeof(state->category_text),
        "%s",
        category_text);

    state->initialized = true;
}

static void draw_air_quality_panels(float pm25, float voc)
{
    char value_text[16];

    const AqiInfo pm25_info = pm25_aqi_info(pm25);
    snprintf(value_text, sizeof(value_text), "%.0f", pm25);

    draw_panel(
        PANEL_LEFT_X,
        PANEL_Y,
        PANEL_WIDTH,
        PANEL_HEIGHT,
        "PM2.5",
        value_text,
        "ug/m3",
        pm25_info.label,
        pm25_info.bg_color,
        pm25_info.fg_color,
        0);

    const AqiInfo voc_info = voc_aqi_info(voc);
    snprintf(value_text, sizeof(value_text), "%.0f", voc);

    draw_panel(
        PANEL_RIGHT_X,
        PANEL_Y,
        PANEL_WIDTH,
        PANEL_HEIGHT,
        "VOC",
        value_text,
        "(1 - 500)",
        voc_info.label,
        voc_info.bg_color,
        voc_info.fg_color,
        1);
}

extern "C" void display_init(void)
{
    initArduino();

#if defined(TFT_BL) && defined(TFT_BACKLIGHT_ON) && (TFT_BL >= 0)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    delay(10);
#endif

    tft.init();

#if defined(TFT_BL) && (TFT_BL >= 0)
    if (ledcAttach(TFT_BL, 20000, 8)) {
        ledcWrite(TFT_BL, 125);
    } else {
        ESP_LOGW(TAG, "Failed to initialize backlight PWM");
    }
#endif

    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    /* Cache font heights once for panel content alignment. */
    tft.loadFont(Arial_Bold_44);
    s_value_font_height = tft.fontHeight();
    tft.unloadFont();

    tft.loadFont(Arial_Regular_16);
    s_label_font_height = tft.fontHeight();
    tft.unloadFont();

    draw_header();
    draw_air_quality_panels(0.0f, 0.0f);
    draw_footer(0.0f, 0.0f);

    ESP_LOGI(TAG, "UI profile: ST7789 GMT020");
    ESP_LOGI(TAG, "TFT hardware: %s", USER_SETUP_INFO);

    ESP_LOGI(
        TAG,
        "TFT GPIO: MOSI=%d SCLK=%d MISO=%d "
        "CS=%d DC=%d RST=%d BL=%d",
        TFT_MOSI,
        TFT_SCLK,
        TFT_MISO,
        TFT_CS,
        TFT_DC,
        TFT_RST,
        DISPLAY_TFT_BL);

    ESP_LOGI(
        TAG,
        "TFT native size: %dx%d; runtime size: %dx%d",
        TFT_WIDTH,
        TFT_HEIGHT,
        tft.width(),
        tft.height());
}

extern "C" void display_set_link_status(
    bool wifi_connected,
    bool mstp_connected)
{
    s_wifi_connected = wifi_connected ? 1 : 0;
    draw_wifi_indicator(false);

    /* This compact profile does not display an MS/TP pilot. */
    (void)mstp_connected;
}

extern "C" void display_update_values(
    float pm25,
    float temperature,
    float humidity,
    float voc,
    float temp_ds18b20)
{
    /* DS18B20 is not displayed in this profile. */
    (void)temp_ds18b20;

    draw_air_quality_panels(pm25, voc);
    draw_footer(temperature, humidity);
}
