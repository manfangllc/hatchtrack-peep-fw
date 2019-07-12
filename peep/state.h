#ifndef _STATE_H
#define _STATE_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Enums *****/

enum peep_state {
  PEEP_STATE_MIN = 0,

  PEEP_STATE_UNKNOWN = 0,
  PEEP_STATE_BLE_CONFIG,
  PEEP_STATE_DEEP_SLEEP,
  PEEP_STATE_MEASURE,
  PEEP_STATE_MEASURE_CONFIG,

  PEEP_STATE_MAX = 0xFFFF,
};

/***** Global Functions *****/

bool
peep_set_state(enum peep_state state);

bool
peep_get_state(enum peep_state * state);

#endif
