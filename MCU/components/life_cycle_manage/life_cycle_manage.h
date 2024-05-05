#ifndef _LIFE_CYCLE_MANAGE_H_
#define _LIFE_CYCLE_MANAGE_H_

#include <stdint.h>

typedef enum {
  Atom336,
  MySetting,
  MyClock,
  ActivityMax,
} Activity_t;

void life_cycle_manage_init();

void life_cycle_manage_create(int32_t activity_num);

void life_cycle_manage_delete();

#endif