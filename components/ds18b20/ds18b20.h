#ifndef DS18B20_H
#define DS18B20_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read the temperature from the DS18B20 connected to GPIO18.
 *
 * The function preserves the existing behavior:
 * - lazy GPIO initialization
 * - up to three attempts
 * - 750 ms conversion delay
 * - scratchpad CRC validation
 *
 * @param temperature_c Receives the temperature in degrees Celsius.
 * @return true when a valid temperature was read; otherwise false.
 */
bool ds18b20_read_temperature(float *temperature_c);

#ifdef __cplusplus
}
#endif

#endif /* DS18B20_H */
