//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_rwLock.h>

namespace rendinst
{

extern SmartReadWriteFifoLock ccExtra;

inline void riex_lock_write() DAG_TS_ACQUIRE(ccExtra) { ccExtra.lockWrite(); }
inline void riex_unlock_write() DAG_TS_RELEASE(ccExtra) { ccExtra.unlockWrite(); }

inline void riex_lock_read() DAG_TS_ACQUIRE_SHARED(ccExtra) { ccExtra.lockRead(); }
inline void riex_unlock_read() DAG_TS_RELEASE_SHARED(ccExtra) { ccExtra.unlockRead(); }

struct ScopedRIExtraWriteLock
{
  ScopedRIExtraWriteLock() DAG_TS_ACQUIRE(ccExtra) { riex_lock_write(); }
  ~ScopedRIExtraWriteLock() DAG_TS_RELEASE(ccExtra) { riex_unlock_write(); }
};

struct ScopedRIExtraReadLock
{
  ScopedRIExtraReadLock() DAG_TS_ACQUIRE_SHARED(ccExtra) { riex_lock_read(); }
  ~ScopedRIExtraReadLock() DAG_TS_RELEASE_SHARED(ccExtra) { riex_unlock_read(); }
};

} // namespace rendinst