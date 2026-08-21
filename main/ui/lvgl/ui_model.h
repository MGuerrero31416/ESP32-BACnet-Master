#ifndef UI_MODEL_H
#define UI_MODEL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float pm25;
    float temperature;
    float humidity;
    float voc;
    float temp_ds18b20;
    bool wifi_connected;
    bool mstp_connected;
} ui_model_t;

const ui_model_t *ui_model_get(void);

void ui_model_set_values(
    float pm25,
    float temperature,
    float humidity,
    float voc,
    float temp_ds18b20);

void ui_model_set_link(bool wifi_connected, bool mstp_connected);

#ifdef __cplusplus
}
#endif

#endif /* UI_MODEL_H */
