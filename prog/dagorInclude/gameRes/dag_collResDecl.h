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
template <typename T>
struct default_delete;
template <typename T, typename D>
class unique_ptr;
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
// Caller-owned pose of a CollisionResource (defined in dag_collisionResource.h). The alias here
// covers signatures and locals only; holding one as a data member additionally needs
// <EASTL/unique_ptr.h> (or the full resource header), like the fwd-declared types above.
struct CollisionResourceInstance;
using CollisionResourceInstancePtr = eastl::unique_ptr<CollisionResourceInstance, eastl::default_delete<CollisionResourceInstance>>;
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
//   bits 0..1    : sub-tri index 0..3 (which triangle of a BLAS double-quad leaf was hit:
//                  0/1 = quad A tri 1/2, 2/3 = quad B tri 1/2). Zero for single-tri backends.
//   bits 2..43   : 42-bit lookup token, encoding selected by the type flag (bit 46):
//                    type=0 (non-BLAS): per-node source face index (within the CollisionNode's
//                      indicesOfs range).
//                    type=1, nodeBlas=0: OPAQUE leaf token inside the active grid's BLAS
//                      (currently the 32-bit soa4::LeafRef word of the hit leaf). Only
//                      CollisionResource interprets it; external code must treat it as an
//                      opaque identity.
//                    type=1, nodeBlas=1: the same opaque leaf token, addressing the node's OWN
//                      per-node BLAS chunk tree instead of a grid.
//   bit  44      : nodeBlas flag. Only meaningful when type=1; 1 = the token addresses the
//                  node's per-node BLAS chunk rather than a combined behavior grid.
//   bit  45      : grid flag. Only meaningful when type=1 and nodeBlas=0; 0=gridForTraceable,
//                  1=gridForCollidable. Stamped from the active grid so a post-trace
//                  getNodeFaceVertsByRef reads the right grid's blasData when a node lives in
//                  both grids.
//   bit  46      : type flag (0=non-BLAS srcFace, 1=BLAS token). See bits 2..43 above.
//   bit  47      : non-tri flag (set by makeForNonTri for box/sphere/capsule leaves). Lives
//                  outside the data-offset field so tri_ref::make(node,0) and
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
// Go through the tri_ref:: accessors (nodeIndex / hasTri / isValid / faceIndex / blasToken /
// isBlas / isCollidableGrid / subTriIndex), never the bit layout, so the encoding can evolve.
using tri_ref_t = uint64_t;
namespace tri_ref
{
inline constexpr int SUB_TRI_BITS = 2;   // double-quad leaves: 4 sub-triangles (0..3)
inline constexpr int DATA_OFS_BITS = 42; // 43 - 1: one bit ceded to NODE_BLAS so the fields still tile a 64-bit ref
inline constexpr int NODE_BLAS_BITS = 1;
inline constexpr int GRID_BITS = 1;
inline constexpr int TYPE_BITS = 1;
inline constexpr int NON_TRI_BITS = 1;
inline constexpr int NODE_IDX_BITS = 16;
inline constexpr int DATA_OFS_SHIFT = SUB_TRI_BITS;
inline constexpr int NODE_BLAS_SHIFT = DATA_OFS_SHIFT + DATA_OFS_BITS;
inline constexpr int GRID_SHIFT = NODE_BLAS_SHIFT + NODE_BLAS_BITS;
inline constexpr int TYPE_SHIFT = GRID_SHIFT + GRID_BITS;
inline constexpr int NON_TRI_SHIFT = TYPE_SHIFT + TYPE_BITS;
inline constexpr int NODE_IDX_SHIFT = NON_TRI_SHIFT + NON_TRI_BITS;
inline constexpr uint64_t SUB_TRI_MASK = (1ULL << SUB_TRI_BITS) - 1;
inline constexpr uint64_t DATA_OFS_MASK = ((1ULL << DATA_OFS_BITS) - 1) << DATA_OFS_SHIFT;
inline constexpr uint64_t NODE_BLAS_MASK = 1ULL << NODE_BLAS_SHIFT;
inline constexpr uint64_t GRID_MASK = 1ULL << GRID_SHIFT;
inline constexpr uint64_t TYPE_MASK = 1ULL << TYPE_SHIFT;
inline constexpr uint64_t NON_TRI_MASK = 1ULL << NON_TRI_SHIFT;

// Per-node BLAS refs split the DATA_OFS field: the low NODE_BLAS_OFS_BITS hold the leaf token (the
// 32-bit soa4::LeafRef word) of the node's own chunk tree, the top NODE_BLAS_GEN_BITS carry the
// resource's nodeBlasBuildId at mint time. compactNodeBlasData()/bake re-chunk the per-node trees,
// so a ref minted before such a rebuild would otherwise decode wrong-but-in-bounds geometry; the
// stamped id lets getNodeFaceVertsByRef reject it. Grid BLAS refs (make_blas) carry the same
// 32-bit token but keep the full DATA_OFS width with no generation stamp -- the grids are never
// re-chunked. The token fits the 34-bit split field with room to spare.
inline constexpr int NODE_BLAS_GEN_BITS = 8;
inline constexpr int NODE_BLAS_OFS_BITS = DATA_OFS_BITS - NODE_BLAS_GEN_BITS;
inline constexpr int NODE_BLAS_GEN_SHIFT = DATA_OFS_SHIFT + NODE_BLAS_OFS_BITS;
inline constexpr uint64_t NODE_BLAS_OFS_MASK = ((1ULL << NODE_BLAS_OFS_BITS) - 1) << DATA_OFS_SHIFT;
inline constexpr uint64_t NODE_BLAS_GEN_MASK = ((1ULL << NODE_BLAS_GEN_BITS) - 1) << NODE_BLAS_GEN_SHIFT;
static_assert((NODE_BLAS_OFS_MASK | NODE_BLAS_GEN_MASK) == DATA_OFS_MASK, "nodeBlas ofs+gen must tile DATA_OFS");

inline constexpr tri_ref_t invalid() { return ~uint64_t(0); }
// Non-BLAS make: type=0, grid bit unused (0). data_offset is the per-node source face index; the
// sub-tri field stays 0 (single-tri backend, no double-quad leaf). uint64_t parameter admits the
// full DATA_OFS_BITS width; face-index callers promote silently.
inline constexpr tri_ref_t make(uint32_t node_index, uint64_t data_offset)
{
  return (uint64_t(node_index) << NODE_IDX_SHIFT) | ((data_offset << DATA_OFS_SHIFT) & DATA_OFS_MASK);
}
// Which of a CollisionResource's two BLAS grids a hit's geometry lives in. Passed to make_blas
// (stamped into the GRID bit) and recovered by isCollidableGrid(), so a post-trace
// getNodeFaceVertsByRef decodes through the matching grid's quantization frame.
enum class GridSelector
{
  Traceable,
  Collidable
};
// BLAS make: type=1, grid bit set when grid == GridSelector::Collidable, blas_token is the opaque
// leaf token inside that grid's BLAS (the soa4::LeafRef word the walkers hand their callbacks).
inline constexpr tri_ref_t make_blas(uint32_t node_index, uint64_t blas_token, uint32_t sub_tri, GridSelector grid)
{
  return (uint64_t(node_index) << NODE_IDX_SHIFT) | TYPE_MASK | (grid == GridSelector::Collidable ? GRID_MASK : 0ULL) |
         ((blas_token << DATA_OFS_SHIFT) & DATA_OFS_MASK) | (uint64_t(sub_tri) & SUB_TRI_MASK);
}
// Per-node-BLAS make: type=1 + nodeBlas, blas_token is the opaque leaf token inside the node's own
// BLAS chunk tree (soa4::LeafRef word; grid bit unused). build_gen is the resource's nodeBlasBuildId
// at mint time (low NODE_BLAS_GEN_BITS), so a later re-chunk is detectable.
inline constexpr tri_ref_t make_node_blas(uint32_t node_index, uint64_t blas_token, uint32_t sub_tri, uint32_t build_gen)
{
  return (uint64_t(node_index) << NODE_IDX_SHIFT) | TYPE_MASK | NODE_BLAS_MASK |
         ((blas_token << DATA_OFS_SHIFT) & NODE_BLAS_OFS_MASK) | ((uint64_t(build_gen) << NODE_BLAS_GEN_SHIFT) & NODE_BLAS_GEN_MASK) |
         (uint64_t(sub_tri) & SUB_TRI_MASK);
}
// For non-triangle hits (box/sphere/capsule leaves): nodeIndex preserved, non-tri flag set so
// hasTri() returns false. Data offset and sub-tri are zero -- callers must not rely on those.
inline constexpr tri_ref_t makeForNonTri(uint32_t node_index) { return (uint64_t(node_index) << NODE_IDX_SHIFT) | NON_TRI_MASK; }

inline constexpr bool isValid(tri_ref_t r) { return r != invalid(); }
inline constexpr uint32_t nodeIndex(tri_ref_t r) { return uint32_t(r >> NODE_IDX_SHIFT); }
// Sub-triangle index 0..3 of a BLAS double-quad leaf (0/1 = quad A tri 1/2, 2/3 = quad B tri 1/2).
// 0 for single-tri backends. No bool accessor: the field is wider than one bit now, so a "!= 0"
// test would silently fold sub-tris 1..3 together.
inline constexpr uint32_t subTriIndex(tri_ref_t r) { return uint32_t(r & SUB_TRI_MASK); }
inline constexpr bool hasTri(tri_ref_t r) { return isValid(r) && (r & NON_TRI_MASK) == 0; }
// "combined behavior grid BLAS hit" -- per-node chunk hits answer isNodeBlas() instead.
inline constexpr bool isBlas(tri_ref_t r) { return hasTri(r) && (r & TYPE_MASK) != 0 && (r & NODE_BLAS_MASK) == 0; }
inline constexpr bool isNodeBlas(tri_ref_t r) { return hasTri(r) && (r & TYPE_MASK) != 0 && (r & NODE_BLAS_MASK) != 0; }
inline constexpr bool isCollidableGrid(tri_ref_t r) { return (r & GRID_MASK) != 0; }
// Generic data-offset accessor (full DATA_OFS_BITS width): source face index for non-BLAS refs,
// the opaque leaf token for grid BLAS refs. Node-BLAS refs interleave the generation stamp in this
// field -- use nodeBlasToken(). Prefer the type-checked faceIndex() / blasToken().
inline constexpr uint64_t dataOffset(tri_ref_t r) { return (r & DATA_OFS_MASK) >> DATA_OFS_SHIFT; }
// Per-node source face index for a non-BLAS triangle hit, or -1 for BLAS / non-tri / invalid.
// BLAS hits carry no face index; use getNodeFaceVertsByRef to decode their geometry.
inline constexpr int faceIndex(tri_ref_t r) { return (hasTri(r) && (r & TYPE_MASK) == 0) ? (int)dataOffset(r) : -1; }
// Returns the OPAQUE leaf token for a combined-grid BLAS triangle hit, or 0 for non-grid-BLAS /
// invalid refs. Only CollisionResource decodes it (getNodeFaceVertsByRef); external code may use
// it solely as an identity value.
inline constexpr uint64_t blasToken(tri_ref_t r) { return isBlas(r) ? dataOffset(r) : 0u; }
// Returns the opaque leaf token inside the node's own chunk for a per-node BLAS hit, or 0. Masks
// off the generation sub-field that shares the DATA_OFS region (see NODE_BLAS_OFS_MASK).
inline constexpr uint64_t nodeBlasToken(tri_ref_t r) { return isNodeBlas(r) ? ((r & NODE_BLAS_OFS_MASK) >> DATA_OFS_SHIFT) : 0u; }
// Resource build id stamped into a per-node BLAS ref at mint time; compared against the resource's
// current nodeBlasBuildId to reject refs that predate a chunk re-pack/re-chunk. 0 for other refs.
inline constexpr uint32_t nodeBlasGeneration(tri_ref_t r)
{
  return isNodeBlas(r) ? uint32_t((r & NODE_BLAS_GEN_MASK) >> NODE_BLAS_GEN_SHIFT) : 0u;
}
// Rebase the nodeIndex field by `delta`, preserving lookup-token, grid, type, non-tri, and
// sub-tri bits. Used when re-emitting a child resource's intersection list into a parent's
// namespace (e.g. AttachableVisualModel's collNodeIndexBase offset). Invalid refs pass through
// unchanged so the nodeIndex wrap-around doesn't corrupt the ~0ULL sentinel.
inline constexpr tri_ref_t rebaseNodeIndex(tri_ref_t r, uint32_t delta)
{
  return isValid(r) ? r + (uint64_t(delta) << NODE_IDX_SHIFT) : r;
}
} // namespace tri_ref
