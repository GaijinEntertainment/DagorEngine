//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_tab.h>
#include <vecmath/dag_vecMath.h>
#include <daBVH/dag_bvhBuild.h>
#include <stdint.h>

namespace build_bvh
{
// Greedy edge-paired quad primitives + BVH serialization

struct QuadPrim
{
  uint32_t v[4]; // quad: shared=(v0,v2), unique=(v1,v3). single: v3=~0u
  uint32_t v0() const { return v[0]; }
  uint32_t v1() const { return v[1]; }
  uint32_t v2() const { return v[2]; }
  uint32_t v3() const { return v[3] & 0x7FFFFFFF; }
  bool isFan() const { return v[3] >> 31; }
  bool isSingle() const { return v[3] == ~0u; }
};

// Build greedy edge-paired quad primitives from a triangle mesh.
// Requires meshopt-optimized indices (optIdx) and vertices (verts4).
// Returns quad+single primitives suitable for BVH building.
// Instantiated for IdxT = uint16_t and uint32_t; QuadPrim::v[] is uint32 internally either way.
// `prims` uses Tab<T> (MemPtrAllocator) so callers can back it with framemem_ptr() for
// transient per-worker builds, or the default (midmem) for long-lived storage.
template <typename IdxT>
void buildQuadPrims(Tab<QuadPrim> &prims, int &quadCount, int &singleCount, const IdxT *optIdx, int faceCount, const vec4f *verts4);

// Write quantized box node (useHalves: FP16 encoding, otherwise UINT16)
void writeQuadBox(uint8_t *blasData, int dataOffset, vec4f bmin, vec4f bmax, vec4f scale, vec4f ofs, uint32_t skip,
  bool useHalves = false);

// Write a single quad/single leaf
void writeQuadLeaf(uint8_t *blasData, const bbox3f *nodes, const QuadPrim *prims, vec4f scale, vec4f ofs, int vertDataOfs, int node,
  int &dataOffset, int vertStride = 8, bool useHalves = false);

// Serialize pre-paired quad BVH to buffer (no sibling merging, just walks the BVH)
int writeQuadBVH2(uint8_t *blasData, const bbox3f *nodes, const QuadPrim *prims, vec4f scale, vec4f ofs, int vertDataOfs, int node,
  int root, int &dataOffset, int vertStride = 8, bool useHalves = false);

// Serialize a completed quad-prim BVH + its vertex positions into a self-contained BLAS byte chunk
// (uint16 tree nodes followed by per-vertex float3 quantized into [0, 65535] space, ready for
// downstream FP16 re-encode). out_data is resized to the final chunk size.
//
// Root-agnostic: handles both internal-root trees (multi-primitive) and leaf-root trees (single-primitive).
//
// Vertex input is a raw byte pointer + stride so arbitrary vertex struct layouts work: the first
// 12 bytes at every vert_stride_bytes offset are read as (x, y, z) floats. Any trailing fields
// (normal, uv, colour, padding) are skipped. vert_stride_bytes must be >= 12.
// Positions must be finite and in the range covered by `box`.
//
// Preconditions: prims and nodes built via buildQuadPrims + create_bvh_node_sah; root indexes into nodes.
void writeQuadBLAS(dag::Vector<uint8_t> &out_data, bbox3f box, const bbox3f *nodes, int root, const QuadPrim *prims, int prims_count,
  const uint8_t *verts_data, int vert_stride_bytes, int verts_count);

// Build per-primitive AABBs for SAH tree (sets bmin.w = primIndex, bmax.w = 0)
void addQuadPrimitivesAABBList(bbox3f *boxes, const QuadPrim *prims, int primCount, const vec4f *verts);

// Pack vertex in box-space [0,65535] as 21-bit-per-component into 8 bytes.
// Format: uint64 with x[20:0] | y[41:21] | z[62:42], stores round(value * 32)
void packVert21(uint8_t *dst, vec4f quantized_xyz);

} // namespace build_bvh
