#ifndef _LED_H
#define _LED_H

/***** Includes *****/

#include <stdbool.h>

/***** Global Functions *****/

extern bool
led_init(void);

extern void
led_red(bool is_on);

extern void
led_green(bool is_on);

extern void
led_blue(bool is_on);

#endif
