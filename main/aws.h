#ifndef _AWS_H
#define _AWS_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Global Functions *****/

extern bool
aws_connect(uint8_t * buf, uint32_t len);

extern bool
aws_disconnect(void);

extern bool
aws_publish_json(uint8_t * json_str, uint32_t json_len);

#endif
