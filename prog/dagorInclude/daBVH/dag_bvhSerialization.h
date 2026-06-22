//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <vecmath/dag_vecMathDecl.h>
#include <daBVH/dag_bvhBuild.h>

namespace build_bvh
{

struct BVHNodeBBox
{
  uint32_t minMax[3];
  uint32_t skip;
};

enum
{
  BLAS_IS_BOX = -1
};

template <typename IdxT>
bool checkIfIsBox(const IdxT *indices, int index_count, const vec4f *vertices, int vertex_count, const bbox3f &box);

} // namespace build_bvh
