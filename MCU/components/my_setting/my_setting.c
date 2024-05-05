#include "my_setting.h"
//
#include "VFD_8_MD_06INKM.h"
#include "ble_ut.h"
#include "life_cycle_manage.h"
#include "my_bt.h"
#include "my_clock.h"
#include "my_common.h"
#include "my_wifi.h"
//
#include "button_gpio.h"
#include "cJSON.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot_button.h"
//
#include "string.h"

#define TAG "my setting"

#define MY_NVS_PARTITION_NAME "vfdclk"

#define NVS_MY_SETTING_NS "my setting"
#define NVS_KEY_MY_SETTING_CONFIG "user config"

#define JSON_MY_SETTING_MAIN "mySetting"
#define JSON_KEY_MY_SETTING_BRIGHTNESS "brightness"

typedef struct {
  TaskHandle_t main_task_handle;
  QueueHandle_t main_queue_handle;
  esp_event_handler_instance_t send_config_event_handler_inst_handle;
  esp_event_handler_instance_t disconnect_event_handler_inst_handle;
  button_handle_t iot_button_handle;
  uint32_t is_exists : 1;
} MySettingContext_t;

typedef enum {
  Configuration,
  Close,
} MySettingJsonType_t;

typedef enum {
  kMySettingMainMsgTypeResume,
  kMySettingMainMsgTypePause,
  kMySettingMainMsgTypeDelete,
  kMySettingMainMsgTypeShow,
} MySettingMainMsgType_t;

typedef struct {
  MySettingMainMsgType_t type;
  union {
    struct {
      char str[8];
    } show;
  };
} MySettingMainMsg_t;

// void my_setting_handler_msg(BleUtPkgInfo_t *pkg) {
//   ESP_LOGI(TAG, "new config json: %s", (char *)pkg->p_data);

//   cJSON *msg_json = cJSON_Parse(pkg->p_data);
//   if (msg_json == NULL) {
//     ESP_LOGW(TAG, "parse config json fail");
//     return;
//   }

//   cJSON *msg_type_json = cJSON_GetObjectItem(msg_json, "Type");
//   if (msg_type_json == NULL) {
//     ESP_LOGW(TAG, "handler mag fail: type not found");
//     return;
//   }

//   MySettingJsonType_t type = cJSON_GetNumberValue(msg_type_json);

//   if (type == Configuration) {
//   } else if (type == Close) {
//   }
// }

void my_setting_handler_new_config(BleUtPkgInfo_t *pkg) {
  ESP_LOGI(TAG, "new config json: %s", (char *)pkg->p_data);

  cJSON *config_json = cJSON_Parse(pkg->p_data);
  if (config_json == NULL) {
    ESP_LOGW(TAG, "parse config json fail");
    return;
  }

  // my_setting_analysis_config_json(config_json);
  VFD_analysis_config_json(config_json);
  my_clock_analysis_config_json(config_json);
  my_wifi_analysis_config_json(config_json);

  cJSON_Delete(config_json);
}

void my_setting_send_config() {
  cJSON *config_json = cJSON_CreateObject();

  VFD_get_config_json(config_json);
  my_wifi_get_config_json(config_json);
  my_clock_get_config_json(config_json);

  char *json_str = cJSON_PrintUnformatted(config_json);
  ESP_LOGI(TAG, "send config: %s", json_str);

  cJSON_Delete(config_json);

  ble_ut_send_to_client(json_str, strlen(json_str));

  cJSON_free(json_str);
}

void my_setting_main_task(void *param) {
  MySettingContext_t *context_handle = (MySettingContext_t *)param;

  MySettingMainMsg_t main_msg;
  while (true) {
    xQueueReceive(context_handle->main_queue_handle, &main_msg, portMAX_DELAY);
    if (main_msg.type == kMySettingMainMsgTypeResume) {
      VFD_8_MD_06INKM_aquire(portMAX_DELAY);
    } else if (main_msg.type == kMySettingMainMsgTypePause) {
      VFD_8_MD_06INKM_release();
    } else if (main_msg.type == kMySettingMainMsgTypeDelete) {
      vTaskDelete(NULL);
    } else if (main_msg.type == kMySettingMainMsgTypeShow) {
      VFD_8_MD_06INKM_show_str(main_msg.show.str, 0, 8);
    }
  }
}

void my_setting_create_fail_clear(void *pv_context) {
  if (pv_context == NULL) {
    return;
  }

  MySettingContext_t *context_handle = (MySettingContext_t *)pv_context;

  if (context_handle->main_task_handle != NULL) {
    vTaskDelete(context_handle->main_task_handle);
  }

  if (context_handle->main_queue_handle != NULL) {
    vQueueDelete(context_handle->main_queue_handle);
  }

  if (context_handle->send_config_event_handler_inst_handle != NULL) {
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister_with(
        my_bt_event_loop_handle, BLE_UT_EVENT, kBleUtEventToClientEnable,
        context_handle->send_config_event_handler_inst_handle));
  }

  if (context_handle->disconnect_event_handler_inst_handle != NULL) {
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister_with(
        my_bt_event_loop_handle, MY_BT_EVENT, kMyBtEventDisconnect,
        context_handle->disconnect_event_handler_inst_handle));
  }

  if (context_handle->iot_button_handle != NULL) {
    iot_button_delete(context_handle->iot_button_handle);
  }
}

void *my_setting_on_create() {
  static MySettingContext_t sigleton_context;

  ESP_LOGI(TAG, "on create");

  if (sigleton_context.is_exists == 1) {
    ESP_LOGW(TAG, "an instance already exists");
    return NULL;
  }

  uint8_t random_address[6];
  esp_fill_random(random_address, 6);
  random_address[0] |= 0xC0;

  ESP_LOGI(TAG, "random address: %x:%x:%x:%x:%x:%x", random_address[0],
           random_address[1], random_address[2], random_address[3],
           random_address[4], random_address[5]);

  bt_base_init_with_random_address(random_address);
  bt_gatts_init();
  my_bt_register_gatt_app_cb(ble_ut_gatts_handler);
  ble_ut_init();

  sigleton_context.main_queue_handle =
      xQueueCreate(4, sizeof(MySettingMainMsg_t));
  if (sigleton_context.main_queue_handle == NULL) {
    ESP_LOGE(TAG, "create queue fail");
    my_setting_create_fail_clear(&sigleton_context);
    return NULL;
  }

  xTaskCreate(my_setting_main_task, NULL, configMINIMAL_STACK_SIZE * 2,
              &sigleton_context, 16, &sigleton_context.main_task_handle);
  if (sigleton_context.main_task_handle == NULL) {
    ESP_LOGE(TAG, "create task fail");
    my_setting_create_fail_clear(&sigleton_context);
    return NULL;
  }

  button_config_t iot_button_config = {
      .type = BUTTON_TYPE_GPIO,
      .gpio_button_config =
          {
              .gpio_num = 4,
              .active_level = 1,
          },
  };
  sigleton_context.iot_button_handle = iot_button_create(&iot_button_config);
  if (sigleton_context.iot_button_handle == NULL) {
    ESP_LOGE(TAG, "create iot button fail");
    my_setting_create_fail_clear(&sigleton_context);
    return NULL;
  }

  return &sigleton_context;
}

void my_setting_on_resume(void *pv_context) {
  MySettingContext_t *context_handle = (MySettingContext_t *)pv_context;

  ESP_LOGI(TAG, "on resume");

  esp_err_t esp_err;

  esp_err = esp_event_handler_instance_register_with(
      ble_ut_event_loop_handle, BLE_UT_EVENT, kBleUtEventToClientEnable,
      my_setting_send_config, NULL,
      &context_handle->send_config_event_handler_inst_handle);
  ESP_ERROR_CHECK(esp_err);

  esp_err = esp_event_handler_instance_register_with(
      my_bt_event_loop_handle, MY_BT_EVENT, kMyBtEventDisconnect,
      (esp_event_handler_t)esp_restart, NULL,
      &context_handle->disconnect_event_handler_inst_handle);
  ESP_ERROR_CHECK(esp_err);

  esp_err = iot_button_register_cb(context_handle->iot_button_handle,
                                   BUTTON_LONG_PRESS_START,
                                   (button_cb_t)esp_restart, NULL);
  ESP_ERROR_CHECK(esp_err);

  uint8_t manufacturer_data[14] = {
      0xFF,
      0xFF,
      (uint8_t)(MODEL_CODE & 0xFF),
      (uint8_t)((MODEL_CODE >> 8) & 0xFF),
      (uint8_t)(HARDWARE_VERSION_CODE & 0xFF),
      (uint8_t)((HARDWARE_VERSION_CODE >> 8) & 0xFF),
      (uint8_t)(SOFTWARE_VERSION_CODE & 0xFF),
      (uint8_t)((SOFTWARE_VERSION_CODE >> 8) & 0xFF),
  };
  char random_device_code_str[8];
  esp_fill_random(manufacturer_data + 8, 14);
  for (size_t i = 0; i < 6; i++) {
    manufacturer_data[i + 8] %= 10;
    random_device_code_str[i + 1] = manufacturer_data[i + 8] + 48;
  }
  ESP_LOGI(TAG, "random device code: %u%u%u%u%u%u", manufacturer_data[8],
           manufacturer_data[9], manufacturer_data[10], manufacturer_data[11],
           manufacturer_data[12], manufacturer_data[13]);
  my_bt_register_gatt_app_cb(ble_ut_gatts_handler);
  ble_ut_set_to_server_handler(my_setting_handler_new_config);
  my_bt_start_adv_with_manufacturer_data(manufacturer_data,
                                         sizeof(manufacturer_data));

  MySettingMainMsg_t main_msg = {
      .type = kMySettingMainMsgTypeResume,
  };
  xQueueSendToFront(context_handle->main_queue_handle, &main_msg,
                    portMAX_DELAY);
  main_msg.type = kMySettingMainMsgTypeShow;
  memcpy(main_msg.show.str, random_device_code_str, 8);
  xQueueSend(context_handle->main_queue_handle, &main_msg, portMAX_DELAY);
}

void my_setting_on_pause(void *pv_context) {
  MySettingContext_t *context_handle = (MySettingContext_t *)pv_context;

  ESP_LOGI(TAG, "on pause");

  ESP_ERROR_CHECK(esp_event_handler_instance_unregister_with(
      my_bt_event_loop_handle, BLE_UT_EVENT, kBleUtEventToClientEnable,
      context_handle->send_config_event_handler_inst_handle));
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister_with(
      my_bt_event_loop_handle, MY_BT_EVENT, kMyBtEventDisconnect,
      context_handle->disconnect_event_handler_inst_handle));

  ESP_ERROR_CHECK(iot_button_unregister_cb(context_handle->iot_button_handle,
                                           BUTTON_LONG_PRESS_START));

  ble_ut_delete_to_server_handler();

  xQueueReset(context_handle->main_queue_handle);
  MySettingMainMsg_t main_msg = {
      .type = kMySettingMainMsgTypePause,
  };
  xQueueSendToFront(context_handle->main_queue_handle, &main_msg,
                    portMAX_DELAY);
}

void my_setting_on_destroy(void *pv_context) {
  MySettingContext_t *context_handle = (MySettingContext_t *)pv_context;

  ESP_LOGI(TAG, "on destroy");

  ble_ut_deinit();

  my_bt_base_deinit();

  iot_button_delete(context_handle->iot_button_handle);

  MySettingMainMsg_t main_msg = {
      .type = kMySettingMainMsgTypeDelete,
  };
  xQueueSendToFront(context_handle->main_queue_handle, &main_msg,
                    portMAX_DELAY);

  context_handle->is_exists = 0;
}
