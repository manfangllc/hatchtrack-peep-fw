/***** Includes *****/

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "nvs_flash.h"

#include "hal.h"
#include "memory.h"
#include "memory_measurement_db.h"
#include "state.h"
#include "system.h"
#include "tasks.h"

#if (defined(PEEP_TEST_STATE_MEASURE) || (PEEP_TEST_STATE_MEASURE_CONFIG))
   // SSID of the WiFi AP connect to.
   #define _TEST_WIFI_SSID     "Cisco38992"

   // Password of the WiFi AP to connect to.
   #define _TEST_WIFI_PASSWORD "@GuestsMonitored@1776"
#endif

/***** Global Data *****/
static TaskContext_t TaskContext;

/***** Local Functions *****/

   /* Utility function to return WiFi SSID/Password that is stored in the */
   /* NVM.                                                                */
static bool get_wifi_ssid_pasword(char *ssid, char *password)
{
  bool    RetVal;
  int32_t len;

  len = memory_get_item(MEMORY_ITEM_WIFI_SSID, (uint8_t *) ssid, WIFI_SSID_LEN_MAX);
  if (len > 0)
  {
     LOGI("SSID = %s", ssid);

     len = memory_get_item(MEMORY_ITEM_WIFI_PASS, (uint8_t *)password, WIFI_PASSWORD_LEN_MAX);
     if (len <= 0)
     {
        LOGI("PASS = %s", password);

        RetVal = true;
     }
     else
     {
        RetVal = false;
     }
  }
  else
  {
     RetVal = false;
  }

  return RetVal;
}

/***** Global Functions *****/

void app_main(void)
{

   bool          r     = true;
   int32_t       len   = 0;
   peep_state_t  state = PEEP_STATE_UNKNOWN;
   uint32_t      DeepSleepTime;

#if (defined (PEEP_TEST_STATE_DEEP_SLEEP) || defined(PEEP_TEST_STATE_MEASURE) || defined(PEEP_TEST_STATE_MEASURE_CONFIG) || defined (PEEP_TEST_STATE_BLE_CONFIG))
   {
      uint32_t n = 10;
      do
      {
         LOGI("Starting in %d...", n--);
         vTaskDelay(1000 / portTICK_PERIOD_MS);
      } while (n);
   }
#endif

   nvs_flash_init();
   r = memory_init();
   RESULT_TEST_ERROR_TRACE(r);

   r = memory_measurement_db_init();
   RESULT_TEST_ERROR_TRACE(r);

#if defined (PEEP_TEST_STATE_DEEP_SLEEP)

   if (hal_deep_sleep_is_wakeup_push_button())
   {
      LOGI("wakeup cause push button");
   }
   else if (hal_deep_sleep_is_wakeup_timer())
   {
      LOGI("wakeup cause timer");
   }
   else {
      LOGI("unknown wakeup cause");
   }
   hal_deep_sleep_timer_and_push_button(60);
   // will not return from above

#elif defined(PEEP_TEST_STATE_MEASURE)

   (void) len;
   state = PEEP_STATE_MEASURE;

#elif defined(PEEP_TEST_STATE_MEASURE_CONFIG)

   (void) len;
   state = PEEP_STATE_MEASURE_CONFIG;

#elif defined (PEEP_TEST_STATE_BLE_CONFIG)

   (void) len;
   state = PEEP_STATE_BLE_CONFIG;

#else

   /* Get the current application state.                                */
   if (!peep_get_state(&state))
   {
     LOGI("invalid peep state, set PEEP_STATE_BLE_CONFIG");

     peep_set_state(state);
     state = PEEP_STATE_BLE_CONFIG;
   }

#endif

   LOGI("Peep State %u\n\n", state);

   /* Initialize the task context.                                      */
   memset(&TaskContext,  0,  sizeof(TaskContext));

   TaskContext.EventGroup   = xEventGroupCreate();
   TaskContext.CurrentState = state;

   /* If we are in anything other than the BLE config state we should   */
   /* already have the SSID/password.                                   */
   if(TaskContext.CurrentState != PEEP_STATE_BLE_CONFIG)
   {
      /* Get the WiFi SSID and password.                                */
      if(!get_wifi_ssid_pasword(TaskContext.SSID, TaskContext.Password))
      {
#if (defined(PEEP_TEST_STATE_MEASURE) || (PEEP_TEST_STATE_MEASURE_CONFIG))

         /* No SSID/Password stored in NVM.   Store it now.             */
         strcpy(TaskContext.SSID,     _TEST_WIFI_SSID);
         strcpy(TaskContext.Password, _TEST_WIFI_PASSWORD);

         LOGI("Test State SSID=%s, PASS=%s", TaskContext.SSID, TaskContext.Password);

         LOGI("saving WiFI SSID %s", TaskContext.SSID);
         memory_set_item(MEMORY_ITEM_WIFI_SSID, (uint8_t *)TaskContext.SSID, WIFI_SSID_LEN_MAX);

         LOGI("saving WiFI password %s", TaskContext.Password);
         memory_set_item(MEMORY_ITEM_WIFI_PASS, (uint8_t *)TaskContext.Password, WIFI_PASSWORD_LEN_MAX);

#else

         /* Error occurred, move to BLE Config state to get WiFi and    */
         /* SSID.                                                       */
         peep_set_state(PEEP_STATE_BLE_CONFIG);

         TaskContext.CurrentState = PEEP_STATE_BLE_CONFIG;

#endif
      }
   }

   if(TaskContext.EventGroup != NULL)
   {
      /* Loop forever in the main application.                          */
      while(1)
      {
         /* Initialize the deep sleep time.                             */
         DeepSleepTime = 0;

         /* Handle current state.                                       */
         switch(TaskContext.CurrentState)
         {
            case PEEP_STATE_BLE_CONFIG:
               /* Use BLE to get the WiFi SSID/Password information     */
               DeepSleepTime = task_ble_config_wifi_credentials(&TaskContext);
               break;
            case PEEP_STATE_MEASURE:
               /* Enter the measurement state.                          */
               DeepSleepTime = task_measure(&TaskContext);
               break;
            case PEEP_STATE_MEASURE_CONFIG:
               /* Wait for updated measurement data from cloud.         */
               DeepSleepTime = task_measure_config(&TaskContext);
               break;
            default:
               LOGE("Invalid Peep State %u\n\n", TaskContext.CurrentState);

               DeepSleepTime = 5;
               break;
         }

         /* If requested, enter deep sleep.   Note deep sleep is only   */
         /* exited by a system reboot.                                  */
         if(DeepSleepTime > 0)
         {
            LOGI("Entering deep sleep for %u or until button press....", DeepSleepTime);

            /* Enter deep sleep, function never returns.                */
            hal_deep_sleep_timer_and_push_button(DeepSleepTime);
         }
         else
         {
            /* small delay in cases where we are not entering deep sleep*/
            /* between states.                                          */
            vTaskDelay(100);
         }
      }
   }
   else
   {
      LOGE("failed to allocate event group");
      hal_deep_sleep_timer_and_push_button(1);
   }
}
