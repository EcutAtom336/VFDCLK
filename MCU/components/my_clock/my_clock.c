#include "my_clock.h"
//
#include "VFD_8_MD_06INKM.h"
#include "life_cycle_manage.h"
#include "my_buzzer.h"
#include "my_common.h"
#include "my_sntp.h"
//
#include "button_gpio.h"
#include "cJSON.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portable.h"
#include "iot_button.h"
#include "nvs_flash.h"
//
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
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

typedef struct {
  uint32_t hour_prompt : 1;
  struct {
    uint16_t on_Dmin;
    uint16_t off_Dmin;
  } timing_switch_screen;
} MyClockConfig_t;

typedef enum {
  kMyClockMainMsgTypePause,
  kMyClockMainMsgTypeResume,
  kMyClockMainMsgTypeDelete,
  kMyClockMainMsgTypeShow,
  kMyClockMainMsgTypeHourPrompt,
} MyClockMainMsgType_t;

typedef struct {
  MyClockMainMsgType_t type;
  union {
    struct {
      char str[8];
    } show;
  };
} MyClockMainMsg_t;

typedef struct {
  MyClockConfig_t config; //
  TaskHandle_t main_task_handle;
  QueueHandle_t main_queue_handle;
  TaskHandle_t clk_task_handle;
  button_handle_t iot_button_handle;
} MyClockContext_t;

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
  return start < end ? val >= start && val < end : !(val >= end && val < start);
}

static inline uint32_t my_clock_get_gap(uint32_t from, uint32_t to,
                                        uint32_t max) {
  return from <= to ? to - from : max - from + to;
}

void my_clock_clk_task(void *param) {
  MyClockContext_t *context_handle = (MyClockContext_t *)param;

  struct timeval tv;
  struct tm *p_tm;
  uint32_t delay_ms = 1000;

  uint32_t notify_val = 0;
  TickType_t wait_notify_tick = portMAX_DELAY;

  MyClockMainMsg_t main_msg;

  while (true) {
    if (xTaskNotifyWaitIndexed(1, 0, 0xffffffff, &notify_val,
                               wait_notify_tick) == pdTRUE) {
      /*
       *| ... | bit2       | bit1      | bit0     |
       *| ... | on destroy | on resume | on pause |
       */
      if (notify_val & ((uint32_t)1) << 0) {
        wait_notify_tick = portMAX_DELAY;
      }
      if (notify_val & ((uint32_t)1) << 1) {
        wait_notify_tick = 0;
      }
      if (notify_val & ((uint32_t)1) << 2) {
        vTaskDelete(NULL);
      }
      continue;
    }

    gettimeofday(&tv, NULL);
    p_tm = localtime(&tv.tv_sec);

    // 定时开关
    if (context_handle->config.timing_switch_screen.on_Dmin !=
            context_handle->config.timing_switch_screen.off_Dmin &&
        check_u32_in_range(
            context_handle->config.timing_switch_screen.off_Dmin * 60,
            context_handle->config.timing_switch_screen.on_Dmin * 60, DAY_SEC,
            p_tm->tm_hour * 3600 + p_tm->tm_min * 60 + p_tm->tm_sec)) {
      ESP_LOGI(TAG, "open clock after %lu s",
               my_clock_get_gap(
                   p_tm->tm_hour * 3600 + p_tm->tm_min * 60 + p_tm->tm_sec,
                   context_handle->config.timing_switch_screen.on_Dmin * 60,
                   DAY_SEC));
      main_msg.type = kMyClockMainMsgTypeShow;
      memcpy(main_msg.show.str, "        ", 8);
      xQueueSend(context_handle->main_queue_handle, &main_msg, portMAX_DELAY);
      vTaskDelay(pdMS_TO_TICKS(
          my_clock_get_gap(
              p_tm->tm_hour * 3600 + p_tm->tm_min * 60 + p_tm->tm_sec,
              context_handle->config.timing_switch_screen.on_Dmin * 60,
              DAY_SEC) *
          1000));
      continue;
    }

    // 整点提示
    if (tv.tv_sec % 3600 == 0 && context_handle->config.hour_prompt != 0) {
      main_msg.type = kMyClockMainMsgTypeHourPrompt;
      xQueueSend(context_handle->main_queue_handle, &main_msg, portMAX_DELAY);
    }

    main_msg.type = kMyClockMainMsgTypeShow;
    if (tv.tv_sec - my_sntp_get_last_sync_time() > DAY_SEC) {
      delay_ms = 500;
      strftime((char *)main_msg.show.str, 9,
               tv.tv_usec % 1000000 > 500000 ? "%H:%M:%S" : "%H %M %S",
               localtime(&tv.tv_sec));
    } else {
      delay_ms = 1000;
      strftime((char *)main_msg.show.str, 9, "%H:%M:%S", localtime(&tv.tv_sec));
    }

    xQueueSend(context_handle->main_queue_handle, &main_msg, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }
}

void my_clock_main_task(void *param) {
  MyClockContext_t *context = (MyClockContext_t *)param;

  MyClockMainMsg_t main_msg;

  while (true) {
    xQueueReceive(context->main_queue_handle, &main_msg, portMAX_DELAY);
    if (main_msg.type == kMyClockMainMsgTypePause) {
      VFD_8_MD_06INKM_release();
    } else if (main_msg.type == kMyClockMainMsgTypeResume) {
      VFD_8_MD_06INKM_aquire(portMAX_DELAY);
    } else if (main_msg.type == kMyClockMainMsgTypeDelete) {
      context->main_task_handle = NULL;
      vTaskDelete(NULL);
    } else if (main_msg.type == kMyClockMainMsgTypeShow) {
      VFD_8_MD_06INKM_show_str(main_msg.show.str, 0, 8);
    } else if (main_msg.type == kMyClockMainMsgTypeHourPrompt) {
      my_buzzer_custom_tone(4000, 1000, 10);
    }
  }
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
  esp_err_t esp_err;
  esp_err = nvs_open_from_partition(NVS_VFDCLK_PARTITION_NAME, NVS_MY_CLOCK_NS,
                                    NVS_READONLY, &nvs_ro_handle);
  if (esp_err != ESP_OK) {
    ESP_LOGW(TAG, "get config open nvs fail, use default config, reason: %s",
             esp_err_to_name(esp_err));
    memcpy(p_config, &default_config, sizeof(MyClockConfig_t));
    return;
  }

  size_t my_clock_config_size = sizeof(MyClockConfig_t);
  esp_err = nvs_get_blob(nvs_ro_handle, NVS_KEY_MY_CLOCK_CONFIG, p_config,
                         &my_clock_config_size);
  if (esp_err != ESP_OK) {
    ESP_LOGW(TAG, "get config read fail, use default config,reason: %s",
             esp_err_to_name(esp_err));
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

void start_setting_wrap(void *button_handle, void *usr_data) {
  ESP_LOGI(TAG, "start setting");
  life_cycle_manage_create(MySetting);
}

void my_clock_create_fail_clear(void *pv_context) {
  if (pv_context == NULL) {
    return;
  }

  MyClockContext_t *context_handle = (MyClockContext_t *)pv_context;

  if (context_handle->main_queue_handle != NULL) {
    vQueueDelete(context_handle->main_queue_handle);
  }

  if (context_handle->main_task_handle != NULL) {
    vTaskDelete(context_handle->main_task_handle);
  }

  if (context_handle->clk_task_handle) {
    vTaskDelete(context_handle->clk_task_handle);
  }

  if (context_handle->iot_button_handle != NULL) {
    iot_button_delete(context_handle->iot_button_handle);
  }

  vPortFree(pv_context);
}

void *my_clock_on_create() {
  MyClockContext_t *context_handle = pvPortMalloc(sizeof(MyClockContext_t));

  ESP_LOGI(TAG, "on create");

  if (context_handle == NULL) {
    ESP_LOGW(TAG, "malloc fail");
    my_clock_create_fail_clear(context_handle);
    return NULL;
  }

  // context->clock_wakeup_timer_handle =
  //     xTimerCreate(NULL, pdMS_TO_TICKS(1), pdFALSE, (void *)0, NULL);
  // if (context->clock_wakeup_timer_handle == NULL) {
  //   ESP_LOGW(TAG, "clock wakeup timer create fail");
  //   return NULL;
  // }

  context_handle->main_queue_handle = xQueueCreate(4, sizeof(MyClockMainMsg_t));
  if (context_handle->main_queue_handle == NULL) {
    ESP_LOGW(TAG, "create main queue fail");
    my_clock_create_fail_clear(context_handle);
    return NULL;
  }

  xTaskCreate(my_clock_main_task, NULL, configMINIMAL_STACK_SIZE * 2,
              context_handle, 16, &context_handle->main_task_handle);
  if (context_handle->main_task_handle == NULL) {
    ESP_LOGW(TAG, "create main task fail");
    my_clock_create_fail_clear(context_handle);
    return NULL;
  }

  xTaskCreate(my_clock_clk_task, NULL, configMINIMAL_STACK_SIZE * 2,
              context_handle, 12, &context_handle->clk_task_handle);
  if (context_handle->clk_task_handle == NULL) {
    ESP_LOGW(TAG, "create clk task fail");
    my_clock_create_fail_clear(context_handle);
    return NULL;
  }

  button_config_t button_config = {
      .type = BUTTON_TYPE_GPIO,
      .gpio_button_config =
          {
              .gpio_num = 4,
              .active_level = 1,
          },
  };
  context_handle->iot_button_handle = iot_button_create(&button_config);
  if (context_handle->iot_button_handle == NULL) {
    ESP_LOGW(TAG, "create iot button fail");
    my_clock_create_fail_clear(context_handle);
    return NULL;
  }

  return context_handle;
}

void my_clock_on_resume(void *pv_context) {
  MyClockContext_t *context_handle = (MyClockContext_t *)pv_context;

  esp_err_t esp_err;

  ESP_LOGI(TAG, "on resume");

  my_clock_get_config(&context_handle->config);

  esp_err =
      iot_button_register_cb(context_handle->iot_button_handle,
                             BUTTON_LONG_PRESS_START, start_setting_wrap, NULL);
  ESP_ERROR_CHECK(esp_err);

  MyClockMainMsg_t main_msg = {
      .type = kMyClockMainMsgTypeResume,
  };
  xQueueSendToFront(context_handle->main_queue_handle, &main_msg,
                    portMAX_DELAY);
  xTaskNotifyIndexed(context_handle->clk_task_handle, 1, ((uint32_t)1) << 1,
                     eSetBits);
}

void my_clock_on_pause(void *pv_context) {
  MyClockContext_t *context_handle = (MyClockContext_t *)pv_context;

  ESP_LOGI(TAG, "on pause");

  ESP_ERROR_CHECK(iot_button_unregister_cb(context_handle->iot_button_handle,
                                           BUTTON_LONG_PRESS_START));

  xQueueReset(context_handle->main_queue_handle);
  MyClockMainMsg_t main_msg = {
      .type = kMyClockMainMsgTypePause,
  };
  xQueueSendToFront(context_handle->main_queue_handle, &main_msg,
                    portMAX_DELAY);
  xTaskNotifyIndexed(context_handle->clk_task_handle, 1, ((uint32_t)1) << 0,
                     eSetBits);
}

void my_clock_on_destroy(void *pv_context) {
  MyClockContext_t *context_handle = (MyClockContext_t *)pv_context;

  ESP_LOGI(TAG, "on destroy");

  esp_err_t esp_err;

  esp_err = iot_button_delete(context_handle->iot_button_handle);
  ESP_ERROR_CHECK(esp_err);

  MyClockMainMsg_t main_msg = {
      .type = kMyClockMainMsgTypeDelete,
  };
  xQueueSendToFront(context_handle->main_queue_handle, &main_msg,
                    portMAX_DELAY);
  xTaskNotifyIndexed(context_handle->clk_task_handle, 1, ((uint32_t)1) << 2,
                     eSetBits);

  vPortFree(pv_context);
}
