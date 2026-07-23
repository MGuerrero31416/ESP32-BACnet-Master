#ifndef DISPLAY_H
#define DISPLAY_H
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the display */
void display_init(void);

/* Update display with BACnet object values */
        void display_update_values(
            float pm25,
            float temperature,
            float humidity,
            float voc,
            float temp_ds18b20);

/* Update header link indicators (WiFi and MS/TP). */
void display_set_link_status(bool wifi_connected, bool mstp_connected);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */
