//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_relocatableFixedVector.h>
#include <generic/dag_tab.h>
#include <dag/dag_vector.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_hlsl_floatx.h>

namespace build_bvh
{

static constexpr float blas_size_eps = 0.0001; // 0.1mm

typedef uint16_t bvh_float;
inline void calculateBounds(const bbox3f *bboxData, const bbox3f *end, bbox3f &box)
{
  v_bbox3_init_empty(box);
  for (; bboxData != end; bboxData++)
    v_bbox3_add_box(box, *bboxData);
}

inline float calculateSurfaceArea(const bbox3f &bbox)
{
  vec3f l = v_bbox3_size(bbox);
  l = v_dot3_x(l, v_perm_yzxw(l));
  return v_extract_x(l);
}

// Which axes the SAH sweep may split along. XZ is for heightfield-shaped geometry queried
// mostly by vertical lines and steep rays (land tracers): there the Y-centroid ordering of a
// flat node is spatial noise, and its winning splits pair children that overlap in XZ, which
// every descent then visits twice. Keep XYZ for omnidirectional-ray BLASes: on tilted sheets
// Y splits cut across the height gradient and their thin child slabs reject near misses at
// the first box test (measured: XZ costs 10-20% closest-hit/any-hit on curved-shell meshes,
// wins ~10% on flat-terrain heights and short rays, and is neutral on compact solids).
enum class SplitAxes : uint8_t
{
  XYZ,
  XZ
};

// `nodes` output uses Tab<T> (MemPtrAllocator) so callers can route growth through any
// IMemAlloc -- pass framemem_ptr() for transient per-worker BLAS builds, or the default
// (midmem) when the tree outlives the caller's stack frame.
// presort_use_radix_threshold: box count from which the presort uses the stable LSD radix
// sort instead of introsort (default = the measured crossover). Equal keys keep their input
// order under radix instead of introsort's tie placement, so the tree BYTES differ between
// the two sorts -- SAH cost, depth and sizes are equivalent (measured via swrtRiBench SAH
// metrics). Pass ~0u to restore the historical introsort permutation bit-for-bit.
// split_axes deliberately precedes presort_use_radix_threshold (against append-at-end evolution
// convention): axis choice is the parameter callers actually vary, and no caller passes the
// threshold positionally.
int create_bvh_node_sah(Tab<bbox3f> &nodes, bbox3f *boxes, const uint32_t boxes_cnt, int max_children_count, int &max_depth,
  SplitAxes split_axes = SplitAxes::XYZ, uint32_t presort_use_radix_threshold = 48);

void addPropToPrimitivesAABBList(bbox3f *boxes, const uint16_t *indices, const vec4f *verts, int faces);
void addPropToPrimitivesAABBList(bbox3f *boxes, const uint32_t *indices, const vec4f *verts, int faces);

bbox3f calcBox(const vec4f *vertices, int vertex_count);

// Sentinel for leafOrderVertexFetch's minOff/maxOff meaning "use the quad-BLAS default range". A real
// minOff is a negative signed-offset bound, so the default must sit outside the valid offset domain
// rather than rely on a sign test that would swallow a caller-requested negative bound.
static constexpr int LEAF_OFF_DEFAULT = -1048576;

// Vertex-layout helper for the quad-BLAS builders, minimizing the duplication forced by the leaf's
// signed offset range. It renumbers a node's referenced verts into the SAH triangle-partition order
// (co-leaf / spatially-adjacent verts get adjacent indices; unreferenced verts dropped) -- a
// span-minimizing replacement for meshopt's vertex-fetch reorder, so a quad leaf's offsets-from-base
// hold almost every triangle. Any triangle no apex can encode within [minOff, maxOff] then has its
// verts duplicated, sharing copies within maxOff-sized blocks (a vertex shared by several over-spread
// tris in one block is copied once) rather than 3 private copies per tri -- this second phase is internal,
// so callers cannot skip or misorder it. idx is in/out: on entry it holds the caller's actual source
// vertex indices (live input, not a zero-filled scratch buffer); it is read and then idx[0..idxCount)
// is renumbered in place to index outVerts, whose final size is returned. `minOff`/`maxOff` are the
// leaf's signed per-vertex offset range; leaving them at LEAF_OFF_DEFAULT selects the quad-BLAS range
// [QUAD_O_MIN, QUAD_O_MAX]. Templated on the index width (uint16_t / uint32_t) like buildQuadPrims;
// uint16 callers must keep outVerts <= 65536.
template <class IdxT>
unsigned leafOrderVertexFetch(IdxT *idx, unsigned idxCount, const vec4f *srcVerts, unsigned srcVertCount, dag::Vector<vec4f> &outVerts,
  int minOff = LEAF_OFF_DEFAULT, int maxOff = LEAF_OFF_DEFAULT);

}; // namespace build_bvh
