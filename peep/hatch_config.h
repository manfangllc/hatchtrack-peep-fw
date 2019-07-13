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

   /* Defines number of measurements to take before publishing to AWS. */
   /* Taking measurements ever 15 mins, so 4 every hour and we want to */
   /* publish every 12 hours (unless something else occurs to force us */
   /* to publish early.                                                */
#define HATCH_CONFIGURATION_DEFAULT_MEASUREMENT_PUBLISH_THRESHOLD              (4 * 12)

   /* The default measurement interval is seconds (15 minutes).        */
#define HATCH_CONFIGURATION_DEFAULT_MEASURE_INTERVAL_SEC                       (15 * 60)

   /* Default end time for measurement state.   Translates to:         */
   /*                                                                  */
   /*       GMT: Tuesday, January 19, 2038 3:14:07 AM                  */
   /*                                                                  */
#define HATCH_CONFIGURATION_DEFAULT_END_UNIX_TIMESTAMP                         (2147483647)

   /* Indicates if the stored hatch configuration is valid or not.     */
#define HATCH_CONFIGURATION_VALID_MAGIC_WORD                                   0xDEADBEEF

   /* The following define the temperature golden range and how many   */
   /* consecutive measurements above/below the range should take place */
   /* before doing an early publish of the measurements to the cloud.  */
#define HATCH_CONFIGURATION_HIGH_TEMPERATURE_CELSIUS                           ((float)39.4444)
#define HATCH_CONFIGURATION_LOW_TEMPERATURE_CELSIUS                            ((float)36.6666)

#define HATCH_CONFIGURATION_DEFAULT_CONSECUTIVE_HIGH_TEMP_BEFORE_REPORTING     1
#define HATCH_CONFIGURATION_DEFAULT_CONSECUTIVE_LOW_TEMP_BEFORE_REPORTING      8

#define HATCH_CONFIG_INIT(config)                                                                                    \
  do {                                                                                                               \
    (config).uuid[0]                           = 0;                                                                  \
    (config).magic_word                        = HATCH_CONFIGURATION_VALID_MAGIC_WORD;                               \
    (config).consecutive_low_readings          = 0;                                                                  \
    (config).consecutive_low_readings_error    = HATCH_CONFIGURATION_DEFAULT_CONSECUTIVE_LOW_TEMP_BEFORE_REPORTING;  \
    (config).consecutive_high_readings         = 0;                                                                  \
    (config).consecutive_high_readings_error   = HATCH_CONFIGURATION_DEFAULT_CONSECUTIVE_HIGH_TEMP_BEFORE_REPORTING; \
    (config).low_temperature                   = HATCH_CONFIGURATION_LOW_TEMPERATURE_CELSIUS;                        \
    (config).high_temperature                  = HATCH_CONFIGURATION_HIGH_TEMPERATURE_CELSIUS;                       \
    (config).measurements_since_last_published = 0;                                                                  \
    (config).measurements_before_publishing    = HATCH_CONFIGURATION_DEFAULT_MEASUREMENT_PUBLISH_THRESHOLD;          \
    (config).end_unix_timestamp                = HATCH_CONFIGURATION_DEFAULT_END_UNIX_TIMESTAMP;                     \
    (config).measure_interval_sec              = HATCH_CONFIGURATION_DEFAULT_MEASURE_INTERVAL_SEC;                   \
    (config).temperature_offset_celsius        = 0;                                                                  \
  } while (0)

#define IS_HATCH_CONFIG_VALID(config) \
  (0xDEADBEEF == (config).magic_word) ? true : false

/***** Structs *****/

// Talk to Michael about what these values are for

struct hatch_configuration
{
  char uuid[UUID_BUF_LEN];
  uint32_t magic_word;
  uint32_t consecutive_low_readings;
  uint32_t consecutive_low_readings_error;
  float    low_temperature;
  uint32_t consecutive_high_readings;
  uint32_t consecutive_high_readings_error;
  float    high_temperature;
  uint32_t measurements_since_last_published;
  uint32_t measurements_before_publishing;
  uint32_t end_unix_timestamp;
  uint32_t measure_interval_sec;
  uint32_t temperature_offset_celsius;
};

#endif
