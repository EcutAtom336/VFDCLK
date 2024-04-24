#ifndef _MY_WIFI_H_
#define _MY_WIFI_H_

#include "cJSON.h"

void my_wifi_init();

void my_wifi_disconnect();

void my_wifi_get_config_json(cJSON *config_json_parent);

void my_wifi_analysis_config_json(cJSON *config_json_parent);

#endif
