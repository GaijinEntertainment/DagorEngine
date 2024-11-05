//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#ifdef __cplusplus

#include <generic/dag_span.h>
#include <memory/dag_memPtrAllocator.h>
#include <dag/dag_vector.h>

template <typename T>
using Tab = dag::Vector<T, dag::MemPtrAllocator, false, uint32_t>;

namespace dag
{
template <typename T>
inline void set_allocator(Tab<T> &v, IMemAlloc *m)
{
  v.get_allocator().m = m;
}
template <typename T>
inline IMemAlloc *get_allocator(const Tab<T> &v)
{
  return v.get_allocator().m;
}
} // namespace dag

#endif // __cplusplus
