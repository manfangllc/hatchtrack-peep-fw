#ifndef _JSON_PARSE_H
#define _JSON_PARSE_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

#include "hatch_config.h"

/***** Global Functions *****/

/**
 * Parse a JSON message that looks like the following...
 * {
 *    "wifiSSID" : "my-wifi-ssid",
 *    "wifiPassword" : "my-wifi-password"
 * }
 */
extern bool
json_parse_wifi_credentials_msg(char * js, char * ssid, uint32_t ssid_max_len,
  char * pass, uint32_t pass_max_len);

/**
 * Parse a JSON message that looks like the following...
 * {
 *    "endUnixTimestamp", 1551935397,
 *    "measureIntervalSec", 900
 * }
 */

extern bool
json_parse_wifi_credentials_msg(char * js, char * ssid, uint32_t ssid_max_len,
  char * pass, uint32_t pass_max_len);

extern bool
json_parse_hatch_config_msg(char * js, struct hatch_configuration * config);

#endif
