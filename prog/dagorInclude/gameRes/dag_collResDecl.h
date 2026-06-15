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

// Opaque self-describing identifier of a hit inside a CollisionResource. Stored on
// IntersectedNode (the engine's trace-hit struct) and used as the lookup key for
// CollisionResource::getNodeFaceVertsByRef. Carries node identity, a backend-specific lookup
// token, and flags that select the token's interpretation.
//
// Bit layout (uint64_t):
//   bit  0       : sub-tri flag (the hit was the leaf's second triangle -- BLAS quad-leaves
//                  use this for the second triangle of a quad). Zero for single-tri backends.
//   bits 1..44   : 44-bit lookup token, encoding selected by the type flag (bit 46):
//                    type=0 (non-BLAS): per-node source face index (within the CollisionNode's
//                      indicesOfs range).
//                    type=1 (BLAS):     byte offset of the leaf body inside the active grid's
//                      blasData (= the dataOfs iterateFiltered passes to leafCb = leaf box
//                      start + BVH_BLAS_NODE_SIZE). 44 bits = up to 16 TB, plenty.
//   bit  45      : grid flag. Only meaningful when type=1; 0=gridForTraceable, 1=gridForCollidable.
//                  Stamped from the active grid so a post-trace getNodeFaceVertsByRef reads the
//                  right grid's blasData when a node lives in both grids.
//   bit  46      : type flag (0=non-BLAS srcFace, 1=BLAS blasOffset). See bits 1..44 above.
//   bit  47      : non-tri flag (set by makeForNonTri for box/sphere/capsule leaves). Lives
//                  outside the data-offset field so tri_ref::make(node,0,false) and
//                  tri_ref::makeForNonTri(node) don't collide bit-for-bit.
//   bits 48..63  : nodeIndex (16 bits -- matches CollisionNode::nodeIndex storage width)
//
// Sentinels:
//   tri_ref::invalid()              -- ~0ULL, "no hit recorded"
//   tri_ref::makeForNonTri(nodeIdx) -- nodeIndex + non-tri flag; hasTri()/faceIndex() return
//                                     false/-1. Used by BOX/SPHERE/CAPSULE leaves (hit, no tri).
//
// Equality means "same triangle of the same node, same backend identity" -- the dedup key used
// by damage-model trace logging.
//
// Go through the tri_ref:: accessors (nodeIndex / hasTri / isValid / faceIndex / blasOffset /
// isBlas / isCollidableGrid / subTri), never the bit layout, so the encoding can evolve.
using tri_ref_t = uint64_t;
namespace tri_ref
{
inline constexpr int SUB_TRI_BITS = 1;
inline constexpr int DATA_OFS_BITS = 44;
inline constexpr int GRID_BITS = 1;
inline constexpr int TYPE_BITS = 1;
inline constexpr int NON_TRI_BITS = 1;
inline constexpr int NODE_IDX_BITS = 16;
inline constexpr int DATA_OFS_SHIFT = SUB_TRI_BITS;
inline constexpr int GRID_SHIFT = DATA_OFS_SHIFT + DATA_OFS_BITS;
inline constexpr int TYPE_SHIFT = GRID_SHIFT + GRID_BITS;
inline constexpr int NON_TRI_SHIFT = TYPE_SHIFT + TYPE_BITS;
inline constexpr int NODE_IDX_SHIFT = NON_TRI_SHIFT + NON_TRI_BITS;
inline constexpr uint64_t SUB_TRI_MASK = (1ULL << SUB_TRI_BITS) - 1;
inline constexpr uint64_t DATA_OFS_MASK = ((1ULL << DATA_OFS_BITS) - 1) << DATA_OFS_SHIFT;
inline constexpr uint64_t GRID_MASK = 1ULL << GRID_SHIFT;
inline constexpr uint64_t TYPE_MASK = 1ULL << TYPE_SHIFT;
inline constexpr uint64_t NON_TRI_MASK = 1ULL << NON_TRI_SHIFT;

inline constexpr tri_ref_t invalid() { return ~uint64_t(0); }
// Non-BLAS make: type=0, grid bit unused (0). data_offset is the per-node source face index.
// uint64_t parameter admits the full DATA_OFS_BITS width; face-index callers promote silently.
inline constexpr tri_ref_t make(uint32_t node_index, uint64_t data_offset, bool sub_tri)
{
  return (uint64_t(node_index) << NODE_IDX_SHIFT) | ((data_offset << DATA_OFS_SHIFT) & DATA_OFS_MASK) | (sub_tri ? 1ULL : 0ULL);
}
// BLAS make: type=1, grid bit set per is_collidable_grid, data_offset is the leaf byte offset
// inside that grid's blasData (= the dataOfs iterateFiltered passes to leafCb). uint64_t
// parameter so byte offsets exceeding 4 GB round-trip.
inline constexpr tri_ref_t make_blas(uint32_t node_index, uint64_t blas_offset, bool sub_tri, bool is_collidable_grid)
{
  return (uint64_t(node_index) << NODE_IDX_SHIFT) | TYPE_MASK | (is_collidable_grid ? GRID_MASK : 0ULL) |
         ((blas_offset << DATA_OFS_SHIFT) & DATA_OFS_MASK) | (sub_tri ? 1ULL : 0ULL);
}
// For non-triangle hits (box/sphere/capsule leaves): nodeIndex preserved, non-tri flag set so
// hasTri() returns false. Data offset and sub-tri are zero -- callers must not rely on those.
inline constexpr tri_ref_t makeForNonTri(uint32_t node_index) { return (uint64_t(node_index) << NODE_IDX_SHIFT) | NON_TRI_MASK; }

inline constexpr bool isValid(tri_ref_t r) { return r != invalid(); }
inline constexpr uint32_t nodeIndex(tri_ref_t r) { return uint32_t(r >> NODE_IDX_SHIFT); }
inline constexpr bool subTri(tri_ref_t r) { return (r & SUB_TRI_MASK) != 0; }
inline constexpr bool hasTri(tri_ref_t r) { return isValid(r) && (r & NON_TRI_MASK) == 0; }
inline constexpr bool isBlas(tri_ref_t r) { return hasTri(r) && (r & TYPE_MASK) != 0; }
inline constexpr bool isCollidableGrid(tri_ref_t r) { return (r & GRID_MASK) != 0; }
// Generic data-offset accessor (full DATA_OFS_BITS width): source face index for non-BLAS refs,
// leaf byte offset for BLAS refs. uint64_t return so >4 GB BLAS offsets round-trip. Prefer the
// type-checked faceIndex() / blasOffset().
inline constexpr uint64_t dataOffset(tri_ref_t r) { return (r & DATA_OFS_MASK) >> DATA_OFS_SHIFT; }
// Per-node source face index for a non-BLAS triangle hit, or -1 for BLAS / non-tri / invalid.
// BLAS hits carry no face index; use getNodeFaceVertsByRef to decode their geometry.
inline constexpr int faceIndex(tri_ref_t r) { return (hasTri(r) && (r & TYPE_MASK) == 0) ? (int)dataOffset(r) : -1; }
// Returns the BLAS leaf byte offset for a BLAS triangle hit, or 0 for non-BLAS / invalid refs.
inline constexpr uint64_t blasOffset(tri_ref_t r) { return isBlas(r) ? dataOffset(r) : 0u; }
// Rebase the nodeIndex field by `delta`, preserving lookup-token, grid, type, non-tri, and
// sub-tri bits. Used when re-emitting a child resource's intersection list into a parent's
// namespace (e.g. AttachableVisualModel's collNodeIndexBase offset). Invalid refs pass through
// unchanged so the nodeIndex wrap-around doesn't corrupt the ~0ULL sentinel.
inline constexpr tri_ref_t rebaseNodeIndex(tri_ref_t r, uint32_t delta)
{
  return isValid(r) ? r + (uint64_t(delta) << NODE_IDX_SHIFT) : r;
}
} // namespace tri_ref
