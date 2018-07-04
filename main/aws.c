/***** Includes *****/

#include "aws.h"
#include "system.h"
#include "memory.h"
#include "state.h"

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
aws_connect(uint8_t * buf, uint32_t len)
{
	bool r = true;
	int32_t m = 0;
	int32_t n = 0;

	IoT_Client_Init_Params mqtt_params = iotClientInitParamsDefault;
	IoT_Client_Connect_Params connect_params = iotClientConnectParamsDefault;
	IoT_Error_t err = SUCCESS;

	mqtt_params.enableAutoReconnect = false;
	mqtt_params.pHostURL = AWS_HOST_NAME;
	mqtt_params.port = AWS_PORT_NUMBER;
	mqtt_params.mqttCommandTimeout_ms = 20000;
	mqtt_params.tlsHandshakeTimeout_ms = 5000;
	mqtt_params.isSSLHostnameVerify = true;
	mqtt_params.disconnectHandler = disconnectCallbackHandler;
	mqtt_params.disconnectHandlerData = NULL;
	connect_params.keepAliveIntervalInSec = 10;
	connect_params.isCleanSession = true;
	connect_params.MQTTVersion = MQTT_3_1_1;
	connect_params.isWillMsgPresent = false;

	if (r) {
		n = memory_get_item(MEMORY_ITEM_ROOT_CA, buf + m, len - m);
		if (n > 0) {
			mqtt_params.pRootCALocation = (const char *) &(buf[m]);
			m += n;
			buf[m++] = '\0';
			printf("%s\n", mqtt_params.pRootCALocation);
		}
		else {
			r = false;
		}
	}

	if (r) {
		n = memory_get_item(MEMORY_ITEM_DEV_CERT, buf + m, len - m);
		if (n > 0) {
			mqtt_params.pDeviceCertLocation = (const char *) &(buf[m]);
			m += n;
			buf[m++] = '\0';
			printf("%s\n", mqtt_params.pDeviceCertLocation);
		}
		else {
			r = false;
		}
	}

	if (r) {
		n = memory_get_item(MEMORY_ITEM_DEV_PRIV_KEY, buf + m, len - m);
		if (n > 0) {
			mqtt_params.pDevicePrivateKeyLocation = (const char *) &(buf[m]);
			m += n;
			buf[m++] = '\0';
			printf("%s\n", mqtt_params.pDevicePrivateKeyLocation);
		}
		else {
			r = false;
		}
	}

	if (r) {
		n = memory_get_item(MEMORY_ITEM_UUID, buf + m, len - m);
		if (n > 0) {
			connect_params.pClientID = (const char *) &(buf[m]);
			connect_params.clientIDLen = (uint16_t) n;
			m += n;
			buf[m++] = '\0';
			printf("%s\n", connect_params.pClientID);
		}
		else {
			r = false;
		}
	}

	connect_params.pClientID = "6d37c3ba-e669-438a-a858-2dcc8d7fb078";
	connect_params.clientIDLen = strlen(connect_params.pClientID);

	if (r) {
		err = aws_iot_mqtt_init(&_client, &mqtt_params);
		RETURN_TEST(SUCCESS == err, "aws_iot_mqtt_init returned error %d\n", err);
	}

	if (r) {
		ESP_LOGI(__func__, "Connecting to AWS...");
		do {
			err = aws_iot_mqtt_connect(&_client, &connect_params);
			if(SUCCESS != err) {
				ESP_LOGE(
					__func__,
					"Error(%d) connecting to %s:%d",
					err,
					mqtt_params.pHostURL,
					mqtt_params.port);
				vTaskDelay(1000 / portTICK_RATE_MS);
			}
		} while (SUCCESS != err);
		printf("Connected to AWS!\n");
	}

	return r;
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
