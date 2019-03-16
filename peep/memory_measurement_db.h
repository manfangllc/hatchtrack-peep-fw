#ifndef _HATCH_MEASUREMENT_DB_H
#define _HATCH_MEASUREMENT_DB_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

#include "hatch_measurement.h"

/***** Global Functions *****/

extern bool
memory_measurement_db_init(void);

extern bool
memory_measurement_db_add(struct hatch_measurement * meas);

extern uint32_t
memory_measurement_db_total(void);

extern bool
memory_measurement_db_delete_all(void);

extern bool
memory_measurement_db_read_open(void);

extern bool
memory_measurement_db_read_entry(struct hatch_measurement * p_meas);

extern bool
memory_measurement_db_read_close(void);

#endif


