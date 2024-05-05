#include "ble_common.h"
//
#include "esp_gatt_defs.h"
#include <stdint.h>

uint16_t cur_mtu = 23;

ConnectInfo_t connect_info;

uint16_t primary_uuid = ESP_GATT_UUID_PRI_SERVICE;
uint16_t secondary_uuid = ESP_GATT_UUID_SEC_SERVICE;
uint16_t include_uuid = ESP_GATT_UUID_INCLUDE_SERVICE;
uint16_t characteristic_decalaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
//
uint8_t char_prop_r = ESP_GATT_CHAR_PROP_BIT_READ;
uint8_t char_prop_w = ESP_GATT_CHAR_PROP_BIT_WRITE;
uint8_t char_prop_r_w =
    ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
uint8_t char_prop_i = ESP_GATT_CHAR_PROP_BIT_INDICATE;
uint8_t char_prop_n = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
