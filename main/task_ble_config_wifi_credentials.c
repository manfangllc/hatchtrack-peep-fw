/***** Includes *****/

#include "tasks.h"
#include "ble_server.h"
#include "hal.h"
#include "json_parse.h"
#include "memory.h"
#include "state.h"
#include "system.h"
#include "wifi.h"

/***** Extern Data *****/

extern const uint8_t _uuid_start[]   asm("_binary_uuid_txt_start");
extern const uint8_t _uuid_end[]   asm("_binary_uuid_txt_end");

/***** Local Data *****/

static EventGroupHandle_t _sync_event_group = NULL;
static const int SYNC_BIT = BIT0;
static const int BUTTON_BIT = BIT1;

static char * _ssid = NULL;
static char * _pass = NULL;

/***** Local Functions *****/

static void
_push_button_callback(bool is_pressed)
{
  BaseType_t task_yield = pdFALSE;

  xEventGroupSetBitsFromISR(_sync_event_group, BUTTON_BIT, &task_yield);

  if (task_yield) {
    portYIELD_FROM_ISR();
  }
}

static void
_ble_write_callback(uint8_t * buf, uint16_t len)
{
  json_parse_wifi_credentials_msg(
    (char *) buf,
    _ssid,
    WIFI_SSID_LEN_MAX,
    _pass,
    WIFI_PASSWORD_LEN_MAX);

  if ((0 != _ssid[0]) && (0 != _pass[0])) {
    xEventGroupSetBits(_sync_event_group, SYNC_BIT);
  }
}

static void
_ble_read_callback(uint8_t * buf, uint16_t * len, uint16_t max_len)
{
  snprintf((char *) buf, max_len, "{\n\"uuid\": \"%s\"\n}", _uuid_start);
  *len = strnlen((char *) buf, max_len);
  LOGI("sending...\n%s", (char *) buf);
}

void
task_ble_config_wifi_credentials(void * arg)
{
  EventBits_t bits = 0;
  bool r = true;

  // Install gpio isr service. We do this here because the subsystems register
  // their own ISRs for the pins they use for user input.
  gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

  _ssid = malloc(WIFI_SSID_LEN_MAX);
  _pass = malloc(WIFI_PASSWORD_LEN_MAX);
  if ((NULL == _ssid) || (NULL == _pass)) {
    LOGE_TRAP("failed to allocate memory");
  }

  _sync_event_group = xEventGroupCreate();
  memset(_ssid, 0, WIFI_SSID_LEN_MAX);
  memset(_pass, 0, WIFI_PASSWORD_LEN_MAX);

  if (r) {
    LOGI("initialize bluetooth low energy");
    r = ble_init();
    if (!r) LOGE("failed to initialize bluetooth");
  }

  ble_register_write_callback(_ble_write_callback);
  ble_register_read_callback(_ble_read_callback);

  if (r) {
    LOGI("initialize push button");
    r = hal_init_push_button(_push_button_callback);
    if (!r) LOGE("failed to initialize push button");
  }

#if defined (PEEP_TEST_STATE_BLE_CONFIG)
  while (1) {
    // Wait for BLE config to complete.
    bits = xEventGroupWaitBits(
      _sync_event_group,
      SYNC_BIT | BUTTON_BIT,
      false,
      false,
      portMAX_DELAY);

    LOGI("got event");

    if (bits & SYNC_BIT) {
      xEventGroupClearBits(_sync_event_group, SYNC_BIT);
      LOGI("ssid = %s", _ssid);
      LOGI("password = %s", _pass);
      _ssid[0] = 0;
      _pass[0] = 0;
    }
    else if (bits & BUTTON_BIT) {
      xEventGroupClearBits(_sync_event_group, BUTTON_BIT);
      LOGI("push button");
    }
  }
#else
  // Wait for BLE config to complete.
  bits = xEventGroupWaitBits(
    _sync_event_group,
    SYNC_BIT | BUTTON_BIT,
    true,
    false,
    portMAX_DELAY);

  if (bits & SYNC_BIT) {
    LOGI("received WiFi SSID and password");

    LOGI("saving WiFI SSID");
    memory_set_item(
      MEMORY_ITEM_WIFI_SSID,
      (uint8_t *) _ssid,
      WIFI_SSID_LEN_MAX);

    // feed watchdog
    vTaskDelay(100);

    LOGI("saving WiFI password");
    memory_set_item(
      MEMORY_ITEM_WIFI_PASS,
      (uint8_t *) _pass,
      WIFI_PASSWORD_LEN_MAX);

    // feed watchdog
    vTaskDelay(100);

    LOGI("saving Peep state");
    enum peep_state state = PEEP_STATE_MEASURE;
    memory_set_item(
      MEMORY_ITEM_STATE,
      (uint8_t *) &state,
      sizeof(enum peep_state));
  }
  else if (bits & BUTTON_BIT) {
    LOGI("push button");
  }
#endif

  hal_deep_sleep_push_button();
}
