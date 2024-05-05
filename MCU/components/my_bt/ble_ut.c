#include "ble_ut.h"
//
#include "ble_common.h"
//
#include "esp_event.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
//
#include <string.h>

ESP_EVENT_DEFINE_BASE(BLE_UT_EVENT);

#define TAG "ble ut"

// 更名为 通用传输 技术（universal transmission，简写ut）

enum {
  kBleUtSvc,

  kBleUtCharDeclRx,
  kBleUtCharRx,

  kBleUtCharDeclTx,
  kBleUtCharTx,
  kBleUtCharDescTx,

  kBleUtAttrMax,
};

esp_event_loop_handle_t ble_ut_event_loop_handle;

BleUtToServerHandler_t to_server_handler;

esp_gatt_if_t ble_ut_gatt_if = ESP_GATT_IF_NONE;
uint16_t attr_handle_list[kBleUtAttrMax];

struct {
  TaskHandle_t to_client;
  TaskHandle_t to_server_handler;
} ble_ut_task_handle;

struct {
  QueueHandle_t to_client_tight;
  QueueHandle_t to_server_tight;
} ble_ut_queue_handle;

static uint16_t attr_handle_table[kBleUtAttrMax];

// 5c111a6e-53b0-46f4-aff5-8505f943411b
static uint8_t ble_ut_svc_uuid[] = {0x1b, 0x41, 0x43, 0xf9, 0x05, 0x85,
                                    0xf5, 0xaf, 0xf4, 0x46, 0xb0, 0x53,
                                    0x6e, 0x1a, 0x11, 0x5c};
// f69cc8c9-358e-6d98-604b-51217440a5b5
static uint8_t ble_ut_char_to_server_uuid[] = {
    0xb5, 0xa5, 0x40, 0x74, 0x21, 0x51, 0x4b, 0x60,
    0x98, 0x6d, 0x8e, 0x35, 0xc9, 0xc8, 0x9c, 0xf6};
static uint8_t ut_to_server_buf;
// 1b4143f9-0585-f5af-f446-b0536e1a115c
static uint8_t ble_ut_char_to_client_uuid[] = {
    0x5c, 0x11, 0x1a, 0x6e, 0x53, 0xb0, 0x46, 0xf4,
    0xaf, 0xf5, 0x85, 0x05, 0xf9, 0x43, 0x41, 0x1b};
static uint8_t ut_to_client_buf;
static uint16_t ut_to_client_cccd;

esp_gatts_attr_db_t ble_ut_gatts_attr_db[kBleUtAttrMax] = {
    [kBleUtSvc] =
        {
            {ESP_GATT_AUTO_RSP},
            {
                .uuid_length = ESP_UUID_LEN_16,
                .uuid_p = (uint8_t *)&primary_uuid,
                .perm = ESP_GATT_PERM_READ,
                .max_length = sizeof(ble_ut_svc_uuid),
                .length = sizeof(ble_ut_svc_uuid),
                .value = ble_ut_svc_uuid,
            },
        },
    [kBleUtCharDeclRx] =
        {
            {ESP_GATT_AUTO_RSP},
            {
                .uuid_length = ESP_UUID_LEN_16,
                .uuid_p = (uint8_t *)&characteristic_decalaration_uuid,
                .perm = ESP_GATT_PERM_READ,
                .max_length = sizeof(char_prop_w),
                .length = sizeof(char_prop_w),
                .value = &char_prop_w,
            },
        },
    [kBleUtCharRx] =
        {
            {ESP_GATT_AUTO_RSP},
            {
                .uuid_length = ESP_UUID_LEN_128,
                .uuid_p = ble_ut_char_to_server_uuid,
                .perm = ESP_GATT_PERM_WRITE,
                .max_length = 256,
                .length = 1,
                .value = &ut_to_server_buf,
            },
        },
    [kBleUtCharDeclTx] =
        {
            {ESP_GATT_AUTO_RSP},
            {
                .uuid_length = ESP_UUID_LEN_16,
                .uuid_p = (uint8_t *)&characteristic_decalaration_uuid,
                .perm = ESP_GATT_PERM_READ,
                .max_length = sizeof(char_prop_n),
                .length = sizeof(char_prop_n),
                .value = &char_prop_n,
            },
        },
    [kBleUtCharTx] =
        {
            {ESP_GATT_AUTO_RSP},
            {
                .uuid_length = ESP_UUID_LEN_128,
                .uuid_p = ble_ut_char_to_client_uuid,
                .perm = ESP_GATT_PERM_READ,
                .max_length = 256,
                .length = 1,
                .value = &ut_to_client_buf,
            },
        },
    [kBleUtCharDescTx] =
        {
            {ESP_GATT_AUTO_RSP},
            {
                .uuid_length = ESP_UUID_LEN_16,
                .uuid_p = (uint8_t *)&character_client_config_uuid,
                .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                .max_length = sizeof(ut_to_client_cccd),
                .length = sizeof(ut_to_client_cccd),
                .value = (uint8_t *)&ut_to_client_cccd,
            },
        },
};

void ble_ut_gatts_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                          esp_ble_gatts_cb_param_t *param) {
  if (event == ESP_GATTS_REG_EVT && param->reg.app_id == kBleAppIdUt)
    ble_ut_gatt_if = gatts_if;
  if (gatts_if != ESP_GATT_IF_NONE && gatts_if != ble_ut_gatt_if)
    return;
  switch (event) {
  case ESP_GATTS_REG_EVT:
    if (param->reg.status == ESP_GATT_OK) {
      ESP_LOGI(TAG, "ESP_GATTS_REG_EVT success");
      if (param->reg.app_id == kBleAppIdUt) {
        ESP_ERROR_CHECK(esp_ble_gatts_create_attr_tab(
            ble_ut_gatts_attr_db, gatts_if, kBleUtAttrMax, 0));
      }
    } else {
      ESP_LOGE(TAG, "ESP_GATTS_REG_EVT fail, code: %u", param->reg.status);
    }
    break;

  case ESP_GATTS_UNREG_EVT:
    ESP_LOGI(TAG, "ESP_GATTS_UNREG_EVT");
    break;

  case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    if (param->add_attr_tab.status == ESP_GATT_OK) {
      if (param->add_attr_tab.svc_inst_id == 0) {
        memcpy(attr_handle_table, param->add_attr_tab.handles,
               sizeof(uint16_t) * param->add_attr_tab.num_handle);
        ESP_ERROR_CHECK(
            esp_ble_gatts_start_service(attr_handle_table[kBleUtSvc]));
      }
    }
    break;

  case ESP_GATTS_READ_EVT:
    ESP_LOGI(TAG, "ESP_GATTS_READ_EVT");
    break;

  case ESP_GATTS_WRITE_EVT:
    ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT");
    if (param->write.handle == attr_handle_table[kBleUtCharRx]) {
      static struct {
        uint32_t all_size;
        uint32_t remain_size;
        void *p_data;
      } to_server_tmp;
      if (to_server_tmp.p_data == NULL && param->write.len == 5 &&
          ((uint8_t *)param->write.value)[0] == 1) {
        ESP_LOGI(TAG, "pkg info raw: %u ,%u ,%u ,%u ,%u", param->write.value[0],
                 param->write.value[1], param->write.value[2],
                 param->write.value[3], param->write.value[4]);
        to_server_tmp.all_size = ((uint32_t *)(param->write.value + 1))[0];
        to_server_tmp.remain_size = to_server_tmp.all_size;
        to_server_tmp.p_data = pvPortMalloc(to_server_tmp.all_size);
        ESP_LOGI(TAG, "receive pkg info: all size: %ld",
                 to_server_tmp.all_size);
      } else if (to_server_tmp.p_data != NULL) {
        memcpy(to_server_tmp.p_data +
                   (to_server_tmp.all_size - to_server_tmp.remain_size),
               param->write.value, param->write.len);
        to_server_tmp.remain_size -= param->write.len;
        if (to_server_tmp.remain_size == 0) {
          BleUtPkgInfo_t to_server_tight_pkg = {
              .size = to_server_tmp.all_size,
              .p_data = to_server_tmp.p_data,
          };
          xQueueSend(ble_ut_queue_handle.to_server_tight, &to_server_tight_pkg,
                     portMAX_DELAY);
          to_server_tmp.p_data = NULL;
        }
      }
      ESP_LOGI(TAG, "remain size: %ld", to_server_tmp.remain_size);
    } else if (param->write.handle == attr_handle_table[kBleUtCharDescTx]) {
      memcpy(&ut_to_client_cccd, param->write.value, param->write.len);
      esp_event_post_to(ble_ut_event_loop_handle, BLE_UT_EVENT,
                        kBleUtEventToClientEnable, NULL, 0, portMAX_DELAY);
    }
    break;

  default:
    ESP_LOGI(TAG, "%s not implement, event id: %u", __func__, event);
    break;
  }
}

void ble_ut_to_client_task() {
  BleUtPkgInfo_t to_client_pkg;
  uint32_t remain_size = 0;
  while (true) {
    if (remain_size == 0) {
      xQueueReceive(ble_ut_queue_handle.to_client_tight, &to_client_pkg,
                    portMAX_DELAY);
      remain_size = to_client_pkg.size;
      uint8_t pkg_info[5] = {1};
      *((uint32_t *)(pkg_info + 1)) = to_client_pkg.size;
      ESP_LOGI(TAG, "pkg info raw: %u ,%u ,%u ,%u ,%u", pkg_info[0],
               pkg_info[1], pkg_info[2], pkg_info[3], pkg_info[4]);
      ESP_ERROR_CHECK(esp_ble_gatts_send_indicate(
          ble_ut_gatt_if, connect_info.conn_id, attr_handle_table[kBleUtCharTx],
          5, pkg_info, false));
    } else {
      uint16_t current_size = remain_size;
      if (current_size > cur_mtu - 3 - 4)
        current_size = cur_mtu - 3 - 4;
      ESP_ERROR_CHECK(esp_ble_gatts_send_indicate(
          ble_ut_gatt_if, connect_info.conn_id, attr_handle_table[kBleUtCharTx],
          current_size,
          to_client_pkg.p_data + (to_client_pkg.size - remain_size), false));
      remain_size -= current_size;
      if (remain_size == 0)
        vPortFree(to_client_pkg.p_data);
    }
    vTaskDelay(pdMS_TO_TICKS(8));
  }
}

void ble_ut_send_to_client(void *p_data, uint32_t size) {
  if (connect_info.is_connect) {
    if (ut_to_client_cccd & 0x1) {
      BleUtPkgInfo_t to_client_tight_pkg;
      to_client_tight_pkg.size = size;
      to_client_tight_pkg.p_data = pvPortMalloc(size);
      if (to_client_tight_pkg.p_data != NULL) {
        memcpy(to_client_tight_pkg.p_data, p_data, size);
        xQueueSend(ble_ut_queue_handle.to_client_tight, &to_client_tight_pkg,
                   portMAX_DELAY);
      } else {
        ESP_LOGW(TAG, "ble ut send malloc fail");
      }
    } else {
      ESP_LOGW(TAG, "notify disable");
    }
  } else {
    ESP_LOGW(TAG, "no connection");
  }
}

// void ble_ut_send_to_client_test_task() {
//   uint8_t test_data1[8] = {0};
//   for (size_t i = 0; i < sizeof(test_data1); i++)
//     test_data1[i] = i;
//   uint8_t test_data2[66] = {0};
//   for (size_t i = 0; i < sizeof(test_data2); i++)
//     test_data2[i] = i;
//   while (true) {
//     vTaskDelay(pdMS_TO_TICKS(3000));
//     ble_ut_send_to_client(test_data2, sizeof(test_data2));
//   }
// }

void ble_ut_to_server_handler_task() {
  BleUtPkgInfo_t to_server_pkg;
  while (true) {
    xQueueReceive(ble_ut_queue_handle.to_server_tight, &to_server_pkg,
                  portMAX_DELAY);
    if (to_server_handler != NULL) {
      to_server_handler(&to_server_pkg);
    }
    vPortFree(to_server_pkg.p_data);
  }
}

void ble_ut_init() {
  ble_ut_queue_handle.to_client_tight = xQueueCreate(8, sizeof(BleUtPkgInfo_t));
  if (ble_ut_queue_handle.to_client_tight == NULL) {
    ESP_LOGE(TAG, "create tx queue fail, reboot...");
    esp_restart();
  }
  ble_ut_queue_handle.to_server_tight = xQueueCreate(8, sizeof(BleUtPkgInfo_t));
  if (ble_ut_queue_handle.to_server_tight == NULL) {
    ESP_LOGE(TAG, "create rec queue fail, reboot...");
    esp_restart();
  }

  ESP_ERROR_CHECK(esp_ble_gatts_app_register(kBleAppIdUt));

  esp_event_loop_args_t event_loop_args = {
      .queue_size = 8,
      .task_stack_size = configMINIMAL_STACK_SIZE * 2,
      .task_core_id = 0,
      .task_name = "ble ut loop task",
      .task_priority = 12,
  };
  ESP_ERROR_CHECK(
      esp_event_loop_create(&event_loop_args, &ble_ut_event_loop_handle));

  xTaskCreate(ble_ut_to_client_task, "ble ut tx task",
              configMINIMAL_STACK_SIZE * 2, NULL, 8,
              &ble_ut_task_handle.to_client);
  xTaskCreate(ble_ut_to_server_handler_task, "ble ut to server handler task",
              configMINIMAL_STACK_SIZE * 2, NULL, 8,
              &ble_ut_task_handle.to_server_handler);
}

void ble_ut_set_to_server_handler(BleUtToServerHandler_t cb) {
  if (cb != NULL) {
    to_server_handler = cb;
  }
}

void ble_ut_delete_to_server_handler() { to_server_handler = NULL; }

void ble_ut_deinit() {
  esp_ble_gatts_app_unregister(ble_ut_gatt_if);

  vTaskDelete(ble_ut_task_handle.to_client);
  vTaskDelete(ble_ut_task_handle.to_server_handler);
  vQueueDelete(ble_ut_queue_handle.to_client_tight);
  vQueueDelete(ble_ut_queue_handle.to_server_tight);
}
