#include "VFD_8_MD_06INKM.h"
#include "life_cycle_manage.h"
#include "my_buzzer.c"
#include "my_sntp.h"
#include "my_spi.h"
#include "my_wifi.h"
//
#include "button_gpio.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "iot_button.h"
#include "nvs_flash.h"

#define TAG "main"

void app_main(void) {
  esp_err_t esp_err;

  esp_err = nvs_flash_init();
  if (esp_err != ESP_OK) {
    ESP_LOGW(
        TAG,
        "initialize default nvs partition fail, reason: %s, erase default nvs "
        "partition",
        esp_err_to_name(esp_err));
    esp_err = nvs_flash_erase();
    if (esp_err != ESP_OK) {
      ESP_LOGE(TAG, "erase default nvs partition fail, reason: %s, reboot...",
               esp_err_to_name(esp_err));
      esp_restart();
      return;
    }
    ESP_LOGI(TAG, "erase default nvs partition success");
    esp_err = nvs_flash_init();
    if (esp_err != ESP_OK) {
      ESP_LOGE(
          TAG,
          "initialize default nvs partition fail again, reason: %s, reboot...",
          esp_err_to_name(esp_err));
      esp_restart();
      return;
    }
  }
  ESP_LOGI(TAG, "initialize default nvs partition success");

  esp_err = nvs_flash_init_partition("vfdclk");
  if (esp_err != ESP_OK) {
    ESP_LOGW(
        TAG,
        "initialize vfdclk nvs partition fail, reason: %s, erase vfdclk nvs "
        "partition",
        esp_err_to_name(esp_err));
    esp_err = nvs_flash_erase_partition("");
    if (esp_err != ESP_OK) {
      ESP_LOGE(TAG, "erase vfdclk nvs partition fail, reason: %s, reboot...",
               esp_err_to_name(esp_err));
      esp_restart();
      return;
    }
    ESP_LOGI(TAG, "erase vfdclk nvs partition success");
    esp_err = nvs_flash_init_partition("vfdclk");
    if (esp_err != ESP_OK) {
      ESP_LOGE(
          TAG,
          "initialize vfdclk nvs partition fail again, reason: %s, reboot...",
          esp_err_to_name(esp_err));
      esp_restart();
      return;
    }
  }
  ESP_LOGI(TAG, "vfdclk nvs partition init success");

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  setenv("TZ", "CST-8", 1);
  tzset();
  my_sntp_init();
  my_spi_init();
  VFD_8_MD_06INKM_init();
  // VFD_8_MD_06INKM_aquire(portMAX_DELAY);
  // VFD_8_MD_06INKM_show_str("12:00:00", 0, 8);
  // return;
  my_buzzer_init();
  my_wifi_init();
  life_cycle_manage_init();
  life_cycle_manage_create(Atom336);

  // my_buzzer_tone(MY_BUZZER_TONE_NAME_START, 10);

  heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
}