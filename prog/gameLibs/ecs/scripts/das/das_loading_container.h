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
  eastl::vector<eastl::unique_ptr<T>> cache;

  static decltype(auto) getTLSValue()
  {
    static thread_local eastl::unique_ptr<T> value;
    return (value);
  }

  static T *getOrCreateValuePtr()
  {
    auto &value = getTLSValue();
    if (EASTL_UNLIKELY(!value))
      value = eastl::make_unique<T>();
    return value.get();
  }

  void thisThreadDone()
  {
    if (auto &value = getTLSValue(); value)
      cache.emplace_back(eastl::move(value));
  }

  void clear()
  {
    getTLSValue().reset();
    cache.clear();
  }
};

} // namespace bind_dascript
