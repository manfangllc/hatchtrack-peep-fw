#ifndef _JSON_PARSE_H
#define _JSON_PARSE_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Global Functions *****/

extern bool
json_parse_wifi_credentials_msg(char * js, char * ssid, uint32_t ssid_max_len,
  char * pass, uint32_t pass_max_len);

#endif
