// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_critSec.h>
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <windows.h>
#define HAVE_WINAPI                    1
#define PLATFORM_CRITICAL_SECTION_TYPE CRITICAL_SECTION
G_STATIC_ASSERT(alignof(CritSecStorage) == alignof(CRITICAL_SECTION));
#elif _TARGET_C1 | _TARGET_C2

#else
#include <pthread.h>
#include <time.h>
#include <errno.h>
#define HAVE_WINAPI                    0
#define PLATFORM_CRITICAL_SECTION_TYPE pthread_mutex_t
#endif


namespace csimpl
{
using type = PLATFORM_CRITICAL_SECTION_TYPE;
inline type *get(void *p) { return (type *)p; }
namespace lockcount
{
#if DAG_CS_CUSTOM_LOCKSCOUNT
inline volatile int &ref(void *p) { return ((CritSecStorage *)p)->locksCount; }
inline int value(void *p) { return ((CritSecStorage *)p)->locksCount; }
static inline void release(void *p) { interlocked_release_store(lockcount::ref(p), 0); }
static inline void increment(void *p) { interlocked_increment(lockcount::ref(p)); }
static inline void decrement(void *p)
{
  G_ASSERTF(lockcount::ref(p) > 0, "unlock not-locked CC? lockCount=%d", lockcount::ref(p));
  interlocked_decrement(lockcount::ref(p));
}
#else
inline void release(void *) {}
inline void increment(void *) {}
inline void decrement(void *) {}
#endif
} // namespace lockcount
} // namespace csimpl
