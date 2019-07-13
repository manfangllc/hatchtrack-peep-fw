/***** Includes *****/

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "nvs_flash.h"

#include "hal.h"
#include "memory.h"
#include "memory_measurement_db.h"
#include "state.h"
#include "system.h"
#include "tasks.h"

/***** Global Functions *****/

void
app_main()
{
  enum peep_state state = PEEP_STATE_UNKNOWN;
  bool is_state_invalid = false;
  bool r = true;
  int32_t len = 0;

#if defined (PEEP_TEST_STATE_DEEP_SLEEP) || \
    defined(PEEP_TEST_STATE_MEASURE) || \
    defined(PEEP_TEST_STATE_MEASURE_CONFIG) || \
    defined (PEEP_TEST_STATE_BLE_CONFIG)
  {
    uint32_t n = 10;
    do {
      LOGI("Starting in %d...", n--);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    } while (n);
  }
#endif

  nvs_flash_init();
  r = memory_init();
  RESULT_TEST_ERROR_TRACE(r);

  r = memory_measurement_db_init();
  RESULT_TEST_ERROR_TRACE(r);

#if defined (PEEP_TEST_STATE_DEEP_SLEEP)

  if (hal_deep_sleep_is_wakeup_push_button()) {
    LOGI("wakeup cause push button");
  }
  else if (hal_deep_sleep_is_wakeup_timer()) {
    LOGI("wakeup cause timer");
  }
  else {
    LOGI("unknown wakeup cause");
  }
  hal_deep_sleep_timer_and_push_button(60);
  // will not return from above

#elif defined(PEEP_TEST_STATE_MEASURE)

  (void) len;
  state = PEEP_STATE_MEASURE;

#elif defined(PEEP_TEST_STATE_MEASURE_CONFIG)

  (void) len;
  state = PEEP_STATE_MEASURE_CONFIG;

#elif defined (PEEP_TEST_STATE_BLE_CONFIG)

  (void) len;
  state = PEEP_STATE_BLE_CONFIG;

#else

  if (!peep_get_state(&state)) {
    LOGI("no valid state");
    is_state_invalid = true;
  }

  if (is_state_invalid) {
    LOGI("set state BLE configuration");
    state = PEEP_STATE_BLE_CONFIG;

    peep_set_state(state);
  }

#endif

  if (PEEP_STATE_BLE_CONFIG == state) {
    LOGD("PEEP_STATE_BLE_CONFIG");
    xTaskCreate(
      task_ble_config_wifi_credentials,
      "ble config task",
      8192,
      NULL,
      2,
      NULL);
  }
  else if (PEEP_STATE_MEASURE == state) {
    LOGD("PEEP_STATE_MEASURE");
    xTaskCreate(
      task_measure,
      "measurement task",
      10240,
      NULL,
      2,
      NULL);
  }
  else if (PEEP_STATE_MEASURE_CONFIG == state) {
    LOGD("PEEP_STATE_MEASURE_CONFIG");
    xTaskCreate(
      task_measure_config,
      "measurement config task",
      8192,
      NULL,
      2,
      NULL);
  }
}
