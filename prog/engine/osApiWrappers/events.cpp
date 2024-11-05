// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_events.h>
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <supp/_platform.h>
#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_APPLE
#include <mach/clock.h>
#include <mach/mach.h>
#include <sys/time.h>
#endif
#include <util/dag_globDef.h>

void os_event_create(os_event_t *e, const char *name, bool manual_reset, bool init)
{
  G_UNUSED(manual_reset);
  int ret = -1;
  G_ASSERT(e);
#if _TARGET_PC_WIN | _TARGET_XBOX
  G_STATIC_ASSERT(OS_WAIT_INFINITE == INFINITE);
  G_STATIC_ASSERT(OS_WAIT_OK == WAIT_OBJECT_0);
  G_STATIC_ASSERT(OS_WAIT_TIMEOUTED == WAIT_TIMEOUT);
  G_STATIC_ASSERT(sizeof(os_event_t) == sizeof(HANDLE));
  ((void)name); // be aware of side effect events with same name on multiple application instances
  *e = CreateEvent(NULL, manual_reset, init, NULL);
  ret = *e ? 0 : GetLastError();
#elif _TARGET_C1 | _TARGET_C2

#elif defined(__GNUC__)
  (void)(name);
  ret = pthread_mutex_init(&e->mutex, NULL);
  pthread_condattr_t *pca = NULL;
#if !(_TARGET_APPLE | _TARGET_C3 | (_TARGET_ANDROID && __ANDROID_API__ < 21))
  pthread_condattr_t ca;
  pthread_condattr_init(&ca);
  pthread_condattr_setclock(&ca, CLOCK_MONOTONIC);
  pca = &ca;
#endif
  if (ret == 0)
  {
    ret = pthread_cond_init(&e->event, pca);
    if (ret != 0)
      pthread_mutex_destroy(&e->mutex);
  }
  if (pca)
    pthread_condattr_destroy(pca);
  e->raised = init;
#endif
  if (ret != 0)
    DAG_FATAL("failed to create os event '%s' with error %d (0x%x)", name, ret, ret);
}

int os_event_destroy(os_event_t *e)
{
  int ret;
  if (!e)
    return -1;
#if _TARGET_PC_WIN | _TARGET_XBOX
  ret = CloseHandle(*e) ? 0 : -1;
#elif _TARGET_C1 | _TARGET_C2

#elif defined(__GNUC__)
  ret = 0;
  if (pthread_cond_destroy(&e->event) != 0)
    ret = -1;
  if (pthread_mutex_destroy(&e->mutex) != 0)
    ret = -2;
#endif
  return ret;
}

int os_event_set(os_event_t *e)
{
  int ret;
  if (!e)
    return -1;
#if _TARGET_PC_WIN | _TARGET_XBOX
  ret = SetEvent(*e) ? 0 : -1;
#elif _TARGET_C1 | _TARGET_C2

#elif defined(__GNUC__)
  ret = pthread_mutex_lock(&e->mutex);
  if (ret == 0)
  {
    if (!e->raised)
    {
      e->raised = 1;
      pthread_mutex_unlock(&e->mutex);
      ret = pthread_cond_signal(&e->event);
    }
    else
      ret = pthread_mutex_unlock(&e->mutex);
  }
#endif
  return ret;
}

int os_event_reset(os_event_t *e)
{
  int ret;
  if (!e)
    return -1;
#if _TARGET_PC_WIN | _TARGET_XBOX
  ret = ResetEvent(*e) ? 0 : -1;
#elif _TARGET_C1 | _TARGET_C2

#elif defined(__GNUC__)
  ret = pthread_mutex_lock(&e->mutex);
  if (ret == 0)
  {
    e->raised = 0;
    ret = pthread_mutex_unlock(&e->mutex);
  }
#endif
  return ret;
}

static int os_event_wait_impl(os_event_t *e, unsigned timeout_ms, bool auto_reset)
{
  int ret;
  if (!e)
    return -1;
#if _TARGET_PC_WIN | _TARGET_XBOX
  G_UNUSED(auto_reset); // Used the one specified on creation
  ret = WaitForSingleObjectEx(*e, timeout_ms, TRUE);
#elif _TARGET_C1 | _TARGET_C2

#elif defined(__GNUC__)
  ret = pthread_mutex_lock(&e->mutex);
  if (ret == 0)
  {
    if (!e->raised)
    {
      if (timeout_ms == OS_WAIT_IGNORE)
        ret = OS_WAIT_TIMEOUTED;
      else if (timeout_ms == OS_WAIT_INFINITE)
      {
        do
        {
          ret = pthread_cond_wait(&e->event, &e->mutex);
        } while (!e->raised && ret == 0);
      }
      else
      {
        struct timespec ts;
#if _TARGET_APPLE // OS X does not have clock_gettime, use gettimeofday
        struct timeval tv;
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000;
#elif _TARGET_C3


#else
        clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
        ts.tv_sec += timeout_ms / 1000;              // seconds
        ts.tv_nsec += (timeout_ms % 1000) * 1000000; // nanoseconds
        if (ts.tv_nsec >= 1000000000)
        {
          ts.tv_nsec -= 1000000000;
          ++ts.tv_sec;
        }
        do
        {
#if (_TARGET_ANDROID && __ANDROID_API__ < 21 && !defined(__clang__))
          ret = pthread_cond_timedwait_monotonic_np(&e->event, &e->mutex, &ts);
#else
          ret = pthread_cond_timedwait(&e->event, &e->mutex, &ts);
#endif
        } while (!e->raised && ret == 0);
      }
      if (ret == 0 && auto_reset) // we only acquire event if wait succeeded
        e->raised = 0;
    }
    else
    {
      ret = OS_WAIT_OK;
      if (auto_reset)
        e->raised = 0;
    }
    pthread_mutex_unlock(&e->mutex);
  }
#endif
  return ret;
}

int os_event_wait(os_event_t *e, unsigned timeout_ms) { return os_event_wait_impl(e, timeout_ms, true); }

int os_event_wait_noreset(os_event_t *e, unsigned timeout_ms) { return os_event_wait_impl(e, timeout_ms, false); }

#define EXPORT_PULL dll_pull_osapiwrappers_events
#include <supp/exportPull.h>
