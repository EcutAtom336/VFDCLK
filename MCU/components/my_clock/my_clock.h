#ifndef _MY_CLOCK_H_
#define _MY_CLOCK_H_

#include "cJSON.h"

void my_clock_init();

void my_clock_deinit();

void my_clock_get_config_json(cJSON *config_json_parent);

void my_clock_analysis_config_json(cJSON *config_json_parent);

#endif
