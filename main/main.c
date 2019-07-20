/***** Includes *****/

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>

#include "nvs_flash.h"

#include "hal.h"
#include "memory.h"
#include "memory_measurement_db.h"
#include "state.h"
#include "system.h"
#include "tasks.h"
#include "OSAL.h"

#if (defined(PEEP_TEST_STATE_MEASURE) || (PEEP_TEST_STATE_MEASURE_CONFIG))
   // SSID of the WiFi AP connect to.
   #define _TEST_WIFI_SSID     "Cisco38992"

   // Password of the WiFi AP to connect to.
   #define _TEST_WIFI_PASSWORD "@GuestsMonitored@1776"
#endif

#define __USE_BUTTON_INTERRUPT__                0
#define ONE_SECOND_IN_MICROSECONDS              (1 * 1000 * 1000)
#define BUTTON_PRESS_TIME_FOR_RESET_US          (9500000)
#define BUTTON_PRESS_TIME_FOR_RESET_MS          (9500)
#define BUTTON_PRESS_DEBOUNCE_TIME_MS           (50)

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

   /* The following function calculates time duration handling possible */
   /* timer wrap.                                                       */
static int64_t CalculateTimeDuration(int64_t CurrentTime, int64_t PreviousTime)
{
   int64_t Duration;

   if(CurrentTime >= PreviousTime)
   {
      Duration = CurrentTime - PreviousTime;
   }
   else
   {
      Duration = (INT64_MAX - PreviousTime) + CurrentTime;
   }

   return(Duration);
}

   /* Callback to handle push button state changes.                     */
static void push_button_isr(bool is_pressed)
{
#if __USE_BUTTON_INTERRUPT__

   if(is_pressed)
   {
      /* Signal the button press positive edge.                         */
      OSAL_SetEventBitsISR(OSAL_EVENT_BITS_BUTTON_POSEDGE);
   }
   else
   {
      /* Signal the button press negative edge.                         */
      OSAL_SetEventBitsISR(OSAL_EVENT_BITS_BUTTON_NEGEDGE);
   }

#endif
}

   /* This function is called for a task reset.   It resets the state   */
   /* to BLE Config and clears the WiFi SSID/Password info.             */
static void ResetTaskState(void)
{
   LOGI("Resetting task state...");

   /* Go to BLE Config state.                                           */
   peep_set_state(PEEP_STATE_BLE_CONFIG);
   TaskContext.CurrentState = PEEP_STATE_BLE_CONFIG;

   /* Reset WiFi SSID and password.                                     */
   memset(TaskContext.SSID,      0,  WIFI_SSID_LEN_MAX);
   memset(TaskContext.Password,  0,  WIFI_PASSWORD_LEN_MAX);

   memory_set_item(MEMORY_ITEM_WIFI_SSID, (uint8_t *)TaskContext.SSID,     WIFI_SSID_LEN_MAX);
   memory_set_item(MEMORY_ITEM_WIFI_PASS, (uint8_t *)TaskContext.Password, WIFI_PASSWORD_LEN_MAX);
}

/***** Global Functions *****/

static void PushButtonTask(void* arg)
{
#if __USE_BUTTON_INTERRUPT__
   EventBits_t EventBits;
#endif

   int64_t     CurrentTime;
   int64_t     ButtonPressTime;
   int64_t     Duration;
   uint8_t     FirstLoop;

#if __USE_BUTTON_INTERRUPT__

   // Install gpio isr service. We do this here because the subsystems register
   // their own ISRs for the pins they use for user input.
   gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

#endif

   /* Initialize the push button.                                       */
   hal_init_push_button(push_button_isr);

   /* Check to see if the push button caused us to wakeup.   if so check*/
   /* to see if the push button is still pressed.                       */
   if((hal_deep_sleep_is_wakeup_push_button()) && (hal_push_button_pressed()))
   {
      /* Only in the case of waking up from deep sleep via push button  */
      /* will we bypass logic that waits on the posedge from the ISR    */
      /* before tracking the push button state with debounce.           */
      OSAL_SetEventBits(OSAL_EVENT_BITS_BUTTON_POSEDGE);
   }

   LOGI("Push Button Thread Start");

   /* Loop forever.                                                     */
   while(1)
   {

#if __USE_BUTTON_INTERRUPT__
      EventBits = OSAL_WaitEventBitsNoReserved(OSAL_EVENT_BITS_BUTTON_POSEDGE, true, portMAX_DELAY);

      if(EventBits & OSAL_EVENT_BITS_BUTTON_POSEDGE)
#else

      if(!hal_push_button_pressed())
      {
         vTaskDelay(BUTTON_PRESS_DEBOUNCE_TIME_MS/portTICK_PERIOD_MS);
         continue;
      }

      if(1)
#endif
      {

         /* Get current time.                                              */
         CurrentTime = esp_timer_get_time();

         LOGI("Button Press at %lld", CurrentTime);

         ButtonPressTime = esp_timer_get_time();
         Duration        = 0;
         FirstLoop       = 1;
         while(Duration < BUTTON_PRESS_TIME_FOR_RESET_US)
         {
            vTaskDelay(BUTTON_PRESS_DEBOUNCE_TIME_MS/portTICK_PERIOD_MS);

            if(hal_push_button_pressed())
            {
               if(FirstLoop == 1)
               {
                  ButtonPressTime = esp_timer_get_time();
                  FirstLoop       = 0;
               }

               CurrentTime = esp_timer_get_time();
               Duration    = CalculateTimeDuration(CurrentTime, ButtonPressTime);

               LOGI("Button Press at %lld, Pressed for %lld ms", CurrentTime, Duration / 1000);
            }
            else
            {
               CurrentTime = esp_timer_get_time();
               Duration    = CalculateTimeDuration(CurrentTime, ButtonPressTime);

               LOGI("Button Press at %lld, Pressed for %lld ms", CurrentTime, Duration / 1000);
               break;
            }
         }

         /* Signal that the reserved state has occurred.             */
         if(Duration >= BUTTON_PRESS_TIME_FOR_RESET_US)
         {
            CurrentTime = esp_timer_get_time();

            LOGI("Resetting application state at %lld", CurrentTime);

            OSAL_SetEventBits(OSAL_EVENT_BITS_RESERVED);
         }

#if (__USE_BUTTON_INTERRUPT__ == 0)

         else
         {
            OSAL_SetEventBits(OSAL_EVENT_BITS_BUTTON_NEGEDGE);
         }

#endif
      }
   }
}

static void MainTask(void* arg)
{
   int64_t       Time1;
   int64_t       Time2;
   int64_t       Duration;
   uint32_t      DeepSleepTime;
   EventBits_t   EventMask;

   LOGI("Main Task Start Start");

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

   /* Loop forever in the main application.                             */
   while(1)
   {
      /* Check to see if the button press event for reset event         */
      /* occurred.                                                      */
      EventMask = OSAL_GetEventBits();
      if(!(EventMask & OSAL_EVENT_BITS_RESERVED))
      {
         /* Handle current state.                                       */
         switch(TaskContext.CurrentState)
         {
            case PEEP_STATE_BLE_CONFIG:
               /* Use BLE to get the WiFi SSID/Password information.    */
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

         /* Check to see if we exited due to button press.              */
         EventMask = OSAL_GetEventBits();
         if(!(EventMask & OSAL_EVENT_BITS_RESERVED))
         {
            /* If requested, enter deep sleep.   Note deep sleep is only*/
            /* exited by a system reboot.                               */
            if(DeepSleepTime > 0)
            {
               /* If button is currently not pressed enter sleep mode,  */
               /* otherwise wait for button to be released before doing */
               /* so.                                                   */
               if(!hal_push_button_pressed())
               {
                  LOGI("Entering deep sleep for %u or until button press....", DeepSleepTime);

                  /* Enter deep sleep, function never returns.          */
                  hal_deep_sleep_timer_and_push_button(DeepSleepTime);
               }
               else
               {
                  LOGI("Button pressed, wait for button to not be pressed....");

                  /* Get current time before calling wait event bits.   */
                  Time1 = esp_timer_get_time();

                  /* Wait for the button negative edge (button no longer*/
                  /* pressed.                                           */
                  EventMask = OSAL_WaitEventBits(OSAL_EVENT_BITS_BUTTON_NEGEDGE, (DeepSleepTime * 1000));

                  /* Get time after calling WaitEventBits.              */
                  Time2 = esp_timer_get_time();

                  /* Clear the events that are set.                     */
                  OSAL_ClearEventBits(EventMask);

                  LOGI("Button pressed waiting, Time2 %lld Time1 %lld EventMask %u", Time2, Time1, (unsigned int)EventMask);

                  /* Check to see what woke us up.                      */
                  if(EventMask & OSAL_EVENT_BITS_RESERVED)
                  {
                     /* Reset task state.                               */
                     ResetTaskState();
                  }
                  else
                  {
                     /* if we did not receive the button negative edge  */
                     /* event then we timed out waiting for the button  */
                     /* to be released so we can now just run the       */
                     /* application state.                              */
                     if(EventMask & OSAL_EVENT_BITS_BUTTON_NEGEDGE)
                     {
                        /* Button was released but was not pressed long */
                        /* enough to trigger a task state reset.   So we*/
                        /* will adjust our deep sleep time by however   */
                        /* long we waited for the button to be released.*/
                        Duration = CalculateTimeDuration(Time2, Time1);

                        LOGI("Button press release, OSAL_WaitEventBits() Duration %lld, Deep Sleep Time %u", Duration, DeepSleepTime);

                        /* Did we wait on event for longer than 1       */
                        /* second?                                      */
                        if(Duration >= ONE_SECOND_IN_MICROSECONDS)
                        {
                           /* Convert duration to seconds from          */
                           /* microseconds.                             */
                           Duration /= ONE_SECOND_IN_MICROSECONDS;

                           /* Check to see that deep sleep is greater   */
                           /* than the time we just spent in            */
                           /* OSAL_WaitEventBits.                       */
                           if(DeepSleepTime > Duration)
                           {
                              /* Adjust deep sleep time.                */
                              DeepSleepTime -= Duration;

                              /* Enter deep sleep, function never       */
                              /* returns.                               */
                              hal_deep_sleep_timer_and_push_button(DeepSleepTime);
                           }
                           else
                           {
                              LOGI("Deep Sleep time exceeded even after button press: %u", DeepSleepTime);
                           }
                        }
                        else
                        {
                           /* we waited on the event for less than a    */
                           /* second so just enter deep sleep for the   */
                           /* full requested time.                      */
                           hal_deep_sleep_timer_and_push_button(DeepSleepTime);
                        }
                     }
                     else
                     {
                        LOGI("Deep Sleep time exceeded waiting on event: %u", DeepSleepTime);
                     }
                  }
               }
            }
            else
            {
               LOGI("No deep sleep time, entering vTaskDelay 100 ticks");

               /* small delay in cases where we are not entering deep   */
               /* sleep between states.                                 */
               vTaskDelay(100);
            }
         }
         else
         {
            /* Clear the button press event.                            */
            OSAL_ClearEventBits(EventMask);

            /* Reset task state.                                        */
            ResetTaskState();
         }
      }
      else
      {
         /* Clear the button press event.                               */
         OSAL_ClearEventBits(EventMask);

         /* Reset task state.                                           */
         ResetTaskState();
      }
   }
}

void app_main(void)
{
   bool          r     = true;
   peep_state_t  state = PEEP_STATE_UNKNOWN;

#if (defined (PEEP_TEST_STATE_DEEP_SLEEP) || defined(PEEP_TEST_STATE_MEASURE) || defined(PEEP_TEST_STATE_MEASURE_CONFIG) || defined (PEEP_TEST_STATE_BLE_CONFIG))
   //xxx {
   //xxx    uint32_t n = 10;
   //xxx    do
   //xxx    {
   //xxx       LOGI("Starting in %d...", n--);
   //xxx       vTaskDelay(1000 / portTICK_PERIOD_MS);
   //xxx    } while (n);
   //xxx }
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
   else
   {
      LOGI("unknown wakeup cause");
   }
   hal_deep_sleep_timer_and_push_button(60);
   // will not return from above

#elif defined(PEEP_TEST_STATE_MEASURE)

   state = PEEP_STATE_MEASURE;

#elif defined(PEEP_TEST_STATE_MEASURE_CONFIG)

   state = PEEP_STATE_MEASURE_CONFIG;

#elif defined (PEEP_TEST_STATE_BLE_CONFIG)

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

   /* Initialize abstaction layer.                                      */
   if(!OSAL_Init())
   {
      LOGE("failed to initialize abstraction layer");
      hal_deep_sleep_timer_and_push_button(1);
   }

   /* Initialize the task context.                                      */
   memset(&TaskContext,  0,  sizeof(TaskContext));

   TaskContext.CurrentState = state;

   /* Start the push button task.                                       */
   xTaskCreate(PushButtonTask, "PushButton", 4096, NULL, 10, NULL);

   /* Start the main task.                                              */
   xTaskCreate(MainTask, "MainTask", 10920, NULL, 10, NULL);
}
