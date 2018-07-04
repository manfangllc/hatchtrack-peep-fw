/***** Includes *****/

#include "system.h"
#include "state.h"
#include "memory.h"

/***** Global Functions *****/

bool
peep_set_state(enum peep_state state)
{
	int32_t len = 0;

	len = memory_set_item(
		MEMORY_ITEM_STATE,
		(uint8_t *) &state,
		sizeof(enum peep_state));

	return (sizeof(enum peep_state) == len) ? true : false;
}

bool
peep_get_state(enum peep_state * state)
{
	int32_t len = 0;

	len = memory_get_item(
		MEMORY_ITEM_STATE,
		(uint8_t *) state,
		sizeof(enum peep_state));

	return (sizeof(enum peep_state) == len) ? true : false;
}
