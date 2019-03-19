/***** Includes *****/

#include "aws_mqtt.h"
#include "aws_mqtt_common.h"
#include "system.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"

/***** Defines *****/

#define AWS_TOPIC_NAME "hatchtrack/data/put"

/***** Local Data *****/

static AWS_IoT_Client _client;

/***** Local Functions *****/

void
_disconnect_cb(AWS_IoT_Client *pClient, void *data)
{
  ESP_LOGW(__func__, "MQTT Disconnect");
  IoT_Error_t rc = FAILURE;

  if(NULL == pClient) {
    return;
  }

  if(aws_iot_is_autoreconnect_enabled(pClient)) {
    LOGI("auto reconnect is enabled, attempting reconnect...");
  }
  else {
    LOGI("auto reconnect not enabled, attempting manual reconnect...");
    rc = aws_iot_mqtt_attempt_reconnect(pClient);
    if(NETWORK_RECONNECTED == rc) {
      LOGI("manual reconnect successful");
    }
    else {
      LOGE("manual reconnect failed (%d)", rc);
    }
  }
}

void
_subscribe_cb(AWS_IoT_Client * p_client, char * topic, uint16_t topic_len,
  IoT_Publish_Message_Params * params, void * p_data)
{
  aws_subscribe_cb cb = p_data;

  cb(params->payload, params->payloadLen);
}

/***** Global Functions *****/

bool
aws_mqtt_init(char * root_ca, char * client_cert, char * client_key, 
  char * client_id, int32_t timeout_sec)
{
  IoT_Client_Init_Params mqtt_params = iotClientInitParamsDefault;
  IoT_Client_Connect_Params connect_params = iotClientConnectParamsDefault;
  IoT_Error_t err = SUCCESS;
  bool r = true;

  mqtt_params.enableAutoReconnect = false;
  mqtt_params.pHostURL = AWS_HOST_NAME;
  mqtt_params.port = AWS_PORT_NUMBER;
  mqtt_params.mqttCommandTimeout_ms = 20000;
  mqtt_params.tlsHandshakeTimeout_ms = 5000;
  mqtt_params.isSSLHostnameVerify = true;
  mqtt_params.disconnectHandler = _disconnect_cb;
  mqtt_params.disconnectHandlerData = NULL;
  mqtt_params.pRootCALocation = (const char *) root_ca;
  mqtt_params.pDeviceCertLocation = (const char *) client_cert;
  mqtt_params.pDevicePrivateKeyLocation = (const char *) client_key;
  connect_params.pClientID = (const char *) client_id;
  connect_params.clientIDLen = (uint16_t) strlen(client_id);
  connect_params.keepAliveIntervalInSec = 10;
  connect_params.isCleanSession = true;
  connect_params.MQTTVersion = MQTT_3_1_1;
  connect_params.isWillMsgPresent = false;

  if (r) {
    // Feed watchdog.
    vTaskDelay(20 / portTICK_RATE_MS);
    err = aws_iot_mqtt_init(&_client, &mqtt_params);
    RESULT_TEST(SUCCESS == err, "aws_iot_mqtt_init returned error %d\n", err);
  }

  if (r) {
    LOGI("Connecting to AWS...");
    do {
      r = true;
      err = aws_iot_mqtt_connect(&_client, &connect_params);
      if(SUCCESS != err) {
        r = false;
        LOGE(
          "Error connecting to %s:%d (%d)",
          mqtt_params.pHostURL,
          mqtt_params.port,
          err);
        vTaskDelay(1000 / portTICK_RATE_MS);
      }
    } while ((SUCCESS != err) && ((--timeout_sec) > 0));
    LOGI("Connected to AWS!\n");
  }

  return r;
}

bool
aws_mqtt_disconnect(void)
{
  IoT_Error_t r = SUCCESS;

  r = aws_iot_mqtt_disconnect(&_client);

  return (SUCCESS == r) ? true : false;
}

bool
aws_mqtt_publish(char * topic, char * message, bool retain)
{
  IoT_Publish_Message_Params publish_params;
  IoT_Error_t r = SUCCESS;

  // NOT SUPPORTED BY AWS!
  (void) retain;

  publish_params.qos = QOS0;
  publish_params.isRetained = 0;
  publish_params.payload = (void *) message;
  publish_params.payloadLen = strlen(message);

  printf("%s\n%s\n", topic, message);
  r = aws_iot_mqtt_publish(&_client, topic, strlen(topic), &publish_params);

  return (SUCCESS == r) ? true : false;
}

bool
aws_mqtt_subscribe(char * topic, aws_subscribe_cb cb)
{
  IoT_Error_t rc = FAILURE;

  rc = aws_iot_mqtt_subscribe(
    &_client,
    topic,
    strlen(topic),
    QOS0,
    _subscribe_cb,
    cb);

  if(SUCCESS != rc) {
    LOGE("Error subscribing : %d", rc);
    return false;
  }
  else {
    LOGI("Subscribed to %s\n", topic);
  }

  return true;
}

bool
aws_mqtt_subscribe_poll(uint32_t poll_ms)
{
  IoT_Error_t rc = FAILURE;

  rc = aws_iot_mqtt_yield(&_client, poll_ms);
  if(SUCCESS != rc) {
    LOGE("Error polling : %d", rc);
    return false;
  }

  return true;
}

bool
aws_mqtt_unsubscribe(char * topic)
{
  IoT_Error_t rc = FAILURE;

  rc = aws_iot_mqtt_unsubscribe(&_client, topic, strlen(topic));
  if(SUCCESS != rc) {
    LOGE("Error unsubsribe : %d", rc);
    return false;
  }

  return true;
}
