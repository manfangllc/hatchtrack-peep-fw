#ifndef _UART_SERVER_H
#define _UART_SERVER_H

/***** Includes *****/

#include "system.h"

/***** Global Functions *****/

extern void
uart_server_register_rx_callback(void(*callback)(uint8_t * buf, uint32_t len));

extern bool
uart_server_enable(uint8_t * buf, uint32_t len);

extern bool
uart_server_disable(void);

extern bool
uart_server_tx(uint8_t * buf, uint32_t len);

#endif

