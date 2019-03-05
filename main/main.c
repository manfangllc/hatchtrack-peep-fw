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

// Comment this out to enter deep sleep when not active.
#define _NO_DEEP_SLEEP 1
#define _MEASURE_INTERVAL_SEC 15
#define _BUFFER_LEN 8192

/***** Local Data *****/

static uint8_t _buffer[_BUFFER_LEN];

extern const uint8_t _root_ca_start[]   asm("_binary_root_ca_txt_start");
extern const uint8_t _root_ca_end[]   asm("_binary_root_ca_txt_end");

extern const uint8_t _cert_start[]   asm("_binary_cert_txt_start");
extern const uint8_t _cert_end[]   asm("_binary_cert_txt_end");

extern const uint8_t _key_start[]   asm("_binary_key_txt_start");
extern const uint8_t _key_end[]   asm("_binary_key_txt_end");

extern const uint8_t _uuid_start[]   asm("_binary_uuid_txt_start");
extern const uint8_t _uuid_end[]   asm("_binary_uuid_txt_end");

extern const uint8_t _ssid_start[]   asm("_binary_wifi_ssid_txt_start");
extern const uint8_t _ssid_end[]   asm("_binary_wifi_ssid_txt_end");

extern const uint8_t _pass_start[]   asm("_binary_wifi_pass_txt_start");
extern const uint8_t _pass_end[]   asm("_binary_wifi_pass_txt_end");

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
  char * root_ca = (char *) _root_ca_start;
  char * cert = (char *) _cert_start;
  char * key = (char *) _key_start;
  char * ssid = (char *) _ssid_start;
  char * password = (char *) _pass_start;
  char * msg = (char *) &_buffer[0];
  float temperature = 0.0;
  float humidity = 0.0;
  bool r = true;

  LOGI("start");

  if (r) {
    r = hal_init();
  }

  if (r) {
    r = wifi_connect(ssid, password);
  }

  if (r) {
    r = iot_mqtt_init(root_ca, cert, key);
  }

#ifdef _NO_DEEP_SLEEP
  uint32_t n = 0;

  while (r) {
    if (r) {
      r = hal_read_temperature_humdity(&temperature, &humidity);
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
        (const char *) _uuid_start,
        (int) time(NULL),
        temperature,
        humidity);

        r = iot_mqtt_publish("hatchtrack/data/put", msg, false);
    }

    if (r) {
      n++;
      LOGI("iteration %d\n", n);
      vTaskDelay((_MEASURE_INTERVAL_SEC * 1000) / portTICK_PERIOD_MS);
    }
  }

#else
  if (r) {
    r = hal_read_temperature_humdity(&temperature, &humidity);
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
      (const char *) _uuid_start,
      (int) time(NULL),
      temperature,
      humidity);

      r = iot_mqtt_publish("/test", msg, true);
  }
#endif

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
