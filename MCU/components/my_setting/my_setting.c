#include "my_setting.h"
//
#include "VFD_8_MD_06INKM.h"
#include "ble_ut.h"
#include "common.h"
#include "my_bt.h"
#include "my_clock.h"
#include "my_wifi.h"
//
#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "iot_button.h"
#include "nvs_flash.h"

//
#include "string.h"

#define TAG "my setting"

#define MY_NVS_PARTITION_NAME "vfdclk"

#define NVS_MY_SETTING_NS "my setting"
#define NVS_KEY_MY_SETTING_CONFIG "user config"

#define JSON_MY_SETTING_MAIN "mySetting"
#define JSON_KEY_MY_SETTING_BRIGHTNESS "brightness"

ESP_EVENT_DEFINE_BASE(MY_SETTING_EVENT);

typedef struct {
  uint8_t brightness;
} MySettingConfig_t;

// esp_event_loop_handle_t my_setting_event_loop_handle;

TaskHandle_t setting_task_handle = NULL;

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

void my_setting_on_start() {}

void my_setting_on_recv() {}

void my_setting_on_stop() {
  esp_restart();
  // my_clock_init();
  // ESP_ERROR_CHECK(esp_event_handler_unregister_with(
  //     my_setting_on_stop, MY_BT_EVENT, kMyBtEventConnect,
  //     my_bt_event_loop_handle));
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

void my_setting_task() {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    // stop other task
    ESP_ERROR_CHECK(esp_event_post(MY_SETTING_EVENT, kMySettingEventStart, NULL,
                                   0, portMAX_DELAY));
    my_clock_deinit();
    // gen random device code and random address
    uint8_t manufacturer_data[14];
    uint8_t random_for_device_code_string[8] = {"        "};
    uint8_t random_for_address[6];
    esp_fill_random(manufacturer_data, 14);
    esp_fill_random(random_for_address, 6);
    random_for_address[0] |= 0xC0;
    manufacturer_data[0] = 0xFF;
    manufacturer_data[1] = 0xFF;
    manufacturer_data[2] = (uint8_t)(MODEL_CODE & 0xFF);
    manufacturer_data[3] = (uint8_t)((MODEL_CODE >> 8) & 0xFF);
    manufacturer_data[4] = (uint8_t)(HARDWARE_VERSION_CODE & 0xFF);
    manufacturer_data[5] = (uint8_t)((HARDWARE_VERSION_CODE >> 8) & 0xFF);
    manufacturer_data[6] = (uint8_t)(SOFTWARE_VERSION_CODE & 0xFF);
    manufacturer_data[7] = (uint8_t)((SOFTWARE_VERSION_CODE >> 8) & 0xFF);
    for (size_t i = 0; i < 6; i++) {
      manufacturer_data[i + 8] %= 10;
      random_for_device_code_string[i + 1] = manufacturer_data[i + 8] + 48;
    }
    ESP_LOGI(TAG, "random device code: %u%u%u%u%u%u", manufacturer_data[8],
             manufacturer_data[9], manufacturer_data[10], manufacturer_data[11],
             manufacturer_data[12], manufacturer_data[13]);
    ESP_LOGI(TAG, "random address: %x:%x:%x:%x:%x:%x", random_for_address[0],
             random_for_address[1], random_for_address[2],
             random_for_address[3], random_for_address[4],
             random_for_address[5]);
    // show random device code
    VFD_8_MD_06INKM_show_str(random_for_device_code_string, 0, 8);
    // start ble
    bt_base_init_with_random_address(random_for_address);
    bt_gatts_init();
    my_bt_register_gatt_app_cb(ble_ut_gatts_handler);
    my_bt_start_adv_with_manufacturer_data(manufacturer_data, 14);
    ble_ut_init();

    // ble_ut_set_to_server_handler(my_setting_ble_ut_test);
    ble_ut_set_to_server_handler(my_setting_handler_new_config);

    // ESP_ERROR_CHECK(esp_event_handler_register_with(
    //     ble_ut_event_loop_handle, BLE_UT_EVENT, kBleUtEventToClientEnable,
    //     my_setting_test_ble_ut, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        ble_ut_event_loop_handle, BLE_UT_EVENT, kBleUtEventToClientEnable,
        my_setting_send_config, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(
        my_bt_event_loop_handle, MY_BT_EVENT, kMyBtEventDisconnect,
        my_setting_on_stop, NULL));

    vTaskDelay(pdMS_TO_TICKS(3000));
    vTaskDelete(NULL);

    VFD_8_MD_06INKM_show_str("conn ed ", 0, 8);

    ESP_LOGI(TAG, "setting complete");
    ulTaskNotifyTake(pdTRUE, 0);
  }
}

void my_setting_trig_isr() {
  vTaskNotifyGiveFromISR(setting_task_handle, NULL);
}

void my_setting_trig() { vTaskNotifyGiveFromISR(setting_task_handle, NULL); }

void my_setting_init() {
  MySettingConfig_t config;
  memset(&config, 0, sizeof(MySettingConfig_t));

  button_handle_t button_handle;
  button_config_t button_config = {
      .long_press_time = 3000,
      .short_press_time = BUTTON_SHORT_PRESS_TIME_MS,
      .type = BUTTON_TYPE_GPIO,
      .gpio_button_config =
          {
              .gpio_num = 4,
              .active_level = 1,
          },
  };
  button_handle = iot_button_create(&button_config);
  if (button_handle == NULL) {
  }
  ESP_ERROR_CHECK(iot_button_register_cb(button_handle, BUTTON_LONG_PRESS_START,
                                         my_setting_trig, NULL));

  // esp_event_loop_args_t event_loop_args = {
  //     .queue_size = 8,
  //     .task_stack_size = configMINIMAL_STACK_SIZE,
  //     .task_core_id = 0,
  //     .task_name = "my setting event loop task",
  //     .task_priority = 5,
  // };
  // ESP_ERROR_CHECK(
  //     esp_event_loop_create(&event_loop_args,
  //     &my_setting_event_loop_handle));

  // ESP_ERROR_CHECK(esp_event_handler_register_with(
  //     my_bt_event_loop_handle, BLE_UT_EVENT, kBleUtEventToClientEnable,
  //     my_setting_send_config, NULL));

  xTaskCreate(my_setting_task, "my setting task", configMINIMAL_STACK_SIZE * 2,
              NULL, 5, &setting_task_handle);
  // gpio_config_t my_gpio_config = {
  //     .intr_type = GPIO_INTR_NEGEDGE,
  //     .mode = GPIO_MODE_INPUT,
  //     .pin_bit_mask = (uint64_t)1 << 9,
  //     .pull_down_en = GPIO_PULLDOWN_DISABLE,
  //     .pull_up_en = GPIO_PULLUP_ENABLE,
  // };
  // ESP_ERROR_CHECK(gpio_config(&my_gpio_config));
  // ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_SHARED));
  // ESP_ERROR_CHECK(gpio_isr_handler_add(9, my_setting_trig_isr, NULL));
}
