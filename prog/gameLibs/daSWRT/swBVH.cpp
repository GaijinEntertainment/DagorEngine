// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daSWRT/swBVH.h>
#include <generic/dag_relocatableFixedVector.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_stlqsort.h>
#include <math/dag_TMatrix.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_carray.h>
#include <render/noiseTex.h>
#include <util/dag_parallelFor.h>
#include <osApiWrappers/dag_spinlock.h>
#include <shaders/dag_computeShaders.h>
#include <util/dag_convar.h>
#include <math/dag_viewMatrix.h>
#include <math/dag_hlsl_floatx.h>

#include "shaders/swBVHDefine.hlsli"

namespace build_bvh
{
struct BLASData
{
  Tab<uint8_t> data;
  BLASData(uint32_t s) : data(s) {}
};
struct TLASData
{
  Tab<uint8_t> data;
  TLASData(uint32_t s) : data(s) {}
};

#define GLOBAL_VARS_LIST       \
  VAR(swrt_shadow_target)      \
  VAR(swrt_shadow_target_size) \
  VAR(swrt_checkerboard_frame) \
  VAR(swrt_dir_to_sun_basis)   \
  VAR(swrt_shadow_planes)      \
  VAR(swrt_shadow_mask)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

typedef uint16_t bvh_float;
struct Area
{
  float left, right;
};
struct SplitHelper
{
  dag::Vector<bbox3f> &curNodes;
  uint32_t getCurrentNodes() const { return curNodes.size(); }
  void reWriteNode(uint32_t at, const bbox3f &b) { curNodes[at] = b; }
  void writeNode(const bbox3f &b) { curNodes.push_back(b); }

  dag::Vector<Area> area;
  int maxChildrenCount = 2;
  bool allowAnyChildrenCount = true;
  int maxDepth = 0;
  SplitHelper(dag::Vector<bbox3f> &n) : curNodes(n) {}
  void start() { area.clear(); }
};

typedef SplitHelper CreateTLASHelper;

enum
{
  BLAS_IS_BOX = -1
};

__forceinline void v_write_float_to_half_up(uint16_t *__restrict m, vec4f v) { v_stui_half(m, v_float_to_half_up_lo(v)); }
__forceinline void v_write_float_to_half_down(uint16_t *__restrict m, vec4f v) { v_stui_half(m, v_float_to_half_down_lo(v)); }

__forceinline void v_write_float_to_half_round(uint16_t *__restrict m, vec4f v) { v_stui_half(m, v_float_to_half_rtne_lo(v)); }

static constexpr float blas_size_eps = 0.0001; // 0.1mm
inline void calculateBounds(const bbox3f *bboxData, const bbox3f *end, bbox3f &box)
{
  v_bbox3_init_empty(box);
  for (; bboxData != end; bboxData++)
    v_bbox3_add_box(box, *bboxData);
}

static void sortAlongAxis(bbox3f *boxes, bbox3f *end, int axis)
{
  const int axisMask = 1 << axis;
  stlsort::sort(boxes, end, [axisMask](const bbox3f &a, const bbox3f &b) {
    int mask = v_signmask(v_cmp_gt(v_add(b.bmin, b.bmax), v_add(a.bmin, a.bmax)));
    return mask & axisMask;
  });
}

inline float calculateSurfaceArea(const bbox3f &bbox)
{
  vec3f l = v_bbox3_size(bbox);
  l = v_dot3_x(l, v_perm_yzxw(l));
  return v_extract_x(l);
}

// based on https://github.com/kayru/RayTracedShadows/blob/master/Source/BVHBuilder.cpp

static int findBestSplit(bbox3f *bboxData, bbox3f *end, SplitHelper &h, Point2 &surface_area, Point2 &costSplit)
{
  const int count = end - bboxData;

  int axis = 0;
  int bestSplit = 0;
  // int globalBestSplit = begin;
  float splitCost = FLT_MAX;

  int split = 0;
  h.area.resize(count);

  for (int i = 0; i < 3; i++)
  {
    sortAlongAxis(bboxData, end, i);
    bbox3f boundsLeft, boundsRight;
    v_bbox3_init_empty(boundsLeft);
    v_bbox3_init_empty(boundsRight);

    for (int indexLeft = 0; indexLeft < count; ++indexLeft)
    {
      int indexRight = count - indexLeft - 1;

      v_bbox3_add_box(boundsLeft, bboxData[indexLeft]);
      v_bbox3_add_box(boundsRight, bboxData[indexRight]);

      // h.area[indexLeft].left = indexLeft == 0 ? calculateTriArea(bboxData[indexLeft], h) : calculateSurfaceArea(boundsLeft);
      // h.area[indexRight].right = indexLeft == 0 ? calculateTriArea(bboxData[indexRight], h) : calculateSurfaceArea(boundsRight);
      h.area[indexLeft].left = calculateSurfaceArea(boundsLeft);
      h.area[indexRight].right = calculateSurfaceArea(boundsRight);
    }

    float bestCost = FLT_MAX;
    Point2 bestArea(0, 0);
    Point2 bestCostSplit(0, 0);
    for (int mid = 1; mid < count; ++mid)
    {
      float surfaceAreaLeft = h.area[mid - 1].left;
      float surfaceAreaRight = h.area[mid].right;

      float childrenCostLeft = mid; // we actually better use actual cost. for tlas it is very different than one (number of triangles)
      float childrenCostRight = count - mid;

      float costLeft = surfaceAreaLeft * childrenCostLeft;
      float costRight = surfaceAreaRight * childrenCostRight;

      float cost = costLeft + costRight;
      if (cost < bestCost)
      {
        bestSplit = mid;
        bestCost = cost;
        bestArea = Point2(surfaceAreaLeft, surfaceAreaRight);
        bestCostSplit = Point2(costLeft, costRight);
      }
    }

    if (bestCost < splitCost)
    {
      split = bestSplit;
      splitCost = bestCost;
      axis = i;
      surface_area = bestArea;
      costSplit = bestCostSplit;
    }
  }

  // sort along that axis
  if (axis != 2) // otherwise already sorted
    sortAlongAxis(bboxData, end, axis);
  return split;
}

struct ChildInfo
{
  uint32_t s = 0, c = 0;
  float area = 0, cost = 0;
};

static int createBVHNodeSAH(bbox3f *bboxData, bbox3f *end, SplitHelper &h, int depth = 1, float currentCost = 1e29f)
{
  const uint32_t count = end - bboxData;
  const int nodeIndex = (int)h.getCurrentNodes();
  h.maxDepth = max(h.maxDepth, depth);

  if (count == 1 && depth != 1)
  {
    // this is a leaf node
    h.writeNode(*bboxData);
  }
  else
  {
    bbox3f bounds;
    calculateBounds(bboxData, end, bounds);

    h.writeNode(bounds);

    const int curChildrenCount = min<int>(count, h.maxChildrenCount);
    enum
    {
      MAX_CHILDREN_COUNT = 8
    };
    G_ASSERT(curChildrenCount < MAX_CHILDREN_COUNT);

    carray<ChildInfo, MAX_CHILDREN_COUNT + 1> children;
    children[0] = ChildInfo{0, count, 0, currentCost};
    int childrenCount = 1;
    // find the best axis to sort along and where the split should be according to SAH
    debug_flush(true);
    for (;;)
    {
      const ChildInfo i = children[childrenCount - 1];
      if (i.c == 1 || i.cost < 0)
        break;
      --childrenCount;
      Point2 area, cost;
      const uint32_t split = findBestSplit(bboxData + i.s, bboxData + i.s + i.c, h, area, cost);
      // debug("depth %d: split %d..%d range childrenCount=%d/%d, split = %d cost %@ currentCost = %f",
      //  depth, i.s, i.c, childrenCount, curChildrenCount,
      //  split, cost, currentCost);
      if (h.allowAnyChildrenCount && cost.x + cost.y > currentCost && (split > 1 || i.s - split > 1))
      {
        children[childrenCount] = i;
        children[childrenCount].cost = -i.cost;
        childrenCount++;
      }
      else
      {
        children[childrenCount++] = ChildInfo{i.s, split, area.x, cost.x};
        children[childrenCount++] = ChildInfo{i.s + split, i.c - split, area.y, cost.y};
        if (childrenCount >= curChildrenCount)
          break;
      }
      stlsort::sort(children.begin(), children.begin() + childrenCount, [](const auto &a, const auto &b) {
        if (b.cost >= 0 && b.c > 1 && a.c == 1) // return a.c > b.c;
          return true;
        return a.cost < b.cost;
      });
    }
    stlsort::sort(children.begin(), children.begin() + childrenCount, [](const auto &a, const auto &b) { return a.area > b.area; });
    // if (h.maxChildrenCount > 2)
    //{
    //   debug("depth %d, children count %d/%d", depth, childrenCount, curChildrenCount);
    //   for (int i = 0; i < childrenCount; ++i)
    //     debug("depth %d: child %d count %d", depth, i, children[i].c);
    // }

    // create the two branches
    const int startAt = h.getCurrentNodes();
    int totalChildrenCount = childrenCount;
    if (h.allowAnyChildrenCount)
    {
      for (int i = 0; i < childrenCount; ++i)
      {
        // debug("depth %d: child %d count %d", depth, i, children[i].c);
        if (children[i].cost < 0)
        {
          totalChildrenCount += children[i].c - 1;
          int ci = children[i].s, ce = ci + children[i].c;
          stlsort::sort(bboxData + ci, bboxData + ce,
            [](const auto &a, const auto &b) { return calculateSurfaceArea(a) > calculateSurfaceArea(b); });
          for (; ci < ce; ++ci)
            h.writeNode(bboxData[ci]);
        }
      }
    }
    for (int i = 0; i < childrenCount; ++i)
    {
      // debug("depth %d: child %d count %d", depth, i, children[i].c);
      if (!h.allowAnyChildrenCount || children[i].cost > 0)
        createBVHNodeSAH(bboxData + children[i].s, bboxData + children[i].s + children[i].c, h, depth + 1, children[i].cost);
    }
    const int endAt = h.getCurrentNodes();

    // this is an intermediate Node
    bounds.bmin = v_perm_xyzd(bounds.bmin, v_cast_vec4f(v_splatsi(-(endAt - startAt))));
    bounds.bmax = v_perm_xyzd(bounds.bmax, v_cast_vec4f(v_splatsi(totalChildrenCount)));
    h.reWriteNode(nodeIndex, bounds);
    // node.bmin = v_perm_xyzd(node.bmin, v_cast_vec4f(v_splatsi(-1-leftIndex)));
    // node.bmax = v_perm_xyzd(node.bmax, v_cast_vec4f(v_splatsi(-1-rightIndex)));
  }

  return nodeIndex;
}

static void addPropToPrimitivesAABBList(bbox3f *boxes, const uint16_t *indices, const vec4f *verts, int faces)
{
  for (uint32_t j = 0; j < faces; j++, indices += 3, boxes++)
  {
    int index0 = indices[0];
    int index1 = indices[1];
    int index2 = indices[2];

    bbox3f box;
    v_bbox3_init(box, verts[index0]);
    v_bbox3_add_pt(box, verts[index1]);
    v_bbox3_add_pt(box, verts[index2]);
    box.bmin = v_perm_xyzd(box.bmin, v_cast_vec4f(v_splatsi(j)));
    box.bmax = v_perm_xyzd(box.bmax, v_cast_vec4f(v_splatsi(0)));
    *boxes = box;
  }
}

struct WriteTLASTreeInfo
{
  const bbox3f *nodes;
  uint8_t *bboxData;
  const mat43f *tms;
  const int *bvhStartOffsets;
  const bbox3f *blasBoxes = nullptr;
  const uint16_t *boxDist = nullptr;
  dag::Vector<int> *tlasLeavesOffsets = nullptr;
  uint32_t nodesOffset = 0, nodesSize = 0;
  uint32_t leavesOffset = 0, leavesSize = 0;
};

struct WriteBVHTreeInfo
{
  uint8_t *bboxData;
  const bbox3f *nodes = nullptr;
  const uint16_t *indices = nullptr;
  const vec4f *verts = nullptr;
  vec4f scale, ofs;
};


struct TLASNode
{
  uint32_t minMax[3];
  uint32_t skip;
};

struct TLASLeaf
{
  float origin[3];
  float scale[3];

  uint32_t invTm[3]; // 3x2

  uint32_t blasStart;
  bvh_float maxScale;
  uint16_t distToTreatAsBox;
};

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

inline void writeStopBVHNode(const WriteBVHTreeInfo &info, int &data_offset)
{
  BVHNodeBBox *node = (BVHNodeBBox *)(info.bboxData + data_offset);
  data_offset += sizeof(BVHNodeBBox);
  memset(node, 0, sizeof(BVHNodeBBox));

  node->skip = 0; // empty node
}

static void write_pair_halves(uint32_t *dest, vec4i v0, vec4i v1)
{
  vec4i tm = v_ori(v_andi(v0, v_splatsi(0xffff)), v_sll(v1, 16));
  alignas(16) uint32_t tmi[4];
  v_sti(tmi, tm);
  memcpy(dest, tmi, sizeof(uint16_t) * 6);
}

static void writeBVHTreeLeaf(const WriteBVHTreeInfo &info, const bbox3f box, int tri1, int &dataOffset)
{
  // leaf node, write triangle vertices
  {
    const int ind = tri1 * 3;
    const int index0 = (info.indices[ind + 0]);
    const int index1 = (info.indices[ind + 1]);
    const int index2 = (info.indices[ind + 2]);
    BVHLeafBBox *bbox = (BVHLeafBBox *)(info.bboxData + dataOffset);
    vec4f v0 = info.verts[index0], v1 = info.verts[index1], v2 = info.verts[index2];

    v0 = v_madd(v0, info.scale, info.ofs);
    v1 = v_madd(v1, info.scale, info.ofs);
    v2 = v_madd(v2, info.scale, info.ofs);
    v1 = v_sub(v1, v0);
    v2 = v_sub(v2, v0);
    write_pair_halves(bbox->v0v1, v_float_to_half_rtne(v0), v_float_to_half_rtne(v1));
    uint16_t halves[4];
    v_write_float_to_half_round(halves, v2);
    memcpy(&bbox->v2y, halves + 1, sizeof(int16_t) * 2);

    vec4f bmin = box.bmin, bmax = box.bmax;
    bmin = v_madd(bmin, info.scale, info.ofs);
    bmax = v_madd(bmax, info.scale, info.ofs);
    write_pair_halves(bbox->minMax, v_float_to_half_down(bmin), v_float_to_half_up(bmax));

    bbox->skip = halves[0] | 0x80000000;
    // when on the left branch, how many half4 elements we need to skip to reach the right branch?
    dataOffset += sizeof(BVHLeafBBox);
  }
}

static int writeBVHTree(const WriteBVHTreeInfo &info, int node, int root, int &dataOffset, int depth = 1)
{
  G_STATIC_ASSERT(BVH_BLAS_LEAF_SIZE == sizeof(BVHLeafBBox) / BVH_BLAS_ELEM_SIZE && sizeof(BVHLeafBBox) % BVH_BLAS_ELEM_SIZE == 0);
  G_STATIC_ASSERT(BVH_BLAS_NODE_SIZE == sizeof(BVHNodeBBox) / BVH_BLAS_ELEM_SIZE && sizeof(BVHNodeBBox) % BVH_BLAS_ELEM_SIZE == 0);
  G_ASSERTF(depth <= BVH_MAX_BLAS_DEPTH, "%d blas depth too big, we should rebuild tree with better balancing");

  int faceIndex = v_extract_wi(v_cast_vec4i(info.nodes[node].bmin));
  int childrenCount = v_extract_wi(v_cast_vec4i(info.nodes[node].bmax));
  G_ASSERT(node != root || faceIndex < 0);

  if (faceIndex < 0) // intermediate node
  {
    G_ASSERT(childrenCount > 0);
    const int nodeSize = -faceIndex;

    int tempdataOffset = 0;
    // do not write the root node, as Top structure will only allow rays that do intersect node
    if (node != root)
    {
      // this is an intermediate node, write bounding box

      dataOffset += sizeof(BVHNodeBBox);

      tempdataOffset = dataOffset;
    }

    // check if we can collapse two sister leaf nodes
    int startNode = node + 1;
    for (int i = 0; i < childrenCount; ++i)
      startNode += writeBVHTree(info, startNode, root, dataOffset, depth + 1);
    G_ASSERTF(startNode == nodeSize + node + 1, "startNode = %d nodeSize = %d + node=%d + 1", startNode, nodeSize, node);

    if (node != root)
    {
      // when on the left branch, how many half4 elements we need to skip to reach the right branch?
      const int offset = (dataOffset - tempdataOffset) / BVH_BLAS_ELEM_SIZE;
      G_ASSERT(dataOffset % BVH_BLAS_ELEM_SIZE == 0 && tempdataOffset % BVH_BLAS_ELEM_SIZE == 0);
      G_ASSERT(offset != BVH_BLAS_LEAF_SIZE); // should not be happening, as leaf is 3, node is 2, and we have two nodes written
      BVHNodeBBox *bbox = (BVHNodeBBox *)(info.bboxData + tempdataOffset - sizeof(BVHNodeBBox));
      vec4f bmin = info.nodes[node].bmin, bmax = info.nodes[node].bmax;
      bmin = v_madd(bmin, info.scale, info.ofs);
      bmax = v_madd(bmax, info.scale, info.ofs);
      write_pair_halves(bbox->minMax, v_float_to_half_down(bmin), v_float_to_half_up(bmax));
      bbox->skip = offset;
    }
    return nodeSize + 1;
  }
  else
  {
    G_ASSERT(childrenCount == 0);
    writeBVHTreeLeaf(info, info.nodes[node], faceIndex, dataOffset);
    return 1;
  }
}

inline TLASNode *writeTLASNode(WriteTLASTreeInfo &info, bbox3f_cref box)
{
  TLASNode *node = (TLASNode *)(info.bboxData + info.nodesOffset);
  info.nodesOffset += sizeof(TLASNode);
  ;
  G_ASSERT(info.nodesOffset <= info.nodesSize);

  write_pair_halves(node->minMax, v_float_to_half_down(box.bmin), v_float_to_half_up(box.bmax));

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
  uint16_t dim_as_box_dist)
{
  TLASNode *leafNode = (TLASNode *)writeTLASNode(info, box);
  leafNode->skip = info.leavesOffset | (1 << 31);

  TLASLeaf *leaf = (TLASLeaf *)(info.bboxData + info.leavesOffset);
  info.leavesOffset += sizeof(TLASLeaf);
  G_ASSERT(info.leavesOffset <= info.leavesSize && info.leavesOffset % 4 == 0);
  const vec3f ext = v_max(v_bbox3_size(blas_box), v_splats(blas_size_eps));
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

  // TLASLeaf *leaf = (TLASLeaf *)writeTLASNode(info, box, data_offset);
  // data_offset += sizeof(TLASLeaf) - sizeof(TLASNode);
  leaf->maxScale = v_extract_xi(v_float_to_half_up(maxScale));
  leaf->distToTreatAsBox = dim_as_box_dist;
  memcpy(leaf->origin, &origin, sizeof(float) * 3);
  memcpy(leaf->scale, &scale, sizeof(float) * 3);
  write_pair_halves(leaf->invTm, v_float_to_half_rtne(invTm.col0), v_float_to_half_rtne(invTm.col1));

  G_STATIC_ASSERT(sizeof(TLASLeaf) % 4 == 0);
  G_STATIC_ASSERT(sizeof(TLASNode) == TLAS_NODE_ELEM_SIZE);
  G_STATIC_ASSERT(sizeof(TLASLeaf) == TLAS_LEAF_ELEM_SIZE);
  // const float maxScale = (lengthSq(invTm.getcol(0)) + lengthSq(invTm.getcol(1)) + lengthSq(invTm.getcol(2)));;

  leaf->blasStart = bvh_start;
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
    int bvhStart = info.bvhStartOffsets[modelId];
    G_ASSERT(info.leavesOffset % 4 == 0);
    if (info.tlasLeavesOffsets)
      (*info.tlasLeavesOffsets)[instanceId] = info.leavesOffset;
    writeTLASLeaf(info, info.nodes[node], info.tms[instanceId], info.blasBoxes[modelId], bvhStart, info.boxDist[modelId]);
    return 1;
  }
}

inline void writeTLASTreeRoot(WriteTLASTreeInfo &info)
{
  writeTLASTree(info, 0, 1);
  writeStopTLASNode(info);
}

static bbox3f calcBox(const vec4f *vertices, int vertex_count)
{
  bbox3f box;
  v_bbox3_init(box, vertices[0]);

  for (int i = 1; i < vertex_count; ++i)
    v_bbox3_add_pt(box, vertices[i]);
  return box;
}
static bool checkIfIsBox(const uint16_t *indices, int index_count, const vec4f *vertices, int vertex_count, const bbox3f &box)
{
  if (index_count != 12 * 3 || vertex_count != 8)
    return false;

  for (int i = 0; i < vertex_count; ++i)
  {
    int minMask = v_signmask(v_cmp_lt(v_abs(v_sub(vertices[i], box.bmin)), v_splats(1e-6f))) & 7;
    int maxMask = v_signmask(v_cmp_lt(v_abs(v_sub(vertices[i], box.bmax)), v_splats(1e-6f))) & 7;
    if ((minMask | maxMask) != 7)
      return false;
  }
  for (int i = 0; i < index_count; i += 3)
  {
    vec4f v0 = vertices[indices[i]];
    const vec4f n = v_cross3(v_sub(vertices[indices[i + 1]], v0), v_sub(vertices[indices[i + 2]], v0));
    const vec4f threshold = v_mul(v_length3(n), v_splats(1e-6f));
    const vec4f nA = v_abs(n);
    if (__popcount(v_signmask(v_cmp_gt(nA, threshold))) != 1)
      return false;
    // todo: there should be exactly 2 faces facing each axis (+-x, +-y, +-z)
  }
  return true;
}

}; // namespace build_bvh

using namespace build_bvh;

void RenderSWRT::clearTLASSourceData()
{
  matrices.clear();
  matrices.shrink_to_fit();
  instances.clear();
  instances.shrink_to_fit();
  tlasLeavesOffsets.clear();
  tlasLeavesOffsets.shrink_to_fit();
}

void RenderSWRT::clearBLASBuiltData()
{
  blasStartOffsets.clear();
  blasStartOffsets.shrink_to_fit();
  blasBoxes.clear();
  blasBoxes.shrink_to_fit();
  blasDimAsBox.clear();
  blasDimAsBox.shrink_to_fit();
}

void RenderSWRT::clearBLASSourceData()
{
  models.clear();
  models.shrink_to_fit();
}
void RenderSWRT::clearTLASBuffers()
{
  topBuf.close();
  topLeavesBuf.close();
}
void RenderSWRT::clearBLASBuffers() { bottomBuf.close(); }

void RenderSWRT::close()
{
  if (inited)
    release_blue_noise();
  mat.reset();
  purgeMeshData();
  clearTLASSourceData();
  clearBLASBuiltData();
  clearBLASSourceData();
  clearTLASBuffers();
  clearBLASBuffers();

  del_it(checkerboard_shadows_swrt_cs);
  del_it(shadows_swrt_cs);
  del_it(mask_shadow_swrt_cs);

  inited = false;
}

RenderSWRT::~RenderSWRT() { close(); }

void RenderSWRT::init()
{
  close();
  init_and_get_blue_noise();
  inited = true;
#define CS(a) a = new_compute_shader(#a)
  CS(checkerboard_shadows_swrt_cs);
  CS(shadows_swrt_cs);
  CS(mask_shadow_swrt_cs);
#undef CS

  rt.init("draw_collision_swrt");
}

int RenderSWRT::addBoxModel(vec4f bmin, vec4f bmax)
{
  Info info = {};
  int modelId = models.size();
  G_ASSERT(modelId == blasBoxes.size());
  models.push_back(info);
  blasBoxes.push_back(bbox3f{bmin, bmax});
  blasDimAsBox.push_back(0xffff);
  return modelId;
}

int RenderSWRT::addModel(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count,
  float dim_as_box_dist)
{
  Info info = {};
  bbox3f box = calcBox((const vec4f *)vertices, vertex_count);
  G_ASSERT((index_count % 3) == 0);
  if (!checkIfIsBox(indices, index_count, (const vec4f *)vertices, vertex_count, box))
  {
    info.faces = index_count / 3;
    info.ibO = ibd.size();
    ibd.resize(info.ibO + index_count);

    for (int i = 0; i < index_count; i++)
      ibd[info.ibO + i] = indices[i];

    info.verts = vertex_count;
    info.vbO = vbd.size();
    vbd.insert(vbd.end(), (const vec4f *)vertices, (const vec4f *)vertices + vertex_count);
  }

  int modelId = models.size();
  G_ASSERT(modelId == blasBoxes.size());
  G_ASSERT(modelId == blasDimAsBox.size());
  models.push_back(info);
  blasDimAsBox.push_back(dim_as_box_dist > 0 ? clamp<int>(ceilf(dim_as_box_dist + 0.001), 0, 65535) : 65535);
  blasBoxes.push_back(box);
  return modelId;
}

void RenderSWRT::addToLastModel(const uint16_t *indices, int index_count, const Point3_vec4 *vertices, int vertex_count)
{
  G_ASSERT_RETURN(models.size() > 0, );

  Info &info = models[models.size() - 1];

  G_ASSERT((index_count % 3) == 0);
  info.faces += index_count / 3;
  int startInd = ibd.size();
  ibd.resize(info.ibO + 3 * info.faces);

  int addInd = vbd.size() - info.vbO;

  for (int i = 0; i < index_count; i++)
    ibd[startInd + i] = indices[i] + addInd;

  info.verts += vertex_count;
  vbd.insert(vbd.end(), (const vec4f *)vertices, (const vec4f *)vertices + vertex_count);
}


void RenderSWRT::build_tlas(dag::Vector<bbox3f> &tlas_boxes, const dag::Vector<bbox3f> &blas_boxes)
{
  int64_t reft = profile_ref_ticks();
  /*
  for leaf:
    min.w = instance id
    max.w = model id
  for node:
    min.w = -1 - left node id
    max.w = -1 - right node id
  */

  int n = (int)matrices.size();
  dag::Vector<bbox3f> leaves;
  leaves.resize(n);

  for (int i = 0; i < n; i++)
  {
    mat44f tm;
    v_mat43_transpose_to_mat44(tm, matrices[i]);
    int modelId = instances[i];
    v_bbox3_init(leaves[i], tm, blas_boxes[modelId]);
    leaves[i].bmin = v_perm_xyzd(leaves[i].bmin, v_cast_vec4f(v_splatsi(i)));
    leaves[i].bmax = v_perm_xyzd(leaves[i].bmax, v_cast_vec4f(v_splatsi(modelId)));
  }

  tlas_boxes.reserve(leaves.size() * 2);

  CreateTLASHelper helper = {tlas_boxes};
  helper.maxChildrenCount = 4;
  int root = createBVHNodeSAH(leaves.begin(), leaves.end(), helper);
  debug("built BVH TLAS tree depth %d in %dus, %d instances, %d nodes", helper.maxDepth, profile_time_usec(reft), matrices.size(),
    tlas_boxes.size());
  G_ASSERTF(helper.maxDepth <= BVH_MAX_TLAS_DEPTH, "%d tlas depth too big, we should rebuild tree with better balancing");
  G_ASSERT(root == 0);
}

bool RenderSWRT::buildModel(int model, SplitHelper &h, dag::Vector<bbox3f> &tempBoxes, int &root)
{
  const Info &info = models[model];
  if (models[model].faces == models[model].FACES_IS_BOX)
  {
    root = -1;
    return false;
  }
  tempBoxes.resize(info.faces);
  build_bvh::addPropToPrimitivesAABBList(tempBoxes.begin(), ibd.begin() + info.ibO, vbd.begin() + info.vbO, info.faces);
  h.maxChildrenCount = 4;
  h.start();
  root = build_bvh::createBVHNodeSAH(tempBoxes.begin(), tempBoxes.end(), h);
  return true;
}

void RenderSWRT::destroy(BLASData *&d) { del_it(d); }
void RenderSWRT::destroy(TLASData *&d) { del_it(d); }

enum
{
  PAGE_SIZE = 65536,
  PAGE_MASK = PAGE_SIZE - 1
};

static void ensure_buf_size_and_update(UniqueBufHolder &buf, const uint8_t *data, uint32_t size, const char *name)
{
  const uint32_t cSize = buf ? buf.getBuf()->getSize() : 0;
  const uint32_t nextSize = ((size + PAGE_MASK) & ~PAGE_MASK);
  if (cSize < size || cSize * 2 > nextSize)
  {
    buf.close();
    buf = dag::create_sbuffer(sizeof(uint32_t), nextSize / sizeof(uint32_t),
      SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, //|SBCF_BIND_UNORDERED
      0, name);
  }
  if (size)
  {
    const int64_t reft = profile_ref_ticks();
    buf.getBuf()->updateData(0, size, data, 0);
    debug("update %s in %dus", name, profile_time_usec(reft));
  }
}

static void ensure_buf_size_and_update(UniqueBufHolder &buf, const Tab<uint8_t> &data, const char *name)
{
  ensure_buf_size_and_update(buf, data.begin(), data.size(), name);
}

void RenderSWRT::copyToGPUAndDestroy(BLASData *d)
{
  if (!d)
    return;
  ensure_buf_size_and_update(bottomBuf, d->data, "bvh_bottom_structures");
  del_it(d);
}

void RenderSWRT::copyToGPUAndDestroy(TLASData *d)
{
  if (!d)
    return;
  if (VariableMap::isVariablePresent(get_shader_variable_id("bvh_top_leaves", true)))
  {
    ensure_buf_size_and_update(topLeavesBuf, (const uint8_t *)tlasLeavesOffsets.data(),
      tlasLeavesOffsets.size() * sizeof(*tlasLeavesOffsets.begin()), "bvh_top_leaves");
  }

  ensure_buf_size_and_update(topBuf, d->data, "bvh_top_structures");
  del_it(d);
}


build_bvh::BLASData *RenderSWRT::buildBottomLevelStructures(uint32_t workers)
{
  if (models.size() == 0)
    return nullptr;

  dag::Vector<int> roots;
  int64_t reft = profile_ref_ticks();
  uint32_t maxModelFaces = 0;
  for (auto &info : models)
  {
    if (info.faces > 0)
      maxModelFaces = max<uint32_t>(maxModelFaces, info.faces);
  }

  const uint32_t totalFaces = (ibd.size() / 3);
  roots.resize(models.size(), -1);
  if (maxModelFaces == 0)
  {
    debug("SW BVH: no models with faces");
    return nullptr;
  }

  dag::Vector<bbox3f> nodes;
  nodes.reserve(totalFaces * 2); // at least amount of faces
  enum
  {
    MAX_WORKERS = 8,
    QUANT = 4
  };
  const int buildWorkers = min<int>(workers, min<int>(min<int>(threadpool::get_num_workers(), models.size() / QUANT), MAX_WORKERS));
  int maxDepth = 0;
  if (buildWorkers == 0)
  {
    SplitHelper h{nodes};
    dag::Vector<bbox3f> tempBoxes;
    for (int i = 0, ie = models.size(); i < ie; i++)
      buildModel(i, h, tempBoxes, roots[i]);
    maxDepth = h.maxDepth;
  }
  else
  {
    struct Context
    {
      OSSpinlock spinLock;
      dag::Vector<bbox3f> threadNodes[MAX_WORKERS + 1];
      dag::Vector<bbox3f> boxes[MAX_WORKERS + 1];
      dag::RelocatableFixedVector<SplitHelper, MAX_WORKERS + 1, false> threadHelpers;
      dag::Vector<bbox3f> &nodes;
      dag::Vector<int> &roots;
      int maxDepth = 0;
      Context(dag::Vector<bbox3f> &n, dag::Vector<int> &r, int workers) : nodes(n), roots(r)
      {
        for (int i = 0; i < workers + 1; ++i)
          threadHelpers.emplace_back(threadNodes[i]);
      }
      void addModels(uint32_t tbegin, uint32_t tend, uint32_t thread_id)
      {
        SplitHelper &h = threadHelpers[thread_id];
        OSSpinlockScopedLock writeLock(spinLock);
        uint32_t rootNodeStart = nodes.size();
        nodes.insert(nodes.end(), h.curNodes.begin(), h.curNodes.end());
        for (int i = tbegin; i < tend; i++)
          if (roots[i] >= 0)
            roots[i] += rootNodeStart;
        maxDepth = max(maxDepth, h.maxDepth);
      }
    } context(nodes, roots, buildWorkers);
    threadpool::parallel_for(
      0, models.size(), QUANT,
      [&](uint32_t tbegin, uint32_t tend, uint32_t thread_id) {
        SplitHelper &h = context.threadHelpers[thread_id];
        h.curNodes.clear();
        bool addModels = false;
        for (int i = tbegin; i < tend; i++)
          addModels |= buildModel(i, h, context.boxes[thread_id], context.roots[i]);
        if (addModels)
          context.addModels(tbegin, tend, thread_id);
      },
      buildWorkers);
    maxDepth = context.maxDepth;
  }

  // stop nodes are written
  const uint32_t blasSize = (nodes.size() - totalFaces) * sizeof(build_bvh::BVHNodeBBox) + totalFaces * sizeof(build_bvh::BVHLeafBBox);

  debug("built BVH BLAS in %dus, %d models %d nodes %d faces, %d bytes, maxDepth %d", profile_time_usec(reft), models.size(),
    nodes.size(), totalFaces, blasSize, maxDepth);
  G_ASSERT(blasSize % BVH_BLAS_ELEM_SIZE == 0 && blasSize % 4 == 0);
  reft = profile_ref_ticks();

  blasStartOffsets.resize(models.size());
  BLASData *ret = new BLASData{blasSize};
  uint8_t *bvhData = ret->data.begin();
  {
    build_bvh::WriteBVHTreeInfo info{bvhData, nodes.begin()};

    int dataOffset = 0;

    for (int i = 0, ie = roots.size(); i < ie; i++)
    {
      if (models[i].faces == models[i].FACES_IS_BOX)
      {
        blasStartOffsets[i] = BLAS_IS_BOX;
        continue;
      }
      G_ASSERT(dataOffset % BVH_BLAS_ELEM_SIZE == 0);
      blasStartOffsets[i] = dataOffset / BVH_BLAS_ELEM_SIZE;
      vec3f center = v_bbox3_center(blasBoxes[i]);
      vec4f maxExt = v_sub(blasBoxes[i].bmax, blasBoxes[i].bmin);
      // vec3f maxExt = v_max(v_max(v_splat_x(boxExt), v_splat_y(boxExt)), v_splat_z(boxExt));
      info.scale = v_rcp(v_max(maxExt, v_splats(build_bvh::blas_size_eps)));
      info.ofs = v_mul(v_neg(center), info.scale);
      info.verts = vbd.begin() + models[i].vbO;
      info.indices = ibd.begin() + models[i].ibO;
      build_bvh::writeBVHTree(info, roots[i], roots[i], dataOffset);
      build_bvh::writeStopBVHNode(info, dataOffset);
      G_ASSERTF(dataOffset <= blasSize, "dataOffset=%d blasSize=%d, model %d", dataOffset, blasSize, i);
    }

    G_ASSERTF(dataOffset == blasSize, "dataOffset=%d blasSize=%d", dataOffset, blasSize);
    // dataOffset < blasSize can happen now, as we save less data, due to collapse of leaves

    // memset(&bvhData[dataOffset], 0, sizeof(build_bvh::BVHNodeBBox));
  }

  debug("write BVH BLAS in %dus", profile_time_usec(reft));
  return ret;
}

build_bvh::TLASData *RenderSWRT::buildTopLevelStructures()
{
  if (blasStartOffsets.empty() || matrices.empty())
    return nullptr;

  // N leaves + (N - 1) intermediate nodes + stop node
  dag::Vector<bbox3f> tlasBoxes;
  build_tlas(tlasBoxes, blasBoxes);

  G_ASSERTF(tlasBoxes.size() >= matrices.size(), "%d > %d", tlasBoxes.size(), matrices.size());
  const uint32_t tlasSize = (tlasBoxes.size() + 1) * sizeof(build_bvh::TLASNode) + matrices.size() * sizeof(build_bvh::TLASLeaf);

  int64_t reft = profile_ref_ticks();

  TLASData *ret = new TLASData{tlasSize};
  uint8_t *bvhData = ret->data.begin();
  {
    build_bvh::WriteTLASTreeInfo info = {};
    tlasLeavesOffsets.clear();
    if (topLeavesBuf)
    {
      tlasLeavesOffsets.resize(matrices.size());
      info.tlasLeavesOffsets = &tlasLeavesOffsets;
    }
    else
      info.tlasLeavesOffsets = nullptr;
    info.nodes = tlasBoxes.begin();
    info.bboxData = bvhData;
    info.tms = matrices.begin();
    info.bvhStartOffsets = blasStartOffsets.begin();
    info.blasBoxes = blasBoxes.begin();
    info.boxDist = blasDimAsBox.begin();

    info.nodesOffset = 0;
    info.nodesSize = (tlasBoxes.size() + 1) * sizeof(build_bvh::TLASNode);
    info.leavesOffset = info.nodesSize;
    info.leavesSize = tlasSize;

    build_bvh::writeTLASTreeRoot(info);
    G_ASSERTF(info.nodesOffset == info.nodesSize, "dataOffset=%d tlasSize=%d", info.nodesOffset, info.nodesSize);
    G_ASSERTF(info.leavesOffset == info.leavesSize, "dataOffset=%d tlasSize=%d", info.leavesOffset, info.leavesSize);
  }

  debug("write BVH TLAS in %dus of %d bytes", profile_time_usec(reft), tlasSize);
  return ret;

  // G_ASSERT(build_bvh::max_tlas_depth <= BVH_MAX_TLAS_DEPTH);
}

void RenderSWRT::reserveAddInstances(int cnt)
{
  matrices.reserve(matrices.size() + cnt);
  instances.reserve(instances.size() + cnt);
}

void RenderSWRT::drawRT()
{
  if (!topBuf)
    return;

  if (!mat)
    mat.reset(new_shader_material_by_name_optional("draw_collision_swrt"));
  if (mat)
  {
    mat->addRef();
    elem = mat->make_elem();
  }
  else
    return;
  TIME_D3D_PROFILE(bvh_rt);
  rt.render();
}

uint32_t RenderSWRT::getTileBufferSizeDwords(uint32_t w, uint32_t h)
{
  return (uint32_t(w + SWRT_TILE_SIZE - 1) / SWRT_TILE_SIZE * uint32_t(h + SWRT_TILE_SIZE - 1) / SWRT_TILE_SIZE);
}

void RenderSWRT::renderShadowFrustumTiles(int w, int h, const Point3 &to_sun_direction, float sun_angle_size)
{
  if (!mask_shadow_swrt_cs)
    return;
  DA_PROFILE_GPU;
  // validate for vertical sun
  float2 sinCosA = float2(sinf(sun_angle_size), cosf(sun_angle_size));
  const float lenXZ = sqrtf(to_sun_direction.x * to_sun_direction.x + to_sun_direction.z * to_sun_direction.z);
  float3 lN, rN, tN, bN, nN;
  if (lenXZ > 1e-2f)
  {
    float2 to_sun_light_dir_xz(to_sun_direction.x / lenXZ, to_sun_direction.z / lenXZ);

    lN = -float3(to_sun_light_dir_xz.y * sinCosA.y - to_sun_light_dir_xz.x * sinCosA.x, 0,
      -to_sun_light_dir_xz.y * sinCosA.x - to_sun_light_dir_xz.x * sinCosA.y);
    rN = -float3(-to_sun_light_dir_xz.y * sinCosA.y - to_sun_light_dir_xz.x * sinCosA.x, 0,
      -to_sun_light_dir_xz.y * sinCosA.x + to_sun_light_dir_xz.x * sinCosA.y);
    nN = float3(to_sun_light_dir_xz.x, 0, to_sun_light_dir_xz.y);
    float3 directLeftN = float3(to_sun_light_dir_xz.y, 0, -to_sun_light_dir_xz.x); // no rotation with sun size

    // validate for horizontal sun
    float3 ortoSun = fabsf(to_sun_direction.y) < 1e-6f ? float3(0, -1, 0) : normalize(cross(directLeftN, to_sun_direction));
    bN = (makeTM(directLeftN, sun_angle_size) % -ortoSun);
    tN = (makeTM(directLeftN, -sun_angle_size) % ortoSun);
  }
  else
  {
    // code is checking only 4 first planes though
    lN = float3(sinCosA.y, sinCosA.x, 0);
    rN = float3(-sinCosA.y, sinCosA.x, 0);
    bN = float3(0, sinCosA.x, sinCosA.y);
    nN = float3(0, sinCosA.x, -sinCosA.y);
    tN = to_sun_direction;
  }
  float4 planes[4] = {
    float4(lN.x, rN.x, bN.x, nN.x), float4(lN.y, rN.y, bN.y, nN.y), float4(lN.z, rN.z, bN.z, nN.z), float4(tN.x, tN.y, tN.z, 0)};
  ShaderGlobal::set_color4_array(swrt_shadow_planesVarId, planes, countof(planes));

  mask_shadow_swrt_cs->dispatchThreads((w + SWRT_TILE_SIZE - 1) / SWRT_TILE_SIZE, (h + SWRT_TILE_SIZE - 1) / SWRT_TILE_SIZE, 1);
}

void RenderSWRT::renderShadows(const Point3 &to_sun_direction, float sun_size, uint32_t checkerboad_frame,
  const ManagedBufView &shadow_mask_buf, const ManagedTexView &shadow_target)
{
  if (!shadows_swrt_cs || !shadow_target)
    return;

  TMatrix orthoSun;
  view_matrix_from_look(to_sun_direction, orthoSun);
  float4 basis[3] = {float4(orthoSun.getcol(0), tanf(sun_size)), float4(orthoSun.getcol(1), 0), float4(orthoSun.getcol(2), sun_size)};
  ShaderGlobal::set_color4_array(swrt_dir_to_sun_basisVarId, basis, countof(basis));

  TextureInfo ti;
  shadow_target.getTex2D()->getinfo(ti, 0);
  const int w = ti.w, h = ti.h;
  const int srcW = checkerboad_frame != NO_CHECKER_BOARD ? w * 2 : w; // fixme!! source is not w*2
  ShaderGlobal::set_int4(swrt_shadow_target_sizeVarId, w, h, srcW - 1, h - 1);
  ShaderGlobal::set_buffer(swrt_shadow_maskVarId, shadow_mask_buf.getBufId());
  ShaderGlobal::set_texture(swrt_shadow_targetVarId, shadow_target.getTexId());
  ShaderGlobal::set_int(swrt_checkerboard_frameVarId, checkerboad_frame);

  DA_PROFILE_GPU;
  renderShadowFrustumTiles(srcW, h, to_sun_direction, sun_size);
  d3d::resource_barrier({shadow_mask_buf.getBuf(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
  if (checkerboad_frame != NO_CHECKER_BOARD)
    checkerboard_shadows_swrt_cs->dispatchThreads(w, h, 1);
  else
    shadows_swrt_cs->dispatchThreads(w, h, 1);

  d3d::resource_barrier({shadow_target.getTex2D(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
}
