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


/***** Local Data *****/

static AWS_IoT_Client _client;
static char * _thing_name;

/***** Local Functions *****/

static void
_shadow_get_cb(const char *pThingName, ShadowActions_t action,
  Shadow_Ack_Status_t status, const char *pReceivedJsonDocument,
  void *pContextData)
{
  if (SHADOW_GET != action) {
    LOGE("expected SHADOW_GET, but got %d", action);
    return;
  }

  if (SHADOW_ACK_ACCEPTED != status) {
    LOGE("expected SHADOW_ACK_ACCEPTED, but got %d", status);
    return;
  }

  (void) pThingName;

  printf("%s\n", pReceivedJsonDocument);
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
    err = aws_iot_shadow_set_autoreconnect_status(&_client, true);
    if (SUCCESS != err) {
        LOGE("aws_iot_shadow_set_autoreconnect_status error (%d)", err);
        r = false;
    }
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
aws_mqtt_shadow_get(uint8_t buf, uint32_t buf_len)
{
  IoT_Error_t err = SUCCESS;
  bool r = true;

  err = aws_iot_shadow_get(
    &_client,
    _thing_name,
    _shadow_get_cb,
    NULL,
    60,
    false);

  return r;
}
