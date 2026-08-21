# LVGL UI Architecture

Applies to the `CONFIG_USER_DISPLAY_LVGL_TDISPLAY_S3` display profile.

The LVGL user interface is split into four layers so that screens can be added,
reordered, or restyled without touching board bring-up code.

```
main/ui/
  display.h                       Public API used by the app supervisor
  profiles/
    display_lvgl_tdisplay_s3.cpp  Board bring-up only (panel + touch)
  lvgl/
    ui_port.h/.c                  LVGL runtime: drivers, task, tick, lock
    ui_theme.h/.c                 Colors, fonts, widget factories
    ui_model.h/.c                 Latest sensor values and link state
    ui_manager.h/.c               Screen registry and navigation
    ui_screens.c                  The registration table
    screens/
      screen_measurements.c
      screen_air_quality.c
      screen_placeholder.c
      screen_settings.c
      screen_wifi.c
```

Carousel order: Measurements, Air Quality, Placeholder, Settings. Wi-Fi is a
drill-down page reached from the Settings tile.

## Layers

### 1. Board profile — `profiles/display_lvgl_tdisplay_s3.cpp`

Owns everything hardware specific and nothing else:

- LCD power rail (`BOARD_LCD_POWER_EN_PIN`), `TFT_eSPI` init, rotation, backlight PWM.
- `panel_flush()` — pushes a rendered LVGL area to the panel.
- `panel_touch()` — reads the CST816 and rotates its portrait coordinates 90 degrees
  clockwise into landscape LVGL coordinates.
- Implements the three `display.h` entry points by forwarding to `ui_port` /
  `ui_manager` / `ui_model`.

A second LVGL board only needs a new file at this layer; all screens are reused.

### 2. LVGL port — `ui_port.h`

```c
esp_err_t ui_port_init(const ui_port_config_t *cfg);
bool ui_port_lock(uint32_t timeout_ms);
void ui_port_unlock(void);
```

`ui_port_init()` receives the resolution and the two board hooks, then sets up the
draw buffer (DMA-capable heap allocation), the display driver, the pointer input
device, the 1 ms `lv_tick_inc` timer, and the `lvgl` FreeRTOS task.

LVGL is **not** thread safe. The internal task holds a recursive mutex while calling
`lv_timer_handler()`. Any code touching LVGL from another task must wrap the calls:

```c
if (ui_port_lock(50)) {
    /* LVGL calls here */
    ui_port_unlock();
}
```

### 3. Theme — `ui_theme.h`

All colors, fonts, and spacing constants live here, so restyling every screen is a
one-file change. It also provides the factories screens are expected to use:

| Function | Purpose |
|---|---|
| `ui_theme_screen_create()` | Black, non-scrollable root screen object |
| `ui_theme_content_create()` | Transparent vertical flex container, inset by `UI_NAV_ZONE_W` on both sides so it never overlaps the navigation arrows |
| `ui_theme_row_create()` | Transparent horizontal flex row; first child aligns left, last child aligns right |
| `ui_theme_label_create()` / `ui_theme_title_create()` | Styled labels |
| `ui_theme_card_create()` | Rounded tile used for settings entries |
| `ui_theme_value_set()` | `printf`-formats a float into a label |

Screens use flex layout rather than absolute pixel positions, so they survive font
and resolution changes.

### 4. Model — `ui_model.h`

A single `ui_model_t` holds the latest measurements and link status. Producers call
`ui_model_set_values()` / `ui_model_set_link()`; the model takes the LVGL lock and
calls `ui_manager_notify_data()`, which forwards to the active screen's `on_data`
callback. Screens never read globals directly, and producers never know which
screens exist.

### 5. Manager and registry — `ui_manager.h`, `ui_screens.c`

Each screen is described by a descriptor:

```c
typedef struct {
    const char *id;
    bool in_carousel;      /* reachable via left/right arrows */
    const char *parent_id; /* back-arrow target for non-carousel screens */
    lv_obj_t *(*create)(void);
    void (*on_data)(const ui_model_t *model);
    void (*on_show)(void);
} ui_screen_desc_t;
```

`ui_screens.c` lists the descriptors. That array order **is** the carousel order.
The manager:

- creates screens lazily on first display and caches the `lv_obj_t *`;
- adds the `<` / `>` arrows itself, based on the nearest carousel neighbour, and
  omits an arrow at the ends of the list — so dead arrows cannot occur;
- adds a back arrow to `parent_id` for screens with `in_carousel = false`;
- routes `ui_manager_bind_nav(obj, "id")` clicks for drill-down targets such as the
  Wi-Fi tile on the Settings screen;
- dispatches model updates only to the screen currently on display.

## Adding a screen

1. Create `main/ui/lvgl/screens/screen_<name>.c`:

```c
#include "ui_manager.h"
#include "ui_theme.h"

static lv_obj_t *create(void)
{
    lv_obj_t *screen = ui_theme_screen_create();
    lv_obj_t *content = ui_theme_content_create(screen);

    ui_theme_title_create(content, "My Screen");
    return screen;
}

const ui_screen_desc_t screen_my = {
    .id = "my",
    .in_carousel = true,
    .parent_id = NULL,
    .create = create,
    .on_data = NULL,
    .on_show = NULL,
};
```

2. Declare and list it in `main/ui/lvgl/ui_screens.c`.
3. Add the source path to the `CONFIG_USER_DISPLAY_LVGL_TDISPLAY_S3` branch in
   `main/CMakeLists.txt`.

Do not create the navigation arrows yourself; the manager owns them. Keep content
inside `ui_theme_content_create()` so the arrow zones stay clear.

## Common tasks

| Task | Where |
|---|---|
| Reorder screens | `g_ui_screens[]` in `ui_screens.c` |
| Change colors or fonts | `ui_theme.h` |
| Add a value to the UI | Add a field to `ui_model_t`, set it in `ui_model_set_*`, read it in a screen `on_data` |
| Add a drill-down page | `in_carousel = false`, set `parent_id`, call `ui_manager_bind_nav()` from the parent screen |
| Change panel or touch wiring | `display_lvgl_tdisplay_s3.cpp` only |
| Change LVGL task priority or buffer size | `ui_port.c` / `ui_port_config_t` |

## Threading summary

| Context | Rule |
|---|---|
| `lvgl` task | Holds the lock while running `lv_timer_handler()` every 5 ms |
| `esp_timer` | `lv_tick_inc(1)` every 1 ms; no LVGL object access |
| Sensor / app tasks | Must go through `ui_model_set_*`, which locks internally |
| `display_init()` | Locks explicitly around `ui_manager_init()` |
