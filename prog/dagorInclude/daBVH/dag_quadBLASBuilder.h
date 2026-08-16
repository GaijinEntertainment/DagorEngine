//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_tab.h>
#include <vecmath/dag_vecMath.h>
#include <daBVH/dag_bvhBuild.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include <stdint.h>

namespace build_bvh
{
// Tight upper bound on tree bytes for a quad-BVH built by create_bvh_node_sah.
// writeDoubleQuadBVH2 suppresses the root internal node's own box (its parent -- the TLAS leaf
// or single-BLAS caller -- carries that bbox), so only (internals - 1) internals emit bytes.
// Leaves always emit. When root is itself a leaf (single-prim BLAS), internals == 0 and the
// clamp collapses to prims_count * BVH_BLAS_LEAF_SIZE = one leaf.
inline int calcBLASTreeBytes(int total_nodes, int prims_count)
{
  const int internals = total_nodes - prims_count;
  const int internalBytes = internals > 1 ? (internals - 1) * BVH_BLAS_NODE_SIZE : 0;
  return internalBytes + prims_count * BVH_BLAS_LEAF_SIZE;
}

// Greedy edge-paired quad primitives + BVH serialization

struct QuadPrim
{
  uint32_t v[4] = {}; // quad: shared=(v0,v2), apexes=(v1,v3). single: v3=~0u
  bool bFwd = true;   // quad: triB winds v0->v2 across the shared edge (false: v2->v0). encodeQuad turns
                      // this into the leaf's flipSecond so triB keeps its source winding even when the mesh
                      // is not consistently wound. Unused for singles. Default true = manifold-consistent.
  uint8_t user = 0;   // per-leaf user bits (QUAD_LEAF_USER_*), opaque to daBVH: prims that disagree are
                      // never combined. Rides on the prim because prim order is not face order.
  uint32_t v0() const { return v[0]; }
  uint32_t v1() const { return v[1]; }
  uint32_t v2() const { return v[2]; }
  uint32_t v3() const { return v[3]; }
  bool isSingle() const { return v[3] == ~0u; }
};

// A double-quad leaf primitive: quad a plus an optional second quad b (= up to 4 triangles).
// buildDoubleQuadPrims pairs spatially-adjacent quads into these; each becomes one 28-byte leaf.
struct DoubleQuadPrim
{
  QuadPrim a;
  QuadPrim b;
  bool hasB;
};

// Pair quad prims into double-quad prims via SAH-sibling pairing: build a scratch SAH tree over the
// quad boxes, then pair leaves that share an internal-node parent when their union AABB stays tight
// (SA-ratio gate, merge_factor). If vert_group != nullptr (one group id per vertex), two quads pair
// only when they share a group -- required where a leaf is attributed to a single source by its first
// vertex (collision per-node filtering / tri_ref). Quads whose QuadPrim::user differs never pair
// either, so a leaf always carries one user value. Pairs that overflow the leaf offset encoding are
// left unpaired. `out` is cleared and filled.
void buildDoubleQuadPrims(dag::Vector<DoubleQuadPrim> &out, const QuadPrim *prims, int prims_count, const vec4f *verts,
  const uint32_t *vert_group = nullptr, float merge_factor = 1.5f);

// Per-double-quad AABB list for the SAH tree (sets bmin.w = dqIndex, bmax.w = 0).
void addDoubleQuadPrimitivesAABBList(bbox3f *boxes, const DoubleQuadPrim *dq, int dq_count, const vec4f *verts);

// Serialize a pre-built double-quad BVH (nodes/root from create_bvh_node_sah over
// addDoubleQuadPrimitivesAABBList boxes) into the tree byte region. Returns descendant node count.
int writeDoubleQuadBVH2(uint8_t *blasData, const bbox3f *nodes, const DoubleQuadPrim *dq, vec4f scale, vec4f ofs, int vertDataOfs,
  int node, int root, int &dataOffset, int vertStride = 8, bool useHalves = false);

// Serialize a completed double-quad BVH + its float3 vertex positions into a self-contained BLAS byte
// chunk (28-byte leaves, 12-byte float3 verts), laid out [tree][verts]. Returns true on success;
// returns false (and clears out_data) if the tree+verts span would push a leaf base past the unsigned
// 24-bit field (QUAD_BASE_BYTE_MAX). Callers must treat a false result as "no BLAS", not a valid or box result.
bool writeDoubleQuadBLAS(dag::Vector<uint8_t> &out_data, bbox3f box, const bbox3f *nodes, int root, const DoubleQuadPrim *dq,
  int dq_count, const uint8_t *verts_data, int vert_stride_bytes, int verts_count);

// Build greedy edge-paired quad primitives from a triangle mesh.
// Expects indices (optIdx) + vertices (verts4) already prepared by build_bvh::leafOrderVertexFetch
// (SAH-leaf order, quad-window over-spread duplicated; see dag_bvhBuild.h), not the removed meshopt
// vertex-fetch. Returns quad+single primitives suitable for BVH building.
// Instantiated for IdxT = uint16_t and uint32_t; QuadPrim::v[] is uint32 internally either way.
// `prims` uses Tab<T> (MemPtrAllocator) so callers can back it with framemem_ptr() for
// transient per-worker builds, or the default (midmem) for long-lived storage.
// face_user (optional, one QUAD_LEAF_USER_BITS value per SOURCE face, index-aligned with optIdx --
// leafOrderVertexFetch renumbers vertices but never moves faces): two faces pair into a quad only
// when their values match, and the value rides on the prim. nullptr = every prim gets 0, which
// reproduces the pre-user-bits build byte for byte.
template <typename IdxT>
void buildQuadPrims(Tab<QuadPrim> &prims, int &quadCount, int &singleCount, const IdxT *optIdx, int faceCount, const vec4f *verts4,
  const uint8_t *face_user = nullptr);

// Write quantized box node (useHalves: FP16 encoding, otherwise UINT16)
void writeQuadBox(uint8_t *blasData, int dataOffset, vec4f bmin, vec4f bmax, vec4f scale, vec4f ofs, uint32_t skip,
  bool useHalves = false);

// Build per-primitive AABBs for SAH tree (sets bmin.w = primIndex, bmax.w = 0)
void addQuadPrimitivesAABBList(bbox3f *boxes, const QuadPrim *prims, int primCount, const vec4f *verts);

// Pack vertex in box-space [0,65535] as 21-bit-per-component into 8 bytes.
// Format: uint64 with x[20:0] | y[41:21] | z[62:42], stores round(value * 32)
void packVert21(uint8_t *dst, vec4f quantized_xyz);

} // namespace build_bvh
