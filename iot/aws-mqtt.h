#ifndef _AWS_H
#define _AWS_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Global Functions *****/

extern bool
aws_mqtt_init(char * root_ca, char * client_cert, char * client_key,
  char * client_id);

extern bool
aws_mqtt_disconnect(void);

// NOTE: AWS DOES NOT SUPPORT "RETAIN" OF MQTT CURRENTLY
extern bool
aws_mqtt_publish(char * topic, char * message, bool retain);

#endif
