#ifndef _OSAL_H_
#define _OSAL_H_

#include "system.h"

   /* The following define the event bits that may be used with the     */
   /* OSAL_WaitEventBits() function.                                    */
#define OSAL_EVENT_BITS_BLE_SYNC           (BIT0)
#define OSAL_EVENT_BITS_HATCH_CONFIG_RECV  (BIT1)
#define OSAL_EVENT_BITS_WIFI_CONNECTED     (BIT2)
#define OSAL_EVENT_BITS_WIFI_DISCONNECTED  (BIT3)
#define OSAL_EVENT_BITS_BUTTON_NEGEDGE     (BIT4)
#define OSAL_EVENT_BITS_BUTTON_POSEDGE     (BIT5)

#define OSAL_EVENT_BITS_RESERVED           (BIT7)

bool OSAL_Init(void);
void OSAL_Deinit(void);

EventBits_t OSAL_WaitEventBits(EventBits_t Bits2WaitFor, TickType_t MaxWaitTimeMs);
EventBits_t OSAL_WaitEventBitsNoReserved(EventBits_t Bits2WaitFor, bool ClearOnExit, TickType_t MaxWaitTimeMs);
void OSAL_SetEventBits(EventBits_t Bits2Set);
void OSAL_SetEventBitsISR(EventBits_t Bits2Set);
void OSAL_ClearEventBits(EventBits_t Events2Clear);
EventBits_t OSAL_GetEventBits(void);

#endif
