#ifndef _VFD_8_MD_06INKM_H_
#define _VFD_8_MD_06INKM_H_

#include "VFD_type_defs.h"
//
#include "cJSON.h"
//
#include <stdint.h>

void VFD_8_MD_06INKM_init();

void VFD_8_MD_06INKM_stand_by_sw(VfdState_t status);

void VFD_8_MD_06INKM_show_char(uint8_t c, uint8_t start_pos_base_0);

void VFD_8_MD_06INKM_show_str(void *s, uint8_t start_pos_base_0,
                              uint8_t string_length);

void VFD_8_MD_06INKM_display_sw(VfdState_t status);

void VFD_get_config_json(cJSON *config_json_parent);

void VFD_analysis_config_json(cJSON *config_json_parent);

#endif
