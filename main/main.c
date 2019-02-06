/***** Includes *****/

#include <time.h>

#include "nvs_flash.h"
#include "system.h"
#include "aws.h"
#include "wifi.h"
#include "led.h"
#include "message.h"
#include "ble_server.h"
#include "uart_server.h"
#include "memory.h"
#include "state.h"
#include "hal.h"
#include "iot_mqtt.h"

/***** Defines *****/

#define _MEASURE_INTERVAL_SEC 15
#define _WIFI_SSID ("icxc-nika")
#define _WIFI_PASSWORD ("November141990")

#define _BUFFER_LEN 8192

/***** Local Data *****/

static uint8_t _buffer[_BUFFER_LEN];

/***** Local Functions *****/

static void
_deep_sleep(uint32_t sec)
{
  esp_err_t r = ESP_OK;
  uint64_t wakeup_time_usec = sec * 1000000;

  printf("Entering %d second deep sleep\n", sec);

  r = esp_sleep_enable_timer_wakeup(wakeup_time_usec);
  RESULT_TEST((ESP_OK == r), "failed to set wakeup timer");

  // Will not return from the above function.
  esp_deep_sleep_start();
}

static void
_measure_task(void * arg)
{
  char * ssid = _WIFI_SSID;
  char * password = _WIFI_PASSWORD;
  char * msg = (char *) &_buffer[0];
  float temperature = 0.0;
  float humidity = 0.0;
  bool r = true;

  LOGI("start");

  if (r) {
    r = hal_init();
  }

  if (r) {
    r = hal_read_temperature_humdity(&temperature, &humidity);
  }

  if (r) {
    r = wifi_connect(ssid, password);
  }

  if (r) {
    r = iot_mqtt_init();
  }

  if (r) {
    sprintf(
      msg,
      "{\n"
      "\"hatch\": \"%s\",\n"
      "\"time\": %d,\n"
      "\"temperature\": %f,\n"
      "\"humidity\": %f\n"
      "}",
      "hello",
      (int) time(NULL),
      temperature,
      humidity);

      r = iot_mqtt_publish("/test", msg, true);
  }

  _deep_sleep(_MEASURE_INTERVAL_SEC);
}

/***** Global Functions *****/

void
app_main()
{
  nvs_flash_init();
  led_init();

  xTaskCreate(_measure_task, "measurement task", 8192, NULL, 2, NULL);
}
