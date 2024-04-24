#include "my_disp.h"
//
#include "freertos/FreeRTOS.h"
//

typedef struct {
  uint8_t brightness;
} MyDispConfig_t;

void my_disp_get_config() {
  const MyDispConfig_t default_config = {
    .brightness = 8,
  }
}
void my_disp_init() {}