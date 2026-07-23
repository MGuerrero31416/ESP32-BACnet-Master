#include "display.h"

void display_init(void)
{
}

void display_update_values(
    float pm25,
    float temperature,
    float humidity,
    float voc,
    float temp_ds18b20)
{
    (void)pm25;
    (void)temperature;
    (void)humidity;
    (void)voc;
    (void)temp_ds18b20;
}

void display_set_link_status(
    bool wifi_connected,
    bool mstp_connected)
{
    (void)wifi_connected;
    (void)mstp_connected;
}