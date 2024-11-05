// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "destroyEvent.h"
#include <EASTL/list.h>
#include <EASTL/functional.h>
#include <osApiWrappers/dag_critSec.h>
#include <debug/dag_assert.h>

using CallbackStorageT = eastl::list<eastl::function<void()>>;
static CallbackStorageT on_before_window_destroyed_callbacks;
static WinCritSec mutex;

namespace d3d
{
BeforeWindowDestroyedCookie *register_before_window_destroyed_callback(eastl::function<void()> callback)
{
  WinAutoLock lock{mutex};
  on_before_window_destroyed_callbacks.emplace_front(eastl::move(callback));
  return reinterpret_cast<BeforeWindowDestroyedCookie *>(on_before_window_destroyed_callbacks.begin().mpNode);
}
void unregister_before_window_destroyed_callback(BeforeWindowDestroyedCookie *cookie)
{
  G_ASSERT(cookie);
  auto node = reinterpret_cast<CallbackStorageT::iterator::node_type *>(cookie);
  WinAutoLock lock{mutex};
  on_before_window_destroyed_callbacks.DoFreeNode(node);
}
} // namespace d3d

void on_before_window_destroyed()
{
  WinAutoLock lock{mutex};
  for (auto &callback : on_before_window_destroyed_callbacks)
  {
    callback();
  }
  on_before_window_destroyed_callbacks.clear();
}
