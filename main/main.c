/***** Includes *****/

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "lwip/apps/sntp.h"

#include "nvs_flash.h"
#include "json_parse.h"
#include "system.h"
#include "aws-mqtt.h"
#include "ble_server.h"
#include "wifi.h"
#include "led.h"
#include "message.h"
#include "ble_server.h"
#include "uart_server.h"
#include "memory.h"
#include "state.h"
#include "hal.h"

/***** Defines *****/

// Comment this out to enter deep sleep when not active.
//#define _NO_DEEP_SLEEP 1
#define _MEASURE_INTERVAL_SEC 60
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

static char _ssid[WIFI_SSID_LEN_MAX] = {0};
static char _pass[WIFI_PASSWORD_LEN_MAX] = {0};

static EventGroupHandle_t _sync_event_group = NULL;
const int SYNC_BIT = BIT0;

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

static bool
init_time(void)
{
  const int retry_max = 10;
  struct tm timeinfo = {0};
  time_t now = 0;
  int retry = 0;
  bool r = true;

  time(&now);
  localtime_r(&now, &timeinfo);
  // Is time set? If not, tm_year will be (1970 - 1900).
  if (timeinfo.tm_year < (2016 - 1900)) {
    LOGI("time not set, initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // wait for time to be set
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_max) {
      LOGI("waiting for time to sync... (%d/%d)", retry, retry_max);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      time(&now);
      localtime_r(&now, &timeinfo);
    }
  }

  if (retry < retry_max) {
    LOGI(
      "%d-%d-%d %d:%d:%d",
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec);
    r = true;
  }
  else {
    LOGE("failed to sync time");
    r = false;
  }

  return r;
}

void
_ble_write_callback(uint8_t * buf, uint16_t len)
{
  json_parse_wifi_credentials_msg(
    (char *) buf,
    _ssid,
    WIFI_SSID_LEN_MAX,
    _pass,
    WIFI_PASSWORD_LEN_MAX);

  if ((0 != _ssid[0]) && (0 != _pass[0])) {
    xEventGroupSetBits(_sync_event_group, SYNC_BIT);
  }
}

void
_ble_read_callback(uint8_t * buf, uint16_t * len, uint16_t max_len)
{
  sprintf((char *) buf, "{\n\"uuid\": \"%s\"\n}", _uuid_start);
  LOGI("sending...\n%s", (char *) buf);
}

static void
_ble_config_wifi_credentials_task(void * arg)
{
  EventBits_t bits = 0;
  bool r = true;

  memset(_ssid, 0, WIFI_SSID_LEN_MAX);
  memset(_pass, 0, WIFI_PASSWORD_LEN_MAX);

  r = ble_init();
  if (!r) {
    LOGE("failed to initialize bluetooth");
  }

  ble_register_write_callback(_ble_write_callback);
  ble_register_read_callback(_ble_read_callback);

  // Wait for BLE config to complete.
  bits = xEventGroupWaitBits(
    _sync_event_group,
    SYNC_BIT,
    false,
    true,
    portMAX_DELAY);

  {
    enum peep_state state = PEEP_STATE_MEASURE_CONFIG;
    memory_set_item(
      MEMORY_ITEM_WIFI_SSID,
      (uint8_t *) _ssid,
      WIFI_SSID_LEN_MAX);

    // feed watchdog
    vTaskDelay(10);

    memory_set_item(
      MEMORY_ITEM_WIFI_PASS,
      (uint8_t *) _pass,
      WIFI_PASSWORD_LEN_MAX);

    // feed watchdog
    vTaskDelay(10);

    memory_set_item(
      MEMORY_ITEM_STATE,
      (uint8_t *) &state,
      sizeof(enum peep_state));
  }

  _deep_sleep(0);
}

static void
_measure_task(void * arg)
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

  _deep_sleep(_MEASURE_INTERVAL_SEC);
}

static void
_measure_config_cb(uint8_t * buf, uint16_t len)
{
  memcpy(_buffer, buf, len);
  _buffer[len] = 0;

  printf("%s\n", _buffer);
}

static void
_measure_config_task(void * arg)
{
  EventBits_t bits = 0;
  char * mqtt_topic = "hatchtrack/data/put";
  char * root_ca = (char *) _root_ca_start;
  char * cert = (char *) _cert_start;
  char * key = (char *) _key_start;
  char * ssid = _ssid;
  char * pass = _pass;
  bool is_configured = false;
  bool r = true;
  int len = 0;

  LOGI("start");

  if (r) {
    len = memory_get_item(
      MEMORY_ITEM_WIFI_SSID,
      (uint8_t *) ssid,
      WIFI_SSID_LEN_MAX);
    if (len <= 0) {
      r = false;
    }
    else {
      LOGI("SSID = %s\n", ssid);
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
      LOGI("PASS = %s\n", pass);
    }
  }

  if (r) {
    r = hal_init();
  }

  if (r) {
    r = wifi_connect(ssid, pass);
  }

  if (r) {
    r = init_time();
  }

  if (r) {
    r = aws_mqtt_init(root_ca, cert, key, (char*) _uuid_start);
  }

  if (r) {
    r = aws_mqtt_subsribe(mqtt_topic, _measure_config_cb);
  }

  while ((r) && (!is_configured)) {
    bits = 0;
    while (0 == (bits & SYNC_BIT)) {
      aws_mqtt_subsribe_poll(1000);

      // Wait for BLE config to complete.
      bits = xEventGroupWaitBits(
        _sync_event_group,
        SYNC_BIT,
        false,
        true,
        1000 / portTICK_PERIOD_MS);
    }

    aws_mqtt_unsubscribe(mqtt_topic);

  }

  _deep_sleep(_MEASURE_INTERVAL_SEC);
}

/***** Global Functions *****/

void
app_main()
{
  enum peep_state state = PEEP_STATE_UNKNOWN;
  bool r = true;
  int32_t len = 0;

  _sync_event_group = xEventGroupCreate();

  nvs_flash_init();

  r = memory_init();
  RESULT_TEST_ERROR_TRACE(r);

  len = memory_get_item(
    MEMORY_ITEM_STATE,
    (uint8_t * ) &state,
    sizeof(enum peep_state));

  if (sizeof(enum peep_state) != len) {
    state = PEEP_STATE_BLE_CONFIG;
    memory_set_item(
      MEMORY_ITEM_STATE,
      (uint8_t *) &state,
      sizeof(enum peep_state));
  }

  if (PEEP_STATE_BLE_CONFIG == state) {
    xTaskCreate(
      _ble_config_wifi_credentials_task,
      "ble config task",
      8192,
      NULL,
      2,
      NULL);
  }
  else if (PEEP_STATE_MEASURE == state) {
    xTaskCreate(
      _measure_task,
      "measurement task",
      8192,
      NULL,
      2,
      NULL);
  }
  else if (PEEP_STATE_MEASURE_CONFIG == state) {
    xTaskCreate(
      _measure_config_task,
      "measurement config task",
      8192,
      NULL,
      2,
      NULL);
  }
}
