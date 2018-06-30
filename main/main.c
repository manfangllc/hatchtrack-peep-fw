/***** Includes *****/

#include "nvs_flash.h"
#include "system.h"
#include "wifi.h"
#include "message.h"
#include "ble_server.h"
#include "uart_server.h"
#include "memory.h"
#include "state.h"
//#include "sensor.h"

/***** Defines *****/

#define PEEP_STATE_DEEP_SLEEP_TIME_MIN 15
#define BUFFER_LENGTH 8192

/***** Local Data *****/

static uint8_t _buffer[BUFFER_LENGTH];
static bool(*_config_tx)(uint8_t * buf, uint32_t len) = NULL;

/***** Local Functions *****/

static void
deep_sleep(uint32_t sec)
{
	esp_err_t r = ESP_OK;
	uint64_t wakeup_time_usec = sec * 1000000;

	printf("Entering %d second deep sleep\n", sec);

	r = esp_sleep_enable_timer_wakeup(wakeup_time_usec);
	RETURN_TEST((ESP_OK == r), "failed to set wakeup timer");

	// Will not return from the above function.
	esp_deep_sleep_start();
}

static void
_config_rx(uint8_t * buf, uint32_t len)
{
	const uint32_t tx_len_max = BUFFER_LENGTH / 2;
	uint32_t tx_len = 0;
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
		//ESP_LOG_BUFFER_HEXDUMP(__func__, tx, tx_len, ESP_LOG_ERROR);
	}
}

static void
_state_ble_config(void)
{
	uint32_t half = BUFFER_LENGTH / 2;
	bool r = true;

	r = ble_enable(_buffer, half);
	RETURN_TEST(r, "failed to enable BLE\n");

	if (r) {
		r = message_init(_buffer + half, half);
	}

	while (1) {
		// TODO: better sleep mechanism
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	r = ble_disable();
	RETURN_TEST(r, "failed to disable BLE\n");
}

static void
_state_uart_config(void)
{
	uint32_t half = BUFFER_LENGTH / 2;
	bool r = true;

	if (r) {
		r = message_init(_buffer + half, half);
	}

	if (r) {
		uart_server_register_rx_callback(_config_rx);
		_config_tx = uart_server_tx;
	}

	if (r) {
		r = uart_server_enable(_buffer, half);
		RETURN_TEST(r, "failed to enable UART\n");
	}

	while (1) {
		// TODO: better sleep mechanism
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	r = uart_server_disable();
	RETURN_TEST(r, "failed to disable UART\n");
}

static void
_state_measure(void)
{
	const uint32_t next_measure_delay_sec = 15 * 60;
	enum peep_state next = PEEP_STATE_UNKNOWN;
	bool r = true;

	vTaskDelay(10000 / portTICK_PERIOD_MS);

	if (r) {
		r = wifi_connect();
	}

	if (r) {
		next = PEEP_STATE_MEASURE;
		memory_set_item(MEMORY_ITEM_STATE, (uint8_t*) &next, sizeof(next));
		deep_sleep(next_measure_delay_sec);
	}
	else {
		next = PEEP_STATE_UART_CONFIG;
		memory_set_item(MEMORY_ITEM_STATE, (uint8_t*) &next, sizeof(next));
		deep_sleep(0);
	}
}

/***** Global Functions *****/

void
app_main()
{
	volatile enum peep_state state = PEEP_STATE_UNKNOWN;
	bool r = true;

	nvs_flash_init();

	r = memory_init();
	RETURN_TEST(r, "failed to initialize memory\n");

	//r = memory_get_item(MEMORY_ITEM_STATE, (uint8_t*) &state, sizeof(state));
	//if (false == r) {
		//printf("failed to read state\n");
		//state = PEEP_STATE_UNKNOWN;
	//}
	state = PEEP_STATE_UART_CONFIG;

	switch(state) {
	case (PEEP_STATE_BLE_CONFIG):
		_state_ble_config();
		break;

	case (PEEP_STATE_UART_CONFIG):
		_state_uart_config();
		break;

	case (PEEP_STATE_MEASURE):
		_state_measure();
		break;

	case (PEEP_STATE_UNKNOWN):
	default:
		state = PEEP_STATE_UART_CONFIG;
		memory_set_item(MEMORY_ITEM_STATE, (uint8_t*) &state, sizeof(state));
		deep_sleep(0);
	}

	// should not return from above code...
}
