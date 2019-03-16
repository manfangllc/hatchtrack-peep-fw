/***** Includes *****/

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "nvs_flash.h"

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

#ifdef PEEP_TEST_STATE_MEASURE
  (void) len;
  state = PEEP_STATE_MEASURE;
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
    xTaskCreate(
      task_ble_config_wifi_credentials,
      "ble config task",
      8192,
      NULL,
      2,
      NULL);
  }
  else if (PEEP_STATE_MEASURE == state) {
    xTaskCreate(
      task_measure,
      "measurement task",
      40960,
      NULL,
      2,
      NULL);
  }
  else if (PEEP_STATE_MEASURE_CONFIG == state) {
    xTaskCreate(
      task_measure_config,
      "measurement config task",
      8192,
      NULL,
      2,
      NULL);
  }
}
