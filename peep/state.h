#ifndef _STATE_H
#define _STATE_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Enums *****/

typedef enum  {
  PEEP_STATE_MIN = 0,

  PEEP_STATE_UNKNOWN = 0,
  PEEP_STATE_BLE_CONFIG,
  PEEP_STATE_MEASURE,
  PEEP_STATE_MEASURE_CONFIG,

  PEEP_STATE_MAX = 0xFFFF,
} peep_state_t;

/***** Global Functions *****/

bool peep_set_state(peep_state_t state);
bool peep_get_state(peep_state_t *state);

#endif
