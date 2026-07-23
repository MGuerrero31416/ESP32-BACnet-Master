#ifndef DS18B20_SENSOR_SERVICE_H
#define DS18B20_SENSOR_SERVICE_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ds18b20_sensor_service_init(void);

void ds18b20_sensor_service_cycle(void);

#ifdef __cplusplus
}
#endif

#endif /* DS18B20_SENSOR_SERVICE_H */