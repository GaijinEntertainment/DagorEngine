//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_lockProfiler.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threadSafety.h>
#include <debug/dag_assert.h>

#if _TARGET_PC_WIN | _TARGET_XBOX
typedef struct /*_RTL_SRWLOCK*/
{
  void *Ptr;
} os_rwlock_t;

extern "C" __declspec(dllimport) unsigned long __stdcall GetLastError();
extern "C" __declspec(dllimport) unsigned long __stdcall TlsAlloc();
extern "C" __declspec(dllimport) void *__stdcall TlsGetValue(unsigned long dwTlsIndex);
extern "C" __declspec(dllimport) int __stdcall TlsSetValue(unsigned long dwTlsIndex, void *lpTlsValue);
extern "C" __declspec(dllimport) int __stdcall TlsFree(unsigned long dwTlsIndex);
#else
#include <pthread.h>
typedef pthread_rwlock_t os_rwlock_t;
#endif

#include <supp/dag_define_KRNLIMP.h>

KRNLIMP void os_rwlock_init(os_rwlock_t &);
KRNLIMP void os_rwlock_destroy(os_rwlock_t &);
KRNLIMP void os_rwlock_acquire_read_lock(os_rwlock_t &);
KRNLIMP bool os_rwlock_try_acquire_read_lock(os_rwlock_t &);
KRNLIMP void os_rwlock_release_read_lock(os_rwlock_t &);
KRNLIMP void os_rwlock_acquire_write_lock(os_rwlock_t &);
KRNLIMP bool os_rwlock_try_acquire_write_lock(os_rwlock_t &);
KRNLIMP void os_rwlock_release_write_lock(os_rwlock_t &);

// Warn: not reentrant (recursive), use OSReentrantReadWriteLock if you need it
// This is faster version and should be considered default choice.
class DAG_TS_CAPABILITY("mutex") OSReadWriteLock
{
  os_rwlock_t lock;
  // Windows SRW Locks can't be read locked recursively (but doesn't deadlock when doing so unless writer also present)
#if DAGOR_DBGLEVEL > 0 && (_TARGET_PC_WIN || _TARGET_XBOX) && defined(_DEBUG_TAB_)
  unsigned long dbgrtls = TlsAlloc(); // Note: ABI change when building with CheckedContainers=on/off
  void debugVerifyNoCurThreadReadLocked()
  {
    G_ASSERTF(!TlsGetValue(dbgrtls), "Attempt to recursive read lock (perhaps OSReentrantReadWriteLock should've been used?)");
    TlsSetValue(dbgrtls, (void *)(size_t)1);
  }
  void debugSetNoCurThreadReadLocked()
  {
    G_ASSERTF(TlsGetValue(dbgrtls), "Attempt to read unlock unowned by this thread lock");
    TlsSetValue(dbgrtls, NULL);
  }

public:
  OSReadWriteLock()
  {
    os_rwlock_init(lock);
    G_FAST_ASSERT(dbgrtls != /*TLS_OUT_OF_INDEXES*/ ~0u);
  }
  ~OSReadWriteLock()
  {
    G_VERIFYF(TlsFree(dbgrtls), "%d", GetLastError());
    os_rwlock_destroy(lock);
  }
#else
  void debugVerifyNoCurThreadReadLocked() {}
  void debugSetNoCurThreadReadLocked() {}

public:
  OSReadWriteLock() { os_rwlock_init(lock); }
  ~OSReadWriteLock() { os_rwlock_destroy(lock); }
#endif
  OSReadWriteLock(const OSReadWriteLock &) = delete;
  OSReadWriteLock &operator=(const OSReadWriteLock &) = delete;

  void lockWrite(::da_profiler::desc_id_t profile_token = ::da_profiler::DescWriteLock) DAG_TS_ACQUIRE()
  {
    ScopeLockProfiler<da_profiler::NoDesc> lp(profile_token);
    os_rwlock_acquire_write_lock(lock);
  }
  bool tryLockWrite() DAG_TS_TRY_ACQUIRE(true) { return os_rwlock_try_acquire_write_lock(lock); }
  void unlockWrite() DAG_TS_RELEASE() { os_rwlock_release_write_lock(lock); }

  void lockRead(::da_profiler::desc_id_t profile_token = ::da_profiler::DescReadLock) DAG_TS_ACQUIRE_SHARED()
  {
    debugVerifyNoCurThreadReadLocked();
    ScopeLockProfiler<da_profiler::NoDesc> lp(profile_token);
    os_rwlock_acquire_read_lock(lock);
  }
  bool tryLockRead() DAG_TS_TRY_ACQUIRE_SHARED(true)
  {
    debugVerifyNoCurThreadReadLocked();
    return os_rwlock_try_acquire_read_lock(lock);
  }
  void unlockRead() DAG_TS_RELEASE_SHARED()
  {
    debugSetNoCurThreadReadLocked();
    os_rwlock_release_read_lock(lock);
  }
};

// Note: it's slower then OSReadWriteLock, both read & write reentrancy (recursion) supported,
// but intermixing different kind of locks isn't
// On Windows/Xbox max number of instances is limited by number of TLS slots (~1088)
class DAG_TS_CAPABILITY("mutex") OSReentrantReadWriteLock
{
  os_rwlock_t lock;
#if _TARGET_PC_WIN | _TARGET_XBOX
  // "Note that no thread identifier will ever be 0" according to
  // https://learn.microsoft.com/en-us/windows/win32/procthread/thread-handles-and-identifiers
  /* DWORD */ volatile unsigned long wtid = 0;
  unsigned long getWriteThreadId() const { return wtid; }
  void setWriteThreadId(unsigned long tid = 0) { wtid = tid; }

  /* DWORD */ unsigned long rtls; // MS platform's SRW locks doesn't support reentrant (recursive) read (shared) locks
  void rtlsCreate()
  {
    rtls = TlsAlloc();
    G_FAST_ASSERT(rtls != /*TLS_OUT_OF_INDEXES*/ ~0u);
  }
  void rtlsDestroy() { G_VERIFYF(TlsFree(rtls), "%d", GetLastError()); }
  size_t rtlsGet(size_t = 0) { return (size_t)TlsGetValue(rtls); }
  void rtlsSet(size_t v) { TlsSetValue(rtls, (void *)v); }
#else
  volatile int64_t wtid = 0; // 0 is used to init std::thread::id on all platforms
  int64_t getWriteThreadId() const
  {
    return __atomic_load_n(&wtid, __ATOMIC_RELAXED);
  } // Note: write is always happens under lock (hence relaxed)
  void setWriteThreadId(int64_t tid)
  {
    G_FAST_ASSERT(tid != 0);
    __atomic_store_n(&wtid, tid, __ATOMIC_RELAXED);
  }
  void setWriteThreadId() { __atomic_store_n(&wtid, 0, __ATOMIC_RELAXED); }
#if _TARGET_APPLE // Apple's read locks are also not reentrant in presense of writer
  pthread_key_t rtls;
  void rtlsCreate() { G_VERIFY(pthread_key_create(&rtls, NULL) == 0); }
  void rtlsDestroy() { G_VERIFY(pthread_key_delete(rtls) == 0); }
  size_t rtlsGet(size_t = 0) { return (size_t)pthread_getspecific(rtls); }
  void rtlsSet(size_t v)
  {
    int r = pthread_setspecific(rtls, (void *)v);
    G_VERIFYF(!r, "%d", r);
  }
#else
  void rtlsCreate() {}
  void rtlsDestroy() {}
  constexpr size_t rtlsGet(size_t v = 0) { return v; }
  void rtlsSet(size_t) {}
#endif
#endif
  int wcount = 0;

public:
  OSReentrantReadWriteLock()
  {
    os_rwlock_init(lock);
    rtlsCreate();
  }
  ~OSReentrantReadWriteLock()
  {
    rtlsDestroy();
    os_rwlock_destroy(lock);
  }

  OSReentrantReadWriteLock(const OSReentrantReadWriteLock &) = delete;
  OSReentrantReadWriteLock &operator=(const OSReentrantReadWriteLock &) = delete;

  void lockWrite(::da_profiler::desc_id_t profile_token = ::da_profiler::DescWriteLock) DAG_TS_ACQUIRE()
  {
    ScopeLockProfiler<da_profiler::NoDesc> lp(profile_token);
    decltype(wtid) ctid = get_current_thread_id();
    if (getWriteThreadId() != ctid)
    {
      os_rwlock_acquire_write_lock(lock);
      setWriteThreadId(ctid);
    }
    ++wcount;
  }
  bool tryLockWrite() DAG_TS_TRY_ACQUIRE(true)
  {
    decltype(wtid) ctid = get_current_thread_id();
    if (getWriteThreadId() != ctid)
    {
      if (os_rwlock_try_acquire_write_lock(lock))
        setWriteThreadId(ctid);
      else
        return false;
    }
    ++wcount;
    return true;
  }
  void unlockWrite() DAG_TS_RELEASE()
  {
    G_FAST_ASSERT(wcount > 0 && getWriteThreadId() == get_current_thread_id()); // if triggered then it's attempt to unlock unowned
                                                                                // lock
    if (--wcount == 0)
    {
      setWriteThreadId(/*invalid*/);
      os_rwlock_release_write_lock(lock);
    }
  }

  void lockRead(::da_profiler::desc_id_t profile_token = ::da_profiler::DescReadLock) DAG_TS_ACQUIRE_SHARED()
  {
    ScopeLockProfiler<da_profiler::NoDesc> lp(profile_token);
    size_t rcount = rtlsGet();
    if (!rcount)
      os_rwlock_acquire_read_lock(lock);
    rtlsSet(++rcount);
  }
  bool tryLockRead() DAG_TS_TRY_ACQUIRE_SHARED(true)
  {
    size_t rcount = rtlsGet();
    if (!rcount && !os_rwlock_try_acquire_read_lock(lock))
      return false;
    rtlsSet(++rcount);
    return true;
  }
  void unlockRead() DAG_TS_RELEASE_SHARED()
  {
    size_t rcount = rtlsGet(1);
    G_FAST_ASSERT(rcount != 0); // if triggered then it's attempt to unlock not locked for read (by this thread) lock (or lock/unlock
                                // inbalance)
    if (--rcount == 0)
      os_rwlock_release_read_lock(lock);
    rtlsSet(rcount);
  }
};

// Legacy aliases, to be removed
using ReadWriteLock = OSReentrantReadWriteLock; // TODO: change to (non-reentrant) OSReadWriteLock (by default)
using ReadWriteFifoLock = ReadWriteLock;
using SmartReadWriteLock = ReadWriteLock;
using SmartReadWriteFifoLock = ReadWriteLock;

template <typename Lock>
struct DAG_TS_SCOPED_CAPABILITY ScopedLockReadTemplate
{
  ScopedLockReadTemplate(Lock &lock, ::da_profiler::desc_id_t profile_token = ::da_profiler::DescReadLock)
    DAG_TS_ACQUIRE_SHARED(lock) :
    savedLock(&lock)
  {
    savedLock->lockRead(profile_token);
  }

  ScopedLockReadTemplate(Lock *lock, ::da_profiler::desc_id_t profile_token = ::da_profiler::DescReadLock)
    DAG_TS_ACQUIRE_SHARED(lock) :
    savedLock(lock)
  {
    if (savedLock)
      savedLock->lockRead(profile_token);
  }

  ~ScopedLockReadTemplate() DAG_TS_RELEASE() // NOTE: not a typo!
  {
    if (savedLock)
      savedLock->unlockRead();
  }

private:
  Lock *savedLock;

  ScopedLockReadTemplate(const ScopedLockReadTemplate &) = delete;
  ScopedLockReadTemplate &operator=(const ScopedLockReadTemplate &) = delete;
};

template <typename Lock>
struct DAG_TS_SCOPED_CAPABILITY ScopedLockWriteTemplate
{
  ScopedLockWriteTemplate(Lock &lock, ::da_profiler::desc_id_t profile_token = ::da_profiler::DescWriteLock) DAG_TS_ACQUIRE(lock) :
    savedLock(&lock)
  {
    savedLock->lockWrite(profile_token);
  }

  ScopedLockWriteTemplate(Lock *lock, ::da_profiler::desc_id_t profile_token = ::da_profiler::DescWriteLock) DAG_TS_ACQUIRE(lock) :
    savedLock(lock)
  {
    if (savedLock)
      savedLock->lockWrite(profile_token);
  }

  ~ScopedLockWriteTemplate() DAG_TS_RELEASE()
  {
    if (savedLock)
      savedLock->unlockWrite();
  }

private:
  Lock *savedLock;

  ScopedLockWriteTemplate(const ScopedLockWriteTemplate &) = delete;
  ScopedLockWriteTemplate &operator=(const ScopedLockWriteTemplate &) = delete;
};

typedef ScopedLockReadTemplate<SmartReadWriteFifoLock> ScopedLockRead;
typedef ScopedLockWriteTemplate<SmartReadWriteFifoLock> ScopedLockWrite;

#include <supp/dag_undef_KRNLIMP.h>
