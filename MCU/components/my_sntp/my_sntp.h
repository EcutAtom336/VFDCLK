#ifndef _MY_SNTP_H_
#define _MY_SNTP_H_

#include <time.h>
#include <stdbool.h>

void my_sntp_init();

bool my_sntp_time_is_sync();

time_t my_sntp_get_last_sync_time();

#endif
