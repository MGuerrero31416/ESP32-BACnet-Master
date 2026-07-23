#ifndef ANALOG_INPUT_H
#define ANALOG_INPUT_H

#include <stdint.h>
#include <stddef.h>

/* Create and initialize Analog Input objects */
void bacnet_create_analog_inputs(void);

/* NVS helper functions to persist AI properties */
void bacnet_nvs_save_ai_name(uint32_t instance, const char *name, uint16_t length);
void bacnet_nvs_save_ai_desc(uint32_t instance, const char *desc, uint16_t length);
void bacnet_nvs_save_ai_pv(uint32_t instance, float value);
void bacnet_nvs_load_ai(uint32_t instance);

#endif /* ANALOG_INPUT_H */
