#ifndef _MY_EVENT_GROUP_H_
#define _MY_EVENT_GROUP_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

typedef enum {
  kMyEventGroupReset,
  kMyEventGroupSet,
} EventGroupManagerState_t;

typedef enum {
  kEventGroupManagerTimeIsSync,
  kEventGroupManagerScreenIsOn,
  kEventGroupManagerBuzzerIsBusy,
  kEventGroupManagerMax,
} EventGroupManagerEventNum_t;

#define EVENT_GROUP1_TIME_IS_SYNC                                              \
  (uint32_t)(1 << (kEventGroupManagerTimeIsSync % 32))
#define EVENT_GROUP1_SCREEN_IS_ON                                              \
  (uint32_t)(1 << (kEventGroupManagerScreenIsOn % 32))
#define EVENT_GROUP1_BUZZER_IS_BUSY                                            \
  (uint32_t)(1 << (kEventGroupManagerBuzzerIsBusy % 32))
#define simple_event_group_gen_event_bit(event_num)                            \
  (uint32_t)(1 << (event_num % 32))

void event_group_manager_init();

EventGroupHandle_t
event_group_manager_get_handle(EventGroupManagerEventNum_t event_num);

EventGroupManagerState_t
event_group_manager_check_bits(EventGroupManagerEventNum_t event_num);

void event_group_manager_set_bits(EventGroupManagerEventNum_t event_num,
                                  EventGroupManagerState_t new_state);

#endif
