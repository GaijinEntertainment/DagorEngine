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

struct BVHLeafBBox : public BVHNodeBBox
{
  uint32_t v0v1[3];
  bvh_float v2y, v2z;
};

struct WriteBVHTreeInfo
{
  uint8_t *blasData = nullptr;
  const bbox3f *nodes = nullptr;
  const vec4f *verts = nullptr;
  vec4f scale, ofs;
  const uint8_t *writeByteData = nullptr;
  bool writeFaceIndex = false;
  bool useHalves = true;     // true=FP16 boxes+tris, false=UINT16 boxes+tris
  int triIndexBits = 0;      // 0=embedded vertices, 2=2-bit offsets (max dist 3), 8=8-bit offsets (max dist 255)
  bool quadEncoding = false; // quad leaves: 2 tris per leaf, fan/strip, 13-bit single fallback
  int blasLeafSize = sizeof(BVHLeafBBox);
  int vertOffset = 0;
  int vertStride = 12; // bytes per vertex in the indexed vertex buffer (12 for float3, 16 for Point3_vec4)
};

enum
{
  BLAS_IS_BOX = -1
};

int writeBVHTree(const WriteBVHTreeInfo &info, const uint16_t *indices, int node, int root, int &dataOffset);
int writeBVHTree(const WriteBVHTreeInfo &info, const uint32_t *indices, int node, int root, int &dataOffset);

template <typename IdxT>
bool checkIfIsBox(const IdxT *indices, int index_count, const vec4f *vertices, int vertex_count, const bbox3f &box);

} // namespace build_bvh
