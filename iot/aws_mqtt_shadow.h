#ifndef _AWS_MQTT_SHADOW_H
#define _AWS_MQTT_SHADOW_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Global Functions *****/

extern bool
aws_mqtt_shadow_init(char * root_ca, char * client_cert, char * client_key, 
  char * client_id, int32_t timeout_sec);

extern bool
aws_mqtt_shadow_disconnect(void);

extern bool
aws_mqtt_shadow_get(uint8_t buf, uint32_t buf_len);

#endif
