#ifndef _HATCH_MEASUREMENT_H
#define _HATCH_MEASUREMENT_H

/***** Includes *****/

#include <stdint.h>

/***** Structs *****/

struct hatch_measurement {
  uint32_t unix_timestamp;
  float temperature;
  float humidity;
  float air_pressure;
  float gas_resistance;
};

#endif

