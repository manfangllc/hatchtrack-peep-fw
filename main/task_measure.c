/***** Includes *****/

#include "tasks.h"
#include "aws-mqtt.h"
#include "hal.h"
#include "memory.h"
#include "state.h"
#include "system.h"
#include "wifi.h"

/***** Defines *****/

// Comment this out to enter deep sleep when not active.
//#define _NO_DEEP_SLEEP 1
#define _MEASURE_INTERVAL_SEC 60
#define _BUFFER_LEN 2048

/***** Extern Data *****/

extern const uint8_t _root_ca_start[]   asm("_binary_root_ca_txt_start");
extern const uint8_t _root_ca_end[]   asm("_binary_root_ca_txt_end");

extern const uint8_t _cert_start[]   asm("_binary_cert_txt_start");
extern const uint8_t _cert_end[]   asm("_binary_cert_txt_end");

extern const uint8_t _key_start[]   asm("_binary_key_txt_start");
extern const uint8_t _key_end[]   asm("_binary_key_txt_end");

extern const uint8_t _uuid_start[]   asm("_binary_uuid_txt_start");
extern const uint8_t _uuid_end[]   asm("_binary_uuid_txt_end");

/***** Local Data *****/

static uint8_t * _buffer = NULL;
static char * _ssid = NULL;
static char * _pass = NULL;

/***** Global Functions *****/

void
task_measure(void * arg)
{
  char * root_ca = (char *) _root_ca_start;
  char * cert = (char *) _cert_start;
  char * key = (char *) _key_start;
  char * ssid = _ssid;
  char * pass = _pass;
  char * msg = (char *) &_buffer[0];
  float temperature = 0.0;
  float humidity = 0.0;
  float pressure = 0.0;
  float gas_resistance = 0.0;
  bool r = true;
  int len = 0;

  LOGI("start");

  _buffer = malloc(_BUFFER_LEN);
  _ssid = malloc(WIFI_SSID_LEN_MAX);
  _pass = malloc(WIFI_PASSWORD_LEN_MAX);
  if ((NULL == _buffer) || (NULL == _ssid) || (NULL == _pass)) {
    LOGE_TRAP("failed to allocate memory");
  }

  if (r) {
    len = memory_get_item(
      MEMORY_ITEM_WIFI_SSID,
      (uint8_t *) ssid,
      WIFI_SSID_LEN_MAX);
    if (len <= 0) {
      r = false;
    }
    else {
      LOGI("SSID = %s", ssid);
    }
  }

  if (r) {
    len = memory_get_item(
      MEMORY_ITEM_WIFI_PASS,
      (uint8_t *) pass,
      WIFI_PASSWORD_LEN_MAX);
    if (len <= 0) {
      r = false;
    }
    else {
      LOGI("PASS = %s", pass);
    }
  }

  if (r) {
    r = hal_init();
  }

  if (r) {
    r = wifi_connect(ssid, pass);
  }

  if (r) {
    r = aws_mqtt_init(root_ca, cert, key, (char*) _uuid_start);
  }

  if (r) {
    r = hal_read_temperature_humdity_pressure_resistance(
      &temperature,
      &humidity,
      &pressure,
      &gas_resistance);
  }

  if (r) {
    sprintf(
      msg,
      "{\n"
      "\"uuid\": \"%s\",\n"
      "\"temperature\": %f,\n"
      "\"humidity\": %f,\n"
      "\"pressure\": %f,\n"
      "\"gas resistance\": %f\n"
      "}",
      (const char *) _uuid_start,
      temperature,
      humidity,
      pressure,
      gas_resistance);

    r = aws_mqtt_publish("hatchtrack/data/put", msg, false);
  }

  hal_deep_sleep(_MEASURE_INTERVAL_SEC);
}
