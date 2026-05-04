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

}; // namespace build_bvh
