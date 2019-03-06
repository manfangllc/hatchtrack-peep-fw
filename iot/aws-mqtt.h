#ifndef _AWS_H
#define _AWS_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Typedefs *****/

typedef void
(*aws_subscribe_cb)(uint8_t * buf, uint16_t len);

/***** Global Functions *****/

extern bool
aws_mqtt_init(char * root_ca, char * client_cert, char * client_key,
  char * client_id);

extern bool
aws_mqtt_disconnect(void);

// NOTE: AWS DOES NOT SUPPORT "RETAIN" OF MQTT CURRENTLY
extern bool
aws_mqtt_publish(char * topic, char * message, bool retain);

extern bool
aws_mqtt_subsribe(char * topic, aws_subscribe_cb cb);

extern bool
aws_mqtt_subsribe_poll(uint32_t poll_ms);

#endif
