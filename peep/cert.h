#ifndef _CERT_H_
#define _CERT_H_

/***** Includes *****/

#include <stdint.h>

/***** Enums *****/

/* BE CAREFUL MODIFYING THIS, INTERNALS MAKE ASSUMPTIONS ON THE ENUM VALUES */
typedef enum
{
   CERT_ITEM_INVALID = 0,
   CERT_ITEM_ROOT_CA,
   CERT_ITEM_CERTIFICATE,
   CERT_ITEM_UUID,
   CERT_ITEM_KEY,
   CERT_ITEM_LAST_ITEM
} cert_item_t;

/***** Global Functions *****/

extern bool cert_init(void);
extern void cert_deinit(void);
extern bool cert_get_item_size(cert_item_t item, size_t *item_length);
extern bool cert_get_item(cert_item_t item, size_t *item_buffer_length, char *item_buffer);

#endif
