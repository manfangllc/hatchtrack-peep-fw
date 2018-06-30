/***** Includes *****/

#include "aws.h"
#include "system.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"

/***** Defines *****/

#define AWS_HOST_NAME "a1mdhmgt02ub52.iot.us-west-2.amazonaws.com" // unique
#define AWS_PORT_NUMBER 8883 // default

#define AWS_TOPIC_NAME "hatchtrack/data/put"

/***** Local Data *****/

static AWS_IoT_Client _client;

/***** Local Functions *****/

void
ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action,
Shadow_Ack_Status_t status, const char *pReceivedJsonDocument,
void *pContextData)
{
	IOT_UNUSED(pThingName);
	IOT_UNUSED(action);
	IOT_UNUSED(pReceivedJsonDocument);
	IOT_UNUSED(pContextData);

	if(SHADOW_ACK_TIMEOUT == status) {
			ESP_LOGE(__func__, "Update timed out");
	} else if(SHADOW_ACK_REJECTED == status) {
			ESP_LOGE(__func__, "Update rejected");
	} else if(SHADOW_ACK_ACCEPTED == status) {
			ESP_LOGI(__func__, "Update accepted");
	}
}


void
disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data)
{
	ESP_LOGW(__func__, "MQTT Disconnect");
	IoT_Error_t rc = FAILURE;

	if(NULL == pClient) {
		return;
	}

	if(aws_iot_is_autoreconnect_enabled(pClient)) {
		ESP_LOGI(__func__, "Auto Reconnect is enabled, Reconnecting attempt will start now");
	} else {
		ESP_LOGW(__func__, "Auto Reconnect not enabled. Starting manual reconnect...");
		rc = aws_iot_mqtt_attempt_reconnect(pClient);
		if(NETWORK_RECONNECTED == rc) {
			ESP_LOGW(__func__, "Manual Reconnect Successful");
		}
		else {
			ESP_LOGW(__func__, "Manual Reconnect Failed - %d", rc);
		}
	}
}

/***** Global Functions *****/

bool
aws_connect(void)
{
#if 0
	IoT_Client_Init_Params mqtt_params = iotClientInitParamsDefault;
	IoT_Client_Connect_Params connect_params = iotClientConnectParamsDefault;
	IoT_Error_t r = SUCCESS;

	mqtt_params.enableAutoReconnect = false; // We enable this later below
	mqtt_params.pHostURL = AWS_HOST_NAME;
	mqtt_params.port = AWS_PORT_NUMBER;

	mqtt_params.pRootCALocation = (const char *) aws_root_ca_pem;
	mqtt_params.pDeviceCertLocation = (const char *) certificate_pem_crt;
	mqtt_params.pDevicePrivateKeyLocation = (const char *) private_pem_key;

	mqtt_params.mqttCommandTimeout_ms = 20000;
	mqtt_params.tlsHandshakeTimeout_ms = 5000;
	mqtt_params.isSSLHostnameVerify = true;
	mqtt_params.disconnectHandler = disconnectCallbackHandler;
	mqtt_params.disconnectHandlerData = NULL;

	r = aws_iot_mqtt_init(&_client, &mqtt_params);
	RETURN_TEST(SUCCESS == r, "aws_iot_mqtt_init returned error %d\n", r);

	connect_params.keepAliveIntervalInSec = 10;
	connect_params.isCleanSession = true;
	connect_params.MQTTVersion = MQTT_3_1_1;
	/* Client ID is set in the menuconfig of the example */
	connect_params.pClientID = AWS_THING_NAME;
	connect_params.clientIDLen = (uint16_t) strlen(AWS_THING_NAME);
	connect_params.isWillMsgPresent = false;

	ESP_LOGI(__func__, "Connecting to AWS...");
	do {
		r = aws_iot_mqtt_connect(&_client, &connect_params);
		if(SUCCESS != r) {
			ESP_LOGE(
				__func__,
				"Error(%d) connecting to %s:%d",
				r,
				mqtt_params.pHostURL,
				mqtt_params.port);
				vTaskDelay(1000 / portTICK_RATE_MS);
		}
	} while (SUCCESS != r);

#if 0
	/*
	 * Enable Auto Reconnect functionality. Minimum and Maximum time of 
	 * Exponential backoff are set in aws_iot_config.h
	 *
	 *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
	 *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
	 */
	r = aws_iot_mqtt_autoreconnect_set_status(&_client, true);
	RETURN_TEST(
		SUCCESS == r,
		"aws_iot_mqtt_autoreconnect_set_status returned error %d\n",
		r);
#endif
#endif
	return true;
}

bool
aws_disconnect(void)
{
	IoT_Error_t r = SUCCESS;

	r = aws_iot_mqtt_disconnect(&_client);

	return (SUCCESS == r) ? true : false;
}

bool
aws_publish_json(uint8_t * json_str, uint32_t json_len)
{
	IoT_Publish_Message_Params publish_params;
	IoT_Error_t r = SUCCESS;
	const char * topic = AWS_TOPIC_NAME;
	const int topic_len = strlen(topic);

	publish_params.qos = QOS0;
	publish_params.isRetained = 0;
	publish_params.payload = (void *) json_str;
	publish_params.payloadLen = json_len;

	printf("%s\n%s\n", topic, json_str);
	r = aws_iot_mqtt_publish(&_client, topic, topic_len, &publish_params);

	return (SUCCESS == r) ? true : false;
}
