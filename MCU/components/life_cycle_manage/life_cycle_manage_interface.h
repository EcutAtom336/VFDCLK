#ifndef _MY_ACTIVITY_INTERFACE_MANAGE_H_
#define _MY_ACTIVITY_INTERFACE_MANAGE_H_

typedef void *(*OnCreate_t)();
typedef void (*OnPause_t)(void *context_handle);
typedef void (*OnResume_t)(void *context_handle);
typedef void (*OnDestroy_t)(void *context_handle);
typedef void (*MainTask_t)(void *context_handle);

typedef struct {
  OnCreate_t on_create;
  OnPause_t on_pause;
  OnResume_t on_resume;
  OnDestroy_t on_destroy;
} ActivityFunc_t;

typedef enum {
  kRun,
  kPause,
} ActivityState_t;

#endif