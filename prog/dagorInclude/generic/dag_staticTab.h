//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_relocatableFixedVector.h>
#include <generic/dag_span.h>

struct MidmemAlloc;
template <typename T, size_t N>
using StaticTab = dag::RelocatableFixedVector<T, N, false, MidmemAlloc, uint32_t, false>;

namespace dag
{
template <typename T, size_t N>
inline void set_allocator(StaticTab<T, N> &, IMemAlloc *)
{}
template <typename T, size_t N>
inline IMemAlloc *get_allocator(const StaticTab<T, N> &)
{
  return nullptr;
}
} // namespace dag
