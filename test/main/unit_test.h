#ifndef _UNIT_TEST_H
#define _UNIT_TEST_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Global Functions *****/

// returns "true" for 'y' and "false" for 'n'
extern bool
unit_test_prompt_yn(const char * message);

#endif
