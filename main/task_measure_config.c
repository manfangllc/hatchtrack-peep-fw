/***** Includes *****/

#include "lwip/apps/sntp.h"

#include "tasks.h"
#include "aws-mqtt.h"
#include "hal.h"
#include "memory.h"
#include "state.h"
#include "system.h"
#include "wifi.h"

/***** Defines *****/

#define _BUFFER_LEN (2048)

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

static EventGroupHandle_t _sync_event_group = NULL;
static const int SYNC_BIT = BIT0;

/***** Local Functions *****/

static bool
_init_time(void)
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

static void
_measure_config_cb(uint8_t * buf, uint16_t len)
{
  memcpy(_buffer, buf, len);
  _buffer[len] = 0;

  printf("%s\n", _buffer);
}

/***** Global Functions *****/

void
task_measure_config(void * arg)
{
  EventBits_t bits = 0;
  char * mqtt_topic = "hatchtrack/data/put";
  char * root_ca = (char *) _root_ca_start;
  char * cert = (char *) _cert_start;
  char * key = (char *) _key_start;
  char * ssid = NULL;
  char * pass = NULL;
  bool is_configured = false;
  bool r = true;
  int len = 0;

  LOGI("start");

  _sync_event_group = xEventGroupCreate();
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
    r = _init_time();
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

  hal_deep_sleep(0);
}
