/***** Includes *****/

#include <time.h>

#include "tasks.h"
#include "aws_mqtt.h"
#include "aws_mqtt_shadow.h"
#include "hal.h"
#include "hatch_config.h"
#include "hatch_measurement.h"
#include "json_parse.h"
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
  // SSID of the WiFi AP connect to.
  #define _TEST_WIFI_SSID "thesignal"
  // Password of the WiFi AP to connect to.
  #define _TEST_WIFI_PASSWORD "palmerho"
  // Leave this value alone unless you have a good reason to change it.
  #define _TEST_HATCH_CONFIG_UUID "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  // Seconds that Peep should deep sleep before taking a new measurement.
  #define _TEST_HATCH_CONFIG_MEASURE_INTERVAL_SEC 900
  // UTC Unix timestamp at which Peep should stop taking measurements.
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

static struct hatch_configuration _config;
static uint8_t * _buffer = NULL;
static char * _ssid = NULL;
static char * _pass = NULL;

static EventGroupHandle_t _sync_event_group = NULL;
static const int SYNC_BIT = BIT0;

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

static void
_shadow_callback(uint8_t * buf, uint16_t buf_len)
{
  bool r = true;

#ifdef PEEP_TEST_STATE_MEASURE
  LOGI("AWS Shadow: %s", buf);
#endif

  r = json_parse_hatch_config_msg((char *) buf, &_config);
  if (r) {
    xEventGroupSetBits(_sync_event_group, SYNC_BIT);
  }
  else {
    LOGE("error decoding shadow JSON document");
    LOGE("received %d bytes", buf_len);
    ESP_LOG_BUFFER_HEXDUMP(__func__, buf, buf_len, ESP_LOG_ERROR);
  }
}

/***** Global Functions *****/

void
task_measure(void * arg)
{
  EventBits_t bits = 0;
  struct hatch_measurement meas;
  char * peep_uuid = (char *) _uuid_start;
  char * root_ca = (char *) _root_ca_start;
  char * cert = (char *) _cert_start;
  char * key = (char *) _key_start;
  char * ssid = _ssid;
  char * pass = _pass;
  bool is_shadow_connected = false;
  bool is_local_measure = false;
  bool r = true;

  LOGI("start");

  _sync_event_group = xEventGroupCreate();
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

  if (r) {
    LOGD("performing measurement");
    r = hal_read_temperature_humdity_pressure_resistance(
      &(meas.temperature),
      &(meas.humidity),
      &(meas.air_pressure),
      &(meas.gas_resistance));
  }

  if (r && !is_local_measure) {
    LOGD("connecting to WiFi SSID %s", ssid);
    r = wifi_connect(ssid, pass, 15);
    is_local_measure = (r) ? false : true;
    r = true;
  }

  if (r && !is_local_measure) {
    r = aws_mqtt_shadow_init(root_ca, cert, key, peep_uuid, 60);
  }

  if (r && !is_local_measure) {
    r = aws_mqtt_shadow_get(_shadow_callback);
    is_shadow_connected = true;
  }

  while ((r) && (0 == (bits & SYNC_BIT))) {
    TRACE();
    aws_mqtt_shadow_poll(2500);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    bits = xEventGroupWaitBits(
      _sync_event_group,
      SYNC_BIT,
      false,
      true,
      0);
  }

  if (is_shadow_connected) {
    aws_mqtt_shadow_disconnect();
  }

  if (r && !is_local_measure) {
    LOGD("connecting to AWS");
    r = aws_mqtt_init(root_ca, cert, key, peep_uuid, 5);
    is_local_measure = (r) ? false : true;
    r = true;
  }

  if (r) {
    meas.unix_timestamp = time(NULL);
    if (meas.unix_timestamp < _UNIX_TIMESTAMP_THRESHOLD) {
      LOGE("timestamp is invalid");
      r = false;
    }
  }

  if (r) {
    if (meas.unix_timestamp >= _config.end_unix_timestamp) {
      LOGD("end of measurement time reached");
      enum peep_state state = PEEP_STATE_MEASURE_CONFIG;
      memory_set_item(
        MEMORY_ITEM_STATE,
        (uint8_t *) &state,
        sizeof(enum peep_state));
    }
  }

  if (r && !is_local_measure) {
    LOGD("publishing measurement results");
    r = _publish_measurements(
      _buffer,
      _BUFFER_LEN,
      &meas,
      (char *) _uuid_start,
      _config.uuid);
  }
  else if (r && is_local_measure) {
    uint32_t total = 0;

    LOGD("storing measurement results");

    memory_measurement_db_add(&meas);
    total = memory_measurement_db_total();

    LOGI("%d measurements stored", total);
  }

  wifi_disconnect();
  hal_deep_sleep_timer(_config.measure_interval_sec);
}
