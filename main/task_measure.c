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
#define _BUFFER_LEN (2048)
#define _AWS_SHADOW_GET_TIMEOUT_SEC (30)
#define _UNIX_TIMESTAMP_THRESHOLD (1546300800)
#define _HATCH_CONFIG_DEFAULT_MEASURE_INTERVAL_SEC (5 * 60)
#define _HATCH_CONFIG_DEFAULT_END_UNIX_TIMESTAMP (2147483647)

#if defined(PEEP_TEST_STATE_MEASURE) || (PEEP_TEST_STATE_MEASURE_CONFIG)
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

static void
_shadow_callback(uint8_t * buf, uint16_t buf_len)
{
  bool r = true;

#if defined(PEEP_TEST_STATE_MEASURE) || defined(PEEP_TEST_STATE_MEASURE_CONFIG)
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
  bool is_unix_time_in_range = false;
  bool is_local_measure = false;
  bool r = true;

  LOGI("start");

  #if defined(PEEP_TEST_STATE_MEASURE)
  LOGI("PEEP_TEST_STATE_MEASURE");
  #elif defined(PEEP_TEST_STATE_MEASURE_CONFIG)
  LOGI("PEEP_TEST_STATE_MEASURE_CONFIG");
  #endif

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
    LOGI("initializing hardware");
    r = hal_init();
  }

  if (r) {
    LOGI("performing measurement");
    r = hal_read_temperature_humdity_pressure_resistance(
      &(meas.temperature),
      &(meas.humidity),
      &(meas.air_pressure),
      &(meas.gas_resistance));
  }

  if (r && !is_local_measure) {
    LOGI("WiFi connect to SSID %s", ssid);
    r = wifi_connect(ssid, pass, 15);
    is_local_measure = (r) ? false : true;
    r = true;
  }

  if (r && !is_local_measure) {
    LOGI("AWS MQTT shadow connect");
    r = aws_mqtt_shadow_init(root_ca, cert, key, peep_uuid, 60);
  }

  if (r && !is_local_measure) {
    LOGI("AWS MQTT shadow get timeout %d seconds", _AWS_SHADOW_GET_TIMEOUT_SEC);
    r = aws_mqtt_shadow_get(_shadow_callback, _AWS_SHADOW_GET_TIMEOUT_SEC);
    is_shadow_connected = true;
  }

  while (r && !is_local_measure && (0 == (bits & SYNC_BIT))) {
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
    memory_set_item(
      MEMORY_ITEM_HATCH_CONFIG,
      (uint8_t *) &_config,
      sizeof(struct hatch_configuration));

    // feed watchdog
    vTaskDelay(100 / portTICK_PERIOD_MS);

    LOGI("AWS MQTT shadow disconnect");
    aws_mqtt_shadow_disconnect();
  }
  else {
    memory_get_item(
      MEMORY_ITEM_HATCH_CONFIG,
      (uint8_t *) &_config,
      sizeof(struct hatch_configuration));

    if (false == IS_HATCH_CONFIG_VALID(_config)) {
      // Can't get config from AWS nor is there a previous config stored in
      // flash memory. We'll go to sleep for awhile and then try again in at a
      // later point and hope for better results.
      _config.measure_interval_sec = _HATCH_CONFIG_DEFAULT_MEASURE_INTERVAL_SEC;
      r = false;

      LOGE("failed to load previous hatch configuration");
      LOGE("retry in %d seconds", _config.measure_interval_sec);
    }
    else {
      LOGI("loaded previous hatch configuration");
    }
  }

  if (r && !is_local_measure) {
    LOGI("AWS MQTT connect");
    r = aws_mqtt_init(root_ca, cert, key, peep_uuid, 5);
    is_local_measure = (r) ? false : true;
    r = true;
  }

  if (r) {
    meas.unix_timestamp = time(NULL);
    LOGI("current Unix time %d", meas.unix_timestamp);
    if (meas.unix_timestamp < _UNIX_TIMESTAMP_THRESHOLD) {
      LOGE("timestamp is invalid");
      r = false;
    }
  }

  if (r) {
    if (meas.unix_timestamp >= _config.end_unix_timestamp) {
      LOGI("reached end Unix time %d", _config.end_unix_timestamp);
      enum peep_state state = PEEP_STATE_MEASURE_CONFIG;
      memory_set_item(
        MEMORY_ITEM_STATE,
        (uint8_t *) &state,
        sizeof(enum peep_state));
      // we don't need to report this value
      is_unix_time_in_range = false;
    }
    else {
      LOGI("have not reached end Unix time %d", _config.end_unix_timestamp);
      // haven't reached the end, need to push this measurement to the cloud
      is_unix_time_in_range = true;
    }
  }

  if (r && !is_local_measure && is_unix_time_in_range) {
    LOGI("publishing measurement results over MQTT");
    r = _publish_measurements(
      _buffer,
      _BUFFER_LEN,
      &meas,
      (char *) _uuid_start,
      _config.uuid);
  }
  else if (r && is_local_measure && is_unix_time_in_range) {
    uint32_t total = 0;

    LOGI("storing measurement results in internal flash");
    memory_measurement_db_add(&meas);
    total = memory_measurement_db_total();
    LOGI("%d measurements stored", total);
  }

  LOGI("WiFi disconnect");
  wifi_disconnect();
  hal_deep_sleep_timer(_config.measure_interval_sec);
}
