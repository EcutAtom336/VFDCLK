#ifndef _BLE_UT_H_
#define _BLE_UT_H_

#include "esp_event.h"
#include "esp_gatts_api.h"
//
#include <stdint.h>

typedef struct {
  void *p_data;
  uint32_t size;
} BleUtPkgInfo_t;

ESP_EVENT_DECLARE_BASE(BLE_UT_EVENT);

typedef enum {
  kBleUtEventToClientEnable,
  kBleUtEventToClientDisable,
} BleUtEvent_t;

typedef void (*BleUtToServerHandler_t)(BleUtPkgInfo_t *pkg);

extern esp_event_loop_handle_t ble_ut_event_loop_handle;

void ble_ut_init();

void ble_ut_gatts_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                          esp_ble_gatts_cb_param_t *param);

void ble_ut_send_to_client(void *p_data, uint32_t size);

void ble_ut_set_to_server_handler(BleUtToServerHandler_t cb);

#endif