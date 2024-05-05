#include "my_bt.h"
//
#include "ble_common.h"
//
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_event.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatt_defs.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
//
#include <stdbool.h>
#include <string.h>

#define TAG "my bt"
#define MY_BT_DEVICE_NAME "vfdclk"

ESP_EVENT_DEFINE_BASE(MY_BT_EVENT);

esp_event_loop_handle_t my_bt_event_loop_handle;

esp_gatts_cb_t gatt_app_cb;

static void my_gap_event_handler(esp_gap_ble_cb_event_t event,
                                 esp_ble_gap_cb_param_t *param) {
  switch (event) {
  case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
    if (param->adv_data_cmpl.status == ESP_BT_STATUS_SUCCESS) {
      ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT success");
    } else {
      ESP_LOGE(TAG, "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT fail, code: %d",
               param->adv_data_cmpl.status);
    }
    break;
  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
      ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_START_COMPLETE_EVT success");
    } else {
      ESP_LOGE(TAG, "ESP_GAP_BLE_ADV_START_COMPLETE_EVT fail, code: %d",
               param->adv_start_cmpl.status);
    }
    break;
  case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
    if (param->update_conn_params.status == ESP_BT_STATUS_SUCCESS) {
      ESP_LOGI(TAG, "ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT success");
    } else {
      ESP_LOGE(TAG, "ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT fail, code: %d",
               param->update_conn_params.status);
    }
    break;
  default:
    ESP_LOGI(TAG, "gap event handler not implememnt, event: %u", event);
    break;
  }
}

static void my_gatts_event_handler(esp_gatts_cb_event_t event,
                                   esp_gatt_if_t gatts_if,
                                   esp_ble_gatts_cb_param_t *param) {
  ESP_LOGI(TAG, "gatt event, gatt if: %u", gatts_if);
  switch (event) {
  case ESP_GATTS_READ_EVT:
    ESP_LOGI(TAG, "ESP_GATTS_READ_EVT success");
    break;
  case ESP_GATTS_WRITE_EVT:
    ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT success");
    break;
  case ESP_GATTS_CONF_EVT:
    if (param->conf.status == ESP_GATT_OK) {
      ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT success");
    } else {
      ESP_LOGE(TAG, "ESP_GATTS_CONF_EVT fail, code: %d", param->conf.status);
    }
    break;
  case ESP_GATTS_CONNECT_EVT:
    ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT success");
    ESP_LOGI(TAG, "conn id: %u", param->connect.conn_id);
    connect_info.conn_id = param->connect.conn_id;
    connect_info.is_connect = true;

    ESP_ERROR_CHECK(esp_event_post_to(my_bt_event_loop_handle, MY_BT_EVENT,
                                      kMyBtEventConnect, NULL, 0,
                                      portMAX_DELAY));

    break;
  case ESP_GATTS_DISCONNECT_EVT:
    ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason:%d",
             param->disconnect.reason);
    connect_info.is_connect = false;

    ESP_ERROR_CHECK(esp_event_post_to(my_bt_event_loop_handle, MY_BT_EVENT,
                                      kMyBtEventDisconnect, NULL, 0,
                                      portMAX_DELAY));

    // esp_ble_adv_params_t ble_adv_params = {
    //     .adv_int_min = 0x0800,
    //     .adv_int_max = 0x0800,
    //     .adv_type = ADV_TYPE_IND,
    //     .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    //     .channel_map = ADV_CHNL_ALL,
    //     .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    // };
    // esp_ble_gap_start_advertising(&ble_adv_params);
    // 断开连接，cccd会清零吗？
    break;
  case ESP_GATTS_REG_EVT:
    if (param->reg.status == ESP_GATT_OK) {
      ESP_LOGI(TAG, "ESP_GATTS_REG_EVT success");
      ESP_LOGI(TAG, "gatts if: %d", gatts_if);
    } else {
      ESP_LOGE(TAG, "ESP_GATTS_REG_EVT fail, code: %d", param->reg.status);
    }
    break;
  case ESP_GATTS_CREATE_EVT:
    if (param->create.status == ESP_GATT_OK) {
      ESP_LOGI(TAG, "ESP_GATTS_CREATE_EVT success");
    } else {
      ESP_LOGE(TAG, "ESP_GATTS_CREATE_EVT fail, code: %d",
               param->create.status);
    }
    break;
  case ESP_GATTS_START_EVT:
    if (param->start.status == ESP_GATT_OK) {
      ESP_LOGI(TAG, "ESP_GATTS_START_EVT success");
    } else {
      ESP_LOGE(TAG, "ESP_GATTS_START_EVT fail, code: %d", param->start.status);
    }
    break;
  case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    if (param->add_attr_tab.status == ESP_GATT_OK) {
      ESP_LOGI(TAG, "ESP_GATTS_CREAT_ATTR_TAB_EVT success");
    } else {
      ESP_LOGE(TAG, "ESP_GATTS_CREAT_ATTR_TAB_EVT fail, code: %d",
               param->add_attr_tab.status);
    }
    break;
  case ESP_GATTS_MTU_EVT:
    ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT success");
    cur_mtu = param->mtu.mtu;
    break;
  default:
    ESP_LOGI(TAG, "%s not implememnt, event: %u", __func__, event);
    break;
  }
  if (gatt_app_cb != NULL)
    gatt_app_cb(event, gatts_if, param);
}

void my_bt_register_gatt_app_cb(esp_gatts_cb_t cb) {
  if (cb != NULL)
    gatt_app_cb = cb;
}

void my_bt_unregister_gatt_app_cb() { gatt_app_cb = NULL; }

void bt_gatts_init() {
  // register gap event handler
  ESP_ERROR_CHECK(esp_ble_gap_register_callback(my_gap_event_handler));
  // register gatt event handler
  ESP_ERROR_CHECK(esp_ble_gatts_register_callback(my_gatts_event_handler));
  // set device name
  ESP_ERROR_CHECK(esp_bt_dev_set_device_name(MY_BT_DEVICE_NAME));
}

void bt_base_init_with_random_address(void *random_addr) {
  esp_event_loop_args_t event_loop_args = {
      .queue_size = 8,
      .task_stack_size = configMINIMAL_STACK_SIZE,
      .task_core_id = 0,
      .task_name = "my bt event loop",
      .task_priority = 12,
  };
  ESP_ERROR_CHECK(
      esp_event_loop_create(&event_loop_args, &my_bt_event_loop_handle));
  // bt controller init
  esp_bt_controller_config_t bt_ctrler_cfg =
      BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_bt_controller_init(&bt_ctrler_cfg));
  ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
  // bluedroid init
  esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
  ESP_ERROR_CHECK(esp_bluedroid_enable());
  // set mtu
  ESP_ERROR_CHECK(esp_ble_gatt_set_local_mtu(LOCAL_MTU));
  // set random address
  ESP_ERROR_CHECK(esp_ble_gap_set_rand_addr((uint8_t *)random_addr));
}

void my_bt_base_deinit() {
  // bluedroid deinit
  ESP_ERROR_CHECK(esp_bluedroid_disable());
  ESP_ERROR_CHECK(esp_bluedroid_deinit());
  // bt controller deinit
  ESP_ERROR_CHECK(esp_bt_controller_disable());
  ESP_ERROR_CHECK(esp_bt_controller_deinit());

  ESP_ERROR_CHECK(esp_event_loop_delete(my_bt_event_loop_handle));
}

// 全部完成后，再测试
//  void my_bt_kill() { esp_bt_mem_release(ESP_BT_MODE_CLASSIC_BT); }

void my_bt_start_adv_with_manufacturer_data(void *manufacturer_data,
                                            uint8_t len) {
  int16_t ret = 0;
  esp_ble_adv_data_t ble_adv_data = {
      .set_scan_rsp = false,
      .include_name = false,
      .include_txpower = false,
      .min_interval = 0x10,
      .max_interval = 0x20,
      .appearance = 0,
      .manufacturer_len = len,
      .p_manufacturer_data = manufacturer_data,
      .service_data_len = 0,
      .p_service_data = NULL,
      .service_uuid_len = 0,
      .p_service_uuid = NULL,
      .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
  };
  ret = esp_ble_gap_config_adv_data(&ble_adv_data);
  ESP_LOGI(TAG, "esp_ble_gap_config_adv_data: %d", ret);
  esp_ble_adv_params_t ble_adv_params = {
      .adv_int_min = 0x20,
      .adv_int_max = 0x40,
      .adv_type = ADV_TYPE_IND,
      .own_addr_type = BLE_ADDR_TYPE_RANDOM,
      .channel_map = ADV_CHNL_ALL,
      .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
  };
  ret = esp_ble_gap_start_advertising(&ble_adv_params);
  ESP_LOGI(TAG, "esp_ble_gap_start_advertising: %d", ret);
}
