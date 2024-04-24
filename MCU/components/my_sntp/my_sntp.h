#ifndef _MY_SNTP_H_
#define _MY_SNTP_H_

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(MY_SNTP_EVENT);

typedef enum {
  kMySntpSyncSuccess,
  kMySntpSyncFail,
} MySntpEvent_t;

void my_sntp_init();

#endif
