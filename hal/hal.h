#ifndef _HAL_H
#define _HAL_H

/***** Includes *****/

#include <stdbool.h>
#include <stdint.h>

/***** Typedefs *****/

typedef void
(*hal_push_button_cb)(bool is_pressed);

/***** Global Functions *****/

extern bool
hal_init(void);

extern bool
hal_init_push_button(hal_push_button_cb cb);

extern void
hal_deep_sleep_timer(uint32_t sec);

extern void
hal_deep_sleep_push_button(void);

extern bool
hal_read_temperature_humdity_pressure_resistance(float * p_temperature,
  float * p_humidity, float * p_pressure, float * p_gas_resistance);

extern bool
hal_read_accel(float * p_gx, float * p_gy, float * p_gz);

#endif
