/***** Includes *****/

#include "nvs_flash.h"
#include "system.h"
#include "message.h"
#include "ble_server.h"
#include "uart_server.h"
//#include "sensor.h"

/***** Defines *****/

#define PEEP_STATE_DEEP_SLEEP_TIME_MIN 15
#define BUFFER_LENGTH 8192

/***** Enums *****/

enum peep_state {
	PEEP_STATE_UNKNOWN = 0,
	PEEP_STATE_BLE_CONFIG,
	PEEP_STATE_UART_CONFIG,
	PEEP_STATE_DEEP_SLEEP,
	PEEP_STATE_MEASURE,
};

/***** Local Data *****/

volatile enum peep_state _state = PEEP_STATE_UNKNOWN;
static uint8_t _buffer[BUFFER_LENGTH];

static bool(*_config_tx)(uint8_t * buf, uint32_t len) = NULL;

/***** Local Functions *****/

void
_config_rx(uint8_t * buf, uint32_t len)
{
	const uint32_t tx_len_max = BUFFER_LENGTH / 2;
	uint32_t tx_len;
	uint8_t * tx = _buffer;

	bool r = true;

	if (r) {
		r = message_client_write(buf, len);
	}

	if (r) {
		r = message_device_response(tx, &tx_len, tx_len_max);
	}

	if ((r) && (_config_tx)) {
		r = _config_tx(tx, tx_len);
	}
}

void
_state_ble_config(void)
{
	uint32_t half = BUFFER_LENGTH / 2;
	bool r = true;

	r = ble_enable(_buffer, half);
	RETURN_TEST(r, "failed to enable BLE");

	if (r) {
		r = message_init(_buffer + half, half);
	}

	while ((r) && (PEEP_STATE_BLE_CONFIG == _state)) {
		// TODO: better sleep mechanism
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	r = ble_disable();
	RETURN_TEST(r, "failed to disable BLE");
}

void
_state_uart_config(void)
{
	bool r = true;

	if (r) {
		r = uart_server_enable(_buffer, BUFFER_LENGTH / 2);
		RETURN_TEST(r, "failed to enable UART");
	}

	if (r) {
		uart_server_register_rx_callback(_config_rx);
		_config_tx = uart_server_tx;
	}

	while ((r) && (PEEP_STATE_BLE_CONFIG == _state)) {
		// TODO: better sleep mechanism
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	r = uart_server_disable();
	RETURN_TEST(r, "failed to disable UART");
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

		case (PEEP_STATE_UART_CONFIG):
			_state_uart_config();
			break;

		case (PEEP_STATE_MEASURE):
			

		case (PEEP_STATE_DEEP_SLEEP):
		case (PEEP_STATE_UNKNOWN):
		default:
			_state_deep_sleep();
		}
		
	}
} 
