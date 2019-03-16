/***** Includes *****/

#include <time.h>

#include "tasks.h"
#include "aws-mqtt.h"
#include "hal.h"
#include "hatch_measurement.h"
#include "memory.h"
#include "memory_measurement_db.h"
#include "state.h"
#include "system.h"
#include "wifi.h"

/***** Defines *****/

// Comment this out to enter deep sleep when not active.
//#define _NO_DEEP_SLEEP 1
#define _MEASURE_INTERVAL_SEC 60
#define _BUFFER_LEN 2048

#ifdef PEEP_TEST_STATE_MEASURE
  #define TEST_WIFI_SSID "test-point"
  #define TEST_WIFI_PASSWORD "1337-test!"
#endif

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

/***** Local Functions *****/

static bool
_format_json(uint8_t * buf, uint32_t buf_len,
  struct hatch_measurement * meas, char * uuid)
{
  int32_t bytes = 0;
  bool r = true;

  bytes = snprintf(
    (char *) buf,
    buf_len,
    "{\n"
    "\"unixTime\": %d,\n"
    "\"peepUUID\": \"%s\",\n"
    "\"temperature\": %f,\n"
    "\"humidity\": %f,\n"
    "\"pressure\": %f,\n"
    "\"gasResistance\": %f\n"
    "}",
    meas->unix_timestamp,
    (const char *) uuid,
    meas->temperature,
    meas->humidity,
    meas->air_pressure,
    meas->gas_resistance);

  if ((bytes < 0) || (bytes >= buf_len)) {
    r = false;
  }

  return r;
}

static bool
_publish_measurements(uint8_t * buf, uint32_t buf_len,
  struct hatch_measurement * meas, char * uuid)
{
  struct hatch_measurement old;
  uint32_t total = 0;
  bool r = true;

  if (r) {
    _format_json(buf, buf_len, meas, uuid);
  }

  if (r) {
    r = aws_mqtt_publish("hatchtrack/data/put", (char *) buf, false);
  }

  if (r) {
    total = memory_measurement_db_total();
    LOGI("%d old measurements to upload\n", total);
    printf("sizeof=%d\n", sizeof(struct hatch_measurement));
  }

  if (r && total) {

    r = memory_measurement_db_read_open();

    while (r && total--) {
      r = memory_measurement_db_read_entry(&old);

      if (r) {
        printf(
          "%d %f %f %f\n",
          old.unix_timestamp,
          old.temperature,
          old.humidity,
          old.gas_resistance);
      }

    }

    memory_measurement_db_read_close();

    if (r) {
      LOGI("deleting old data\n");
      memory_measurement_db_delete_all();
    }
  }


  return r;
}

static bool
_get_wifi_ssid_pasword(char * ssid, char * password)
{
  int len = 0;
  bool r = true;

#ifdef PEEP_TEST_STATE_MEASURE
  (void) len;
  strcpy(ssid, TEST_WIFI_SSID);
  strcpy(password, TEST_WIFI_PASSWORD);
#else
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
      (uint8_t *) password,
      WIFI_PASSWORD_LEN_MAX);
    if (len <= 0) {
      r = false;
    }
    else {
      LOGI("PASS = %s", password);
    }
  }
#endif

  return r;
}

/***** Global Functions *****/

void
task_measure(void * arg)
{
  struct hatch_measurement meas;
  char * root_ca = (char *) _root_ca_start;
  char * cert = (char *) _cert_start;
  char * key = (char *) _key_start;
  char * ssid = _ssid;
  char * pass = _pass;
  bool is_local_measure = false;
  bool r = true;

  LOGI("start");

  _buffer = malloc(_BUFFER_LEN);
  _ssid = malloc(WIFI_SSID_LEN_MAX);
  _pass = malloc(WIFI_PASSWORD_LEN_MAX);
  if ((NULL == _buffer) || (NULL == _ssid) || (NULL == _pass)) {
    LOGE_TRAP("failed to allocate memory");
  }
  ssid = _ssid;
  pass = _pass;

  if (r) {
    r = _get_wifi_ssid_pasword(ssid, pass);
  }

  if (r) {
    r = hal_init();
  }

  if (r && !is_local_measure) {
    r = wifi_connect(ssid, pass, 15);
    is_local_measure = (r) ? false : true;
    r = true;
  }

  if (r && !is_local_measure) {
    r = aws_mqtt_init(root_ca, cert, key, (char*) _uuid_start, 5);
    is_local_measure = (r) ? false : true;
    r = true;
  }

  if (r) {
    r = hal_read_temperature_humdity_pressure_resistance(
      &(meas.temperature),
      &(meas.humidity),
      &(meas.air_pressure),
      &(meas.gas_resistance));

    meas.unix_timestamp = time(NULL);
  }

  if (r) {
    r = _publish_measurements(_buffer, _BUFFER_LEN, &meas, (char *) _uuid_start);
  }
  else if (is_local_measure) {
    uint32_t total = 0;

    memory_measurement_db_add(&meas);
    total = memory_measurement_db_total();

    LOGI("%d measurements stored\n", total);
  }

  hal_deep_sleep(_MEASURE_INTERVAL_SEC);
}
