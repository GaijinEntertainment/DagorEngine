// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dag/dag_vector.h>
#include <generic/dag_tab.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_stlqsort.h>
#include <generic/dag_carray.h>
#include <memory/dag_framemem.h>
#include <daBVH/dag_bvhBuild.h>

// based on https://github.com/kayru/RayTracedShadows/blob/master/Source/BVHBuilder.cpp
// Presorted SAH (Wald 2007): boxes[] is sorted in-place along axis 0 once; idx1/idx2 are
// framemem-backed index arrays giving axis-1/2 orderings. Every node sweeps all three
// axes in O(count) from these orderings and stable-partitions them -- no per-node resort.

namespace build_bvh
{

static void sortAlongAxis(bbox3f *boxes, bbox3f *end, int axis)
{
  const int axisMask = 1 << axis;
  stlsort::sort(boxes, end, [axisMask](const bbox3f &a, const bbox3f &b) {
    int mask = v_signmask(v_cmp_gt(v_add(b.bmin, b.bmax), v_add(a.bmin, a.bmax)));
    return mask & axisMask;
  });
}

static void sortIndicesByAxis(uint32_t *idx, uint32_t count, const bbox3f *boxes, int axis)
{
  const int axisMask = 1 << axis;
  stlsort::sort(idx, idx + count, [=](uint32_t a, uint32_t b) {
    return v_signmask(v_cmp_gt(v_add(boxes[b].bmin, boxes[b].bmax), v_add(boxes[a].bmin, boxes[a].bmax))) & axisMask;
  });
}

struct ChildInfo
{
  uint32_t s = 0, c = 0;
  float area = 0, cost = 0;
};

struct Area
{
  float left, right;
};

struct SplitHelper
{
  Tab<bbox3f> &curNodes;
  bbox3f *boxes = nullptr; // axis-0-sorted in-place at root; valid for the whole build
  uint32_t *idx1 = nullptr, *idx2 = nullptr;

  uint32_t getCurrentNodes() const { return curNodes.size(); }
  void reWriteNode(uint32_t at, const bbox3f &b) { curNodes[at] = b; }
  void writeNode(const bbox3f &b) { curNodes.push_back(b); }

  dag::Vector<Area, framemem_allocator> area;
  // Partition scratch -- reserved to root size so resize() inside the recursion is a no-op.
  dag::Vector<uint8_t, framemem_allocator> mark;
  dag::Vector<uint32_t, framemem_allocator> remap, tempIdx;
  dag::Vector<bbox3f, framemem_allocator> tempBoxes;

  int maxChildrenCount = 2;
  int maxDepth = 0;
  bool allowAnyChildrenCount = true;
  SplitHelper(Tab<bbox3f> &n) : curNodes(n) {}
};

static inline int findBestSplit(bbox3f *bboxData, bbox3f *end, SplitHelper &h, int &out_axis, Point2 &surface_area, Point2 &costSplit)
{
  const int count = end - bboxData;
  const uint32_t s = uint32_t(bboxData - h.boxes);

  int axis = 0;
  int bestSplit = 0;
  float splitCost = FLT_MAX;

  int split = 0;
  h.area.resize(count);

  for (int i = 0; i < 3; i++)
  {
    // Replaces sortAlongAxis(bboxData, end, i) of the old builder: pick the presorted ordering.
    const uint32_t *idx = (i == 0) ? nullptr : (i == 1) ? h.idx1 : h.idx2;
    auto boxAt = [&](int k) -> const bbox3f & { return idx ? h.boxes[idx[s + k]] : bboxData[k]; };

    bbox3f boundsLeft, boundsRight;
    v_bbox3_init_empty(boundsLeft);
    v_bbox3_init_empty(boundsRight);

    for (int indexLeft = 0; indexLeft < count; ++indexLeft)
    {
      int indexRight = count - indexLeft - 1;

      v_bbox3_add_box(boundsLeft, boxAt(indexLeft));
      v_bbox3_add_box(boundsRight, boxAt(indexRight));

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

      float childrenCostLeft = mid;
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

  out_axis = axis;
  return split;
}

// Stable-partition boxes[s..e) and idx1/idx2[s..e) into left-then-right halves given the
// chosen (axis, K). Axis 0: boxes are already in axis-0 order so the split is positional
// and only idx arrays need filtering. Axis 1/2: walk boxes[] in axis-0 order (preserving
// the invariant in each half) through tempBoxes, build remap[], rewrite idx1/idx2.
static void partitionNode(SplitHelper &h, uint32_t s, uint32_t e, int axis, uint32_t K)
{
  const uint32_t count = e - s;
  h.mark.resize(count);
  memset(h.mark.data(), 0, count);

  if (axis == 0)
  {
    for (uint32_t k = 0; k < K; ++k)
      h.mark[k] = 1;
  }
  else
  {
    const uint32_t *chosen = (axis == 1) ? h.idx1 : h.idx2;
    for (uint32_t k = 0; k < K; ++k)
      h.mark[chosen[s + k] - s] = 1;

    h.tempBoxes.resize(count);
    h.remap.resize(count);
    uint32_t wL = 0, wR = K;
    for (uint32_t k = 0; k < count; ++k)
    {
      if (h.mark[k])
      {
        h.tempBoxes[wL] = h.boxes[s + k];
        h.remap[k] = wL++;
      }
      else
      {
        h.tempBoxes[wR] = h.boxes[s + k];
        h.remap[k] = wR++;
      }
    }
    memcpy(h.boxes + s, h.tempBoxes.data(), count * sizeof(bbox3f));
  }

  // Stable-filter both idx arrays. For axis 0 the box positions are unchanged so old->new
  // is identity; for axis 1/2 we go through remap[] built above.
  h.tempIdx.resize(count);
  auto filterIdx = [&](uint32_t *arr) {
    uint32_t wL = 0, wR = K;
    for (uint32_t k = 0; k < count; ++k)
    {
      const uint32_t oldLocal = arr[k] - s;
      const uint32_t newLocal = (axis == 0) ? oldLocal : h.remap[oldLocal];
      if (h.mark[oldLocal])
        h.tempIdx[wL++] = s + newLocal;
      else
        h.tempIdx[wR++] = s + newLocal;
    }
    memcpy(arr, h.tempIdx.data(), count * sizeof(uint32_t));
  };
  filterIdx(h.idx1 + s);
  filterIdx(h.idx2 + s);
}

static int createBVHNodeSAH(bbox3f *bboxData, bbox3f *end, SplitHelper &h, int depth = 1, float currentCost = 1e29f)
{
  const uint32_t count = end - bboxData;
  const uint32_t s = uint32_t(bboxData - h.boxes);
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
    for (;;)
    {
      const ChildInfo i = children[childrenCount - 1];
      if (i.c == 1 || i.cost <= 0)
        break;
      --childrenCount;
      Point2 area(0, 0), cost(0, 0);
      int bestAxis = 0;
      const uint32_t split = findBestSplit(bboxData + i.s, bboxData + i.s + i.c, h, bestAxis, area, cost);
      if (h.allowAnyChildrenCount && cost.x + cost.y > currentCost && (split > 1 || i.c - split > 1))
      {
        children[childrenCount] = i;
        children[childrenCount].cost = -i.cost;
        childrenCount++;
      }
      else
      {
        partitionNode(h, s + i.s, s + i.s + i.c, bestAxis, split);
        children[childrenCount++] = ChildInfo{i.s, split, area.x, cost.x};
        children[childrenCount++] = ChildInfo{i.s + split, i.c - split, area.y, cost.y};
        if (childrenCount >= curChildrenCount)
          break;
      }
      stlsort::sort(children.begin(), children.begin() + childrenCount, [](const auto &a, const auto &b) {
        bool aLeaf = a.c == 1;
        bool bLeaf = b.c == 1;
        if (aLeaf != bLeaf)
          return bLeaf; // single-element nodes (c==1) should sort last (they're leaves, can't split further)
        return a.cost < b.cost;
      });
    }
    stlsort::sort(children.begin(), children.begin() + childrenCount, [](const auto &a, const auto &b) { return a.area > b.area; });

    const int startAt = h.getCurrentNodes();
    int totalChildrenCount = childrenCount;
    if (h.allowAnyChildrenCount)
    {
      for (int i = 0; i < childrenCount; ++i)
      {
        if (children[i].cost <= 0)
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
      if (!h.allowAnyChildrenCount || children[i].cost > 0)
        createBVHNodeSAH(bboxData + children[i].s, bboxData + children[i].s + children[i].c, h, depth + 1, children[i].cost);
    }
    const int endAt = h.getCurrentNodes();

    bounds.bmin = v_perm_xyzd(bounds.bmin, v_cast_vec4f(v_splatsi(-(endAt - startAt))));
    bounds.bmax = v_perm_xyzd(bounds.bmax, v_cast_vec4f(v_splatsi(totalChildrenCount)));
    h.reWriteNode(nodeIndex, bounds);
  }

  return nodeIndex;
}

template <class Indices>
static void addPropToPrimitivesAABBListTempl(bbox3f *boxes, const Indices *indices, const vec4f *verts, int faces)
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

void addPropToPrimitivesAABBList(bbox3f *boxes, const uint16_t *indices, const vec4f *verts, int faces)
{
  addPropToPrimitivesAABBListTempl(boxes, indices, verts, faces);
}

void addPropToPrimitivesAABBList(bbox3f *boxes, const uint32_t *indices, const vec4f *verts, int faces)
{
  addPropToPrimitivesAABBListTempl(boxes, indices, verts, faces);
}

bbox3f calcBox(const vec4f *vertices, int vertex_count)
{
  bbox3f box;
  v_bbox3_init(box, vertices[0]);

  for (int i = 1; i < vertex_count; ++i)
    v_bbox3_add_pt(box, vertices[i]);
  return box;
}

int create_bvh_node_sah(Tab<bbox3f> &nodes, bbox3f *boxes, const uint32_t boxes_cnt, int max_children_count, int &max_depth)
{
  if (boxes_cnt == 0)
    return (int)nodes.size();

  SplitHelper h{nodes};
  h.maxChildrenCount = max_children_count;
  h.boxes = boxes;

  // Presort: axis 0 in-place (boxes become the implicit axis-0 ordering), idx1/idx2 are
  // index arrays revealing axis-1/2 orderings. Maintained through partitionNode at every split.
  sortAlongAxis(boxes, boxes + boxes_cnt, 0);
  dag::Vector<uint32_t, framemem_allocator> idx1(boxes_cnt), idx2(boxes_cnt);
  for (uint32_t i = 0; i < boxes_cnt; ++i)
    idx1[i] = idx2[i] = i;
  sortIndicesByAxis(idx1.data(), boxes_cnt, boxes, 1);
  sortIndicesByAxis(idx2.data(), boxes_cnt, boxes, 2);
  h.idx1 = idx1.data();
  h.idx2 = idx2.data();

  nodes.reserve(nodes.size() + boxes_cnt * 3 / 2);
  h.area.reserve(boxes_cnt);
  h.mark.reserve(boxes_cnt);
  h.remap.reserve(boxes_cnt);
  h.tempBoxes.reserve(boxes_cnt);
  h.tempIdx.reserve(boxes_cnt);

  int ret = createBVHNodeSAH(boxes, boxes + boxes_cnt, h);
  max_depth = max(max_depth, h.maxDepth);
  return ret;
}

}; // namespace build_bvh
