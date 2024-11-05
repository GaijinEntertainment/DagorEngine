//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <memory/dag_genMemAlloc.h>
#include <generic/dag_span.h>
#include <dag/dag_vector.h>

#include <generic/dag_smallTab_decl.inl>

namespace dag
{
template <typename T, typename A, typename C>
inline void set_allocator(SmallTab<T, A, C> &, IMemAlloc *)
{}
template <typename T, typename A, typename C>
inline IMemAlloc *get_allocator(const SmallTab<T, A, C> &)
{
  return A::getMem();
}
} // namespace dag
