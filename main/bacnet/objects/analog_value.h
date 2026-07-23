#ifndef ANALOG_VALUE_H
#define ANALOG_VALUE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "esp_err.h"

/* Create and initialize Analog Value objects */
void bacnet_create_analog_values(void);

/* NVS helper functions to persist AV properties */
void bacnet_nvs_save_av_name(uint32_t instance, const char *name, uint16_t length);
void bacnet_nvs_save_av_desc(uint32_t instance, const char *desc, uint16_t length);
void bacnet_nvs_save_av_units(uint32_t instance, uint16_t units);
void bacnet_nvs_save_av_pv(uint32_t instance, float value);
void bacnet_nvs_load_av(uint32_t instance);
esp_err_t bacnet_nvs_load_av_pv(
	uint32_t instance,
	float *value,
	bool *found);

#endif /* ANALOG_VALUE_H */
