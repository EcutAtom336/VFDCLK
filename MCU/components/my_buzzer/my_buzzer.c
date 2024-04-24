#include "my_buzzer.h"

#include "my_event_group.h"

#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"

static QueueHandle_t my_buzzer_queue_handle = NULL;
static TaskHandle_t my_buzzer_server_handle = NULL;
// static SemaphoreHandle_t my_buzzer_mutex_handle = NULL;

// extern uint8_t song_qi3_feng1_le3[0] asm("_binary_qi3_feng1_le3_start");

typedef enum {
  MY_BUZZER_TONE_TYPE_INTERNAL,
  MY_BUZZER_TONE_TYPE_CUSTOM,
} my_buzzer_tone_type_t;
typedef struct {
  union {
    uint16_t *p_freq;
    uint16_t custom_freq;
  };
  union {
    uint16_t *p_duration_ms;
    uint16_t custom_duration_ms;
  };
  uint16_t len;
} my_buzzer_tone_info_t;
typedef struct {
  my_buzzer_tone_info_t tone_info;
  my_buzzer_tone_type_t tone_type;
  uint8_t volume;
} my_buzzer_affair_info_t;

static uint16_t freq_start_tone[] = {500, 750, 1000};
static uint16_t duration_ms_start_tone[] = {200, 200, 200};

static uint16_t freq_warning_tone[] = {400};
static uint16_t duration_ms_warning_tone[] = {1000};

static uint16_t *internal_tone_freq[] = {freq_start_tone, freq_warning_tone};
static uint16_t *internal_tone_duration_ms[] = {duration_ms_start_tone,
                                                duration_ms_warning_tone};
static uint16_t internal_tone_len[] = {3, 1};

void my_buzzer_tone(my_buzzer_sound_name_t tone_name, uint8_t volume) {
  if (volume == 0)
    return;
  my_buzzer_affair_info_t affair_info = {
      .volume = volume,
      .tone_type = MY_BUZZER_TONE_TYPE_INTERNAL,
  };
  affair_info.tone_info.len = internal_tone_len[tone_name];
  affair_info.tone_info.p_duration_ms = internal_tone_duration_ms[tone_name];
  affair_info.tone_info.p_freq = internal_tone_freq[tone_name];

  xQueueSend(my_buzzer_queue_handle, &affair_info, portMAX_DELAY);
}
void my_buzzer_custom_tone(uint16_t freq, uint16_t duration_ms,
                           uint8_t volume) {
  if (volume == 0)
    return;
  my_buzzer_affair_info_t affair_info = {
      .volume = volume,
      .tone_type = MY_BUZZER_TONE_TYPE_CUSTOM,
  };
  affair_info.tone_info.len = 1;
  affair_info.tone_info.custom_duration_ms = duration_ms;
  affair_info.tone_info.custom_freq = freq;

  xQueueSend(my_buzzer_queue_handle, &affair_info, portMAX_DELAY);
}
void my_buzzer_handler() {
  my_buzzer_affair_info_t affair_info;
  while (1) {
    xQueueReceive(my_buzzer_queue_handle, &affair_info, portMAX_DELAY);

    if (affair_info.tone_type == MY_BUZZER_TONE_TYPE_CUSTOM) {
      ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0,
                    affair_info.tone_info.custom_freq);
      ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                               affair_info.volume, 0);
      vTaskDelay(pdMS_TO_TICKS(affair_info.tone_info.custom_duration_ms));
      ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 0);
    } else if (affair_info.tone_type == MY_BUZZER_TONE_TYPE_INTERNAL) {
      ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0,
                    *affair_info.tone_info.p_freq++);
      ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                               affair_info.volume, 0);
      vTaskDelay(pdMS_TO_TICKS(*affair_info.tone_info.p_duration_ms++));
      for (size_t i = 0; i < affair_info.tone_info.len - 1; i++) {
        ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0,
                      *affair_info.tone_info.p_freq++);
        vTaskDelay(pdMS_TO_TICKS(*affair_info.tone_info.p_duration_ms++));
      }
      ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 0);
    }
  }
}
void my_buzzer_init() {
  ledc_timer_config_t my_ledc_timer_config = {
      .clk_cfg = LEDC_AUTO_CLK,
      .duty_resolution = LEDC_TIMER_8_BIT,
      .freq_hz = 4000,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_num = LEDC_TIMER_0,
  };
  ledc_timer_config(&my_ledc_timer_config);
  ledc_channel_config_t my_ledc_channel_config = {
      .channel = LEDC_CHANNEL_0,
      .duty = 0,
      .flags.output_invert = 0,
      .gpio_num = 10,
      .hpoint = 0,
      .intr_type = LEDC_INTR_DISABLE,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_sel = LEDC_TIMER_0,
  };
  ledc_channel_config(&my_ledc_channel_config);
  ledc_fade_func_install(ESP_INTR_FLAG_SHARED);
  my_buzzer_queue_handle = xQueueCreate(16, sizeof(my_buzzer_affair_info_t));
  xTaskCreate(my_buzzer_handler, "my buzzer server task",
              configMINIMAL_STACK_SIZE, NULL, 5, &my_buzzer_server_handle);
  // my_buzzer_timer_handle = xTimerCreate("my buzzer timer",
  //                                       pdMS_TO_TICKS(1000),
  //                                       pdFALSE,
  //                                       NULL,
  //                                       my_buzzer_server_timer);
  // my_buzzer_mutex_handle = xSemaphoreCreateMutex();
}
