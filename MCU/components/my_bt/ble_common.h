#ifndef _BLE_COMMON_H_
#define _BLE_COMMON_H_
//
#include <stdint.h>
#include <stdbool.h>

#define LOCAL_MTU 512

extern uint16_t cur_mtu;

typedef struct {
  uint16_t conn_id;
  bool is_connect;
  // unused
  uint16_t mtu;
} ConnectInfo_t;

typedef enum {
  kBleAppIdUt,

  kBleAppIdMax,
} BleAppId_t;

extern ConnectInfo_t connect_info;

extern uint16_t primary_uuid;
extern uint16_t secondary_uuid;
extern uint16_t include_uuid;
extern uint16_t characteristic_decalaration_uuid;
extern uint16_t character_client_config_uuid;
/**/
extern uint8_t char_prop_r;
extern uint8_t char_prop_w;
extern uint8_t char_prop_r_w;
extern uint8_t char_prop_i;
extern uint8_t char_prop_n;

#endif