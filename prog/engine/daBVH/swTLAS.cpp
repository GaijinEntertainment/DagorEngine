// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daBVH/dag_swTLAS.h>
#include <util/dag_stlqsort.h>
#include <math/dag_TMatrix.h>
#include <EASTL/unique_ptr.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_carray.h>

#include "swCommon.h"
#include "shaders/swBVHDefine.hlsli"

namespace build_bvh
{

inline TLASNode *writeTLASNode(WriteTLASTreeInfo &info, bbox3f_cref box)
{
  TLASNode *node = (TLASNode *)(info.bboxData + info.nodesOffset);
  info.nodesOffset += sizeof(TLASNode);

  G_ASSERT(info.nodesOffset <= info.nodesSize);

  vec4i bminI, bmaxI;
  if (info.useHalves)
  {
    bminI = v_float_to_half_down(box.bmin);
    bmaxI = v_float_to_half_up(box.bmax);
  }
  else
  {
    vec4f bmin = v_madd(box.bmin, info.scale, info.ofs);
    vec4f bmax = v_madd(box.bmax, info.scale, info.ofs);
    bminI = v_clampi(v_cvt_floori(bmin), v_zeroi(), v_splatsi(65535));
    bmaxI = v_clampi(v_cvt_ceili(bmax), v_zeroi(), v_splatsi(65535));
  }
  write_pair_halves(node->minMax, bminI, bmaxI);

  return node;
}

inline void writeStopTLASNode(WriteTLASTreeInfo &info)
{
  TLASNode *node = (TLASNode *)(info.bboxData + info.nodesOffset);
  memset(node, 0, sizeof(TLASNode));
  info.nodesOffset += sizeof(TLASNode);
  G_ASSERT(info.nodesOffset <= info.nodesSize);
}

static void writeTLASLeaf(WriteTLASTreeInfo &info, bbox3f_cref box, const mat43f &tm_, bbox3f_cref blas_box, int bvh_start,
  uint32_t bvh_size, uint16_t dim_as_box_dist)
{
  TLASNode *leafNode = (TLASNode *)writeTLASNode(info, box);
  leafNode->skip = info.leavesOffset | (1 << 31);

  TLASLeaf *leaf = (TLASLeaf *)(info.bboxData + info.leavesOffset);
  const bool isBox = (bvh_size == 0);
  G_ASSERT(isBox == (dim_as_box_dist == 0)); // bvh_size==0 and distToTreatAsBox==0 must agree
  const uint32_t leafBaseSize = isBox ? TLASLeaf::BOX_SIZE : sizeof(TLASLeaf);
  info.leavesOffset += leafBaseSize + info.leafExtraSize;
  G_ASSERT(info.leavesOffset <= info.leavesSize && info.leavesOffset % 4 == 0);
  const vec3f ext = v_mul(V_C_HALF, v_max(v_bbox3_size(blas_box), v_splats(blas_size_eps)));
  const vec3f center = v_bbox3_center(blas_box);

  vec3f origin = v_mat43_mul_vec3p(tm_, center);
  mat44f m44;
  v_mat43_transpose_to_mat44(m44, tm_);
  m44.col0 = v_mul(m44.col0, v_splat_x(ext));
  m44.col1 = v_mul(m44.col1, v_splat_y(ext));
  m44.col2 = v_mul(m44.col2, v_splat_z(ext));
  vec3f scale = v_perm_xycd(v_perm_xaxa(v_length3_x(m44.col0), v_length3(m44.col1)), v_length3(m44.col2));
  const vec3f maxScale = v_max(v_max(scale, v_splat_y(scale)), v_splat_z(scale));
  m44.col0 = v_div(m44.col0, v_splat_x(scale));
  m44.col1 = v_div(m44.col1, v_splat_y(scale));
  m44.col2 = v_div(m44.col2, v_splat_z(scale));
  scale = v_rcp(v_max(scale, v_splats(1e-29f)));
  mat33f m33, invTm;
  m33.col0 = m44.col0;
  m33.col1 = m44.col1;
  m33.col2 = m44.col2;
  v_mat33_transpose(invTm, m33); // inversed == transposed for orthonormalized matrices

  // Detect reflection (negative determinant): cross(col0,col1) gives wrong sign for col2.
  // Encode the reflection flag in the sign of scale.x so the decoder can negate col2.
  if (v_extract_x(v_dot3(v_cross3(m33.col0, m33.col1), m33.col2)) < 0.f)
    scale = v_xor(scale, v_cast_vec4f(v_make_vec4i((int)0x80000000, 0, 0, 0)));

  // For uint16 BLAS (non-halves), bake the 32767.5 mapping into origin/scale
  // so traversal doesn't need the per-ray multiply+bias at runtime.
  // newScale = scale * 32767.5 maps [-1,1] -> [-32767.5, 32767.5]
  // newOrigin = origin - m33 * (1/|scale|) absorbs the +32767.5 bias
  if (!info.useHalves)
  {
    vec3f absScale = v_and(scale, v_cast_vec4f(v_make_vec4i(0x7FFFFFFF, -1, -1, -1)));
    vec3f halfExtWorld = v_mat33_mul_vec3(m33, v_rcp(absScale));
    origin = v_sub(origin, halfExtWorld);
    scale = v_mul(scale, v_splats(32767.5f));
  }

  memcpy(leaf->origin, &origin, sizeof(float) * 3);
  memcpy(leaf->scale, &scale, sizeof(float) * 3);
  leaf->maxScale = v_extract_xi(v_float_to_half_up(maxScale));
  leaf->distToTreatAsBox = dim_as_box_dist;
  alignas(16) float col0f[4], col1f[4];
  v_st(col0f, invTm.col0);
  v_st(col1f, invTm.col1);
  memcpy(leaf->invTm, col0f, sizeof(float) * 3);     // -V1086
  memcpy(leaf->invTm + 3, col1f, sizeof(float) * 3); // -V1086

  G_STATIC_ASSERT(sizeof(TLASLeaf) % 4 == 0);
  G_STATIC_ASSERT(TLASLeaf::BOX_SIZE % 4 == 0);

  if (!isBox)
  {
    leaf->blasStart = bvh_start;
    leaf->blasSize = bvh_size;
  }
}

static int writeTLASTree(WriteTLASTreeInfo &info, int node, int depth)
{
  int faceIndex = v_extract_wi(v_cast_vec4i(info.nodes[node].bmin));
  int childrenCount = v_extract_wi(v_cast_vec4i(info.nodes[node].bmax));

  if (faceIndex < 0) // intermediate node
  {
    const int nodeSize = -faceIndex;
    const int tempDataOffset = info.nodesOffset;
    TLASNode *tlasNode = writeTLASNode(info, info.nodes[node]);

    int startNode = node + 1;
    for (int i = 0; i < childrenCount; ++i)
      startNode += writeTLASTree(info, startNode, depth + 1);

    const int skip = (info.nodesOffset - tempDataOffset);
    G_ASSERT(startNode == nodeSize + node + 1 && skip % 4 == 0);

    tlasNode->skip = skip;
    return nodeSize + 1;
  }
  else
  {
    int instanceId = faceIndex;
    int modelId = childrenCount;
    const BLASDataInfo &bd = info.blasData[modelId];
    G_ASSERT(info.leavesOffset % 4 == 0);
    if (info.tlasLeavesOffsets)
      (*info.tlasLeavesOffsets)[instanceId] = info.leavesOffset;
    if (bd.isBox())
      info.numBoxLeaves++;
    writeTLASLeaf(info, info.nodes[node], info.tms[instanceId], info.blasBoxes[modelId], bd.start, bd.size, info.boxDist[modelId]);
    return 1;
  }
}

void writeTLASTreeRoot(WriteTLASTreeInfo &info)
{
  writeTLASTree(info, 0, 1);
  writeStopTLASNode(info);
}

}; // namespace build_bvh