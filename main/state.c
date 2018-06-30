/***** Includes *****/

#include "state.h"
#include "memory.h"

/***** Local Data *****/

static enum peep_state _state = PEEP_STATE_UNKNOWN;
static bool _read_flash = true;

/***** Global Functions *****/

bool
peep_set_state(enum peep_state state)
{
	bool r = true;

	_read_flash = false;
	_state = state;

	r = memory_set_item(
		MEMORY_ITEM_STATE,
		(uint8_t *) &state,
		sizeof(enum peep_state));

	return r;
}

bool
peep_get_state(enum peep_state * state)
{
	bool r = true;

	if (_read_flash) {
		r = memory_set_item(
			MEMORY_ITEM_STATE,
			(uint8_t *) state,
			sizeof(enum peep_state));
		
		if (r) { 
			_read_flash = false;
			_state = *state;
		}
	}
	else {
		*state = _state;
	}

	return r;
}

