// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_rwLock.h>

#if _TARGET_PC_WIN | _TARGET_XBOX
#include <windows.h>

void os_rwlock_init(os_rwlock_t &lock) { InitializeSRWLock((SRWLOCK *)&lock); }
void os_rwlock_destroy(os_rwlock_t &) {} // no-op
void os_rwlock_acquire_read_lock(os_rwlock_t &lock) { AcquireSRWLockShared((SRWLOCK *)&lock); }
bool os_rwlock_try_acquire_read_lock(os_rwlock_t &lock) { return TryAcquireSRWLockShared((SRWLOCK *)&lock); }
void os_rwlock_release_read_lock(os_rwlock_t &lock) { ReleaseSRWLockShared((SRWLOCK *)&lock); }
void os_rwlock_acquire_write_lock(os_rwlock_t &lock) { AcquireSRWLockExclusive((SRWLOCK *)&lock); }
bool os_rwlock_try_acquire_write_lock(os_rwlock_t &lock) { return TryAcquireSRWLockExclusive((SRWLOCK *)&lock); }
void os_rwlock_release_write_lock(os_rwlock_t &lock) { ReleaseSRWLockExclusive((SRWLOCK *)&lock); }

#else // pthread-based implementation assumed
#include <errno.h>
#include <util/dag_globDef.h>

void os_rwlock_init(os_rwlock_t &lock)
{
  int r = pthread_rwlock_init(&lock, /*attr*/ NULL);
  G_ASSERTF(r == 0, "%#x", r);
  G_UNUSED(r);
}
void os_rwlock_destroy(os_rwlock_t &lock)
{
  int r = pthread_rwlock_destroy(&lock);
  G_ASSERTF(r == 0, "%#x", r);
  G_UNUSED(r);
}
void os_rwlock_acquire_read_lock(os_rwlock_t &lock)
{
  int r = pthread_rwlock_rdlock(&lock);
  G_ASSERTF(r == 0, "%#x", r);
  G_UNUSED(r);
}
bool os_rwlock_try_acquire_read_lock(os_rwlock_t &lock)
{
  int r = pthread_rwlock_tryrdlock(&lock);
  G_ASSERTF(r == 0 || r == EBUSY, "%#x", r);
  return r == 0;
}
void os_rwlock_release_read_lock(os_rwlock_t &lock)
{
  int r = pthread_rwlock_unlock(&lock);
  G_ASSERTF(r == 0, "%#x", r);
  G_UNUSED(r);
}
void os_rwlock_acquire_write_lock(os_rwlock_t &lock)
{
  int r = pthread_rwlock_wrlock(&lock);
  G_ASSERTF(r == 0, "%#x", r);
  G_UNUSED(r);
}
bool os_rwlock_try_acquire_write_lock(os_rwlock_t &lock)
{
  int r = pthread_rwlock_trywrlock(&lock);
  G_ASSERTF(r == 0 || r == EBUSY, "%#x", r);
  return r == 0;
}
void os_rwlock_release_write_lock(os_rwlock_t &lock)
{
  int r = pthread_rwlock_unlock(&lock);
  G_ASSERTF(r == 0, "%#x", r);
  G_UNUSED(r);
}

#endif
