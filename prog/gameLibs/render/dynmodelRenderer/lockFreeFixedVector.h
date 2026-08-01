// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_atomic.h>

namespace dynrend
{
template <class T, class Id, int inplace_count>
class LockFreeFixedVector
{
  alignas(T) char buffer[sizeof(T) * inplace_count] = {};
  volatile uint32_t count = 0;

public:
  LockFreeFixedVector() = default;
  ~LockFreeFixedVector() { clear(); }
  EA_NON_COPYABLE(LockFreeFixedVector)

  // =============================================================
  // THREAD-SAFE methods
  T *data() { return reinterpret_cast<T *>(buffer); }
  uint32_t size() const { return interlocked_acquire_load(count); }

  T &operator[](uint32_t id)
  {
    G_ASSERT(id < size());
    return data()[id];
  }

  // =============================================================
  // THREAD-SAFE for other thread-safe methods,
  // but NOT thread-safe for another emplace_back() calls
  template <class... Args>
  Id emplace_back(Args &&...args)
  {
    G_ASSERT_RETURN(count < inplace_count, Id::INVALID);
    new (data() + count, _NEW_INPLACE) T(count, eastl::forward<Args>(args)...);
    Id id{(int)count};
    interlocked_increment(count);
    return id;
  }

  // =============================================================
  // NOT thread-safe methods - must have happens-before order
  // with all other methods
  void clear()
  {
    int i = count;
    count = 0;
    while (i > 0)
      data()[--i].~T();
  }

  T &back()
  {
    G_ASSERT(count > 0);
    return data()[count - 1];
  }

  void pop_back()
  {
    G_ASSERT(count > 0);
    T &elem = back();
    interlocked_decrement(count);
    elem.~T();
  }
};
} // namespace dynrend