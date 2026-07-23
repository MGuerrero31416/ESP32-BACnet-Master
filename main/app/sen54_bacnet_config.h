#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void sen54_bacnet_config_register_callback(void);
esp_err_t sen54_bacnet_config_startup_sync(void);
esp_err_t sen54_bacnet_config_reapply_saved(void);

#ifdef __cplusplus
}
#endif
