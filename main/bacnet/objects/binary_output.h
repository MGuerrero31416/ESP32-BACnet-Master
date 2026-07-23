#ifndef BINARY_OUTPUT_H
#define BINARY_OUTPUT_H

#include <stdint.h>
#include <stddef.h>

/* Create and initialize Binary Output objects */
void bacnet_create_binary_outputs(void);

/* Create Binary Output objects and start GPIO sync task */
void bacnet_create_binary_outputs_with_gpio_sync(void);

/* NVS helper functions to persist BO properties */
void bacnet_nvs_save_bo_name(uint32_t instance, const char *name, uint16_t length);
void bacnet_nvs_save_bo_desc(uint32_t instance, const char *desc, uint16_t length);
void bacnet_nvs_save_bo_pv(uint32_t instance, uint8_t value);
void bacnet_nvs_load_bo(uint32_t instance);

/* Update GPIO state when BO1 (PMS5003_SET) is written from BACnet */
void bacnet_bo1_gpio_update(uint8_t state);

#endif /* BINARY_OUTPUT_H */
