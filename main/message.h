#ifndef _CLIENT_MESSAGE_H
#define _CLIENT_MESSAGE_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Global Functions *****/

extern bool
message_init(uint8_t * buffer, uint32_t length);

extern void
message_free(void);

extern bool
message_client_write(uint8_t * message, uint32_t length);

extern bool
message_device_response(uint8_t * message, uint32_t * length,
uint32_t max_length);

#endif
