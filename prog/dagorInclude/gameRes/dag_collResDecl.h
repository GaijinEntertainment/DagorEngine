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
// CollisionResource::getNodeFaceVertsByRef. Carries (nodeIndex, backend-specific lookup token,
// non-tri flag, sub-tri flag) so node/triangle identity is recoverable from the ref alone via
// the resource's backend.
//
// Bit layout (uint64_t):
//   bit  0       : sub-tri flag (set when the leaf carries multiple triangles and the hit was
//                  the second one -- BLAS quad-leaves use this for the second triangle of a
//                  quad). Backends that can't produce multi-tri leaves leave it zero.
//   bits 1..46   : backend-specific lookup token (46 bits). What it encodes is up to the
//                  backend: today's BLAS dispatch stores the per-node face index here (so
//                  existing per-face consumers keep working via tri_ref::faceIndex()); a
//                  future BVH backend could store a BLAS byte offset for direct leaf decode,
//                  or any other opaque token. getNodeFaceVertsByRef knows the encoding
//                  because the CollisionResource knows which backend is live.
//   bit  47      : non-tri flag (set by makeForNonTri for box/sphere/capsule leaves). Lives
//                  outside the data-offset field so a face-0 mesh hit and a non-tri hit on the
//                  same node stay distinguishable -- without this flag, tri_ref::make(node,0,
//                  false) and tri_ref::makeForNonTri(node) collide bit-for-bit.
//   bits 48..63  : nodeIndex (16 bits -- matches CollisionNode::nodeIndex storage width)
//
// Sentinels:
//   tri_ref::invalid()              -- ~0ULL, "no hit recorded"
//   tri_ref::makeForNonTri(nodeIdx) -- nodeIndex + non-tri flag; hasTri()/faceIndex() return
//                                     false/-1. Used by BOX/SPHERE/CAPSULE leaves where a hit
//                                     exists but no triangle does.
//
// Comparing two tri_ref_t values for equality means "same triangle of the same node" -- this
// is the dedup key used by damage-model trace logging.
//
// Consumers should never touch the bit layout directly; go through the tri_ref:: accessors
// (nodeIndex / hasTri / isValid / faceIndex / subTri) so the encoding can evolve under them.
using tri_ref_t = uint64_t;
namespace tri_ref
{
inline constexpr int SUB_TRI_BITS = 1;
inline constexpr int DATA_OFS_BITS = 46;
inline constexpr int NON_TRI_BITS = 1;
inline constexpr int NODE_IDX_BITS = 16;
inline constexpr int DATA_OFS_SHIFT = SUB_TRI_BITS;
inline constexpr int NON_TRI_SHIFT = SUB_TRI_BITS + DATA_OFS_BITS;
inline constexpr int NODE_IDX_SHIFT = SUB_TRI_BITS + DATA_OFS_BITS + NON_TRI_BITS;
inline constexpr uint64_t SUB_TRI_MASK = (1ULL << SUB_TRI_BITS) - 1;
inline constexpr uint64_t DATA_OFS_MASK = ((1ULL << DATA_OFS_BITS) - 1) << DATA_OFS_SHIFT;
inline constexpr uint64_t NON_TRI_MASK = 1ULL << NON_TRI_SHIFT;

inline constexpr tri_ref_t invalid() { return ~uint64_t(0); }
// data_offset is the backend-specific lookup token; must fit in DATA_OFS_BITS (46 bits). Wider
// than uint32_t because future BVH backends may store BLAS byte offsets that exceed 4 GB --
// existing per-node face-index callers pass values that fit in uint32_t and promote silently.
inline constexpr tri_ref_t make(uint32_t node_index, uint64_t data_offset, bool sub_tri)
{
  return (uint64_t(node_index) << NODE_IDX_SHIFT) | ((data_offset << DATA_OFS_SHIFT) & DATA_OFS_MASK) | (sub_tri ? 1ULL : 0ULL);
}
// For non-triangle hits (box/sphere/capsule leaves): nodeIndex preserved, non-tri flag set so
// hasTri() returns false. Data offset and sub-tri are zero -- callers must not rely on those.
inline constexpr tri_ref_t makeForNonTri(uint32_t node_index) { return (uint64_t(node_index) << NODE_IDX_SHIFT) | NON_TRI_MASK; }

inline constexpr bool isValid(tri_ref_t r) { return r != invalid(); }
inline constexpr uint32_t nodeIndex(tri_ref_t r) { return uint32_t(r >> NODE_IDX_SHIFT); }
inline constexpr bool subTri(tri_ref_t r) { return (r & SUB_TRI_MASK) != 0; }
inline constexpr bool hasTri(tri_ref_t r) { return isValid(r) && (r & NON_TRI_MASK) == 0; }
// Returns the full 46-bit lookup token. Per-node face-index callers can keep using the int
// faceIndex() accessor below; backends that store wider tokens (e.g. BLAS byte offsets) read
// this directly.
inline constexpr uint64_t dataOffset(tri_ref_t r) { return (r & DATA_OFS_MASK) >> DATA_OFS_SHIFT; }
// Returns the per-node source face index for a triangle hit, or -1 for non-tri / invalid refs.
// Future BVH backends may need to route this through CollisionResource::getFaceIndexByRef once
// the lookup-token field stops being the face index directly.
inline constexpr int faceIndex(tri_ref_t r) { return hasTri(r) ? (int)dataOffset(r) : -1; }
// Rebase the nodeIndex field by `delta`, preserving lookup-token, non-tri, and sub-tri bits.
// Used when re-emitting a child resource's intersection list into a parent resource's namespace
// (e.g. AttachableVisualModel's collNodeIndexBase offset). Invalid refs pass through unchanged
// so the wrap-around in the nodeIndex field doesn't corrupt the ~0ULL sentinel.
inline constexpr tri_ref_t rebaseNodeIndex(tri_ref_t r, uint32_t delta)
{
  return isValid(r) ? r + (uint64_t(delta) << NODE_IDX_SHIFT) : r;
}
} // namespace tri_ref
