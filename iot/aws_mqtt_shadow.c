/***** Includes *****/

#include "aws_mqtt_shadow.h"
#include "aws_mqtt_common.h"
#include "system.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"

/***** Defines *****/

#define _SHADOW_JSON_MAX_LEN 256

/***** Local Data *****/

static AWS_IoT_Client _client;
static const char * _thing_name;
static char * _js = NULL;

/***** Local Functions *****/

static void
_shadow_get_cb(const char *pThingName, ShadowActions_t action,
  Shadow_Ack_Status_t status, const char *pReceivedJsonDocument,
  void *pContextData)
{
  aws_mqtt_shadow_cb cb = (aws_mqtt_shadow_cb) pContextData;
  char * js = NULL;

  if (SHADOW_GET != action) {
    LOGE("expected SHADOW_GET, but got %d", action);
    return;
  }

  if (SHADOW_ACK_ACCEPTED != status) {
    LOGE("expected SHADOW_ACK_ACCEPTED, but got %d", status);
    return;
  }

  (void) pThingName;
  js = strstr(pReceivedJsonDocument, "\"desired\":");
  if (js) {
    char * src = (char *) pReceivedJsonDocument;
    uint32_t len = 0;
    uint32_t m = 0;
    uint32_t n = 0;

    js += strlen("\"desired\":");
    m = strlen(src);
    n = js - src;

    while (('}' != src[n]) && (n < m)) {
      len++;
      n++;
    }

    if (('}' == src[n]) && (n < m)) {
      memcpy(_js, js, len + 1);
      _js[len + 1] = 0;

      cb((uint8_t *) _js, strlen(_js) + 1);
    }
  }
}

/***** Global Functions *****/

bool
aws_mqtt_shadow_init(char * root_ca, char * client_cert, char * client_key, 
  char * client_id, int32_t timeout_sec)
{
  ShadowInitParameters_t sp = ShadowInitParametersDefault;
  ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
  IoT_Error_t err = SUCCESS;
  bool r = true;

  sp.pHost = AWS_HOST_NAME;
  sp.port = AWS_PORT_NUMBER;
  sp.pClientCRT = (const char *) client_cert;
  sp.pClientKey = (const char *) client_key;
  sp.pRootCA = (const char *) root_ca;
  sp.enableAutoReconnect = false;
  sp.disconnectHandler = NULL;

  // NOTE: We want to subsribe to this "thing's" shadow MQTT topic. For
  // Hatchtrack, we name each Peep with a 128bit UUID, so we use that for
  // the "thing name". To keep things simple, we'll also use the UUID for the
  // client ID as well. The client ID is really just to make sure that each
  // connection is unique; using the "thing name" for this should be safe as
  // only one particuar Peep and the app (which uses a different unique ID)
  // should ever access the same shadow topic.
  scp.pMyThingName = client_id;
  scp.pMqttClientId = client_id;
  scp.mqttClientIdLen = (uint16_t) strlen(client_id);

  _thing_name = scp.pMyThingName;

  if (r) {
    LOGI("shadow init");
    err = aws_iot_shadow_init(&_client, &sp);
    if (SUCCESS != err) {
      LOGE("aws_iot_shadow_init error (%d)", err);
      r = false;
    }
  }

  if (r) {
    LOGI("shadow connect");
    err = aws_iot_shadow_connect(&_client, &scp);
    if(SUCCESS != err) {
        LOGE("aws_iot_shadow_connect error (%d)", err);
        r = false;
    }
  }

  if (r) {
    LOGI("shadow set autoreconnect");
    err = aws_iot_shadow_set_autoreconnect_status(&_client, true);
    if (SUCCESS != err) {
        LOGE("aws_iot_shadow_set_autoreconnect_status error (%d)", err);
        r = false;
    }
  }

  if (r) {
    _js = malloc(_SHADOW_JSON_MAX_LEN);
  }

  return r;
}

bool
aws_mqtt_shadow_disconnect(void)
{
  IoT_Error_t r = SUCCESS;

  r = aws_iot_shadow_disconnect(&_client);

  return (SUCCESS == r) ? true : false;
}

bool
aws_mqtt_shadow_get(aws_mqtt_shadow_cb cb, uint8_t timeout_sec)
{
  IoT_Error_t err = SUCCESS;

  LOGI("shadow get thing %s", _thing_name);
  err = aws_iot_shadow_get(
    &_client,
    _thing_name,
    _shadow_get_cb,
    cb,
    timeout_sec,
    false);
  if (SUCCESS != err) {
    LOGE("aws_iot_shadow_get error (%d)", err);
  }

  return (SUCCESS == err) ? true : false;
}

bool
aws_mqtt_shadow_poll(uint32_t poll_ms)
{
  IoT_Error_t err = SUCCESS;

  err = aws_iot_shadow_yield(&_client, poll_ms);
  if (SUCCESS != err) {
    LOGE("aws_iot_shadow_yield error (%d)", err);
  }

  return (SUCCESS == err) ? true : false;
}
