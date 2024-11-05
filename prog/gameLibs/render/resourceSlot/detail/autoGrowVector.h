// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_relocatableFixedVector.h>

namespace resource_slot::detail
{

template <typename SomeId, typename T, size_t inplace_count, bool auto_grow = true, typename Allocator = MidmemAlloc>
struct AutoGrowVector
{
  AutoGrowVector() = default;
  explicit AutoGrowVector(int size) : data(size) {}

  T &operator[](SomeId some_id)
  {
    int intId = static_cast<int>(some_id);
    if constexpr (auto_grow)
    {
      if (data.size() <= intId)
        data.resize(intId + 1);
    }
    return data[intId];
  }

  const T &operator[](SomeId some_id) const
  {
    int intId = static_cast<int>(some_id);
    G_ASSERT((0 <= intId) && (intId < data.size()));
    return data[intId];
  }

  intptr_t size() const { return data.size(); };

  T *begin() { return data.begin(); }
  T *end() { return data.end(); }
  const T *begin() const { return data.begin(); }
  const T *end() const { return data.end(); }

private:
  dag::RelocatableFixedVector<T, inplace_count, true, Allocator> data;
};

} // namespace resource_slot::detail