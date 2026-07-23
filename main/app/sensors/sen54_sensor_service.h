#ifndef SEN54_SENSOR_SERVICE_H
#define SEN54_SENSOR_SERVICE_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SEN54_SENSOR_CYCLE_COMPLETE = 0,
    SEN54_SENSOR_RESET_HANDLED
} sen54_sensor_cycle_result_t;

esp_err_t sen54_sensor_service_init(void);

sen54_sensor_cycle_result_t
sen54_sensor_service_cycle(void);

#ifdef __cplusplus
}
#endif

#endif /* SEN54_SENSOR_SERVICE_H */