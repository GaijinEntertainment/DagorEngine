// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daBVH/dag_bvhSerialization.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include "shaders/swBVHDefine.hlsli"
#include <vecmath/dag_vecMath.h>
#include <util/dag_stlqsort.h>
#include <perfMon/dag_cpuFreq.h>
#include <generic/dag_carray.h>
#include "swCommon.h"

namespace build_bvh
{

inline void writeStopBVHNode(const WriteBVHTreeInfo &info, int &data_offset)
{
  BVHNodeBBox *node = (BVHNodeBBox *)(info.blasData + data_offset);
  data_offset += sizeof(BVHNodeBBox);
  memset(node, 0, sizeof(BVHNodeBBox));

  node->skip = 0; // empty node
}

static void write_blas_box(const WriteBVHTreeInfo &info, uint32_t *data, vec4f bmin, vec4f bmax, vec4i &bminI, vec4i &bmaxI)
{
  bmin = v_madd(bmin, info.scale, info.ofs);
  bmax = v_madd(bmax, info.scale, info.ofs);
  if (info.useHalves)
  {
    bminI = v_float_to_half_down(bmin);
    bmaxI = v_float_to_half_up(bmax);
  }
  else
  {
    bminI = v_clampi(v_cvt_floori(bmin), v_zeroi(), v_splatsi(65535));
    bmaxI = v_clampi(v_cvt_ceili(bmax), v_zeroi(), v_splatsi(65535));
  }
  write_pair_halves(data, bminI, bmaxI);
}

static inline void decode_blas_box(const WriteBVHTreeInfo &info, vec4i bminI, vec4i bmaxI, vec4f &bmin, vec4f &bmax)
{
  if (info.useHalves)
  {
    bmin = v_half_to_float(bminI);
    bmax = v_half_to_float(bmaxI);
  }
  else
  {
    bmin = v_cvt_vec4f(bminI);
    bmax = v_cvt_vec4f(bmaxI);
  }
}

static void write_blas_box(const WriteBVHTreeInfo &info, uint32_t *data, vec4f bmin, vec4f bmax)
{
  vec4i mni, mxi;
  write_blas_box(info, data, bmin, bmax, mni, mxi);
}

static void writeBVHTreeLeaf(const WriteBVHTreeInfo &info, const bbox3f box, uint32_t faceIndex, const uint32_t index[3],
  int &dataOffset)
{
  // leaf node, write triangle vertices
  BVHNodeBBox *nodeBbox = (BVHNodeBBox *)(info.blasData + dataOffset);
  vec4i bminI, bmaxI;
  write_blas_box(info, nodeBbox->minMax, box.bmin, box.bmax, bminI, bmaxI);
  unsigned int first = 0;
  if (info.quadEncoding)
  {
    // quad encoding single-tri leaf: unified bit layout (9+9+11), o3=o2 so v3==v2
    int minIndex = min(min(index[0], index[1]), index[2]);
    if (index[1] == minIndex)
      first = 1;
    if (index[2] == minIndex)
      first = 2;
    const int leafOfs = dataOffset + sizeof(BVHNodeBBox);
    *(int *)(info.blasData + leafOfs) = int(index[first] * info.vertStride + info.vertOffset) - leafOfs;
    int o1 = index[(first + 1) % 3] - minIndex - 1;
    int o2 = index[(first + 2) % 3] - minIndex - 1;
    G_ASSERTF(o1 >= 0 && (unsigned)o1 <= QUAD_O1_MAX && o2 >= 0 && (unsigned)o2 <= QUAD_O2_MAX,
      "quad single offset overflow: o1=%d (max %d) o2=%d (max %d)", o1, QUAD_O1_MAX, o2, QUAD_O2_MAX);
    nodeBbox->skip =
      (o1 & QUAD_O1_MASK) | (((unsigned)o2 & QUAD_O2_MASK) << QUAD_O2_SHIFT) | (((unsigned)o2 & QUAD_O3_MASK) << QUAD_O3_SHIFT);
  }
  else if (info.triIndexBits > 0)
  {
    const int bits = info.triIndexBits;
    const unsigned mask = (1u << bits) - 1;
    int minIndex = min(min(index[0], index[1]), index[2]);
    int maxDist = max(max(index[0], index[1]), index[2]) - minIndex;
    G_ASSERTF(maxDist <= (int)(mask + 1), "index dist %d exceeds max %d, need stripification", maxDist, mask + 1);
    if (index[1] == minIndex)
      first = 1;
    if (index[2] == minIndex)
      first = 2;
    const int leafOfs = dataOffset + sizeof(BVHNodeBBox);
    *(int *)(info.blasData + leafOfs) = int(index[first] * info.vertStride + info.vertOffset) - leafOfs;
    if (maxDist <= mask + 1)
      nodeBbox->skip = (((index[(first + 1) % 3] - minIndex - 1) & mask) | (((index[(first + 2) % 3] - minIndex - 1) & mask) << bits));
    else // degenerate
      nodeBbox->skip = ((index[(first + 1) % 3] - minIndex - 1) & mask);
  }
  else
  {
    BVHLeafBBox *bbox = (BVHLeafBBox *)(info.blasData + dataOffset);
    vec4f v0 = info.verts[index[0]], v1 = info.verts[index[1]], v2 = info.verts[index[2]];

    v0 = v_madd(v0, info.scale, info.ofs);
    v1 = v_madd(v1, info.scale, info.ofs);
    v2 = v_madd(v2, info.scale, info.ofs);
    uint16_t halves[4];
    if (info.useHalves)
    {
      write_pair_halves(bbox->v0v1, v_float_to_half_rtne(v0), v_float_to_half_rtne(v1));
      v_write_float_to_half_round(halves, v2);
    }
    else
    {
      v0 = v_add(v0, V_C_HALF);
      v1 = v_add(v1, V_C_HALF);
      v2 = v_add(v2, V_C_HALF);
      vec4i v0i = v_cvt_vec4i(v_clamp(v0, v_zero(), v_splats(65535.f)));
      vec4i v1i = v_cvt_vec4i(v_clamp(v1, v_zero(), v_splats(65535.f)));
      vec4i v2i = v_cvt_vec4i(v_clamp(v2, v_zero(), v_splats(65535.f)));
      write_pair_halves(bbox->v0v1, v0i, v1i);
      v_stui_half(halves, v_packus(v2i));
    }
    memcpy(&bbox->v2y, halves + 1, sizeof(int16_t) * 2); //-V512

    nodeBbox->skip = halves[0];
  }

  nodeBbox->skip |= QUAD_LEAF_FLAG;
  if (info.writeFaceIndex)
  {
    G_ASSERT(faceIndex <= 32767);
    nodeBbox->skip |= faceIndex << 16;
  }
  else if (info.writeByteData)
  {
    const uint32_t v = (info.writeByteData[index[first]] >> 3) | ((info.writeByteData[index[(first + 1) % 3]] >> 3) << 5) |
                       ((info.writeByteData[index[(first + 2) % 3]] >> 3) << 10);
    G_ASSERT(v <= 32767);
    nodeBbox->skip |= v << 16;
  }
  dataOffset += info.blasLeafSize;
}
template <class Indices>
static void writeBVHTreeLeaf(const WriteBVHTreeInfo &info, const Indices *indices, const bbox3f box, int tri1, int &dataOffset)
{
  // leaf node, write triangle vertices
  const int ind = tri1 * 3;
  const uint32_t index[3] = {indices[ind + 0], indices[ind + 1], indices[ind + 2]};
  writeBVHTreeLeaf(info, box, ind, index, dataOffset);
}

template <class Indices>
static int writeBVHTree(const WriteBVHTreeInfo &info, const Indices *indices, int node, int root, int &dataOffset, int depth)
{
  G_STATIC_ASSERT(sizeof(BVHLeafBBox) == 32 && sizeof(BVHNodeBBox) == 16);
  G_STATIC_ASSERT( //-V1063
    BVH_BLAS_NODE_SIZE == sizeof(BVHNodeBBox) / BVH_BLAS_ELEM_SIZE && sizeof(BVHNodeBBox) % BVH_BLAS_ELEM_SIZE == 0);
  G_ASSERTF(depth <= BVH_MAX_BLAS_DEPTH, "%d blas depth too big, we should rebuild tree with better balancing", depth);
  if (info.triIndexBits > 0 || info.quadEncoding)
    G_ASSERT(info.blasLeafSize >= (int)(sizeof(BVHNodeBBox) + sizeof(uint32_t)));
  else
    G_ASSERT(info.blasLeafSize >= (int)sizeof(BVHLeafBBox));

  int faceIndex = v_extract_wi(v_cast_vec4i(info.nodes[node].bmin));
  int childrenCount = v_extract_wi(v_cast_vec4i(info.nodes[node].bmax));
  G_ASSERT(node != root || faceIndex < 0);

  if (faceIndex < 0) // intermediate node
  {
    G_ASSERT(childrenCount > 0);
    const int nodeSize = -faceIndex;

    int tempdataOffset = 0;
    if (node != root)
    {
      dataOffset += sizeof(BVHNodeBBox);
      tempdataOffset = dataOffset;
    }

    int startNode = node + 1;
    for (int i = 0; i < childrenCount; ++i)
      startNode += writeBVHTree(info, indices, startNode, root, dataOffset, depth + 1);
    G_ASSERTF(startNode == nodeSize + node + 1, "startNode = %d nodeSize = %d + node=%d + 1", startNode, nodeSize, node);

    if (node != root)
    {
      const int offset = (dataOffset - tempdataOffset) / BVH_BLAS_ELEM_SIZE;
      G_ASSERT(dataOffset % BVH_BLAS_ELEM_SIZE == 0 && tempdataOffset % BVH_BLAS_ELEM_SIZE == 0);
      G_ASSERT(offset != info.blasLeafSize);
      BVHNodeBBox *bbox = (BVHNodeBBox *)(info.blasData + tempdataOffset - sizeof(BVHNodeBBox));

      vec4f bmin = info.nodes[node].bmin, bmax = info.nodes[node].bmax;
      write_blas_box(info, bbox->minMax, bmin, bmax);
      bbox->skip = offset;
    }
    return nodeSize + 1;
  }
  else
  {
    G_ASSERT(childrenCount == 0);
    writeBVHTreeLeaf(info, indices, info.nodes[node], faceIndex, dataOffset);
    return 1;
  }
}

int writeBVHTree(const WriteBVHTreeInfo &info, const uint16_t *indices, int node, int root, int &dataOffset)
{
  return build_bvh::writeBVHTree(info, indices, node, root, dataOffset, 1);
}

int writeBVHTree(const WriteBVHTreeInfo &info, const uint32_t *indices, int node, int root, int &dataOffset)
{
  return build_bvh::writeBVHTree(info, indices, node, root, dataOffset, 1);
}

template <typename IdxT>
bool checkIfIsBox(const IdxT *indices, int index_count, const vec4f *vertices, int vertex_count, const bbox3f &box)
{
  if (index_count != 12 * 3 || vertex_count != 8)
    return false;
  vec4f threshold = v_max(v_mul(v_splats(1e-6f), v_bbox3_size(box)), v_splats(1e-6f));
  for (int i = 0; i < vertex_count; ++i)
  {
    vec4f minMaskV = v_cmp_lt(v_abs(v_sub(vertices[i], box.bmin)), threshold);
    vec4f maxMaskV = v_cmp_lt(v_abs(v_sub(vertices[i], box.bmax)), threshold);
    vec4f mask = v_or(minMaskV, maxMaskV);
    if (!v_check_xyz_all_true(mask))
      return false;
  }
  for (int i = 0; i < index_count; i += 3)
  {
    vec4f v0 = vertices[indices[i]];
    const vec4f n = v_cross3(v_sub(vertices[indices[i + 1]], v0), v_sub(vertices[indices[i + 2]], v0));
    const vec4f threshold = v_mul(v_length3(n), v_splats(1e-6f));
    const vec4f nA = v_abs(n);
    if (__popcount(7 & v_signmask(v_cmp_gt(nA, threshold))) != 1)
      return false;
  }
  return true;
}

template bool checkIfIsBox<uint16_t>(const uint16_t *, int, const vec4f *, int, const bbox3f &);
template bool checkIfIsBox<uint32_t>(const uint32_t *, int, const vec4f *, int, const bbox3f &);

void packVert21(uint8_t *dst, vec4f quantized_xyz)
{
  // Simple 21-bit-per-component: x[20:0] | y[41:21] | z[62:42]
  // Store round(value * 32), max value = 65535 * 32 = 2097120 < 2^21
  alignas(16) float f[4];
  v_st(f, v_mul(quantized_xyz, v_splats(32.f)));
  uint64_t x = clamp((int)f[0], 0, 0x1FFFFF);
  uint64_t y = clamp((int)f[1], 0, 0x1FFFFF);
  uint64_t z = clamp((int)f[2], 0, 0x1FFFFF);
  uint64_t packed = x | (y << 21) | (z << 42);
  memcpy(dst, &packed, 8);
}

}; // namespace build_bvh
