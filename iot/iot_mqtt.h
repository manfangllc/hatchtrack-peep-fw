#ifndef _IOT_MQTT_H
#define _IOT_MQTT_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Global Functions *****/

extern bool
iot_mqtt_init(void);

extern bool
iot_mqtt_publish(char * topic, char * message, bool retain);

#endif
