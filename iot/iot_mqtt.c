/***** Includes *****/

#include "iot_mqtt.h"
#include "system.h"

#include "mqtt_client.h"

/***** Defines *****/

#define _BROKER_URI "mqtts://mqtt.hatchtrack.com:8883"

/***** Local Data *****/

extern const uint8_t _pem_start[]   asm("_binary_iot_mqtt_pem_start");
extern const uint8_t _pem_end[]   asm("_binary_iot_mqtt_pem_end");

// event group to signal when we are connected & ready to subscribe/publish
static EventGroupHandle_t _mqtt_event_group = NULL;
// bit indicating MQTT is connected
static const int _MQTT_CONNECTED_BIT = BIT0;

esp_mqtt_client_handle_t _client = NULL;

/***** Local Functions *****/

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  esp_mqtt_client_handle_t client = event->client;

  (void) client;

  // your_context_t *context = event->context;
  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      LOGI("MQTT_EVENT_CONNECTED");
      xEventGroupSetBits(_mqtt_event_group, _MQTT_CONNECTED_BIT);
      break;

    case MQTT_EVENT_DISCONNECTED:
      LOGI("MQTT_EVENT_DISCONNECTED");
      xEventGroupClearBits(_mqtt_event_group, _MQTT_CONNECTED_BIT);
      break;

    case MQTT_EVENT_SUBSCRIBED:
      LOGI("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
      break;

    case MQTT_EVENT_UNSUBSCRIBED:
      LOGI("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
      break;

    case MQTT_EVENT_PUBLISHED:
      LOGI("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
      break;

    case MQTT_EVENT_DATA:
      LOGI("MQTT_EVENT_DATA");
      printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
      printf("DATA=%.*s\r\n", event->data_len, event->data);
      break;

    case MQTT_EVENT_ERROR:
      LOGI("MQTT_EVENT_ERROR");
      break;

    default:
      LOGI("Other event id:%d", event->event_id);
      break;
  }
  return ESP_OK;
}

/***** Global Functions *****/

bool
iot_mqtt_init(void)
{
  esp_mqtt_client_config_t mqtt_cfg;
  const TickType_t connect_timeout = 60000 / portTICK_PERIOD_MS;
  EventBits_t bits = 0;
  esp_err_t err = ESP_OK;
  bool r = true;

  memset(&mqtt_cfg, 0, sizeof(esp_mqtt_client_config_t));

  mqtt_cfg.uri = _BROKER_URI;
  mqtt_cfg.event_handle = mqtt_event_handler;
  mqtt_cfg.cert_pem = (const char *) _pem_start;
  mqtt_cfg.disable_auto_reconnect = false;
  _mqtt_event_group = xEventGroupCreate();
  _client = esp_mqtt_client_init(&mqtt_cfg);

  if (r) {
    err = esp_mqtt_client_start(_client);
    if (ESP_OK != err) {
      r = false;
    }
  }

  if (r) {
    /* Wait for WiFI to show as connected */
    bits = xEventGroupWaitBits(
      _mqtt_event_group,
      _MQTT_CONNECTED_BIT,
      false,
      true,
      connect_timeout);

    if (bits & _MQTT_CONNECTED_BIT) {
      LOGI("Successfully connected to %s\n", _BROKER_URI);
    }
    else {
      LOGI("Error connecting to %s\n", _BROKER_URI);
      r = false;
    }
  }

  return r;
}

bool
iot_mqtt_publish(char * topic, char * message, bool retain)
{
  bool r = false;
  int msg_id;

  if (_client) {
    msg_id = esp_mqtt_client_publish(
      _client,
      topic,
      message,
      0,
      0,
      (retain) ? 1 : 0);
    if (msg_id >= 0) {
      r = true;
    }
  }

  return r;
}
