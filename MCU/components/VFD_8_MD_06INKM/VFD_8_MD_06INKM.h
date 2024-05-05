#ifndef _VFD_8_MD_06INKM_H_
#define _VFD_8_MD_06INKM_H_

#include "VFD_type_defs.h"
//
#include "cJSON.h"
#include "portmacro.h"
//
#include <stdint.h>

void VFD_8_MD_06INKM_init();

void VFD_8_MD_06INKM_show_str(char *s, uint8_t start_pos_base_0,
                              uint8_t string_length);

void VFD_get_config_json(cJSON *config_json_parent);

void VFD_analysis_config_json(cJSON *config_json_parent);

VfdErr_t VFD_8_MD_06INKM_aquire(TickType_t wait_tick);

VfdErr_t VFD_8_MD_06INKM_release();

#endif
