#ifndef _MY_BT_H_
#define _MY_BT_H_

#include "esp_event.h"
#include "esp_gatts_api.h"
//
#include <stdint.h>

ESP_EVENT_DECLARE_BASE(MY_BT_EVENT);

typedef enum {
  kMyBtEventConnect,
  kMyBtEventDisconnect,
} MyBtEvent_t;

extern esp_event_loop_handle_t my_bt_event_loop_handle;

void bt_base_init_with_random_address(void *random_addr);

void bt_gatts_init();

void my_bt_start_adv_with_manufacturer_data(void *manufacturer_data,
                                            uint8_t len);

void my_bt_register_gatt_app_cb(esp_gatts_cb_t cb);

#endif
