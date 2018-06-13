#ifndef _BLE_H
#define _BLE_H

/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

/***** Global Functions *****/

extern void
ble_register_write_callback(void(*callback)(uint8_t * buf, uint32_t len));

extern bool
ble_enable(uint8_t * buf, uint32_t len);

extern bool
ble_disable(void);

#endif
