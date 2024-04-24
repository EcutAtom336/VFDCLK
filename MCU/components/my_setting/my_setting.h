#ifndef _MY_SETTING_H_
#define _MY_SETTING_H_

#include "cJSON.h"
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(MY_SETTING_EVENT);

typedef enum {
  kMySettingEventStart,
  kMySettingEventStop,
  kMySettingEventRecvConfig,
  kMySettingEventMax,
} MySettingEvent_t;

// extern esp_event_loop_handle_t my_setting_event_loop_handle;

void my_setting_init();

#endif
