#ifndef _WIFI_H
#define _WIFI_H

/***** Includes *****/

#include <stdbool.h>
#include <stdint.h>

/***** Defines *****/

#define WIFI_SSID_LEN_MAX (32)
#define WIFI_PASSWORD_LEN_MAX (64)

/***** Global Functions *****/

extern bool
wifi_connect(char * ssid, char * password, int32_t timeout_sec);

extern bool
wifi_disconnect(void);

#endif
