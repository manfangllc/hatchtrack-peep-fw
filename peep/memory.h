#ifndef _MEMORY_H
#define _MEMORY_H

/***** Includes *****/

#include <stdint.h>

/***** Enums *****/

/* BE CAREFUL MODIFYING THIS, INTERNALS MAKE ASSUMPTIONS ON THE ENUM VALUES */
enum memory_item {
	MEMORY_ITEM_INVALID = 0,
	MEMORY_ITEM_STATE,          // enum peep_state
	MEMORY_ITEM_UUID,           // string
	MEMORY_ITEM_DATA_LOG,       // TODO
	MEMORY_ITEM_WIFI_SSID,      // string
	MEMORY_ITEM_WIFI_PASS,      // string
	MEMORY_ITEM_ROOT_CA,        // string
	MEMORY_ITEM_DEV_CERT,       // string
	MEMORY_ITEM_DEV_PRIV_KEY,   // string
	MEMORY_ITEM_MEASURE_COUNT,  // uint32_t
	MEMORY_ITEM_MEASURE_SEC,    // uint32_t
};

/***** Global Functions *****/

extern bool
memory_init(void);

extern int32_t
memory_get_item(enum memory_item item, uint8_t * dst, uint32_t len);

extern int32_t
memory_set_item(enum memory_item item, uint8_t * src, uint32_t len);

extern bool
memory_delete_item(enum memory_item item);

#endif