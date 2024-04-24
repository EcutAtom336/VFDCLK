#include "my_event_group.h"
//
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define TAG "simple event group"

EventGroupHandle_t my_event_group_handle_table[kEventGroupManagerMax / 32 + 1] =
    {};

void event_group_manager_init() {
  uint32_t event_group_amount = kEventGroupManagerMax / 32;
  if (kEventGroupManagerMax % 32) event_group_amount++;
  for (size_t i = 0; i < event_group_amount; i++) {
    my_event_group_handle_table[i] = xEventGroupCreate();
  }
}
EventGroupHandle_t event_group_manager_get_handle(
    EventGroupManagerEventNum_t event_num) {
  return my_event_group_handle_table[event_num / 32];
}
EventGroupManagerState_t event_group_manager_check_bits(
    EventGroupManagerEventNum_t event_num) {
  return xEventGroupGetBits(my_event_group_handle_table[event_num / 32]) &
                 (uint32_t)(1 << (event_num % 32))
             ? kMyEventGroupSet
             : kMyEventGroupReset;
}
void event_group_manager_set_bits(EventGroupManagerEventNum_t event_num,
                                  EventGroupManagerState_t new_state) {
  if (new_state == kMyEventGroupSet)
    xEventGroupSetBits(my_event_group_handle_table[event_num / 32],
                       (uint32_t)(1 << (event_num % 32)));
  else
    xEventGroupClearBits(my_event_group_handle_table[event_num / 32],
                         (uint32_t)(1 << (event_num % 32)));
}
