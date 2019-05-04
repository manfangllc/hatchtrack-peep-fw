/***** Includes *****/

#include "tasks.h"
#include "aws_mqtt.h"
#include "aws_mqtt_shadow.h"
#include "hal.h"
#include "json_parse.h"
#include "memory.h"
#include "state.h"
#include "system.h"
#include "wifi.h"

/***** Defines *****/

#define _BUFFER_LEN (2048)

#ifdef PEEP_TEST_STATE_MEASURE_CONFIG
  #define _TEST_WIFI_SSID "thesignal"
  #define _TEST_WIFI_PASSWORD "palmerho"
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
_get_wifi_ssid_pasword(char * ssid, char * password)
{
  int len = 0;
  bool r = true;

#ifdef PEEP_TEST_STATE_MEASURE_CONFIG
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

  r = json_parse_hatch_config_msg((char *) buf, &_config);
  if (r) {
    LOGI("uuid=%s", _config.uuid);
    LOGI("end_unix_timestamp=%d", _config.end_unix_timestamp);
    LOGI("measure_interval_sec=%d", _config.measure_interval_sec);
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
task_measure_config(void * arg)
{
  EventBits_t bits = 0;
  char * peep_uuid = (char *) _uuid_start;
  char * root_ca = (char *) _root_ca_start;
  char * cert = (char *) _cert_start;
  char * key = (char *) _key_start;
  char * ssid = NULL;
  char * pass = NULL;
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

  memset(&_config, 0, sizeof(struct hatch_configuration));

  if (r) {
    r = _get_wifi_ssid_pasword(ssid, pass);
  }

  if (r) {
    r = hal_init();
  }

  if (r) {
    r = wifi_connect(ssid, pass, 15);
  }

  if (r) {
    r = aws_mqtt_shadow_init(root_ca, cert, key, peep_uuid, 5);
  }

  if (r) {
    r = aws_mqtt_shadow_get(_shadow_callback);
  }

  while ((r) && (0 == (bits & SYNC_BIT))) {
    aws_mqtt_shadow_poll(1000);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    bits = xEventGroupWaitBits(
      _sync_event_group,
      SYNC_BIT,
      false,
      true,
      0);
  }

  wifi_disconnect();
  hal_deep_sleep(0);
}
