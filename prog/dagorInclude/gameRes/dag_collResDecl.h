//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

namespace eastl
{
class allocator;
template <typename T, typename A>
class vector;
template <int, typename>
class fixed_function;
template <typename A, typename E, typename C>
class bitvector;
} // namespace eastl

struct MidmemAlloc;
class framemem_allocator;
namespace dag
{
template <typename T, typename A, bool I, typename C>
class Vector;
template <typename T, size_t N, bool O, typename A, typename C, bool I>
class RelocatableFixedVector;
} // namespace dag

class CollisionResource;
struct CollisionTrace;
struct CollisionNode;
struct IntersectedNode;
struct MultirayIntersectedNode;

using CollisionNodeFilter = eastl::fixed_function<64, bool(int)>;
using CollisionNodeMask = eastl::bitvector<framemem_allocator, uintptr_t, eastl::vector<uintptr_t, eastl::allocator>>;
using CollResIntersectionsType = dag::RelocatableFixedVector<IntersectedNode, 64, true, framemem_allocator, uint32_t, true>;
using MultirayCollResIntersectionsType =
  dag::RelocatableFixedVector<MultirayIntersectedNode, 256, true, framemem_allocator, uint32_t, true>;
using TraceCollisionResourceStats = dag::Vector<int, framemem_allocator, true, uint32_t>;
