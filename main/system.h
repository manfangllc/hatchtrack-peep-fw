#ifndef _SYSTEM_H
#define _SYSTEM_H

/***** Includes *****/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/xtensa_api.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/ringbuf.h"
#include "esp_system.h"
#include "esp_log.h"

/***** Defines *****/

#define RETURN_TEST(cond, msg, ...) \
	if ((!(cond))) { \
		while (1) { \
			printf((msg), ##__VA_ARGS__); \
			vTaskDelay(1000 / portTICK_PERIOD_MS); \
		} \
	}

#endif
