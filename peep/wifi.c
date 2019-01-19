/***** Includes *****/

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "wifi.h"
#include "memory.h"
#include "system.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "lwip/err.h"
//#include "apps/sntp/sntp.h"

/***** Local Data *****/

// event group to signal when we are connected & ready to make a request
static EventGroupHandle_t wifi_event_group = NULL;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

//static wifi_ap_record_t * _ap_list = NULL;

/***** Local Functions *****/

static esp_err_t
_event_handler(void *ctx, system_event_t *event)
{
	switch(event->event_id) {
	case SYSTEM_EVENT_STA_START:
		ESP_LOGI(
			__func__,
			"start");
		esp_wifi_connect();
		break;

	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(
			__func__,
			"got ip:%s",
			ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
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
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		break;

	default:
			break;
	}
	return ESP_OK;
}

//static bool
//_init_time(void)
//{
	 //wait for time to be set
	//time_t now = 0;
	//struct tm timeinfo = { 0 };
	//const int retry_count = 10;
	//int retry = 0;

	//while((timeinfo.tm_year < (2016 - 1900)) && (++retry < retry_count)) {
		//ESP_LOGI(
			//__func__,
			//"Waiting for system time to be set... (%d/%d)",
			//retry,
			//retry_count);

		//vTaskDelay(2000 / portTICK_PERIOD_MS);
		//time(&now);
		//localtime_r(&now, &timeinfo);
	//}

	//return (timeinfo.tm_year < (2016 - 1900)) ? false : true;
//}

//static void
//_init_sntp(void)
//{
	//ESP_LOGI(__func__, "Initializing SNTP");
	//sntp_setoperatingmode(SNTP_OPMODE_POLL);
	//sntp_setservername(0, "pool.ntp.org");
	//sntp_init();
//}

/***** Global Functions *****/

bool
wifi_connect(void)
{
	const TickType_t wifi_connect_timeout = 60000 / portTICK_PERIOD_MS;
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	wifi_config_t wifi_config;
	EventBits_t bits = 0;
	int32_t len = 0;
	bool r = true;

	// IF YOU DON'T DO THIS, YOU WILL HAVE A GARBAGE FILLED STRUCT
	memset(&wifi_config, 0, sizeof(wifi_config_t));

	if (r) {
		len = memory_get_item(MEMORY_ITEM_WIFI_SSID, wifi_config.sta.ssid, 32);
		if (len <= 0) {
			r = false;
		}
		else {
			wifi_config.sta.ssid[len] = '\0';
			printf("ssid (%d) = %s\n", len, wifi_config.sta.ssid);
		}
	}

	if (r) {
		len = memory_get_item(MEMORY_ITEM_WIFI_PASS, wifi_config.sta.password, 32);
		if (len <= 0) {
			r = false;
		}
		else {
			wifi_config.sta.password[len] = '\0';
			printf("password (%d) = %s\n", len, wifi_config.sta.password);
		}
	}

	if (r) {
		tcpip_adapter_init();
		wifi_event_group = xEventGroupCreate();
		ESP_ERROR_CHECK( esp_event_loop_init(_event_handler, NULL) );
		ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
		ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
		ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
		ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
		ESP_ERROR_CHECK( esp_wifi_start() );

		/* Wait for WiFI to show as connected */
		bits = xEventGroupWaitBits(
			wifi_event_group,
			WIFI_CONNECTED_BIT,
			false,
			true,
			wifi_connect_timeout);

		if (bits & WIFI_CONNECTED_BIT) {
			ESP_LOGI(__func__, "connected to SSID %s", wifi_config.sta.ssid);
		}
		else {
			ESP_LOGE(__func__, "failed to connect to SSID %s", wifi_config.sta.ssid);
			r = false;
		}
	}

	return r;
}

bool
wifi_disconnect(void)
{
	esp_err_t r = ESP_OK;

	r = esp_wifi_stop();
	if (ESP_OK != r) {
		ESP_LOGE(__func__, "Failed to stop WiFi");
	}

	vEventGroupDelete(wifi_event_group);

	return (ESP_OK == r) ? true : false;
}
