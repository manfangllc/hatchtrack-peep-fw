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

#ifdef PEEP_UNIT_TEST_BUILD
#include "unit_test.h"
#include "unity.h"
#endif

/***** Macros *****/

#define LOGE(format, ...) \
  ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR, __func__, format, ##__VA_ARGS__)
#define LOGW(format, ...) \
  ESP_LOG_LEVEL_LOCAL(ESP_LOG_WARNING, __func__, format, ##__VA_ARGS__)
#define LOGI(format, ...) \
  ESP_LOG_LEVEL_LOCAL(ESP_LOG_INFO, __func__, format, ##__VA_ARGS__)
#define LOGD(format, ...) \
  ESP_LOG_LEVEL_LOCAL(ESP_LOG_DEBUG, __func__, format, ##__VA_ARGS__)

#define TRACE() \
  LOGD("%d", __LINE__);

#define RESULT_TEST(cond, format, ...) \
  if (!(cond)) { \
    while (1) { \
      printf(format, ##__VA_ARGS__); \
      vTaskDelay(1000 / portTICK_PERIOD_MS); \
    } \
  }

#define RESULT_TEST_ERROR_TRACE(cond) \
  if ((!(cond))) { \
    while (1) { \
      printf("%s:%s:%d\n", __FILE__, __func__, __LINE__); \
      vTaskDelay(1000 / portTICK_PERIOD_MS); \
    } \
  }

#define LOGE_TRAP(format, ...) \
  while (1) { \
    ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR, __func__, format, ##__VA_ARGS__); \
    vTaskDelay(1000 / portTICK_PERIOD_MS); \
  }

#endif
