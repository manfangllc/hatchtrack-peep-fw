/***** Includes *****/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "unit_test.h"
#include "unity.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/gpio_struct.h"

/***** Defines ******/

#define UART_RX_BUFFER_LEN 2048

/***** Local Data *****/

static uint8_t _buf[UART_RX_BUFFER_LEN];

/***** Local Functions *****/

static bool
_init_uart(void)
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
    r = uart_set_pin(UART_NUM_0,
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
unit_test_prompt_yn(const char * message)
{
  bool r = true;
  uint32_t len = 0;
  const char const crlf[2] = {'\r', '\n'};
  char yn = 0;

  len = strlen(message);
  strcpy((char *) _buf, message);
  strcpy((char *) _buf + len, " [y/n]: ");

  len = strlen((char *) _buf);

  while (0 == yn) {
    uart_write_bytes(UART_NUM_0, (const char *) _buf, len);
    uart_read_bytes(UART_NUM_0, (uint8_t *) &yn, 1, portMAX_DELAY);

    if ((yn >= 65) && (yn <= 90)) {
      uart_write_bytes(UART_NUM_0, (const char *) &yn, 1);
      // Convert upper case to lower case.
      yn = yn + 32;
    }
    else if ((yn >= 97) && (yn <= 122)) {
      uart_write_bytes(UART_NUM_0, (const char *) &yn, 1);
    }
    else {
      // Not a letter.
      yn = 0;
    }

    if (yn != 0) {
      if ('y' == yn) {
        r = true;
      }
      else if ('n' == yn) {
        r = false;
      }
      else {
        yn = 0;
      }
    }

    uart_write_bytes(UART_NUM_0, (const char*) crlf, 2);
  }

  return r;
}

void
app_main(void)
{
  // Install gpio isr service. We do this here because the subsystems register
  // their own ISRs for the pins they use for user input.
  gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

  _init_uart();

  UNITY_BEGIN();
  unity_run_all_tests();
  UNITY_END();
}
