//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_patchTab.h>

template <typename T>
struct UnpatchedTabReader : private PatchableTab<T>
{
  dag::ConstSpan<T> getAccessor(const void *base) const
  {
    PatchableTab<T> accessor;
    memcpy(&accessor, this, sizeof(PatchableTab<T>));
    accessor.patch((void *)base);
    return make_span_const(accessor.data(), accessor.size());
  }
};

template <typename T>
struct UnpatchedPtrReader : private PatchablePtr<T>
{
  const T *get(const void *base) const
  {
    PatchablePtr<T> accessor;
    memcpy(&accessor, this, sizeof(PatchablePtr<T>));
    accessor.patch((void *)base);
    return accessor.get();
  }
};
