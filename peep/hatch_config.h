#ifndef _HATCH_CONFIG_H
#define _HATCH_CONFIG_H

/***** Includes *****/

#include <stdint.h>

/***** Defines *****/

// string length of UUID of the form "xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx"
#define UUID_STR_LEN (36)
// memory length of UUID of the form "xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx"
// note: need one byte for '\0', other three are for word alignment
#define UUID_BUF_LEN (UUID_STR_LEN + 4)

/***** Structs *****/

struct hatch_configuration {
  char uuid[UUID_BUF_LEN];
  uint32_t end_unix_timestamp;
  uint32_t measure_interval_sec;
};

#endif
