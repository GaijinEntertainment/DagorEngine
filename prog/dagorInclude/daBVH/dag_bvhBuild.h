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

// `nodes` output uses Tab<T> (MemPtrAllocator) so callers can route growth through any
// IMemAlloc -- pass framemem_ptr() for transient per-worker BLAS builds, or the default
// (midmem) when the tree outlives the caller's stack frame.
int create_bvh_node_sah(Tab<bbox3f> &nodes, bbox3f *boxes, const uint32_t boxes_cnt, int max_children_count, int &max_depth);

void addPropToPrimitivesAABBList(bbox3f *boxes, const uint16_t *indices, const vec4f *verts, int faces);
void addPropToPrimitivesAABBList(bbox3f *boxes, const uint32_t *indices, const vec4f *verts, int faces);

bbox3f calcBox(const vec4f *vertices, int vertex_count);

// Vertex-layout helper for the quad-BLAS builders, minimizing the duplication forced by the leaf's
// fixed offset window. It renumbers a node's referenced verts into the SAH triangle-partition order
// (co-leaf / spatially-adjacent verts get adjacent indices; unreferenced verts dropped) -- a
// span-minimizing replacement for meshopt's vertex-fetch reorder, so a quad leaf's offsets-from-base
// hold almost every triangle. Any triangle still spanning more than `window` then has its verts
// duplicated, sharing copies within window-sized blocks (a vertex shared by several over-spread tris
// in one block is copied once) rather than 3 private copies per tri -- this second phase is internal,
// so callers cannot skip or misorder it. idx is in/out: on entry it holds the caller's actual source
// vertex indices (live input, not a zero-filled scratch buffer); it is read and then idx[0..idxCount)
// is renumbered in place to index outVerts, whose final size is returned. `window` is the leaf's
// per-vertex offset range (QUAD_O1_MAX + 1 for quad BLAS). Templated on the index width (uint16_t /
// uint32_t) like buildQuadPrims; uint16 callers must keep outVerts <= 65536.
template <class IdxT>
unsigned leafOrderVertexFetch(IdxT *idx, unsigned idxCount, const vec4f *srcVerts, unsigned srcVertCount, unsigned window,
  dag::Vector<vec4f> &outVerts);

}; // namespace build_bvh
