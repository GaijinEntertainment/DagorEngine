//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

namespace dag
{
template <typename T, typename A, bool I, typename C>
class Vector;
template <typename T, size_t N, bool O, typename A, typename C, bool Z>
class RelocatableFixedVector;
template <typename T>
class Span;
template <typename T>
using ConstSpan = dag::Span<T const>;
struct MemPtrAllocator;
} // namespace dag

namespace eastl
{
class allocator;
}

struct MidmemAlloc;

template <typename T>
using Tab = dag::Vector<T, dag::MemPtrAllocator, false, uint32_t>;
#include <generic/dag_smallTab_decl.inl>
template <typename T, unsigned N>
class carray;
template <typename T, size_t N>
using StaticTab = dag::RelocatableFixedVector<T, N, false, MidmemAlloc, uint32_t, false>;
