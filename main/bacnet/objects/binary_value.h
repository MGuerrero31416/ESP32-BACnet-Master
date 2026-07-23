#ifndef BINARY_VALUE_H
#define BINARY_VALUE_H

#include <stdint.h>
#include <stddef.h>

/* Create and initialize Binary Value objects */
void bacnet_create_binary_values(void);

/* NVS helper functions to persist BV properties */
void bacnet_nvs_save_bv_name(uint32_t instance, const char *name, uint16_t length);
void bacnet_nvs_save_bv_desc(uint32_t instance, const char *desc, uint16_t length);
void bacnet_nvs_save_bv_pv(uint32_t instance, uint8_t value);
void bacnet_nvs_load_bv(uint32_t instance);

#endif /* BINARY_VALUE_H */
