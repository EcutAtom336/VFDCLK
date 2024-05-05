#include "life_cycle_manage.h"
//
#include "life_cycle_manage_interface.h"
//
#include "atom336.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "my_clock.h"
#include "my_setting.h"
//
#include <stdint.h>
#include <string.h>

#define TAG "life cycle manage"

#define MAX_ACTIVITY_NUM 5

typedef enum {
  kLifeCycleManageCreate,
  kLifeCycleManageDelete,
} ManageAction_t;

typedef struct {
  ManageAction_t action;
  int32_t activity_num;
} ActivityManageInfo_t;

ActivityFunc_t activity_func_table[ActivityMax] = {
    [Atom336] =
        {
            .on_create = atom336_on_create,
            .on_pause = atom336_on_pause,
            .on_resume = atom336_on_resume,
            .on_destroy = atom336_on_destroy,
        },
    [MySetting] =
        {
            .on_create = my_setting_on_create,
            .on_pause = my_setting_on_pause,
            .on_resume = my_setting_on_resume,
            .on_destroy = my_setting_on_destroy,
        },
    [MyClock] =
        {
            .on_create = my_clock_on_create,
            .on_pause = my_clock_on_pause,
            .on_resume = my_clock_on_resume,
            .on_destroy = my_clock_on_destroy,
        },
};

TaskHandle_t manager_main_task_handle;
QueueHandle_t manager_ctrl_queue_handle;

void life_cycle_manage_main_task() {
  static void *context_handle_stack[MAX_ACTIVITY_NUM];
  static int32_t activity_stack[MAX_ACTIVITY_NUM];
  int32_t current_index = -1;
  ActivityManageInfo_t manage_info;
  while (true) {
    xQueueReceive(manager_ctrl_queue_handle, &manage_info, portMAX_DELAY);
    if (manage_info.action == kLifeCycleManageCreate) {
      // if has activity, run on pause
      if (current_index >= 0) {
        activity_func_table[activity_stack[current_index]].on_pause(
            context_handle_stack[current_index]);
      }

      current_index++;

      // save func num
      activity_stack[current_index] = manage_info.activity_num;
      // run on create and save context handle
      context_handle_stack[current_index] =
          activity_func_table[activity_stack[current_index]].on_create();
      // check create result
      if (context_handle_stack[current_index] == NULL) {
        ESP_LOGW(TAG, "create activity fail");
        current_index--;
        continue;
      }
      // run on resume
      activity_func_table[activity_stack[current_index]].on_resume(
          context_handle_stack[current_index]);
    } else if (manage_info.action == kLifeCycleManageDelete) {
      // run on pause
      activity_func_table[activity_stack[current_index]].on_pause(
          context_handle_stack[current_index]);
      // run on destroy
      activity_func_table[activity_stack[current_index]].on_destroy(
          context_handle_stack[current_index]);

      current_index--;

      // if has activity, run on resume
      if (current_index >= 0) {
        activity_func_table[activity_stack[current_index]].on_resume(
            context_handle_stack[current_index]);
      }
    }
  }
}

void life_cycle_manage_create(int32_t activity_num) {
  ActivityManageInfo_t manage_info = {
      .action = kLifeCycleManageCreate,
      .activity_num = activity_num,
  };
  xQueueSend(manager_ctrl_queue_handle, &manage_info, portMAX_DELAY);
}

void life_cycle_manage_delete() {
  ActivityManageInfo_t manage_info = {
      .action = kLifeCycleManageDelete,
      .activity_num = -1,
  };
  xQueueSend(manager_ctrl_queue_handle, &manage_info, portMAX_DELAY);
}

void life_cycle_manage_init() {
  manager_ctrl_queue_handle = xQueueCreate(4, sizeof(ActivityManageInfo_t));
  if (manager_ctrl_queue_handle == NULL) {
    ESP_LOGE(TAG, "life cycle ctrl queue create fail, reboot...");
    esp_restart();
  }

  xTaskCreate(life_cycle_manage_main_task, "main task",
              configMINIMAL_STACK_SIZE * 4, NULL, 16,
              &manager_main_task_handle);
  if (manager_main_task_handle == NULL) {
    ESP_LOGE(TAG, "create life cycle main task fail, reboot...");
    esp_restart();
  }
}
