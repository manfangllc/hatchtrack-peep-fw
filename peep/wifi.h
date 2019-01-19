#ifndef _WIFI_H
#define _WIFI_H

/***** Includes *****/

#include <stdbool.h>
#include <stdint.h>

/***** Defines *****/

#define SSID_LEN_MAX (32)
#define PASSWORD_LEN_MAX (64)

/***** Global Functions *****/

// extern bool
// wifi_scan_ap(void);

extern bool
wifi_stop(void);

extern bool
wifi_connect(void);

extern bool
wifi_disconnect(void);

extern bool
wifi_publish_results(float temperature, float humidity, float pressure,
	float quality);

#endif
