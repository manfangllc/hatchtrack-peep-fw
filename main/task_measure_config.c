/***** Includes *****/

#include "tasks.h"
#include "aws_mqtt.h"
#include "aws_mqtt_shadow.h"
#include "hal.h"
#include "json_parse.h"
#include "memory.h"
#include "state.h"
#include "system.h"
#include "wifi.h"
#include "OSAL.h"

/***** Defines *****/

#define _AWS_SHADOW_GET_TIMEOUT_SEC (30)

/***** Extern Data *****/

extern const uint8_t _root_ca_start[]   asm("_binary_root_ca_txt_start");
extern const uint8_t _root_ca_end[]   asm("_binary_root_ca_txt_end");

extern const uint8_t _cert_start[]   asm("_binary_cert_txt_start");
extern const uint8_t _cert_end[]   asm("_binary_cert_txt_end");

extern const uint8_t _key_start[]   asm("_binary_key_txt_start");
extern const uint8_t _key_end[]   asm("_binary_key_txt_end");

extern const uint8_t _uuid_start[]   asm("_binary_uuid_txt_start");
extern const uint8_t _uuid_end[]   asm("_binary_uuid_txt_end");

/***** Local Data *****/

#define EVENT_GROUP_BIT_HATCH_CONFIG_SET      (BIT0)

static struct hatch_configuration _config;

/***** Local Functions *****/

static void _shadow_callback(uint8_t * buf, uint16_t buf_len)
{
  bool r = true;

  r = json_parse_hatch_config_msg((char *) buf, &_config);
  if (r)
  {
     OSAL_SetEventBits(OSAL_EVENT_BITS_HATCH_CONFIG_RECV);
  }
  else
  {
     LOGE("error decoding shadow JSON document");
     LOGE("received %d bytes", buf_len);
     ESP_LOG_BUFFER_HEXDUMP(__func__, buf, buf_len, ESP_LOG_ERROR);
  }
}

/***** Global Functions *****/

uint32_t task_measure_config(TaskContext_t *TaskContext)
{
   bool                Result                = true;
   char               *peep_uuid             = (char *) _uuid_start;
   char               *root_ca               = (char *) _root_ca_start;
   char               *cert                  = (char *) _cert_start;
   char               *key                   = (char *) _key_start;
   uint32_t            DeepSleepTime;
   EventBits_t         BitsRecv              = 0;
   unsigned int        Index;
   const unsigned int  MaxLoopIterations     = 250;

   LOGI("start");

   /* Memset the global config structure.                               */
   memset(&_config, 0, sizeof(struct hatch_configuration));

   /* Attempt to read the config from NVM, not all fields updated via   */
   /* AWS.                                                              */
   memory_get_item(MEMORY_ITEM_HATCH_CONFIG, (uint8_t *) &_config, sizeof(struct hatch_configuration));

   if (IS_HATCH_CONFIG_VALID(_config))
   {
      LOGI("loaded previous hatch configuration");

      LOGI("Magic Word                    : 0x%X", _config.magic_word);
      LOGI("Measurements Interval (s)     : %u",   _config.measure_interval_sec);
      LOGI("Consecutive Low Readings      : %u",   _config.consecutive_low_readings);
      LOGI("Consecutive High Readings     : %u",   _config.consecutive_high_readings);
      LOGI("Measurements since Publishing : %u",   _config.measurements_since_last_published);
      LOGI("Measurements before Publishing: %u",   _config.measurements_before_publishing);
   }
   else
   {
     LOGE("failed to load previous hatch configuration");

     /* Load a default value for the config and store to nvm.  we do    */
     /* this since the AWS configuration does not set all of the        */
     /* required fields.   The fields that aren't current set will be   */
     /* set to their application defaults here and the fields set via   */
     /* AWS will be overwritten later.                                  */
     HATCH_CONFIG_INIT(_config);

     memory_set_item(MEMORY_ITEM_HATCH_CONFIG, (uint8_t *) &_config, sizeof(struct hatch_configuration));
   }

   /* Attempt to connect to wifi.                                       */
   LOGI("WiFi connect to SSID %s", TaskContext->SSID);
   Result = wifi_connect(TaskContext->SSID, TaskContext->Password, 60);
   if(Result)
   {
      LOGI("AWS MQTT shadow connect");
      Result = aws_mqtt_shadow_init(root_ca, cert, key, peep_uuid, 60);
      if (Result)
      {
         LOGI("AWS MQTT shadow get timeout %d seconds", _AWS_SHADOW_GET_TIMEOUT_SEC);
         Result = aws_mqtt_shadow_get(_shadow_callback, _AWS_SHADOW_GET_TIMEOUT_SEC);
         if(Result)
         {
            /* Wait for shadow to be fetched.                           */
            Index = 0;
            while(((BitsRecv & (OSAL_EVENT_BITS_HATCH_CONFIG_RECV | OSAL_EVENT_BITS_RESERVED)) == 0) && (Index++ < MaxLoopIterations))
            {
               /* call AWS polling function.                            */
               if(!aws_mqtt_shadow_poll(2500))
                  break;

               /* wait on the events to be received for 100 ms          */
               /* if nothing received by 100 ms later call the          */
               /* aws_mqtt_shadow_poll() function again.                */
               BitsRecv = OSAL_WaitEventBits(OSAL_EVENT_BITS_HATCH_CONFIG_RECV, (100 / portTICK_PERIOD_MS));
            }

            /* Clear the hatch config received bit.                     */
            OSAL_ClearEventBits(OSAL_EVENT_BITS_HATCH_CONFIG_RECV);

            LOGI("AWS MQTT shadow disconnect");
            aws_mqtt_shadow_disconnect();
         }
         else
         {
            LOGE("Failed to get shadow.");
         }
      }
      else
      {
         LOGE("Failed to init mqtt shadow.");
      }

      /* Disconnect from WiFi.                                          */
      LOGI("WiFi disconnect");
      wifi_disconnect();
   }
   else
   {
      LOGE("Failed to connect to wifi SSID = %s.", TaskContext->SSID);
   }

#if defined(PEEP_TEST_STATE_MEASURE_CONFIG)

   if (BitsRecv & OSAL_EVENT_BITS_RESERVED)
   {
      LOGI("push button");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
   }

   if (BitsRecv & OSAL_EVENT_BITS_HATCH_CONFIG_RECV)
   {
      LOGI("uuid=%s",                       _config.uuid);
      LOGI("end_unix_timestamp=%d",         _config.end_unix_timestamp);
      LOGI("measure_interval_sec=%d",       _config.measure_interval_sec);
      LOGI("temperature_offset_celsius=%d", _config.temperature_offset_celsius);
   }

   DeepSleepTime = 30;

#else

  if (BitsRecv & OSAL_EVENT_BITS_RESERVED)
  {
     LOGI("user pressed push button");
  }

  if (BitsRecv & OSAL_EVENT_BITS_HATCH_CONFIG_RECV)
  {
    LOGI("got AWS MQTT shadow");
    memory_set_item(MEMORY_ITEM_HATCH_CONFIG, (uint8_t *) &_config, sizeof(struct hatch_configuration));

    // feed watchdog
    vTaskDelay(100);

    //Set the state to measure
    peep_set_state(PEEP_STATE_MEASURE);

    TaskContext->CurrentState = PEEP_STATE_MEASURE;

    DeepSleepTime = 0;
  }
  else
  {
    DeepSleepTime = 60;
  }

#endif

  return(DeepSleepTime);
}
