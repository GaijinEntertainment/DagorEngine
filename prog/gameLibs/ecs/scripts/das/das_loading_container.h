// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>

namespace bind_dascript
{
template <class T, int ID = 0>
struct LoadingContainer
{
  thread_local static inline eastl::unique_ptr<T> value;
  eastl::vector<eastl::unique_ptr<T>> cache;

  static T *getOrCreateValuePtr()
  {
    if (EASTL_UNLIKELY(!value))
      value = eastl::make_unique<T>();
    return value.get();
  }

  void thisThreadDone()
  {
    if (value)
      cache.emplace_back(eastl::move(value));
  }

  void clear()
  {
    value.reset();
    cache.clear();
  }
};
} // namespace bind_dascript