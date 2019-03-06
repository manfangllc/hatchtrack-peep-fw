/***** Includes *****/

#include <string.h>

#include "nvs_flash.h"
#include "jsmn.h"
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

// event group to signal when we are connected & ready to make a request
static EventGroupHandle_t config_event_group = NULL;
const int CONFIG_DONE_BIT = BIT0;

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

void
_ble_write_callback(uint8_t * buf, uint16_t len)
{
  static char key[128];
  static char value[128];
  jsmntok_t t[16];
  jsmn_parser p;
  const char * js;
  jsmntok_t json_value;
  jsmntok_t json_key;
  int string_length;
  int key_length;
  int idx;
  int n;
  int r;

  jsmn_init(&p);

  js = (const char *) buf;
  r = jsmn_parse(&p, js, strlen(js), t, sizeof(t)/(sizeof(t[0])));

   /* Assume the top-level element is an object */
  if ((r < 1) || (t[0].type != JSMN_OBJECT)) {
    printf("Object expected\n");
    return;
  }

  for (n = 1; n < r; n++) {
    json_value = t[n+1];
    json_key = t[n];
    string_length = json_value.end - json_value.start;
    key_length = json_key.end - json_key.start;

    for (idx = 0; idx < string_length; idx++){
      value[idx] = js[json_value.start + idx];
    }

    for (idx = 0; idx < key_length; idx++){
      key[idx] = js[json_key.start + idx];
    }

    value[string_length] = '\0';
    key[key_length] = '\0';
    LOGI("decoded: %s = %s\n", key, value);

    if (0 == strcmp(key, "ssid")) {
      strncpy(_ssid, value, WIFI_SSID_LEN_MAX);
    }
    else if (0 == strcmp(key, "password")) {
      strncpy(_pass, value, WIFI_PASSWORD_LEN_MAX);
    }

    n++;
  }

  if ((0 != _ssid[0]) && (0 != _pass[0])) {
    xEventGroupSetBits(config_event_group, CONFIG_DONE_BIT);
  }
}

void
_ble_read_callback(uint8_t * buf, uint16_t * len, uint16_t max_len)
{
  sprintf((char *) buf, "{\n\"uuid\": \"%s\"\n}", _uuid_start);
  LOGI("sending...\n%s", (char *) buf);
}

static void
_ble_config_task(void * arg)
{
  EventBits_t bits = 0;
  bool r = true;

  config_event_group = xEventGroupCreate();
  memset(_ssid, 0, WIFI_SSID_LEN_MAX);
  memset(_pass, 0, WIFI_PASSWORD_LEN_MAX);

  r = ble_init();
  if (!r) {
    ESP_LOGE(__func__, "ble_init() failed!\n");
  }

  ble_register_write_callback(_ble_write_callback);
  ble_register_read_callback(_ble_read_callback);

  // Wait for BLE config to complete.
  bits = xEventGroupWaitBits(
    config_event_group,
    CONFIG_DONE_BIT,
    false,
    true,
    portMAX_DELAY);

  {
    enum peep_state state = PEEP_STATE_MEASURE;
    memory_set_item(
      MEMORY_ITEM_WIFI_SSID,
      (uint8_t *) _ssid,
      WIFI_SSID_LEN_MAX);

    memory_set_item(
      MEMORY_ITEM_WIFI_PASS,
      (uint8_t *) _pass,
      WIFI_PASSWORD_LEN_MAX);

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

/***** Global Functions *****/

void
app_main()
{
  enum peep_state state = PEEP_STATE_UNKNOWN;
  bool r = true;
  int32_t len = 0;

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
    xTaskCreate(_ble_config_task, "ble config task", 8192, NULL, 2, NULL);
  }
  else if (PEEP_STATE_MEASURE == state) {
    xTaskCreate(_measure_task, "measurement task", 8192, NULL, 2, NULL);
  }
}
