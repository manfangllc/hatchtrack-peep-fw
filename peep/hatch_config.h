#ifndef _HATCH_CONFIG_H
#define _HATCH_CONFIG_H

/***** Includes *****/

#include <stdint.h>

/***** Structs *****/

struct hatch_configuration {
  uint32_t end_unix_timestamp;
  uint32_t measure_interval_sec;
}

#endif
