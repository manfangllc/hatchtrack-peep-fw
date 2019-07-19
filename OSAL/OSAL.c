/***** Includes *****/

#include <stdint.h>
#include <stdbool.h>

#include "OSAL.h"
#include "system.h"

EventGroupHandle_t EventGroup;

bool OSAL_Init(void)
{
   bool RetVal = true;

   EventGroup = xEventGroupCreate();
   if(EventGroup == NULL)
   {
      RetVal = false;
   }

   return(RetVal);
}

void OSAL_Deinit(void)
{
   if(EventGroup != NULL)
   {
      vEventGroupDelete(EventGroup);

      EventGroup = NULL;
   }
}

EventBits_t OSAL_WaitEventBits(EventBits_t Bits2WaitFor, TickType_t MaxWaitTimeMs)
{
   EventBits_t RetVal = 0;

   if(EventGroup != NULL)
   {
      /* Always wait on the reserved bit.   Don't clear set bits on exit*/
      /* as the caller can do this if requested.                        */
      if(MaxWaitTimeMs != portMAX_DELAY)
      {
         RetVal = xEventGroupWaitBits(EventGroup, (Bits2WaitFor | OSAL_EVENT_BITS_RESERVED), false, false, (MaxWaitTimeMs / portTICK_PERIOD_MS));
      }
      else
      {
         RetVal = xEventGroupWaitBits(EventGroup, (Bits2WaitFor | OSAL_EVENT_BITS_RESERVED), false, false, portMAX_DELAY);
      }
   }

   return(RetVal);
}

void OSAL_SetEventBits(EventBits_t Bits2Set)
{
   if(EventGroup != NULL)
   {
      xEventGroupSetBits(EventGroup, Bits2Set);
   }
}

void OSAL_SetEventBitsISR(EventBits_t Bits2Set)
{
   BaseType_t xHigherPriorityTaskWoken, xResult;

   if(EventGroup != NULL)
   {
      xHigherPriorityTaskWoken = pdFALSE;

      /* Set events from ISR context.                                   */
      xResult = xEventGroupSetBitsFromISR(EventGroup, Bits2Set, &xHigherPriorityTaskWoken );
      if( xResult == pdPASS )
      {
         if(xHigherPriorityTaskWoken == pdTRUE)
         {
             portYIELD_FROM_ISR();
         }
      }
   }
}

void OSAL_ClearEventBits(EventBits_t Events2Clear)
{
   if(EventGroup != NULL)
   {
      xEventGroupClearBits(EventGroup, Events2Clear);
   }
}
