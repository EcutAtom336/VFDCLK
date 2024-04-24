#ifndef _MY_DISP_H_
#define _MY_DISP_H_

#include <stdint.h>

typedef uint32_t MyDispToken_t;

typedef enum {
  kMyDispSuccess,
  kMyDispFail,
} MyDispState_t;

void my_disp_init();

MyDispState_t my_disp_aquire(MyDispToken_t token);

MyDispState_t my_disp_release(MyDispToken_t token);

MyDispState_t my_disp_show_str();

#endif