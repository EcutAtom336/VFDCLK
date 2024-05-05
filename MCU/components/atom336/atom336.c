#include "atom336.h"
//
#include "VFD_8_MD_06INKM.h"
#include "life_cycle_manage.h"
//
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//
#include <string.h>

#define TAG "atom336"

typedef struct {
  TaskHandle_t main_task_handle;
  QueueHandle_t main_queue_handle;
  uint32_t is_exists : 1;
} Atom336Context_t;

typedef enum {
  kAtom336MainMsgTypeResume,
  kAtom336MainMsgTypePause,
  kAtom336MainMsgTypeDelete,
} Atom336MainMsgType_t;

typedef struct {
  Atom336MainMsgType_t type;
} Atom336MainMsg_t;

void atom336_main_task(void *param) {
  Atom336Context_t *context_handle = (Atom336Context_t *)param;

  uint32_t notify_val;
  TickType_t wait_notify_tick = portMAX_DELAY;

  VFD_8_MD_06INKM_aquire(portMAX_DELAY);

  char str[8];
  char str_atom336[7] = {"Atom336"};
  char str_VFDCLK[7] = {"VFD CLK"};
  for (size_t i = 1; i < 8; i++) {
    memset(str, ' ', sizeof(str));
    memcpy(str, str_atom336, i);
    VFD_8_MD_06INKM_show_str(str, 0, 8);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  vTaskDelay(pdMS_TO_TICKS(3000));
  for (size_t i = 1; i < 8; i++) {
    memcpy(str, str_atom336, sizeof(str_atom336));
    memset(str, ' ', i);
    VFD_8_MD_06INKM_show_str(str, 0, 8);
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  for (size_t i = 1; i < 8; i++) {
    memset(str, ' ', sizeof(str));
    memcpy(str, str_VFDCLK, i);
    VFD_8_MD_06INKM_show_str(str, 0, 8);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  vTaskDelay(pdMS_TO_TICKS(3000));
  for (size_t i = 1; i < 8; i++) {
    memcpy(str, str_VFDCLK, sizeof(str_VFDCLK));
    memset(str, ' ', i);
    VFD_8_MD_06INKM_show_str(str, 0, 8);
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  VFD_8_MD_06INKM_release();

  life_cycle_manage_delete();
  life_cycle_manage_create(MyClock);

  vTaskDelete(NULL);

  Atom336MainMsg_t main_msg;

  while (true) {
    xQueueReceive(context_handle->main_queue_handle, &main_msg, portMAX_DELAY);
  }
}

void atom336_create_fail_clear(void *pv_context) {
  Atom336Context_t *context_handle = (Atom336Context_t *)pv_context;

  if (context_handle->main_queue_handle != NULL) {
    vTaskDelete(context_handle->main_task_handle);
  }

  if (context_handle->main_queue_handle != NULL) {
    vQueueDelete(context_handle->main_queue_handle);
  }
}

void *atom336_on_create() {
  static Atom336Context_t sigleton_context;

  if (sigleton_context.is_exists == 1) {
    ESP_LOGW(TAG, "an instance already exists");
    return NULL;
  }

  sigleton_context.main_queue_handle =
      xQueueCreate(4, sizeof(Atom336MainMsg_t));
  if (sigleton_context.main_queue_handle == NULL) {
    ESP_LOGW(TAG, "create main queue fail");
    atom336_create_fail_clear(&sigleton_context);
    return NULL;
  }

  xTaskCreate(atom336_main_task, NULL, configMINIMAL_STACK_SIZE * 2,
              &sigleton_context, 16, &sigleton_context.main_task_handle);
  if (sigleton_context.main_task_handle == NULL) {
    ESP_LOGE(TAG, "create main task fail");
    atom336_create_fail_clear(&sigleton_context);
    return NULL;
  }

  return &sigleton_context;
}

void atom336_on_pause(void *pv_context) {
  Atom336Context_t *context = (Atom336Context_t *)pv_context;

  ESP_LOGI(TAG, "on pause");

  xTaskNotifyIndexed(context->main_task_handle, 1, ((uint32_t)1) << 0,
                     eSetBits);
}

void atom336_on_resume(void *pv_context) {
  Atom336Context_t *context = (Atom336Context_t *)pv_context;

  ESP_LOGI(TAG, "on resume");

  xTaskNotifyIndexed(context->main_task_handle, 1, ((uint32_t)1) << 1,
                     eSetBits);
}

void atom336_on_destroy(void *pv_context) {
  Atom336Context_t *context = (Atom336Context_t *)pv_context;

  ESP_LOGI(TAG, "on destroy");

  xTaskNotifyIndexed(context->main_task_handle, 1, ((uint32_t)1) << 2,
                     eSetBits);

  context->is_exists = 0;
}
