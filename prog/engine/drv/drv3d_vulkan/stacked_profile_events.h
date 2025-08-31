// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <perfMon/dag_statDrv.h>
#include <generic/dag_objectPool.h>

namespace drv3d_vulkan
{

struct StackedProfileEvents
{
  // only for dev builds with profiler
#if DA_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0
  ::da_profiler::desc_id_t lastChainDescId = -1;
  ObjectPool<::da_profiler::ScopedEvent> storage;
  dag::Vector<::da_profiler::ScopedEvent *> stack;
  void push(::da_profiler::desc_id_t desc_id) { stack.push_back(storage.allocate(desc_id)); }
  void pop()
  {
    if (stack.empty())
      return;
    storage.free(stack.back());
    stack.pop_back();
  }

  void popChained()
  {
    if (lastChainDescId != -1)
    {
      pop();
      lastChainDescId = -1;
    }
  }
  void pushChained(::da_profiler::desc_id_t desc_id)
  {
    if (lastChainDescId == desc_id)
      return;
    popChained();
    push(desc_id);
    lastChainDescId = desc_id;
  }

  void finish()
  {
#if VULKAN_PROFILE_USER_MARKERS_IN_REPLAY > 0
    userMarkDepth = 0;
#endif
    lastChainDescId = -1;
    while (!stack.empty())
      pop();
  }
#else
  void push(::da_profiler::desc_id_t) {}
  void pop() {}

  void pushChained(::da_profiler::desc_id_t) {}
  void popChained() {}

  void finish() {}
#endif

#if VULKAN_PROFILE_USER_MARKERS_IN_REPLAY > 0 && DA_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0

  int userMarkDepth = 0;
  // both methods trigger by user code, add a bit safety to not break profiler
  void pushInterruptChain(const char *user_dyn_marker)
  {
    ::da_profiler::desc_id_t desc_id = ::da_profiler::add_copy_description(nullptr, 0, ::da_profiler::Normal, user_dyn_marker);
    popChained();
    push(desc_id);
    ++userMarkDepth;
  }
  void popInterruptChain()
  {
    if (userMarkDepth == 0)
      return;
    popChained();
    pop();
    --userMarkDepth;
  }
#else
  void popInterruptChain() {}
  void pushInterruptChain(const char *) {}
#endif
};

} // namespace drv3d_vulkan
