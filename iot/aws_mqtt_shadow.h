#ifndef _AWS_MQTT_SHADOW_H
#define _AWS_MQTT_SHADOW_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Typedefs *****/

typedef void
(*aws_mqtt_shadow_cb)(uint8_t * buf, uint16_t len);

/***** Global Functions *****/

extern bool
aws_mqtt_shadow_init(char * root_ca, char * client_cert, char * client_key, 
  char * client_id, int32_t timeout_sec);

extern bool
aws_mqtt_shadow_disconnect(void);

extern bool
aws_mqtt_shadow_get(aws_mqtt_shadow_cb cb, uint8_t timeout_sec);

extern bool
aws_mqtt_shadow_poll(uint32_t poll_ms);

#endif
