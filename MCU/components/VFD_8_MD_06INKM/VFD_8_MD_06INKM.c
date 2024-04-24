#include "VFD_8_MD_06INKM.h"
//
#include "VFD_type_defs.h"
//
#include "cJSON.h"
#include "common.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
//
#include <string.h>

#define TAG "VFD_8_MD_06INKM"

#define NVS_VFD_NS "vfd"
#define NVS_KEY_VFD_CONFIG "config"

#define JSON_KEY_VFD_MAIN "myVfd"
#define JSON_KEY_VFD_BRIGHTNESS "brightness"

// 亮度0-240
// #define VFD_8_MD_06INKM_DEFAULT_BRT 5

#define VFD_8_MD_06INKM_SPI SPI2_HOST
#define VFD_8_MD_06INKM_CS 1

#define VFD_8_MD_06INKM_GEN_CMD_W_DCRAM(DCRAM_address)                         \
  (uint8_t)((DCRAM_address & 0x1F) | 0x20)
#define VFD_8_MD_06INKM_GEN_CMD_W_CGRAM(CGRAM_address)                         \
  (uint8_t)((CGRAM_address & 0x07) | 0x40)
#define VFD_8_MD_06INKM_GEN_CMD_W_ADRAM(ADRAM_address)                         \
  (uint8_t)((ADRAM_address & 0x1F) | 0x60)
#define VFD_8_MD_06INKM_CMD_S_DISP_TIM (uint8_t)0xE0
#define VFD_8_MD_06INKM_DEFAULT_DISP_TIM (uint8_t)0x07
#define VFD_8_MD_06INKM_GEN_CMD_W_URAM(URAM_address)                           \
  (uint8_t)((URAM_address & 0x07) | 0x80)
#define VFD_8_MD_06INKM_CMD_S_DISP_BRT (uint8_t)0xE4
#define VFD_8_MD_06INKM_CMD_DISP_OFF (uint8_t)0xEA
#define VFD_8_MD_06INKM_CMD_DISP_ON (uint8_t)0xE8
#define VFD_8_MD_06INKM_CMD_STAND_BY (uint8_t)0xED
#define VFD_8_MD_06INKM_CMD_ACTIVE (uint8_t)0xEC

typedef struct {
  uint8_t brightness;
} VfdConfig_t;

spi_device_handle_t VFD_8_MD_06INKM_handle;
TaskHandle_t auto_brightness_task_handle;
SemaphoreHandle_t VFD_semaphore_handle;

void VFD_8_MD_06INKM_set_brightness(uint8_t brightness);

// 由于数据量较小，使用中断传输的开销相对较大，所以使用轮询传输

// uint8_t VFD_8_MD_06INKM_custom_char_table[8][5] =
//     {{0, 0, 0, 0, 0},
//      {0, 0, 0, 0, 0},
//      {0, 0, 0, 0, 0},
//      {0, 0, 0, 0, 0},
//      {0, 0, 0, 0, 0},
//      {0, 0, 0, 0, 0},
//      {0, 0, 0, 0, 0},
//      {0, 0, 0, 0, 0}};

void VFD_get_config(VfdConfig_t *p_config) {
  const VfdConfig_t default_config = {
      .brightness = 0,
  };

  nvs_handle_t nvs_ro_handle;
  esp_err_t ret;
  ret = nvs_open_from_partition(NVS_VFDCLK_PARTITION_NAME, NVS_VFD_NS,
                                NVS_READONLY, &nvs_ro_handle);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "get config open nvs fail, use default config, reason: %s",
             esp_err_to_name(ret));
    memcpy(p_config, &default_config, sizeof(VfdConfig_t));
    return;
  }

  size_t my_clock_config_size = sizeof(VfdConfig_t);
  ret = nvs_get_blob(nvs_ro_handle, NVS_KEY_VFD_CONFIG, p_config,
                     &my_clock_config_size);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "get config read fail, use default config,reason: %s",
             esp_err_to_name(ret));
    nvs_close(nvs_ro_handle);
    memcpy(p_config, &default_config, sizeof(VfdConfig_t));
    return;
  }

  nvs_close(nvs_ro_handle);
}

void VFD_save_config(const VfdConfig_t *p_config) {
  nvs_handle_t nvs_rw_handle;
  esp_err_t esp_ret;
  esp_ret = nvs_open_from_partition(NVS_VFDCLK_PARTITION_NAME, NVS_VFD_NS,
                                    NVS_READWRITE, &nvs_rw_handle);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "save config open nvs fail, reason: %s, reboot...",
             esp_err_to_name(esp_ret));
    esp_restart();
  }

  esp_ret = nvs_set_blob(nvs_rw_handle, NVS_KEY_VFD_CONFIG, p_config,
                         sizeof(VfdConfig_t));
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

void VFD_get_config_json(cJSON *config_json_parent) {
  VfdConfig_t config;
  VFD_get_config(&config);

  cJSON *VFD_config_json = cJSON_CreateObject();
  cJSON_AddNumberToObject(VFD_config_json, JSON_KEY_VFD_BRIGHTNESS,
                          config.brightness);

  cJSON_AddItemToObject(config_json_parent, JSON_KEY_VFD_MAIN, VFD_config_json);
}

void VFD_analysis_config_json(cJSON *config_json_parent) {
  VfdConfig_t config;
  VFD_get_config(&config);

  cJSON *VFD_config_json =
      cJSON_GetObjectItem(config_json_parent, JSON_KEY_VFD_MAIN);
  if (VFD_config_json == NULL) {
    ESP_LOGI(TAG, STRING_CONFIG_JSON_NOT_FOUND);
    return;
  }

  cJSON *brightness_json =
      cJSON_GetObjectItem(VFD_config_json, JSON_KEY_VFD_BRIGHTNESS);
  if (brightness_json != NULL) {
    config.brightness = (uint8_t)cJSON_GetNumberValue(brightness_json);
  }

  VFD_save_config(&config);
}

void VFD_auto_brightness_task() {
  VfdConfig_t config;
  VFD_get_config(&config);

  esp_err_t esp_ret;

  adc_cali_handle_t adc_cali_handle;
  adc_cali_scheme_ver_t adc_cali_scheme_ver;
  ESP_ERROR_CHECK(adc_cali_check_scheme(&adc_cali_scheme_ver));
  adc_cali_curve_fitting_config_t adc_cali_curve_fitting_config = {
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .chan = ADC_CHANNEL_2,
      .unit_id = ADC_UNIT_1,
  };
  ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(
      &adc_cali_curve_fitting_config, &adc_cali_handle));

  adc_oneshot_unit_init_cfg_t adc_oneshot_config = {
      .unit_id = ADC_UNIT_1,
      .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  adc_oneshot_unit_handle_t adc_oneshot_handle;
  ESP_ERROR_CHECK(
      adc_oneshot_new_unit(&adc_oneshot_config, &adc_oneshot_handle));

  adc_oneshot_chan_cfg_t adc_oneshot_cannel_config = {
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_oneshot_handle, ADC_CHANNEL_2,
                                             &adc_oneshot_cannel_config));

  int32_t adc_raw;
  int32_t adc_vol;
  uint8_t brightness;

  while (true) {
    adc_oneshot_read(adc_oneshot_handle, ADC_CHANNEL_2, &adc_raw);

    ESP_ERROR_CHECK(
        adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &adc_vol));

    // brightness = 240 * (adc_raw / 4095);
    brightness = adc_raw / (4095 / 20);

    if (brightness == 0)
      brightness = 1;

    // ESP_LOGI(TAG, "adc raw: %ld, adc vol: %ld, brightness: %u", adc_raw,
    //          adc_vol, brightness);

    VFD_8_MD_06INKM_set_brightness(brightness);

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void VFD_8_MD_06INKM_write(uint8_t *p_data, uint8_t len) {
  spi_transaction_t spi_transaction = {
      .flags = SPI_TRANS_MODE_OCT,
      .length = len * 8,
      .rxlength = 0,
      .user = NULL,
  };
  if (len <= 4) {
    spi_transaction.flags |= SPI_TRANS_USE_TXDATA;
    for (size_t i = 0; i < len; i++) {
      spi_transaction.tx_data[i] = *p_data;
      p_data++;
    }
  } else if (len > 4) {
    spi_transaction.tx_buffer = p_data;
  }
  xSemaphoreTake(VFD_semaphore_handle, portMAX_DELAY);
  ESP_ERROR_CHECK(
      spi_device_polling_transmit(VFD_8_MD_06INKM_handle, &spi_transaction));
  xSemaphoreGive(VFD_semaphore_handle);
}

void VFD_8_MD_06INKM_show_char(uint8_t c, uint8_t start_pos_base_0) {
  if (c < 32 || c > 126 || start_pos_base_0 > 7)
    return;
  uint8_t data[] = {VFD_8_MD_06INKM_GEN_CMD_W_DCRAM(start_pos_base_0), c};
  VFD_8_MD_06INKM_write(data, 2);
}

void VFD_8_MD_06INKM_show_str(void *s, uint8_t start_pos_base_0,
                              uint8_t str_len) {
  uint8_t *tmp = (uint8_t *)s;
  if (start_pos_base_0 > 7)
    return;
  for (size_t i = 0; i < str_len; i++) {
    if (*tmp < 32 || *tmp++ > 126)
      // return;
      *tmp = ' ';
  }
  tmp = (uint8_t *)s;
  if (str_len + start_pos_base_0 > 8)
    str_len = 8 - start_pos_base_0;
  uint8_t data[9];
  data[0] = VFD_8_MD_06INKM_GEN_CMD_W_DCRAM(start_pos_base_0);
  memcpy(data + 1, s, str_len);
  VFD_8_MD_06INKM_write(data, str_len + 1);
}

// void VFD_8_MD_06INKM_modify_custom_char(uint8_t *custom_char, uint8_t
// target_pos)
// {
//     if (target_pos > 7)
//         return;
//     // for (size_t i = 0; i < 5; i++)
//     // {
//     //     VFD_8_MD_06INKM_custom_char_table[target_pos][i] = *custom_char++;
//     // }
//     // VFD_8_MD_06INKM_write_CGRAM(target_pos, custom_char, 5);
//     VFD_8_MD_06INKM_write(VFD_8_MD_06INKM_GEN_CMD_W_CGRAM(target_pos),
//     custom_char, 5);
// }

void VFD_8_MD_06INKM_init_display_timing() {
  uint8_t data[] = {VFD_8_MD_06INKM_CMD_S_DISP_TIM,
                    VFD_8_MD_06INKM_DEFAULT_DISP_TIM};
  VFD_8_MD_06INKM_write(data, 2);
}

void VFD_8_MD_06INKM_set_brightness(uint8_t brightness) {
  uint8_t data[] = {VFD_8_MD_06INKM_CMD_S_DISP_BRT, brightness};
  VFD_8_MD_06INKM_write(data, 2);
}

void VFD_8_MD_06INKM_display_sw(VfdState_t status) {
  uint8_t data = status == kVfdEnable ? VFD_8_MD_06INKM_CMD_DISP_ON
                                      : VFD_8_MD_06INKM_CMD_DISP_OFF;
  VFD_8_MD_06INKM_write(&data, 1);
}

void VFD_8_MD_06INKM_enable_display() {}

void VFD_8_MD_06INKM_stand_by_sw(VfdState_t status) {
  uint8_t data = status == kVfdEnable ? VFD_8_MD_06INKM_CMD_STAND_BY
                                      : VFD_8_MD_06INKM_CMD_ACTIVE;

  VFD_8_MD_06INKM_write(&data, 1);
}

void VFD_8_MD_06INKM_init() {
  // interface init
  spi_device_interface_config_t spi_device_interface_config = {
      .command_bits = 0,
      .address_bits = 0,
      .dummy_bits = 0,
      .mode = 3,
      .clock_source = SPI_CLK_SRC_DEFAULT,
      .duty_cycle_pos = 128,
      .cs_ena_pretrans = 2,
      .cs_ena_posttrans = 2,
      .clock_speed_hz = 500 * 1000,
      .input_delay_ns = 0,
      .spics_io_num = VFD_8_MD_06INKM_CS,
      .flags = SPI_DEVICE_BIT_LSBFIRST,
      .queue_size = 1,
      .pre_cb = NULL,
      .post_cb = NULL,
  };
  spi_bus_add_device(VFD_8_MD_06INKM_SPI, &spi_device_interface_config,
                     &VFD_8_MD_06INKM_handle);

  VFD_semaphore_handle = xQueueCreateMutex(1);
  if (VFD_semaphore_handle == NULL) {
    ESP_LOGE(TAG, "vfd mutex create fail, reboot...");
    esp_restart();
  }
  // hardware init
  VfdConfig_t config;
  VFD_get_config(&config);

  VFD_8_MD_06INKM_init_display_timing();
  VFD_8_MD_06INKM_set_brightness(config.brightness);
  VFD_8_MD_06INKM_show_str(&"        ", 0, 8);
  VFD_8_MD_06INKM_display_sw(kVfdEnable);

  if (config.brightness == 0)
    xTaskCreate(VFD_auto_brightness_task, "vfd auto brightness task",
                configMINIMAL_STACK_SIZE * 2, NULL, 8, NULL);

  ESP_LOGI(TAG, "VFD init complete.");
}
