/***** Includes *****/

#include <time.h>

#include "tasks.h"
#include "aws_mqtt.h"
#include "hal.h"
#include "hatch_config.h"
#include "hatch_measurement.h"
#include "memory.h"
#include "memory_measurement_db.h"
#include "state.h"
#include "system.h"
#include "wifi.h"

/***** Defines *****/

// Comment this out to enter deep sleep when not active.
//#define _NO_DEEP_SLEEP 1
#define _BUFFER_LEN 2048

#define _UNIX_TIMESTAMP_THRESHOLD (1546300800)

#ifdef PEEP_TEST_STATE_MEASURE
  #define _TEST_WIFI_SSID "thesignal"
  #define _TEST_WIFI_PASSWORD "palmerho"
  #define _TEST_HATCH_CONFIG_UUID "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  #define _TEST_HATCH_CONFIG_MEASURE_INTERVAL_SEC 900
  #define _TEST_HATCH_CONFIG_END_UNIX_TIMESTAMP 1735084800
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
  struct hatch_measurement * meas, char * peep_uuid, char * hatch_uuid)
{
  int32_t bytes = 0;
  bool r = true;

  bytes = snprintf(
    (char *) buf,
    buf_len,
    "{\n"
    "\"unixTime\": %d,\n"
    "\"peepUUID\": \"%s\",\n"
    "\"hatchUUID\": \"%s\",\n"
    "\"temperature\": %f,\n"
    "\"humidity\": %f,\n"
    "\"pressure\": %f,\n"
    "\"gasResistance\": %f\n"
    "}",
    meas->unix_timestamp,
    (const char *) peep_uuid,
    (const char *) hatch_uuid,
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
  struct hatch_measurement * meas, char * peep_uuid, char * hatch_uuid)
{
  struct hatch_measurement old;
  uint32_t total = 0;
  bool r = true;

  if (r) {
    _format_json(buf, buf_len, meas, peep_uuid, hatch_uuid);
  }

  if (r) {
    r = aws_mqtt_publish("hatchtrack/data/put", (char *) buf, false);
  }

  if (r) {
    total = memory_measurement_db_total();
    LOGI("%d old measurements to upload", total);
  }

  if (r && total) {

    r = memory_measurement_db_read_open();

    while (r && total--) {
      r = memory_measurement_db_read_entry(&old);

      if (r) {
        r = _format_json(buf, buf_len, &old, peep_uuid, hatch_uuid);
      }

      if (r) {
        r = aws_mqtt_publish("hatchtrack/data/put", (char *) buf, false);
      }
    }

    memory_measurement_db_read_close();

    if (r) {
      LOGI("deleting old data");
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
  strcpy(ssid, _TEST_WIFI_SSID);
  strcpy(password, _TEST_WIFI_PASSWORD);
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

static bool
_get_hatch_config(struct hatch_configuration * config)
{
  int len = 0;
  bool r = true;
#ifdef PEEP_TEST_STATE_MEASURE
  (void) len;
  strcpy(config->uuid, _TEST_HATCH_CONFIG_UUID);
  config->end_unix_timestamp = _TEST_HATCH_CONFIG_END_UNIX_TIMESTAMP;
  config->measure_interval_sec = _TEST_HATCH_CONFIG_MEASURE_INTERVAL_SEC;
#else
  if (r) {
    len = memory_get_item(
      MEMORY_ITEM_HATCH_CONFIG,
      (uint8_t *) config,
      sizeof(struct hatch_configuration));
    if (len <= 0) {
      r = false;
    }
    else {
      LOGI("hatch UUID = %s", config->uuid);
    }
  }
#endif

  return r;
}

/***** Global Functions *****/

void
task_measure(void * arg)
{
  struct hatch_configuration config;
  struct hatch_measurement meas;
  char * peep_uuid = (char *) _uuid_start;
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
    r = _get_hatch_config(&config);
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
    r = aws_mqtt_init(root_ca, cert, key, peep_uuid, 5);
    is_local_measure = (r) ? false : true;
    r = true;
  }

  if (r) {
    r = hal_read_temperature_humdity_pressure_resistance(
      &(meas.temperature),
      &(meas.humidity),
      &(meas.air_pressure),
      &(meas.gas_resistance));
  }

  if (r) {
    meas.unix_timestamp = time(NULL);
    if (meas.unix_timestamp < _UNIX_TIMESTAMP_THRESHOLD) {
      LOGE("timestamp is invalid");
      r = false;
    }
  }

  if (r) {
    if (meas.unix_timestamp >= config.end_unix_timestamp) {
      enum peep_state state = PEEP_STATE_MEASURE_CONFIG;
      memory_set_item(
        MEMORY_ITEM_STATE,
        (uint8_t *) &state,
        sizeof(enum peep_state));
    }
  }

  if (r && !is_local_measure) {
    r = _publish_measurements(
      _buffer,
      _BUFFER_LEN,
      &meas,
      (char *) _uuid_start,
      config.uuid);
  }
  else if (r && is_local_measure) {
    uint32_t total = 0;

    memory_measurement_db_add(&meas);
    total = memory_measurement_db_total();

    LOGI("%d measurements stored", total);
  }

  hal_deep_sleep(config.measure_interval_sec);
}
