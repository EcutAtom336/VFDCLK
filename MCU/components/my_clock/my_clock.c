#include "my_clock.h"
//
#include "VFD_8_MD_06INKM.h"
#include "common.h"
#include "my_buzzer.h"
#include "my_event_group.h"
#include "my_sntp.h"
//
#include "cJSON.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portable.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
//
#include <stdint.h>
//
#define TAG "my clock"

// 一星期的第一天是 星期天 捏

#define DAY_SEC 86400
#define DAY_MIN DAY_SEC / 60
#define DAY_HOUR DAY_MIN / 60
#define WEEK_SEC DAY_SEC * 7
#define WEEK_MIN WEEK_SEC / 60
#define WEEK_HOUR WEEK_MIN / 60

#define NVS_MY_CLOCK_NS "my clock"
#define NVS_KEY_MY_CLOCK_CONFIG "config"

#define JSON_KEY_MY_CLOCK_MAIN "myClock"
#define JSON_KEY_MY_CLOCK_ON_SCREEN_DMIN "onSCRNDmin"
#define JSON_KEY_MY_CLOCK_OFF_SCREEN_DMIN "offSCRNDmin"
#define JSON_KEY_MY_CLOCK_HOURLY_CHIME "hourPrompt"

static TaskHandle_t my_clock_task_handle;
static QueueHandle_t my_clock_queue_handle;

typedef struct {
  uint32_t hour_prompt : 1;
  struct {
    uint16_t on_Dmin;
    uint16_t off_Dmin;
  } timing_switch_screen;
} MyClockConfig_t;

void my_clock_get_config(MyClockConfig_t *p_config);

// 屏幕开关时间以分钟设置，但以秒计算

/*
 * 检查 target 值是否在 指定区间 中。
 * 当 start < end, 指定区间 为 [ start, end )，
 * 当 start > end, 指定区间为 [ start, max ] 和 [ 0, end )。
 */
static inline bool check_u32_in_range(uint32_t start, uint32_t end,
                                      uint32_t max, uint32_t val) {
  assert(start != end);
  assert(start <= max);
  assert(end <= max);
  if (start < end)
    return val >= start && val < end;
  else
    return !(val >= end && val < start);
}

static inline uint32_t my_clock_get_gap(uint32_t from, uint32_t to,
                                        uint32_t max) {
  return from <= to ? to - from : max - from + to;
}

static void my_clock_task() {
  MyClockConfig_t config;
  my_clock_get_config(&config);

  VFD_8_MD_06INKM_show_str("VFD CLK ", 0, 8);
  if (!(xEventGroupWaitBits(
            event_group_manager_get_handle(kEventGroupManagerTimeIsSync),
            EVENT_GROUP1_TIME_IS_SYNC, pdFALSE, pdFALSE, pdMS_TO_TICKS(10000)) &
        EVENT_GROUP1_TIME_IS_SYNC))
    VFD_8_MD_06INKM_show_str("        ", 0, 8);
  xEventGroupWaitBits(
      event_group_manager_get_handle(kEventGroupManagerTimeIsSync),
      EVENT_GROUP1_TIME_IS_SYNC, pdFALSE, pdFALSE, portMAX_DELAY);

  struct timeval tv;
  struct tm *p_tm;
  uint32_t delay_ms = 1000;
  bool time_blink = false;
  time_t last_sync_sec;
  uint8_t time_str[8];
  while (true) {
    gettimeofday(&tv, NULL);
    p_tm = localtime(&tv.tv_sec);

    if (xQueueReceive(my_clock_queue_handle, &last_sync_sec, 0) == pdTRUE) {
      if (tv.tv_sec - last_sync_sec >= DAY_SEC) {
        delay_ms = 500;
        time_blink = true;
      } else {
        delay_ms = 1000;
        time_blink = false;
      }
    }

    // 定时开关
    if (config.timing_switch_screen.on_Dmin !=
            config.timing_switch_screen.off_Dmin &&
        check_u32_in_range(config.timing_switch_screen.off_Dmin * 60,
                           config.timing_switch_screen.on_Dmin * 60, DAY_SEC,
                           p_tm->tm_hour * 3600 + p_tm->tm_min * 60 +
                               p_tm->tm_sec)) {
      VFD_8_MD_06INKM_show_str("        ", 0, 8);
      vTaskDelay(pdMS_TO_TICKS(
          my_clock_get_gap(p_tm->tm_hour * 3600 + p_tm->tm_min * 60 +
                               p_tm->tm_sec,
                           config.timing_switch_screen.on_Dmin * 60, DAY_SEC) *
          1000));
      continue;
    }

    // 整点提示
    if (tv.tv_sec % 3600 == 0 && config.hour_prompt != 0)
      my_buzzer_custom_tone(4000, 1000, 20);

    if (time_blink) {
      if (tv.tv_usec % 1000000 > 500000) {
        strftime((char *)time_str, 9, "%H:%M:%S", localtime(&tv.tv_sec));
      } else {
        strftime((char *)time_str, 9, "%H %M %S", localtime(&tv.tv_sec));
      }
    } else {
      strftime((char *)time_str, 9, "%H:%M:%S", localtime(&tv.tv_sec));
    }
    VFD_8_MD_06INKM_show_str(time_str, 0, 8);

    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }
}

void my_clock_sync_time_handler(void *event_handler_arg,
                                esp_event_base_t event_base, int32_t event_id,
                                void *event_data) {
  if (event_base != MY_SNTP_EVENT) {
    return;
  }

  xQueueOverwrite(my_clock_queue_handle, event_data);
  // if (event_id == kMySntpSyncSuccess) {
  // } else if (event_id == kMySntpSyncFail) {
  // }
}

void my_clock_save_config(const MyClockConfig_t *p_config) {
  nvs_handle_t nvs_rw_handle;
  esp_err_t esp_ret;
  esp_ret = nvs_open_from_partition(NVS_VFDCLK_PARTITION_NAME, NVS_MY_CLOCK_NS,
                                    NVS_READWRITE, &nvs_rw_handle);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "save config open nvs fail, reason: %s, reboot...",
             esp_err_to_name(esp_ret));
    esp_restart();
  }

  esp_ret = nvs_set_blob(nvs_rw_handle, NVS_KEY_MY_CLOCK_CONFIG, p_config,
                         sizeof(MyClockConfig_t));
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "save config set fail, reason: %s, reboot...",
             esp_err_to_name(esp_ret));
    esp_restart();
  }

  esp_ret = nvs_commit(nvs_rw_handle);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "nvs commit fail, reason: %s, reboot...",
             esp_err_to_name(esp_ret));
    esp_restart();
  }

  nvs_close(nvs_rw_handle);

  ESP_LOGI(TAG, "save config success");
}

void my_clock_get_config(MyClockConfig_t *p_config) {
  const MyClockConfig_t default_config = {
      .timing_switch_screen =
          {
              .on_Dmin = 360,
              .off_Dmin = 0,
          },
      .hour_prompt = 0,
  };
  nvs_handle_t nvs_ro_handle;
  esp_err_t ret;
  ret = nvs_open_from_partition(NVS_VFDCLK_PARTITION_NAME, NVS_MY_CLOCK_NS,
                                NVS_READONLY, &nvs_ro_handle);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "get config open nvs fail, use default config, reason: %s",
             esp_err_to_name(ret));
    memcpy(p_config, &default_config, sizeof(MyClockConfig_t));
    return;
  }

  size_t my_clock_config_size = sizeof(MyClockConfig_t);
  ret = nvs_get_blob(nvs_ro_handle, NVS_KEY_MY_CLOCK_CONFIG, p_config,
                     &my_clock_config_size);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "get config read fail, use default config,reason: %s",
             esp_err_to_name(ret));
    nvs_close(nvs_ro_handle);
    memcpy(p_config, &default_config, sizeof(MyClockConfig_t));
    return;
  }

  nvs_close(nvs_ro_handle);
}

void my_clock_get_config_json(cJSON *config_json_parent) {
  MyClockConfig_t config;
  my_clock_get_config(&config);

  cJSON *my_clock_config_json = cJSON_CreateObject();
  cJSON_AddNumberToObject(my_clock_config_json,
                          JSON_KEY_MY_CLOCK_ON_SCREEN_DMIN,
                          config.timing_switch_screen.on_Dmin);
  cJSON_AddNumberToObject(my_clock_config_json,
                          JSON_KEY_MY_CLOCK_OFF_SCREEN_DMIN,
                          config.timing_switch_screen.off_Dmin);
  cJSON_AddBoolToObject(my_clock_config_json, JSON_KEY_MY_CLOCK_HOURLY_CHIME,
                        config.hour_prompt);

  cJSON_AddItemToObject(config_json_parent, JSON_KEY_MY_CLOCK_MAIN,
                        my_clock_config_json);
}

void my_clock_analysis_config_json(cJSON *config_json_parent) {
  MyClockConfig_t config;

  my_clock_get_config(&config);

  cJSON *my_clock_config_json =
      cJSON_GetObjectItem(config_json_parent, JSON_KEY_MY_CLOCK_MAIN);
  if (my_clock_config_json == NULL) {
    ESP_LOGI(TAG, "config json not found");
    return;
  }

  cJSON *on_screen_dmin_json = cJSON_GetObjectItem(
      my_clock_config_json, JSON_KEY_MY_CLOCK_ON_SCREEN_DMIN);
  cJSON *off_screen_dmin_json = cJSON_GetObjectItem(
      my_clock_config_json, JSON_KEY_MY_CLOCK_OFF_SCREEN_DMIN);
  if (on_screen_dmin_json != NULL && off_screen_dmin_json != NULL) {
    config.timing_switch_screen.on_Dmin =
        (uint16_t)cJSON_GetNumberValue(on_screen_dmin_json);
    config.timing_switch_screen.off_Dmin =
        (uint16_t)cJSON_GetNumberValue(off_screen_dmin_json);
  }

  cJSON *hourly_chime_json =
      cJSON_GetObjectItem(my_clock_config_json, JSON_KEY_MY_CLOCK_HOURLY_CHIME);
  if (hourly_chime_json != NULL) {
    config.hour_prompt = cJSON_IsTrue(hourly_chime_json);
  }

  my_clock_save_config(&config);
}

void my_clock_deinit() { vTaskDelete(my_clock_task_handle); }

void my_clock_init() {
  xTaskCreate(my_clock_task, "my clock task", configMINIMAL_STACK_SIZE * 2,
              NULL, 8, &my_clock_task_handle);
  if (my_clock_task_handle == NULL) {
  }

  my_clock_queue_handle = xQueueCreate(1, sizeof(time_t));
  if (my_clock_queue_handle == NULL) {
  }

  esp_event_handler_register(MY_SNTP_EVENT, kMySntpSyncSuccess,
                             my_clock_sync_time_handler, NULL);
  esp_event_handler_register(MY_SNTP_EVENT, kMySntpSyncFail,
                             my_clock_sync_time_handler, NULL);
}