//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if _TARGET_C1 | _TARGET_C2


#elif __GNUC__ && !_TARGET_PC_WIN
#include <pthread.h>
#include <errno.h>
#endif

#include <supp/dag_define_COREIMP.h>

#if _TARGET_PC_WIN | _TARGET_XBOX
typedef void *os_event_t; // see HANDLE declaration in WinNt.h
#elif _TARGET_C1 | _TARGET_C2




#elif defined(__GNUC__)
typedef struct _os_event_t
{
  pthread_mutex_t mutex;
  pthread_cond_t event;
  volatile int raised;
} os_event_t;
#else
#error Platform not supported
#endif

#if _TARGET_PC_WIN | _TARGET_XBOX
enum
{
  OS_WAIT_INFINITE = 0xFFFFFFFF, // INFINITE
  OS_WAIT_IGNORE = 0,
  OS_WAIT_OK = 0,         // WAIT_OBJECT_0  WinBase.h
  OS_WAIT_TIMEOUTED = 258 // WAIT_TIMEOUT in WinError.h
};
#else
enum
{
  OS_WAIT_INFINITE = 0,
  OS_WAIT_IGNORE = 0xFFFFFFFF,
  OS_WAIT_OK = 0,
  OS_WAIT_TIMEOUTED = ETIMEDOUT
};
#endif

// all operations return value < 0 on error
KRNLIMP void os_event_create(os_event_t *event, const char *event_name = NULL);
KRNLIMP int os_event_destroy(os_event_t *event);
KRNLIMP int os_event_set(os_event_t *event);
KRNLIMP int os_event_wait(os_event_t *event, unsigned timeout_ms);

#include <supp/dag_undef_COREIMP.h>
