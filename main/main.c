/***** Includes *****/

#include "nvs_flash.h"
#include "system.h"
#include "ble.h"
#include "sensor.h"
#include "deep_sleep.h"

/***** Defines *****/

#define PEEP_STATE_DEEP_SLEEP_TIME_MIN 15
#define BUFFER_LENGTH 8192

/***** Enums *****/

enum peep_state {
	PEEP_STATE_UNKNOWN = 0,
	PEEP_STATE_BLE_CONFIG = 1,
	PEEP_STATE_DEEP_SLEEP = 2,
};

/***** Local Data *****/

volatile enum peep_state _state = PEEP_STATE_UNKNOWN;
static uint8_t _buffer[BUFFER_LENGTH];

/***** Local Functions *****/

void
_config_ble_write_callback(uint8_t * buf, uint32_t len)
{
	(void) buf;
	(void) len;
}

void
_state_ble_config(void)
{
	bool r = true;

	r = ble_enable(_buffer, BUFFER_LENGTH);
	RETURN_TEST(r, "failed to enable BLE");

	while (PEEP_STATE_BLE_CONFIG == _state) {
		// TODO: better sleep mechanism
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	r = ble_disable();
	RETURN_TEST(r, "failed to disable BLE");
}

void
_state_deep_sleep(void)
{
	esp_err_t r = ESP_OK;
	const uint32_t wakeup_time_sec = PEEP_STATE_DEEP_SLEEP_TIME_MIN * 60;
	const uint64_t wakeup_time_usec = wakeup_time_sec * 1000000;

	printf("Entering %d second deep sleep\n", wakeup_time_sec);

	r = esp_sleep_enable_timer_wakeup(wakeup_time_usec);
	RETURN_TEST((ESP_OK == r), "failed to set wakeup timer");

	// Will not return from the above function.
	esp_deep_sleep_start();
}

/***** Global Functions *****/

void
app_main()
{
	// TODO: Check mode, is hatch active?
	// Testing BLE configuration first...

	_state = PEEP_STATE_BLE_CONFIG;

	while (1) {
		switch(_state) {
		case (PEEP_STATE_BLE_CONFIG):
			_state_ble_config();
			break;
		
		case (PEEP_STATE_DEEP_SLEEP):
		case (PEEP_STATE_UNKNOWN):
		default:
			_state_deep_sleep();
		}
		
	}
} 
