#include "my_sntp.h"
//
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
//
#include "time.h"

#define TAG "my sntp"

time_t last_sync_sec;

void my_sntp_set_sync_time_interval(uint32_t interval_ms) {
  ESP_LOGI(TAG, "set sync time interval to %lu ms.", interval_ms);
  if (esp_sntp_get_sync_interval() != interval_ms) {
    esp_sntp_set_sync_interval(interval_ms);
    sntp_restart();
  }
}

void my_sntp_sync_cb(struct timeval *tv) {
  ESP_LOGI(TAG, "enter sntp sync cb.");
  if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
    ESP_LOGI(TAG, "sync time success.");
    last_sync_sec = tv->tv_sec;
    my_sntp_set_sync_time_interval(3600000);
  } else if (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
    ESP_LOGI(TAG, "sync time fail.");
    my_sntp_set_sync_time_interval(15000);
  } else if (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) {
  }
}

void my_sntp_start(void *event_handler_arg, esp_event_base_t event_base,
                   int32_t event_id, void *event_data) {
  if (esp_netif_sntp_start() == ESP_OK)
    ESP_LOGI(TAG, "sntp is start");
  else
    ESP_LOGI(TAG, "sntp start fail");
}

void my_sntp_stop(void *event_handler_arg, esp_event_base_t event_base,
                  int32_t event_id, void *event_data) {
  esp_sntp_stop();
  ESP_LOGI(TAG, "sntp is stop.");
}

time_t my_sntp_get_last_sync_time() { return last_sync_sec; }

void my_sntp_init() {

  esp_sntp_config_t my_sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(
      4, ESP_SNTP_SERVER_LIST("ntp6.aliyun.com", "ntp2.tencent.com",
                              "ntp.ntsc.ac.cn", "cn.ntp.org.cn"));
  my_sntp_config.sync_cb = my_sntp_sync_cb;
  my_sntp_config.start = false;
  ESP_ERROR_CHECK(esp_netif_sntp_init(&my_sntp_config));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, my_sntp_start, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_LOST_IP, my_sntp_stop, NULL, NULL));
  ESP_LOGI(TAG, "SNTP init complete.");
}
