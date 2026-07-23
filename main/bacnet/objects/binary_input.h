#ifndef BINARY_INPUT_H
#define BINARY_INPUT_H

#include <stdint.h>
#include <stddef.h>

/* Create and initialize Binary Input objects */
void bacnet_create_binary_inputs(void);

/* NVS helper functions to persist BI properties */
void bacnet_nvs_save_bi_name(uint32_t instance, const char *name, uint16_t length);
void bacnet_nvs_save_bi_desc(uint32_t instance, const char *desc, uint16_t length);
void bacnet_nvs_save_bi_pv(uint32_t instance, uint8_t value);
void bacnet_nvs_load_bi(uint32_t instance);

#endif /* BINARY_INPUT_H */
