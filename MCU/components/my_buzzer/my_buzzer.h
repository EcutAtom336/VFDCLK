#ifndef _BUZZER_H_
#define _BUZZER_H_

#include "stdint.h"

typedef enum
{
    MY_BUZZER_TONE_NAME_START,
    MY_BUZZER_TONE_NAME_WARNING,
} my_buzzer_sound_name_t;

void my_buzzer_init();

void my_buzzer_tone(my_buzzer_sound_name_t tone_name, uint8_t volume);

void my_buzzer_custom_tone(uint16_t freq, uint16_t duration_ms, uint8_t volume);

#endif
