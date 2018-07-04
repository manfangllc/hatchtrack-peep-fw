#ifndef _SENSOR_H
#define _SENSOR_H

/***** Includes *****/

#include <stdbool.h>

/***** Global Functions *****/

extern bool
sensor_init(void);

extern bool
sensor_measure(float * p_temperature, float * p_humidity);

#endif
