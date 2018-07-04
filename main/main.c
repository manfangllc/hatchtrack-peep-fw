/***** Includes *****/

#include "nvs_flash.h"
#include "system.h"
#include "aws.h"
#include "wifi.h"
#include "message.h"
#include "ble_server.h"
#include "uart_server.h"
#include "memory.h"
#include "state.h"
#include "sensor.h"

/***** Defines *****/

#define PEEP_STATE__deep_sleep_TIME_MIN 15
#define BUFFER_LENGTH 8192

/***** Local Data *****/

static uint8_t _buffer[BUFFER_LENGTH];
static bool(*_config_tx)(uint8_t * buf, uint32_t len) = NULL;
static volatile bool _is_config = false;

/***** Local Functions *****/

static void
_deep_sleep(uint32_t sec)
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
	}

	if (message_is_client_done()) {
		// TODO: Need to delay until last Tx message is sent???
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		_is_config = false;
	}
}

static void
_state_ble_config(void)
{
	uint32_t half = BUFFER_LENGTH / 2;
	bool r = true;

	_is_config = true;
	r = ble_enable(_buffer, half);
	RETURN_TEST(r, "failed to enable BLE\n");

	if (r) {
		r = message_init(_buffer + half, half);
	}

	while (_is_config) {
		// TODO: better sleep mechanism
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	r = ble_disable();
	if (!r) {
		printf("failed to disable BLE\n");
	}
	_deep_sleep(0);
}

static void
_state_uart_config(void)
{
	uint32_t half = BUFFER_LENGTH / 2;
	bool r = true;

	_is_config = true;

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

	while (_is_config) {
		// TODO: better sleep mechanism
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	r = uart_server_disable();
	if (!r) {
		printf("failed to disable uart!\n");
	}
	_deep_sleep(0);
}

static void
_state_measure(void)
{
	const uint32_t next_measure_delay_sec = 15 * 60;
	char * id = (char *) &_buffer[BUFFER_LENGTH / 2];
	char * msg = (char *) &_buffer[0];
	float temperature = 0.0;
	float humidity = 0.0;
	int len = 0;
	bool r = true;

	vTaskDelay(15000 / portTICK_PERIOD_MS);

	if (r) {
		r = wifi_connect();
	}

	if (r) {
		r = aws_connect(_buffer, 8192);
	}

	if (r) {
		r = sensor_init();
	}

	if (r) {
		r = sensor_measure(&temperature, &humidity);
	}

	if (r) {
		len = memory_get_item(MEMORY_ITEM_UUID, (uint8_t *) id, BUFFER_LENGTH / 2);
		if (0 >= len) {
			r = false;
		}
		else {
			id[len] = '\0';
		}
	}

	if (r) {
		sprintf(
			msg,
			"{\n"
			"\"hatch\": \"%s\",\n"
			"\"temperature\": %f,\n"
			"\"humidity\": %f\n"
			"}",
			id,
			temperature,
			humidity);

		if (true != aws_publish_json((uint8_t *) msg, strlen(msg))) {
			printf("Failed to publish message!\n");
		}
	}

	if (r) {
		_deep_sleep(next_measure_delay_sec);
	}
	else {
		peep_set_state(PEEP_STATE_UART_CONFIG);
		_deep_sleep(0);
	}
}

/***** Global Functions *****/

void
app_main()
{
	enum peep_state state = PEEP_STATE_UNKNOWN;
	bool r = true;

	nvs_flash_init();
	r = memory_init();
	RETURN_TEST(r, "failed to initialize memory\n");

	r = peep_get_state(&state);
	if ((false == r) || (PEEP_STATE_UNKNOWN == state)) {
		state = PEEP_STATE_UART_CONFIG;
		peep_set_state(state);
	}

	if (PEEP_STATE_BLE_CONFIG == state) {
		_state_ble_config();
	}
	else if (PEEP_STATE_UART_CONFIG == state) {
		_state_uart_config();
	}
	else if (PEEP_STATE_MEASURE == state) {
		_state_measure();
	}

	// should not return from above code...
}
