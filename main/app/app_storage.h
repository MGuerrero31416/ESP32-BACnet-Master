#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t app_storage_init(void);

/*
 * Returns true during a boot that is restoring compiled defaults
 * instead of loading persisted BACnet object values.
 */
bool app_storage_override_enabled(void);

#ifdef __cplusplus
}
#endif