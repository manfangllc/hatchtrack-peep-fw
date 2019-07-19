/***** Includes *****/

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "lwip/apps/sntp.h"

#include "wifi.h"
#include "system.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "lwip/err.h"
#include "OSAL.h"

/***** Defines *****/

#define POLL_SEC (1)

/***** Local Data *****/

static volatile bool _is_active = false;

/***** Local Functions *****/

static esp_err_t
_event_handler(void *ctx, system_event_t *event)
{
  switch(event->event_id) {
  case SYSTEM_EVENT_STA_START:
    ESP_LOGI(
      __func__,
      "start");
    if (_is_active) {
      esp_wifi_connect();
    }
    break;

  case SYSTEM_EVENT_STA_GOT_IP:
    ESP_LOGI(
      __func__,
      "got ip:%s",
      ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    OSAL_SetEventBits(OSAL_EVENT_BITS_WIFI_CONNECTED);
    OSAL_ClearEventBits(OSAL_EVENT_BITS_WIFI_DISCONNECTED);
    break;

  case SYSTEM_EVENT_AP_STACONNECTED:
    ESP_LOGI(
      __func__,
      "station:"MACSTR" join, AID=%d",
      MAC2STR(event->event_info.sta_connected.mac),
      event->event_info.sta_connected.aid);
      break;

  case SYSTEM_EVENT_AP_STADISCONNECTED:
    ESP_LOGI(
      __func__,
      "station:"MACSTR"leave, AID=%d",
      MAC2STR(event->event_info.sta_disconnected.mac),
      event->event_info.sta_disconnected.aid);
      break;

  case SYSTEM_EVENT_STA_DISCONNECTED:
    ESP_LOGI(
      __func__,
      "disconnected");
    if (_is_active) {
      esp_wifi_connect();
    }
    OSAL_SetEventBits(OSAL_EVENT_BITS_WIFI_DISCONNECTED);
    OSAL_ClearEventBits(OSAL_EVENT_BITS_WIFI_CONNECTED);
    break;

  default:
      break;
  }

  return ESP_OK;
}

static bool
_init_time(int32_t timeout_sec)
{
  const TickType_t poll_ticks = (POLL_SEC * 1000) / portTICK_PERIOD_MS;
  const int retry_max = timeout_sec / POLL_SEC;
  struct tm timeinfo = {0};
  time_t now = 0;
  int retry = 0;
  bool r = true;

  /* Always enable SNTP when connecting to wifi.                        */
  LOGI("initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();

  /* Check to see if the time is up to date.                            */
  time(&now);
  localtime_r(&now, &timeinfo);
  // Is time set? If not, tm_year will be (1970 - 1900).
  if (timeinfo.tm_year < (2016 - 1900)) {
    LOGI("time not set, initializing SNTP");
    // wait for time to be set
    while ((timeinfo.tm_year < (2016 - 1900)) && (++retry < retry_max)) {
      vTaskDelay(poll_ticks);
      time(&now);
      localtime_r(&now, &timeinfo);
    }
  }

  if (retry < retry_max) {
    LOGI(
      "Current Date/Time : %d-%d-%d %d:%d:%d",
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

/***** Global Functions *****/
bool wifi_connect(char * ssid, char * password, int32_t timeout_sec)
{
  const TickType_t   poll_ticks  = (POLL_SEC * 1000);
  wifi_init_config_t cfg         = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t      wifi_config;
  EventBits_t        bits        = 0;
  bool               r           = true;

  // IF YOU DON'T DO THIS, YOU WILL HAVE A GARBAGE FILLED STRUCT
  memset(&wifi_config, 0, sizeof(wifi_config_t));
  // Do one less than max length to make sure values are NULL terminated.
  strncpy((char *) wifi_config.sta.ssid, ssid, WIFI_SSID_LEN_MAX-1);
  strncpy((char *) wifi_config.sta.password, password, WIFI_PASSWORD_LEN_MAX-1);

  if (r) {
    _is_active = true;
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(_event_handler, NULL) );
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    LOGI("%d second connection timeout", timeout_sec);
    while ((0 == (bits & (OSAL_EVENT_BITS_WIFI_CONNECTED | OSAL_EVENT_BITS_RESERVED))) && (timeout_sec > 0))
    {
      /* Wait for WiFI to show as connected */
      bits = OSAL_WaitEventBits(OSAL_EVENT_BITS_WIFI_CONNECTED, poll_ticks);

      if (0 == (bits & OSAL_EVENT_BITS_WIFI_CONNECTED))
      {
         timeout_sec -= POLL_SEC;
      }
    }

    if (bits & OSAL_EVENT_BITS_WIFI_CONNECTED)
    {
      ESP_LOGI(__func__, "connected to SSID %s", wifi_config.sta.ssid);
    }
    else
    {
      ESP_LOGE(__func__, "failed to connect to SSID %s", wifi_config.sta.ssid);
      r = false;
    }
  }

  if (r)
  {
    r = _init_time(timeout_sec);
  }

  return r;
}

bool
wifi_disconnect(void)
{
  esp_err_t r = ESP_OK;
  EventBits_t bits = 0;

  _is_active = false;

  r = esp_wifi_stop();
  if (ESP_OK != r) {
    LOGE("Failed to stop WiFi");
  }

  bits = OSAL_WaitEventBits(OSAL_EVENT_BITS_WIFI_DISCONNECTED, 5000);
  if (0 == (bits & OSAL_EVENT_BITS_WIFI_DISCONNECTED))
  {
    LOGE("Failed to disconnect from WiFi");
  }

  r = esp_wifi_deinit();
  if (ESP_OK != r)
  {
    LOGE("Failed to deinit WiFi");
  }

  OSAL_ClearEventBits(OSAL_EVENT_BITS_WIFI_CONNECTED | OSAL_EVENT_BITS_WIFI_DISCONNECTED);

  return (ESP_OK == r) ? true : false;
}
