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
  bool r = true;
  int32_t len = 0;

  nvs_flash_init();
  r = memory_init();
  RESULT_TEST_ERROR_TRACE(r);

  r = memory_measurement_db_init();
  RESULT_TEST_ERROR_TRACE(r);

#if defined (PEEP_TEST_STATE_DEEP_SLEEP)
  hal_deep_sleep_timer(60);
  // will not return from above
#elif defined(PEEP_TEST_STATE_MEASURE) || defined(PEEP_TEST_STATE_MEASURE_CONFIG)
  (void) len;
  state = PEEP_STATE_MEASURE;
  // NOTE: I don't think PEEP_STATE_MEASURE_CONFIG will really be needed. Will
  // likely remove this in the very near future.
  // state = PEEP_STATE_MEASURE_CONFIG;
#elif defined (PEEP_TEST_STATE_BLE_CONFIG)
  (void) len;
  state = PEEP_STATE_BLE_CONFIG;
#else
  len = memory_get_item(
    MEMORY_ITEM_STATE,
    (uint8_t * ) &state,
    sizeof(enum peep_state));

  if (sizeof(enum peep_state) != len) {
    state = PEEP_STATE_BLE_CONFIG;
    memory_set_item(
      MEMORY_ITEM_STATE,
      (uint8_t *) &state,
      sizeof(enum peep_state));
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
