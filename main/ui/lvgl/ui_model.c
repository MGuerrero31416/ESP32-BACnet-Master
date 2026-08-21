#include "ui_model.h"

#include "ui_manager.h"
#include "ui_port.h"

#define UI_MODEL_LOCK_TIMEOUT_MS 50

static ui_model_t s_model;

const ui_model_t *ui_model_get(void)
{
    return &s_model;
}

static void publish(void)
{
    if (!ui_port_is_ready()) {
        return;
    }

    if (!ui_port_lock(UI_MODEL_LOCK_TIMEOUT_MS)) {
        return;
    }

    ui_manager_notify_data(&s_model);
    ui_port_unlock();
}

void ui_model_set_values(
    float pm25,
    float temperature,
    float humidity,
    float voc,
    float temp_ds18b20)
{
    s_model.pm25 = pm25;
    s_model.temperature = temperature;
    s_model.humidity = humidity;
    s_model.voc = voc;
    s_model.temp_ds18b20 = temp_ds18b20;
    publish();
}

void ui_model_set_link(bool wifi_connected, bool mstp_connected)
{
    s_model.wifi_connected = wifi_connected;
    s_model.mstp_connected = mstp_connected;
    publish();
}
