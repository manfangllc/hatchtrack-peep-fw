/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "uart_server.h"
#include "system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#define UART_RX_BUFFER_LEN (2048)

/***** Structs *****/

struct uart_server {
	TaskHandle_t task_handle;
	void (* rx_callback)(uint8_t * buf, uint32_t len);
	uint8_t * rx_buf;
	uint32_t rx_len;
	uint32_t rx_len_max;
};

/***** Local Data *****/

static struct uart_server _server = {
	.task_handle = NULL,
	.rx_callback = NULL,
	.rx_buf = NULL,
	.rx_len = 0,
	.rx_len_max = 0,
};

/***** Local Functions *****/

static void
_uart_server_rx(void * arg)
{
	TickType_t ticks_to_wait;
	int len = 0;

	while (1) {
		// sit in this loop while data is being received, break out when no data
		do {
			uint32_t n = _server.rx_len;
			// read data...
			len = uart_read_bytes(
				UART_NUM_0,
				&(_server.rx_buf[n]),
				_server.rx_len_max - n,
				100 / portTICK_RATE_MS);

			// adjust offset...
			if (len > 0) _server.rx_len += len;

			// until no more data is read
		} while (0 != len); 

		if ((_server.rx_callback) && (0 != _server.rx_len)) {
			_server.rx_callback(_server.rx_buf, _server.rx_len);
			_server.rx_len = 0;
		}
	}
}

static bool
_init_hardware(void)
{
	uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};
	esp_err_t r = ESP_OK;

	if (ESP_OK == r) {
		r = uart_param_config(UART_NUM_0, &uart_config);
	}

	if (ESP_OK == r) {
		r = uart_set_pin(
			UART_NUM_0,
			UART_PIN_NO_CHANGE,
			UART_PIN_NO_CHANGE,
			UART_PIN_NO_CHANGE,
			UART_PIN_NO_CHANGE);
	}

	if (ESP_OK == r) {
		r = uart_driver_install(UART_NUM_0, UART_RX_BUFFER_LEN, 0, 0, NULL, 0);
	}

	return (ESP_OK == r) ? true : false;
}

/***** Global Functions *****/

bool
uart_server_enable(uint8_t * buf, uint32_t len)
{
	bool r = true;

	if (NULL != _server.task_handle) {
		r = false;
	}

	if (r) {
		r = _init_hardware();
	}

	if (r) {
		//_server.event_queue = xQueueCreate(2, sizeof(bool));
		//if (NULL == _server.event_queue) {
			//r = false;
		//}
	}

	if (r) {
		_server.rx_buf = buf;
		_server.rx_len_max = len;
		xTaskCreate(
			_uart_server_rx,
			"uart_server_rx",
			8192,
			NULL,
			2,
			&(_server.task_handle));
	}

	return r;
}

bool
uart_server_disable(void)
{
	bool r = false;

	if (_server.task_handle) {
		vTaskDelete(_server.task_handle);
		_server.task_handle = NULL;
		_server.rx_buf = NULL;
		_server.rx_len = 0;
		_server.rx_len_max = 0;
		r = true;
	}

	return r;
}

bool
uart_server_tx(uint8_t * tx_buf, uint32_t tx_buf_len)
{
	int tx_len = tx_buf_len;
	int len = 0;

	len = uart_write_bytes(UART_NUM_0, (const char *) tx_buf, tx_len);

	return (len == tx_len) ? true : false;
}

void
uart_server_register_rx_callback(void(*callback)(uint8_t * buf, uint32_t len))
{
	_server.rx_callback = callback;
}
