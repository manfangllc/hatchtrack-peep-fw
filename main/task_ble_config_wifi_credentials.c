/***** Includes *****/

#include "tasks.h"
#include "ble_server.h"
#include "hal.h"
#include "json_parse.h"
#include "memory.h"
#include "state.h"
#include "system.h"
#include "wifi.h"
#include "OSAL.h"

/***** Extern Data *****/

extern const uint8_t _uuid_start[]   asm("_binary_uuid_txt_start");
extern const uint8_t _uuid_end[]   asm("_binary_uuid_txt_end");

/***** Local Data *****/

static char *_ssid = NULL;
static char *_pass = NULL;

/***** Local Functions *****/

static void _ble_write_callback(uint8_t * buf, uint16_t len)
{
   /* Due to re-organize, check for global validity to guard against    */
   /* former task exiting (and clearing these) while BLE callbacks are  */
   /* possible.                                                         */
   if((_ssid != NULL) && (_pass != NULL))
   {
      json_parse_wifi_credentials_msg((char *) buf, _ssid, WIFI_SSID_LEN_MAX, _pass, WIFI_PASSWORD_LEN_MAX);

      if ((0 != _ssid[0]) && (0 != _pass[0]))
      {
         OSAL_SetEventBits(OSAL_EVENT_BITS_BLE_SYNC);
      }
   }
}

static void _ble_read_callback(uint8_t * buf, uint16_t * len, uint16_t max_len)
{
  snprintf((char *) buf, max_len, "{\n\"uuid\": \"%s\"\n}", _uuid_start);
  *len = strnlen((char *) buf, max_len);
  LOGI("sending...\n%s", (char *) buf);
}

uint32_t task_ble_config_wifi_credentials(TaskContext_t *TaskContext)
{
  bool        Result    = true;
  uint32_t    DSReq     = 1;
  EventBits_t BitsRecv  = 0;

  /* Save required context variables that will be modified in callbacks */
  /* into global variables.                                             */
  _ssid = TaskContext->SSID;
  _pass = TaskContext->Password;

  /* Clear the SSID and password since we are in stage to set them.     */
  memset(_ssid, 0, WIFI_SSID_LEN_MAX);
  memset(_pass, 0, WIFI_PASSWORD_LEN_MAX);

  /* Initialize the BLE module.                                         */
  LOGI("initialize bluetooth low energy");

  Result = ble_init();
  if (!Result)
  {
     LOGE("failed to initialize bluetooth");
  }

  ble_register_write_callback(_ble_write_callback);
  ble_register_read_callback(_ble_read_callback);

#if defined(PEEP_TEST_STATE_BLE_CONFIG)

  while (1)
  {
     // Wait for BLE config to complete.
     BitsRecv = OSAL_WaitEventBits(OSAL_EVENT_BITS_BLE_SYNC, portMAX_DELAY);

     // clear received event bits
     OSAL_ClearEventBits(BitsRecv);

     LOGI("got event: %u", (unsigned int)BitsRecv);

     if (BitsRecv & OSAL_EVENT_BITS_BLE_SYNC)
     {
        LOGI("ssid = %s", _ssid);
        LOGI("password = %s", _pass);
        _ssid[0] = 0;
        _pass[0] = 0;
     }
     else if (BitsRecv & OSAL_EVENT_BITS_RESERVED)
     {
        LOGI("push button");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
     }
  }

#else

  // Wait for BLE config to complete.
  BitsRecv = OSAL_WaitEventBits(OSAL_EVENT_BITS_BLE_SYNC, portMAX_DELAY);
  if (BitsRecv & OSAL_EVENT_BITS_BLE_SYNC)
  {
     /* Clear the BLE Sync bit.                                         */
     OSAL_ClearEventBits(OSAL_EVENT_BITS_BLE_SYNC);

     LOGI("received WiFi SSID and password");

     LOGI("saving WiFI SSID");
     memory_set_item(MEMORY_ITEM_WIFI_SSID, (uint8_t *) _ssid, WIFI_SSID_LEN_MAX);

     // feed watchdog
     vTaskDelay(100);

     LOGI("saving WiFI password");
     memory_set_item(MEMORY_ITEM_WIFI_PASS, (uint8_t *) _pass, WIFI_PASSWORD_LEN_MAX);

     // feed watchdog
     vTaskDelay(100);

     struct hatch_configuration config;
     HATCH_CONFIG_INIT(config);
     memory_set_item(MEMORY_ITEM_HATCH_CONFIG, (uint8_t *) &config, sizeof(struct hatch_configuration));

     // feed watchdog
     vTaskDelay(100);

     LOGI("saving Peep state");

     /* Change the task state.                                          */
     peep_set_state(PEEP_STATE_MEASURE_CONFIG);

     TaskContext->CurrentState = PEEP_STATE_MEASURE_CONFIG;

     /* Do not enter deep sleep prior to entering measuring config      */
     /* state.                                                          */
     DSReq = 0;
  }

  /* disable bluetooth.                                                 */
  ble_cleanup();

  /* Clear globals.                                                     */
  _ssid = NULL;
  _pass = NULL;

#endif

  return(DSReq);
}
