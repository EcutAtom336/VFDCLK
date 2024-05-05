#ifndef _MY_SETTING_H_
#define _MY_SETTING_H_

void *my_setting_on_create();

void my_setting_on_pause(void *vp_context);

void my_setting_on_resume(void *vp_context);

void my_setting_on_destroy(void *vp_context);

#endif
