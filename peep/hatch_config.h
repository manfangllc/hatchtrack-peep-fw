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

/***** Macros *****/

#define HATCH_CONFIG_INIT(config) \
  do { \
    (config).uuid[0] = 0; \
    (config).end_unix_timestamp = 0; \
    (config).measure_interval_sec = 0; \
    (config).temperature_offset_celsius = 0; \
  } while (0)

#define IS_HATCH_CONFIG_VALID(config) \
  (0 == (config).uuid[0]) ? false : true

/***** Structs *****/

struct hatch_configuration {
  char uuid[UUID_BUF_LEN];
  uint32_t end_unix_timestamp;
  uint32_t measure_interval_sec;
  uint32_t temperature_offset_celsius;
};

#endif
