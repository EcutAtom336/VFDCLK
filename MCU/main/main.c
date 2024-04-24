// esp32 sdk api 封装程度很高，千万不要尝试以 通用 为目的再次封装，会死的很惨捏
#include "VFD_8_MD_06INKM.h"
#include "my_buzzer.c"
#include "my_clock.h"
#include "my_event_group.h"
#include "my_setting.h"
#include "my_sntp.h"
#include "my_spi.h"
#include "my_wifi.h"
//
// #include "driver/gpio_filter.h"
#include "button_gpio.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "iot_button.h"
#include "soc/io_mux_reg.h"
// #include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#define TAG "main"

void app_main(void) {
  esp_err_t esp_ret;

  if (nvs_flash_init() != ESP_OK) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }
  if (nvs_flash_init_partition("vfdclk") != ESP_OK) {
    ESP_ERROR_CHECK(nvs_flash_erase_partition("vfdclk"));
    ESP_ERROR_CHECK(nvs_flash_init_partition("vfdclk"));
  }
  // ESP_ERROR_CHECK(nvs_flash_erase_partition("vfdclk"));
  // ESP_ERROR_CHECK(nvs_flash_init_partition("vfdclk"));

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  gpio_iomux_out(5, FUNC_MTDI_GPIO5, false);
  gpio_config_t io_config = {
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = (uint64_t)1 << 5,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .pull_up_en = GPIO_PULLUP_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&io_config));
  ESP_ERROR_CHECK(gpio_set_level(5, 1));

  setenv("TZ", "CST-8", 1);
  tzset();
  event_group_manager_init();
  my_sntp_init();
  my_spi_init();
  VFD_8_MD_06INKM_init();
  my_buzzer_init();
  my_wifi_init();
  my_setting_init();
  my_clock_init();

  // my_buzzer_tone(MY_BUZZER_TONE_NAME_START, 10);

  heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
}