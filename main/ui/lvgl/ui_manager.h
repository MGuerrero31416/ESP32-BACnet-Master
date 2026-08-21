#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

#include "lvgl.h"
#include "ui_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One screen of the UI. Add a new screen by writing a file that defines a
   ui_screen_desc_t and adding it to the table in ui_screens.c. */
typedef struct {
    const char *id;
    /* Part of the left/right arrow carousel; order follows the table. */
    bool in_carousel;
    /* Back-arrow target for screens outside the carousel. */
    const char *parent_id;
    lv_obj_t *(*create)(void);
    void (*on_data)(const ui_model_t *model);
    void (*on_show)(void);
} ui_screen_desc_t;

extern const ui_screen_desc_t *const g_ui_screens[];
extern const size_t g_ui_screen_count;

void ui_manager_init(void);
void ui_manager_show(const char *id);
void ui_manager_notify_data(const ui_model_t *model);

/* Make any object navigate to another screen when clicked. */
void ui_manager_bind_nav(lv_obj_t *obj, const char *target_id);

#ifdef __cplusplus
}
#endif

#endif /* UI_MANAGER_H */
