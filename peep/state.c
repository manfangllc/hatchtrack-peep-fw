/***** Includes *****/

#include "system.h"
#include "state.h"
#include "memory.h"

/***** Global Functions *****/

bool peep_set_state(peep_state_t state)
{
  int32_t len = 0;

  len = memory_set_item(MEMORY_ITEM_STATE, (uint8_t *) &state, sizeof(peep_state_t));

  return((sizeof(peep_state_t) == len) ? true : false);
}

bool peep_get_state(peep_state_t * state)
{
  int32_t len = 0;

  len = memory_get_item(MEMORY_ITEM_STATE, (uint8_t *) state, sizeof(peep_state_t));

  return((sizeof(peep_state_t) == len) ? true : false);
}
