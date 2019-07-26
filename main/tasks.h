#ifndef _TASKS_H_
#define _TASKS_H_

#include "wifi.h"
#include "state.h"
#include "system.h"

   /* All tasks must wait for this button and must not re-define this   */
   /* bit.                                                              */
#define EVENT_GROUP_BIT_BUTTON_PRESS    (BIT7)

typedef struct
{
   EventGroupHandle_t  EventGroup;
   peep_state_t        CurrentState;
   char                SSID[WIFI_SSID_LEN_MAX];
   char                Password[WIFI_PASSWORD_LEN_MAX];
   char               *peep_uuid;
   char               *root_ca;
   char               *cert;
   char               *key;
} TaskContext_t;

extern uint32_t task_ble_config_wifi_credentials(TaskContext_t *TaskContext);
extern uint32_t task_measure(TaskContext_t *TaskContext);
extern uint32_t task_measure_config(TaskContext_t *TaskContext);

#endif
