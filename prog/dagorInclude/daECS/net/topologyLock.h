//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_miscApi.h>
#include <daECS/core/entityManager.h>
#include <debug/dag_assert.h>

namespace net
{

// Topology lock for CNetwork's connection set. WriteScope brackets owner-thread mutations;
// ReadScope brackets off-owner reads (skipped on the owner thread via topology_skip_pin()).
// Consumer-provided: returns true when the read pin can be skipped (owner thread / no
// contention). daNetGame: is_this_thread_net_em_owner(); non-DNG consumers: stub returning true.
bool topology_skip_pin();

class TopologyLock
{
public:
  inline static OSReentrantReadWriteLock lock;

  // Our own counter -- OSReentrantReadWriteLock's per-thread read tracking is Win/Xbox/Apple only.
  inline static thread_local int read_scope_depth = 0;
  static bool this_thread_holds_read() { return read_scope_depth > 0; }

  struct WriteScope
  {
    WriteScope(ecs::EntityManager &mgr)
    {
      G_UNUSED(mgr); // used only by G_ASSERTF below (no-op in release)
      G_ASSERTF(mgr.getOwnerThreadId() == get_current_thread_id(),
        "TopologyLock::WriteScope off the EM owner thread (owner=%lld cur=%lld)", (long long)mgr.getOwnerThreadId(),
        (long long)get_current_thread_id());
      lock.lockWrite();
    }
    ~WriteScope() { lock.unlockWrite(); }
    WriteScope(const WriteScope &) = delete;
    WriteScope &operator=(const WriteScope &) = delete;
  };

  struct ReadScope
  {
    bool locked;
    ReadScope() : locked(!topology_skip_pin())
    {
      if (locked)
      {
        lock.lockRead();
        ++read_scope_depth;
      }
    }
    ~ReadScope()
    {
      if (locked)
      {
        --read_scope_depth;
        lock.unlockRead();
      }
    }
    ReadScope(const ReadScope &) = delete;
    ReadScope &operator=(const ReadScope &) = delete;
  };
};

} // namespace net
