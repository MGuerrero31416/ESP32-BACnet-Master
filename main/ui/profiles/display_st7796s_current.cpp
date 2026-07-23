#include "display.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Arial_Bold_44.h"
#include "Arial_Regular_16.h"
#include "User_Settings.h"
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/av.h"
#include "esp_netif.h"
#include "lwip/ip4_addr.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"

static TFT_eSPI tft;
static const char *TAG = "display";

// Rotation 3 produces 480x320 coordinate space on this panel.
#define DISP_X0    0
#define DISP_Y0    0
#define DISP_X1    479
#define DISP_Y1    319
#define DISP_WIDTH 480
#define DISP_HEIGHT 320

#define HEADER_HEIGHT   56
#define HEADER_LED_R    7

// Middle panel layout (Temperature & Humidity, area Y 58..183)
#define MID_PANEL_Y      58
#define MID_PANEL_H      124
#define MID_LEFT_X       6
#define MID_LEFT_W       231
#define MID_RIGHT_X      243
#define MID_RIGHT_W      231

// AQI panel layout (bottom free area: Y 188..319)
#define AQI_SEP_Y        185
#define AQI_PANEL_Y      188
#define AQI_PANEL_H      131
#define AQI_LEFT_X       6
#define AQI_LEFT_W       231
#define AQI_RIGHT_X      243
#define AQI_RIGHT_W      231
#define AQI_TITLE_H      24

// RGB565 compile-time colour constants
#define MAKE_C565(r,g,b) \
    ((uint16_t)((((uint16_t)(r) & 0xF8u) << 8) | \
                (((uint16_t)(g) & 0xFCu) << 3) | \
                ( (uint16_t)(b)           >> 3)))
#define COL_AQI_GOOD      MAKE_C565(  0, 228,   0)  /* Green  */
#define COL_AQI_MODERATE  MAKE_C565(255, 255,   0)  /* Yellow */
#define COL_AQI_SENS      MAKE_C565(255, 126,   0)  /* Orange */
#define COL_AQI_UNHEALTHY MAKE_C565(220,   0,   0)  /* Red    */
#define COL_AQI_VERY_UH   MAKE_C565(143,  63, 151)  /* Purple */
#define COL_AQI_HAZARDOUS MAKE_C565(126,   0,  35)  /* Maroon */
#define COL_AQI_NAVY      MAKE_C565(  0,   0,  80)  /* Title  */
#define HEADER_WIFI_X      376
#define HEADER_MSTP_X      376
#define HEADER_WIFI_LED_Y  17
#define HEADER_MSTP_LED_Y  41

static char s_last_ip_text[24] = "";
static uint32_t s_header_refresh_tick = 0;
static int s_wifi_connected = -1;
static int s_mstp_connected = -1;


// Per-panel state: track what was last drawn so we can skip redraws of static
// elements (title bar, border) when only values change.
struct PanelState {
    uint16_t bg_col;
    char     num_str[16];
    char     cat_str[32];
    bool     initialized;
};
// Indices: 0=Temperature, 1=Humidity, 2=PM2.5 AQI, 3=VOC Index, 4=temp_ds18b20
static PanelState s_panel[5] = {};

// Font heights pre-computed once in display_init to avoid loading/unloading
// fonts on every partial redraw.
static int16_t s_num_h = 0;  // Arial_Bold_44
static int16_t s_lbl_h = 0;  // Arial_Regular_16

static void get_ip_text(char *ip_text, size_t ip_text_size) {
    if (!ip_text || ip_text_size == 0) {
        return;
    }

    snprintf(ip_text, ip_text_size, "N/A");

    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif) {
        esp_netif_ip_info_t ip_info{};
        if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
            snprintf(ip_text, ip_text_size, IPSTR, IP2STR(&ip_info.ip));
        }
    }

    if (ip_text[0] == 'N' && USER_WIFI_USE_STATIC_IP) {
        snprintf(ip_text, ip_text_size, "%s", USER_WIFI_STATIC_IP_ADDR);
    }
}

// ---- AQI category helpers ----------------------------------------

struct AqiInfo {
    uint16_t    bgColor;
    uint16_t    fgColor;
    const char *label;
};

static AqiInfo pm25_aqi_info(float pm25)
{
    if (pm25 < 12.1f)   return { COL_AQI_GOOD,      TFT_BLACK, "Good" };
    if (pm25 < 35.5f)   return { COL_AQI_MODERATE,  TFT_BLACK, "Moderate" };
    if (pm25 < 55.5f)   return { COL_AQI_SENS,       TFT_BLACK, "Unhealt. 4 Sens." };
    if (pm25 < 150.5f)  return { COL_AQI_UNHEALTHY,  TFT_WHITE, "Unhealthy" };
    if (pm25 < 250.5f)  return { COL_AQI_VERY_UH,    TFT_WHITE, "Very Unhealthy" };
    return                     { COL_AQI_HAZARDOUS,   TFT_WHITE, "Hazardous" };
}

static AqiInfo voc_aqi_info(float voc)
{
    if (voc < 100.0f)   return { COL_AQI_GOOD,      TFT_BLACK, "Good" };
    if (voc < 250.0f)   return { COL_AQI_MODERATE,  TFT_BLACK, "Moderate" };
    if (voc < 350.0f)   return { COL_AQI_SENS,       TFT_BLACK, "Polluted" };
    if (voc < 400.0f)   return { COL_AQI_UNHEALTHY,  TFT_WHITE, "Very Polluted" };
    return                     { COL_AQI_VERY_UH,    TFT_WHITE, "Severely Polut." };
}

// Thresholds tuned for Bangkok tropical summer (acclimatised population).
static AqiInfo temp_info(float temp)
{
    if (temp < 20.0f)   return { MAKE_C565(  0, 128, 220), TFT_WHITE, "Too Cold" };
    if (temp < 24.0f)   return { MAKE_C565(  0, 192, 255), TFT_BLACK, "Cool" };
    if (temp < 28.0f)   return { COL_AQI_GOOD,             TFT_BLACK, "Comfortable" };
    if (temp < 33.0f)   return { COL_AQI_MODERATE,         TFT_BLACK, "Warm" };
    if (temp < 38.0f)   return { COL_AQI_SENS,             TFT_BLACK, "Hot" };
    return                     { COL_AQI_UNHEALTHY,         TFT_WHITE, "Very Hot" };
}

// Thresholds tuned for Bangkok tropical indoor conditions.
static AqiInfo hum_info(float hum)
{
    if (hum < 40.0f)    return { COL_AQI_UNHEALTHY, TFT_WHITE, "Very Dry" };
    if (hum < 50.0f)    return { COL_AQI_SENS,       TFT_BLACK, "Dry" };
    if (hum < 70.0f)    return { COL_AQI_GOOD,       TFT_BLACK, "Comfortable" };
    if (hum < 80.0f)    return { COL_AQI_MODERATE,   TFT_BLACK, "Humid" };
    return                     { COL_AQI_VERY_UH,    TFT_WHITE, "Very Humid" };
}

// draw_aqi_panel – only redraws what actually changed to avoid flickering.
//
// full redraw  (bg colour changed or first paint):
//   – border, navy title bar, title text, content area
// partial update (bg colour same, but value/category changed):
//   – content area only; title bar and border are left untouched
//
// panel_idx:  0=Temperature, 1=Humidity, 2=PM2.5 AQI, 3=VOC Index, 4=temp_ds18b20
static void draw_aqi_panel(int16_t px, int16_t py, int16_t pw, int16_t ph,
                            const char *title,
                            const char *num_str,  const char *unit_str,
                            const char *cat_str,
                            uint16_t    bg_col,   uint16_t fg_col,
                            int         panel_idx)
{
    PanelState *st = &s_panel[panel_idx];

    bool full    = !st->initialized || st->bg_col != bg_col;
    bool content = full
                 || strcmp(st->num_str, num_str) != 0
                 || strcmp(st->cat_str, cat_str) != 0;

    if (!content) return;  // nothing to update

    int16_t mid_x  = px + pw / 2;
    int16_t ct_top = py + 2 + AQI_TITLE_H + 1;
    int16_t ct_h   = ph - 2 - AQI_TITLE_H - 1 - 2;

    if (full) {
        // Coloured background + 2-pixel white border
        tft.fillRect(px, py, pw, ph, bg_col);
        tft.drawRect(px,   py,   pw,   ph,   TFT_WHITE);
        tft.drawRect(px+1, py+1, pw-2, ph-2, TFT_WHITE);
        // Navy title bar + separator line
        tft.fillRect(px+2, py+2, pw-4, AQI_TITLE_H, COL_AQI_NAVY);
        tft.drawFastHLine(px+2, py+2+AQI_TITLE_H, pw-4, TFT_WHITE);
        // Title – Arial Regular 16, cyan, centred in title bar
        int16_t tit_y = (py + 2) + (AQI_TITLE_H - s_lbl_h) / 2;
        tft.loadFont(Arial_Regular_16);
        tft.setTextColor(TFT_CYAN, COL_AQI_NAVY, true);
        tft.setTextDatum(TC_DATUM);
        tft.drawString(title, mid_x, tit_y);
        tft.unloadFont();
    }

    // Erase content area and redraw value, unit, category.
    // Uses the same bg_col fill so there is no flash of a different colour.
    tft.fillRect(px+2, ct_top, pw-4, ct_h, bg_col);

    // Vertical block: number + 10 + unit + 6 + category
    int16_t blk_h   = s_num_h + 10 + s_lbl_h + 6 + s_lbl_h;
    int16_t blk_top = ct_top + (ct_h - blk_h) / 2;

    // Value number – Arial Bold 44
    tft.loadFont(Arial_Bold_44);
    tft.setTextColor(fg_col, bg_col, true);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(num_str, mid_x, blk_top + 6);
    tft.unloadFont();

    // Unit and category – Arial Regular 16
    // Temperature (0) and Humidity (1) labels sit 2 px higher than AQI panels.
    int16_t lbl_off = (panel_idx <= 1) ? -2 : 0;
    tft.loadFont(Arial_Regular_16);
    tft.setTextColor(fg_col, bg_col, true);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(unit_str, mid_x, blk_top + s_num_h + 10 + lbl_off);
    tft.drawString(cat_str,  mid_x, blk_top + s_num_h + 10 + s_lbl_h + 2 + lbl_off);
    tft.unloadFont();

    // Persist state for next call
    st->bg_col = bg_col;
    snprintf(st->num_str, sizeof(st->num_str), "%s", num_str);
    snprintf(st->cat_str, sizeof(st->cat_str), "%s", cat_str);
    st->initialized = true;
}

static void draw_aqi_panels(float pm25, float voc) // PM2.5     and VOC Index panels
{
    char num_buf[16];

    AqiInfo pm = pm25_aqi_info(pm25);
    snprintf(num_buf, sizeof(num_buf), "%.0f", pm25);
    draw_aqi_panel(AQI_LEFT_X,  AQI_PANEL_Y, AQI_LEFT_W,  AQI_PANEL_H,
                   "PM2.5 AQI", num_buf, "ug/m3", // AQI category string
                   pm.label, pm.bgColor, pm.fgColor, 2);

    AqiInfo vo = voc_aqi_info(voc);
    snprintf(num_buf, sizeof(num_buf), "%.0f", voc);
    draw_aqi_panel(AQI_RIGHT_X, AQI_PANEL_Y, AQI_RIGHT_W, AQI_PANEL_H,
                   "VOC Index", num_buf, "(1 - 500)", // AQI category string
                   vo.label, vo.bgColor, vo.fgColor, 3);
}

static void draw_mid_panels(float temp, float hum) // Temperature and Humidity panels
{
    char num_buf[16];

    AqiInfo ti = temp_info(temp);
    snprintf(num_buf, sizeof(num_buf), "%.1f", temp);
    draw_aqi_panel(MID_LEFT_X,  MID_PANEL_Y, MID_LEFT_W,  MID_PANEL_H,
                   "Temperature", num_buf, "C", // AQI category string
                   ti.label, ti.bgColor, ti.fgColor, 0);

    AqiInfo hi = hum_info(hum);
    snprintf(num_buf, sizeof(num_buf), "%.1f", hum);
    draw_aqi_panel(MID_RIGHT_X, MID_PANEL_Y, MID_RIGHT_W, MID_PANEL_H,
                   "Humidity", num_buf, "%", // AQI category string
                   hi.label, hi.bgColor, hi.fgColor, 1);
}

// ------------------------------------------------------------------
// Insert dsb18b20 temperature 

static void draw_ds18b20_temperature(
    float temp_ds18b20,
    float sen54_temperature)
{
    char text[16];

    snprintf(
        text,
        sizeof(text),
        "DS %.1f",
        temp_ds18b20);

    /* Match the current Temperature panel background and text colours. */
    AqiInfo ti = temp_info(sen54_temperature);

    /*
     * Small area at the upper-right of the coloured Temperature panel.
     * Adjust X/Y slightly later if desired.
     */
    const int16_t x = MID_LEFT_X + MID_LEFT_W - 7;
    const int16_t y = MID_PANEL_Y + AQI_TITLE_H + 9;

    /* Clear only the small overlay area. */
    tft.fillRect(
        MID_LEFT_X + MID_LEFT_W - 62,
        MID_PANEL_Y + AQI_TITLE_H + 5,
        56,
        14,
        ti.bgColor);

    /* TFT_eSPI built-in font 1: small 6x8 pixel font. */
    tft.setTextFont(1);
    tft.setTextColor(ti.fgColor, ti.bgColor, true);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(text, x, y);
}

static void draw_sen54_temp_comp_overlay(float sen54_temperature)
{
    char line[3][16];

    const float offset = Analog_Value_Present_Value(
        user_av_instance(USER_AV_SEN54_TEMP_COMP_OFFSET));
    const float slope = Analog_Value_Present_Value(
        user_av_instance(USER_AV_SEN54_TEMP_COMP_SLOPE));
    const float time_constant = Analog_Value_Present_Value(
        user_av_instance(USER_AV_SEN54_TEMP_COMP_TIME_CONSTANT));

    snprintf(line[0], sizeof(line[0]), "Off %.2f", offset);
    snprintf(line[1], sizeof(line[1]), "Slp %.3f", slope);
    snprintf(line[2], sizeof(line[2]), "Time %.0f", time_constant);

    AqiInfo ti = temp_info(sen54_temperature);

    const int16_t x = MID_LEFT_X + 7;
    const int16_t line_gap = 9;
    const int16_t y3 = MID_PANEL_Y + MID_PANEL_H - 12;
    const int16_t y2 = y3 - line_gap;
    const int16_t y1 = y2 - line_gap;

    // Clear only the compact lower-left overlay box before redrawing.
    tft.fillRect(MID_LEFT_X + 4, y1 - 1, 62, 30, ti.bgColor);

    tft.setTextFont(1);
    tft.setTextColor(ti.fgColor, ti.bgColor, true);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(line[0], x, y1);
    tft.drawString(line[1], x, y2);
    tft.drawString(line[2], x, y3);
}



// ------------------------------------------------------------------

static void draw_header(void) {
    char line[48];
    char ip_text[24] = "N/A";
    get_ip_text(ip_text, sizeof(ip_text));

    tft.fillRect(DISP_X0, DISP_Y0, DISP_WIDTH, HEADER_HEIGHT, TFT_BLUE);
    tft.drawFastHLine(DISP_X0, HEADER_HEIGHT, DISP_WIDTH, TFT_CYAN);

    tft.loadFont(Arial_Regular_16);
    int16_t fh = tft.fontHeight();
    int16_t y1 = (HEADER_HEIGHT / 2 - fh) / 2;
    int16_t y2 = HEADER_HEIGHT / 2 + (HEADER_HEIGHT / 2 - fh) / 2;
    tft.setTextColor(TFT_WHITE, TFT_BLUE, true);
    tft.setTextDatum(TL_DATUM);

    snprintf(line, sizeof(line), "BACnet ID: %lu", (unsigned long)USER_BACNET_DEVICE_INSTANCE);
    tft.drawString(line, 8, y1);

    snprintf(line, sizeof(line), "IP: %s", ip_text);
    tft.drawString(line, 8, y2);
    tft.unloadFont();

    snprintf(s_last_ip_text, sizeof(s_last_ip_text), "%s", ip_text);
}

static void draw_link_indicators(bool force_redraw)
{
    if (!force_redraw && s_wifi_connected < 0 && s_mstp_connected < 0) {
        return;
    }

    static int last_wifi = -1;
    static int last_mstp = -1;
    if (!force_redraw && last_wifi == s_wifi_connected && last_mstp == s_mstp_connected) {
        return;
    }

    tft.fillRect(312, 2, 166, 52, TFT_BLUE);

    tft.loadFont(Arial_Regular_16);
    int16_t fh = tft.fontHeight();
    int16_t y1 = (HEADER_HEIGHT / 2 - fh) / 2;
    int16_t y2 = HEADER_HEIGHT / 2 + (HEADER_HEIGHT / 2 - fh) / 2;
    tft.setTextColor(TFT_WHITE, TFT_BLUE, true);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("WiFi", 314, y1);
    tft.drawString("MSTP", 314, y2);
    tft.unloadFont();

    tft.fillCircle(HEADER_WIFI_X, HEADER_WIFI_LED_Y, HEADER_LED_R,
                   s_wifi_connected > 0 ? TFT_GREEN : TFT_RED);
    tft.fillCircle(HEADER_MSTP_X, HEADER_MSTP_LED_Y, HEADER_LED_R,
                   s_mstp_connected > 0 ? TFT_GREEN : TFT_RED);

    last_wifi = s_wifi_connected;
    last_mstp = s_mstp_connected;
}

static void refresh_ip_line_if_changed(void) {
    char ip_text[24] = "N/A";
    char line[48];

    get_ip_text(ip_text, sizeof(ip_text));
    if (strcmp(ip_text, s_last_ip_text) == 0) {
        return;
    }

    tft.loadFont(Arial_Regular_16);
    int16_t fh = tft.fontHeight();
    int16_t y2 = HEADER_HEIGHT / 2 + (HEADER_HEIGHT / 2 - fh) / 2;
    tft.fillRect(8, y2, 300, fh + 2, TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE, true);
    tft.setTextDatum(TL_DATUM);
    snprintf(line, sizeof(line), "IP: %s", ip_text);
    tft.drawString(line, 8, y2);
    tft.unloadFont();

    snprintf(s_last_ip_text, sizeof(s_last_ip_text), "%s", ip_text);
}

extern "C" void display_init(void) {
    printf("[DISP] initArduino...\n");
    initArduino();

    printf("[DISP] Profile: ST7796S_CURRENT\n");

    printf("[DISP] backlight on...\n");
    // TFT_eSPI drives the backlight via TFT_BL / TFT_BACKLIGHT_ON in User_Setup.h;
    // assert it explicitly here as well.
        #if defined(TFT_BL) && (TFT_BL >= 0)
            pinMode(TFT_BL, OUTPUT);
            digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
            delay(10);
        #endif

    printf("[DISP] tft.init()...\n");
    tft.init();
    printf("[DISP] tft.init() done, setRotation...\n");
    tft.setRotation(1);

    // print both the UI profile and hardware setup during startup
        ESP_LOGI(TAG, "UI profile: ST7796S CURRENT");
        ESP_LOGI(TAG, "TFT hardware: %s", USER_SETUP_INFO);
        ESP_LOGI(
            TAG,
            "TFT GPIO: MOSI=%d SCLK=%d MISO=%d CS=%d DC=%d RST=%d BL=%d",
                TFT_MOSI,
                TFT_SCLK,
                TFT_MISO,
                TFT_CS,
                TFT_DC,
                TFT_RST,
                TFT_BL);
        ESP_LOGI(
            TAG,
            "TFT native size: %dx%d; runtime size: %dx%d",
                TFT_WIDTH,
                TFT_HEIGHT,
                tft.width(),
                tft.height());

    // Pre-compute font heights used by draw_aqi_panel for content layout.
    // Done once here so those values are available for every panel redraw
    // without needing to load/unload fonts mid-render.
    tft.loadFont(Arial_Bold_44);
    s_num_h = tft.fontHeight();
    tft.unloadFont();
    tft.loadFont(Arial_Regular_16);
    s_lbl_h = tft.fontHeight();
    tft.unloadFont();

    printf("[DISP] fillScreen...\n");
    tft.fillScreen(TFT_BLACK);
    delay(1);

    draw_header();
    draw_link_indicators(true);
    draw_mid_panels(0.0f, 0.0f);
    draw_sen54_temp_comp_overlay(0.0f);
    tft.drawFastHLine(DISP_X0, AQI_SEP_Y, DISP_WIDTH, TFT_CYAN);
    draw_aqi_panels(0.0f, 0.0f);

    printf("Display initialized\n");
}

extern "C" void display_set_link_status(bool wifi_connected, bool mstp_connected)
{
    s_wifi_connected = wifi_connected ? 1 : 0;
    s_mstp_connected = mstp_connected ? 1 : 0;
    draw_link_indicators(false);
}

extern "C" void display_update_values(
                                        float pm25,
                                        float temperature,
                                        float humidity,
                                        float voc,
                                        float temp_ds18b20) { 
    // Refresh header IP occasionally and only if it changed to avoid flicker.
    s_header_refresh_tick++;
    if ((s_header_refresh_tick % 5U) == 0U) {
        refresh_ip_line_if_changed();
        draw_link_indicators(false);
    }

    // draw_mid_panels / draw_aqi_panels delegate change-detection to
    // draw_aqi_panel, which returns immediately if nothing has changed.
    draw_mid_panels(temperature, humidity); // May clear and redraw yellow area
    draw_sen54_temp_comp_overlay(temperature);
    draw_ds18b20_temperature(  // Restores small DS18B20 overlay
        temp_ds18b20,
        temperature);
    draw_aqi_panels(pm25, voc);
}
