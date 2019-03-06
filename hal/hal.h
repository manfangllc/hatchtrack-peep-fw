#ifndef _HAL_H
#define _HAL_H

/***** Includes *****/

#include <stdbool.h>

/***** Global Functions *****/

extern bool
hal_init(void);

extern bool
hal_read_temperature_humdity_pressure_resistance(float * p_temperature,
  float * p_humidity, float * p_pressure, float * p_gas_resistance);

extern bool
hal_read_accel(float * p_gx, float * p_gy, float * p_gz);

#endif
