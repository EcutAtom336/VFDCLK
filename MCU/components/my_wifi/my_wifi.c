#include "my_wifi.h"
//
#include "my_common.h"
//
#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
//
#include "string.h"
//
#define TAG "my wifi"

// #define MY_WIFI_STA_MAX_RECONNECT_TIMES 10
#define MY_WIFI_STA_RECONNECTION_SPAN_MS 1000
// #define MY_WIFI_STA_RETRY_TIMES 0
// #define MY_WIFI_STA_RETRY_SPAN_MS 600000
#define MY_WIFI_AP_SSID "VFDCLK"
// #define WIFI_AP_PASSWORD

#define NVS_MY_WIFI_NS "my wifi"
#define NVS_KEY_MY_WIFI_CONFIG "config"

#define JSON_KEY_MY_WIFI_MAIN "myWifi"
#define JSON_KEY_MY_WIFI_SSID "ssid"
#define JSON_KEY_MY_WIFI_PASSWORD "password"
#define JSON_KEY_MY_WIFI_NEED_AUTH "needAuth"

typedef struct {
  uint32_t need_auth : 1;
  char ssid[32];
  char password[64];
} MyWifiConfig_t;

esp_netif_t *netif_sta_handle;
// esp_netif_t *netif_ap_handle;

esp_event_loop_handle_t my_wifi_sta_event_loop_handle;
esp_event_loop_handle_t my_wifi_ap_event_loop_handle;
esp_event_loop_handle_t my_wifi_ap_sta_event_loop_handle;

TaskHandle_t my_wifi_manage_task_handle;

uint16_t wifi_reconnect_times = 0;

void my_wifi_get_config(MyWifiConfig_t *p_config);

bool my_wifi_check_ssid_is_empty() {
  wifi_config_t wifi_config;
  esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
  ESP_LOGI(TAG, "device wifi ssid:%s", wifi_config.sta.ssid);
  bool empty_ssid_flag = true;
  for (size_t i = 0; i < 32; i++) {
    if (wifi_config.sta.ssid[i] != '\0') {
      empty_ssid_flag = false;
      break;
    }
  }
  if (empty_ssid_flag == true)
    ESP_LOGI(TAG, "ssid empty");
  else
    ESP_LOGI(TAG, "ssid not empty");
  return empty_ssid_flag;
}

void my_wifi_manage_task() {
  MyWifiConfig_t config;
  my_wifi_get_config(&config);

  ESP_LOGI(TAG, "my wifi manage start");
  // while (event_group_manager_check_bits(kEventGroupManagerTimeIsSync) ==
  //        kMyEventGroupReset) {
  //   ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  //   vTaskDelay(pdMS_TO_TICKS(1000));
  //   esp_err_t esp_ret = esp_wifi_connect();
  //   ESP_LOGI(TAG, "connect wifi: %s", esp_err_to_name(esp_ret));
  // }
  // vTaskDelay(pdMS_TO_TICKS(5000));
  // ESP_LOGI(TAG, "disconnect wifi");
  // esp_wifi_disconnect();
  if (config.need_auth == 1) {
    while (true) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  } else {
    while (true) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void my_wifi_save_config(const MyWifiConfig_t *config) {
  nvs_handle_t nvs_rw_handle;
  esp_err_t ret;
  ret = nvs_open_from_partition(NVS_VFDCLK_PARTITION_NAME, NVS_MY_WIFI_NS,
                                NVS_READWRITE, &nvs_rw_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "save config open nvs fail, reason: %s, reboot...",
             esp_err_to_name(ret));
    esp_restart();
  }

  ret = nvs_set_blob(nvs_rw_handle, NVS_KEY_MY_WIFI_CONFIG, config,
                     sizeof(MyWifiConfig_t));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "save config set fail, reason: %s, reboot...",
             esp_err_to_name(ret));
    esp_restart();
  }

  ret = nvs_commit(nvs_rw_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "nvs commit fail, reason: %s, reboot...",
             esp_err_to_name(ret));
    esp_restart();
  }

  nvs_close(nvs_rw_handle);

  ESP_LOGI(TAG, "save config success");
}

void my_wifi_get_config(MyWifiConfig_t *p_config) {
  const MyWifiConfig_t default_config = {
      .ssid = "DESKTOP-12",
      .password = "Z192418.",
      .need_auth = 0,
  };

  nvs_handle_t nvs_ro_handle;
  esp_err_t ret;
  ret = nvs_open_from_partition(NVS_VFDCLK_PARTITION_NAME, NVS_MY_WIFI_NS,
                                NVS_READONLY, &nvs_ro_handle);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "get config open nvs fail, use default config, reason: %s",
             esp_err_to_name(ret));
    memcpy(p_config, &default_config, sizeof(MyWifiConfig_t));
    return;
  }

  size_t my_wifi_config_size = sizeof(MyWifiConfig_t);
  ret = nvs_get_blob(nvs_ro_handle, NVS_KEY_MY_WIFI_CONFIG, p_config,
                     &my_wifi_config_size);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "get config read fail, use default config, reason: %s",
             esp_err_to_name(ret));
    memcpy(p_config, &default_config, sizeof(MyWifiConfig_t));
    nvs_close(nvs_ro_handle);
    return;
  }

  nvs_close(nvs_ro_handle);
}

void my_wifi_get_config_json(cJSON *config_json_parent) {
  MyWifiConfig_t config;
  my_wifi_get_config(&config);

  cJSON *my_wifi_config_json = cJSON_CreateObject();
  cJSON_AddStringToObject(my_wifi_config_json, JSON_KEY_MY_WIFI_SSID,
                          config.ssid);
  cJSON_AddStringToObject(my_wifi_config_json, JSON_KEY_MY_WIFI_PASSWORD,
                          config.password);
  if (config.need_auth == 1)
    cJSON_AddTrueToObject(my_wifi_config_json, JSON_KEY_MY_WIFI_NEED_AUTH);
  else
    cJSON_AddFalseToObject(my_wifi_config_json, JSON_KEY_MY_WIFI_NEED_AUTH);

  cJSON_AddItemToObject(config_json_parent, JSON_KEY_MY_WIFI_MAIN,
                        my_wifi_config_json);
}

void my_wifi_analysis_config_json(cJSON *config_json_parent) {
  MyWifiConfig_t config;
  my_wifi_get_config(&config);
  cJSON *my_wifi_config_json =
      cJSON_GetObjectItem(config_json_parent, JSON_KEY_MY_WIFI_MAIN);
  if (my_wifi_config_json == NULL) {
    ESP_LOGI(TAG, "config not found");
    return;
  }

  cJSON *ssid_json =
      cJSON_GetObjectItem(my_wifi_config_json, JSON_KEY_MY_WIFI_SSID);
  cJSON *password_json =
      cJSON_GetObjectItem(my_wifi_config_json, JSON_KEY_MY_WIFI_PASSWORD);
  if (ssid_json != NULL && password_json != NULL) {
    strcpy(config.ssid, cJSON_GetStringValue(ssid_json));
    strcpy(config.password, cJSON_GetStringValue(password_json));
  }

  cJSON *need_auth_json =
      cJSON_GetObjectItem(my_wifi_config_json, JSON_KEY_MY_WIFI_NEED_AUTH);
  if (need_auth_json != NULL) {
    config.need_auth = cJSON_IsTrue(need_auth_json);
  }

  my_wifi_save_config(&config);
}

void my_wifi_init() {
  ESP_ERROR_CHECK(esp_netif_init());
  netif_sta_handle =
      esp_netif_create_default_wifi_sta(); // 调用此API前需创建默认循环
  // netif_ap_handle =
  //     esp_netif_create_default_wifi_ap();  // 调用此API前需创建默认循环
  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  MyWifiConfig_t config;
  my_wifi_get_config(&config);
  wifi_config_t wifi_config;
  memset(&wifi_config, 0, sizeof(wifi_config_t));
  memcpy(wifi_config.sta.ssid, config.ssid, 32);
  memcpy(wifi_config.sta.password, config.password, 64);
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  if (config.need_auth == 1) {
  } else {
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                               WIFI_EVENT_STA_DISCONNECTED,
                                               (void *)esp_wifi_connect, NULL));
  }

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START,
                                             (void *)esp_wifi_connect, NULL));

  // xTaskCreate(my_wifi_manage_task, "my wifi manage",
  //             configMINIMAL_STACK_SIZE * 2, NULL, 8,
  //             &my_wifi_manage_task_handle);

  ESP_ERROR_CHECK(esp_wifi_start());
}
