#ifndef _HAL_H
#define _HAL_H

/***** Includes *****/

#include <stdbool.h>

/***** Global Functions *****/

extern bool
hal_init(void);

extern bool
hal_measure_temperature_humdity(float * p_temperature, float * p_humidity);

#endif
