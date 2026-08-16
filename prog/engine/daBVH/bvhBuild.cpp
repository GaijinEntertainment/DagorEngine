// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dag/dag_vector.h>
#include <generic/dag_tab.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_stlqsort.h>
#include <util/dag_radix.h>
#include <generic/dag_carray.h>
#include <memory/dag_framemem.h>
#include <daBVH/dag_bvhBuild.h>
#include <daBVH/swBLASLeafDefs.hlsli> // QUAD_O_MIN / QUAD_O_MAX (default leaf-offset window)

// based on https://github.com/kayru/RayTracedShadows/blob/master/Source/BVHBuilder.cpp
// Presorted SAH (Wald 2007): boxes[] is sorted in-place along axis 0 once; idx1/idx2 are
// framemem-backed index arrays giving axis-1/2 orderings. Every node sweeps all three
// axes in O(count) from these orderings and stable-partitions them -- no per-node resort.

namespace build_bvh
{

// Presort key: per-box center*2 along one axis, paired with the box position. Sorting these
// 8-byte pairs makes exactly the same comparison decisions as sorting the boxes (or index
// arrays) with a per-comparison v_add gather -- introsort's resulting permutation depends only
// on the comparison outcomes -- but touches 4x less memory, sequentially.
struct AxisSortKey
{
  float key;
  uint32_t pos;
};

// ascending, the same predicate the replaced box comparator computed via v_cmp_gt(b, a)
static void sortAxisKeys(AxisSortKey *k, uint32_t count)
{
  stlsort::sort(k, k + count, [](const AxisSortKey &a, const AxisSortKey &b) { return a.key < b.key; });
}


// Stable LSD radix sort (util/dag_radix.h) of the (key,pos) pairs by the float key. Gives
// equal keys a DEFINED order (their input order) where introsort's tie placement is
// algorithm-defined, so the resulting permutation -- and the tree built from it -- differs
// on tied keys; quality is equivalent (see dag_bvhBuild.h). Pure integer work:
// deterministic on any thread in any FP mode.
struct AxisSortKeyRadixPred
{
  unsigned operator()(const AxisSortKey &v) const { return radix_float_predicate()(v.key); }
};

static void radixSortAxisKeys(AxisSortKey *k, uint32_t count, AxisSortKey *tmp)
{
  radix_sort_4pass(k, tmp, count, AxisSortKeyRadixPred()); // 4 passes: result lands back in k
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


enum
{
  // Below this count findBestSplit/partitionNode keep all scratch in stack arrays: at mean
  // node count ~10 the shared-vector resizes and small memcpy calls dominate the real work.
  SPLIT_SMALL_COUNT = 8
};

struct SplitHelper
{
  Tab<bbox3f> &curNodes;
  bbox3f *boxes = nullptr; // axis-0-sorted in-place at root; valid for the whole build
  uint32_t *idx1 = nullptr, *idx2 = nullptr;

  uint32_t getCurrentNodes() const { return curNodes.size(); }
  void reWriteNode(uint32_t at, const bbox3f &b) { curNodes[at] = b; }
  void writeNode(const bbox3f &b) { curNodes.push_back(b); }

  // Both-direction area table: the interleaved dual-direction sweep keeps two gather
  // streams in flight (splitting it into two single-direction passes measures slower).
  dag::Vector<Area, framemem_allocator> area;
  // Partition scratch -- reserved to root size so resize() inside the recursion is a no-op.
  dag::Vector<uint8_t, framemem_allocator> mark;
  dag::Vector<uint32_t, framemem_allocator> remap, tempIdx, tempIdx2;
  dag::Vector<bbox3f, framemem_allocator> tempBoxes;

  int maxChildrenCount = 2;
  int maxDepth = 0;
  SplitAxes splitAxes = SplitAxes::XYZ;
  // Hard cap on a node's TOTAL fanout, flattened clusters included. Unsplittable clusters (SAH cost
  // prefers keeping them whole) may still be flattened into direct leaf children, but never past this
  // bound -- fixed-fanout consumers (SoA4 nodes, ordered traversal chunks) rely on it. 252 is the
  // bvhIO stream format limit (higher child-count byte values are leaf markers).
  int allowMaxChildrenCount = 252;
  SplitHelper(Tab<bbox3f> &n) : curNodes(n) {}
};

// out_total_bounds, when non-null, receives the union of the whole range: the axis-0 sweep
// computes it anyway (its forward accumulator after the last element), so the full-range
// caller saves a separate calculateBounds pass. Bit-exact: the same sequential adds.
static inline int findBestSplit(bbox3f *bboxData, bbox3f *end, SplitHelper &h, int &out_axis, Point2 &surface_area, Point2 &costSplit,
  bbox3f *out_total_bounds = nullptr)
{
  const int count = end - bboxData;
  const uint32_t s = uint32_t(bboxData - h.boxes);

  int axis = 0;
  int bestSplit = 0;
  float splitCost = FLT_MAX;

  int split = 0;
  // Most calls are tiny (mean node count ~10 at fanout 4, and the re-split loop issues up
  // to 3 calls per node): keep their area table on the stack, skipping the shared-vector
  // machinery. Same loops either way -- only the storage differs.
  Area areaSmall[SPLIT_SMALL_COUNT];
  Area *area = areaSmall;
  if (count > SPLIT_SMALL_COUNT)
  {
    h.area.resize_noinit(count); // both fields of every element are written by the sweep below
    area = h.area.data();
  }

  for (int i = 0; i < 3; i++)
  {
    if (i == 1 && h.splitAxes == SplitAxes::XZ)
      continue;
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

      area[indexLeft].left = calculateSurfaceArea(boundsLeft);
      area[indexRight].right = calculateSurfaceArea(boundsRight);
    }
    if (i == 0 && out_total_bounds)
      *out_total_bounds = boundsLeft;

    float bestCost = FLT_MAX;
    Point2 bestArea(0, 0);
    Point2 bestCostSplit(0, 0);
    for (int mid = 1; mid < count; ++mid)
    {
      float surfaceAreaLeft = area[mid - 1].left;
      float surfaceAreaRight = area[mid].right;

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

  // Small nodes (the common case, see SPLIT_SMALL_COUNT) keep all scratch on the stack and
  // copy back with plain loops: the shared-vector resizes and sub-32-byte memcpy calls cost
  // more than the partition itself down there. Same loops either way.
  const bool small = count <= SPLIT_SMALL_COUNT;
  alignas(16) bbox3f tempBoxesSmall[SPLIT_SMALL_COUNT];
  uint8_t markSmall[SPLIT_SMALL_COUNT];
  uint32_t remapSmall[SPLIT_SMALL_COUNT], tempIdxSmall[SPLIT_SMALL_COUNT], tempIdx2Small[SPLIT_SMALL_COUNT];

  // Axis 0 needs no mark at all: the split is positional (left half == first K boxes), so
  // the filter below tests oldLocal < K directly. Axis 1/2 keep the mark only for the box
  // scatter; the filter derives the side from remap (wL slots are < K by construction).
  const uint32_t *remap = nullptr;
  if (axis != 0)
  {
    const uint32_t *chosen = (axis == 1) ? h.idx1 : h.idx2;
    uint8_t *mark = markSmall;
    bbox3f *tempBoxes = tempBoxesSmall;
    uint32_t *remapW = remapSmall;
    if (!small)
    {
      h.mark.resize_noinit(count);      // fully overwritten: memset + K marks
      h.tempBoxes.resize_noinit(count); // every slot written by the scatter below
      h.remap.resize_noinit(count);
      mark = h.mark.data();
      tempBoxes = h.tempBoxes.data();
      remapW = h.remap.data();
    }
    memset(mark, 0, count);
    for (uint32_t k = 0; k < K; ++k)
      mark[chosen[s + k] - s] = 1;

    uint32_t wL = 0, wR = K;
    for (uint32_t k = 0; k < count; ++k)
    {
      if (mark[k])
      {
        tempBoxes[wL] = h.boxes[s + k];
        remapW[k] = wL++;
      }
      else
      {
        tempBoxes[wR] = h.boxes[s + k];
        remapW[k] = wR++;
      }
    }
    if (small)
      for (uint32_t k = 0; k < count; ++k)
        h.boxes[s + k] = tempBoxes[k];
    else
      memcpy(h.boxes + s, tempBoxes, count * sizeof(bbox3f));
    remap = remapW;
  }

  // Stable-filter both idx arrays in one pass. For axis 0 the box positions are unchanged
  // so old->new is identity and the side test is positional; for axis 1/2 both come from
  // remap (one load per element instead of a separate mark byte).
  uint32_t *tempIdx = tempIdxSmall, *tempIdx2 = tempIdx2Small;
  if (!small)
  {
    h.tempIdx.resize_noinit(count); // every slot written below (wL/wR cover [0, count))
    h.tempIdx2.resize_noinit(count);
    tempIdx = h.tempIdx.data();
    tempIdx2 = h.tempIdx2.data();
  }
  uint32_t *arr1 = h.idx1 ? h.idx1 + s : nullptr, *arr2 = h.idx2 + s;
  uint32_t wL1 = 0, wR1 = K, wL2 = 0, wR2 = K;
  for (uint32_t k = 0; k < count; ++k)
  {
    if (arr1) // absent for XZ-only builds: axis 1 is never swept or partitioned
    {
      const uint32_t oldLocal1 = arr1[k] - s;
      const uint32_t newLocal1 = remap ? remap[oldLocal1] : oldLocal1; // remap set iff axis != 0 (identity otherwise)
      if (newLocal1 < K)
        tempIdx[wL1++] = s + newLocal1;
      else
        tempIdx[wR1++] = s + newLocal1;
    }
    const uint32_t oldLocal2 = arr2[k] - s;
    const uint32_t newLocal2 = remap ? remap[oldLocal2] : oldLocal2;
    if (newLocal2 < K)
      tempIdx2[wL2++] = s + newLocal2;
    else
      tempIdx2[wR2++] = s + newLocal2;
  }
  if (small)
    for (uint32_t k = 0; k < count; ++k)
    {
      if (arr1)
        arr1[k] = tempIdx[k];
      arr2[k] = tempIdx2[k];
    }
  else
  {
    if (arr1)
      memcpy(arr1, tempIdx, count * sizeof(uint32_t));
    memcpy(arr2, tempIdx2, count * sizeof(uint32_t));
  }
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
    // depth != 1 guard: the root (depth 1) is never emitted as a leaf even for a single element, so
    // tree offset 0 is always an internal node. daSWRT relies on this -- its mask/shadow passes use
    // offset 0 as the "nothing in frustum" sentinel (see swRT.dshl `if (!startOfs)`).
    h.writeNode(*bboxData);
  }
  else
  {
    // The node box comes for free from the first findBestSplit call, whose axis-0 sweep
    // unions the whole range (bit-identical to a calculateBounds pass: same sequential
    // adds). Write a placeholder now to reserve the slot; reWriteNode below stores the
    // real box together with the counts. The empty init is overwritten on every path
    // (first-sweep union or the calculateBounds fallback) -- it only proves initialization
    // to flow analysis (MSVC C4701).
    bbox3f bounds;
    v_bbox3_init_empty(bounds);
    bool boundsKnown = false;
    h.writeNode(*bboxData);

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
      const bool fullRange = i.s == 0 && i.c == count && !boundsKnown;
      const uint32_t split =
        findBestSplit(bboxData + i.s, bboxData + i.s + i.c, h, bestAxis, area, cost, fullRange ? &bounds : nullptr);
      boundsKnown |= fullRange;
      if (cost.x + cost.y > currentCost && (split > 1 || i.c - split > 1))
      {
        // Unsplittable by SAH cost: mark flattened (negative cost; i.cost > 0 here, the pop loop
        // breaks on cost <= 0). The fanout cap below may still revert this to a forced split.
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
          return aLeaf; // leaves (c==1) sort first: the loop pops from the END, so a leaf there would
                        // break the fanout early. Splittable children must stay at the end to be popped.
        return a.cost < b.cost;
      });
    }
    stlsort::sort(children.begin(), children.begin() + childrenCount, [](const auto &a, const auto &b) { return a.area > b.area; });

    if (!boundsKnown) // the pop loop never ran findBestSplit over the full range (count == 1 root)
      calculateBounds(bboxData, end, bounds);

    const int startAt = h.getCurrentNodes();
    int totalChildrenCount = childrenCount;
    for (int i = 0; i < childrenCount; ++i)
      if (children[i].cost <= 0)
        totalChildrenCount += children[i].c - 1;
    // Enforce the fanout cap: un-flatten the widest flattened clusters until the total fits. An
    // un-flattened cluster recurses with an "infinite" cost (same value as the root call) so the next
    // level performs a real split before flattening can be reconsidered for the smaller halves.
    while (totalChildrenCount > h.allowMaxChildrenCount)
    {
      int widest = -1;
      for (int i = 0; i < childrenCount; ++i)
        if (children[i].cost <= 0 && (widest < 0 || children[i].c > children[widest].c))
          widest = i;
      if (widest < 0)
        break; // no flattened slots left: totalChildrenCount == childrenCount <= maxChildrenCount
      if (childrenCount == 1)
      {
        // The whole range is one oversize unsplittable cluster: un-flattening it whole would emit
        // a node with ONE recursive child -- a useless indirection (the child's box IS this node's)
        // that fixed-fanout consumers cannot even represent. Force the split at THIS level instead;
        // both halves recurse with the same infinite cost the single child would have carried.
        const ChildInfo w = children[0];
        Point2 area(0, 0), cost(0, 0);
        int bestAxis = 0;
        const uint32_t split = findBestSplit(bboxData + w.s, bboxData + w.s + w.c, h, bestAxis, area, cost, nullptr);
        partitionNode(h, s + w.s, s + w.s + w.c, bestAxis, split);
        children[0] = ChildInfo{w.s, split, area.x, 1e29f};
        children[1] = ChildInfo{w.s + split, w.c - split, area.y, 1e29f};
        childrenCount = 2;
        totalChildrenCount = 2;
        break;
      }
      totalChildrenCount -= children[widest].c - 1;
      children[widest].cost = 1e29f;
    }
    for (int i = 0; i < childrenCount; ++i)
    {
      if (children[i].cost <= 0)
      {
        int ci = children[i].s, ce = ci + children[i].c;
        stlsort::sort(bboxData + ci, bboxData + ce,
          [](const auto &a, const auto &b) { return calculateSurfaceArea(a) > calculateSurfaceArea(b); });
        for (; ci < ce; ++ci)
          h.writeNode(bboxData[ci]);
      }
    }
    for (int i = 0; i < childrenCount; ++i)
    {
      if (children[i].cost > 0)
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

// The default presort_use_radix_threshold is the measured introsort/radix crossover on real
// keys (swrtRiBench -sortBench): radix pays a fixed histogram + 5-pass overhead, so it is
// 29% slower at 16 elements, break-even at 32, 27% faster at 64 and 5x+ from 2048 up.
int create_bvh_node_sah(Tab<bbox3f> &nodes, bbox3f *boxes, const uint32_t boxes_cnt, int max_children_count, int &max_depth,
  SplitAxes split_axes, uint32_t presort_use_radix_threshold)
{
  if (boxes_cnt == 0)
    return (int)nodes.size();

  // Presort: axis 0 in-place (boxes become the implicit axis-0 ordering), idx1/idx2 are
  // index arrays revealing axis-1/2 orderings. Maintained through partitionNode at every split.
  // All presort scratch is fully overwritten before any read: skip the sized-constructor
  // zero-fill (~25 MB of dead memsets at 437K boxes) via resize_noinit.
  // XZ-only builds never sweep or partition along axis 1, so the whole Y presort (keys, sort,
  // idx1) is skipped and idx1 stays null; partitionNode skips its idx1 maintenance accordingly.
  const bool useAxis1 = split_axes != SplitAxes::XZ;
  dag::Vector<uint32_t, framemem_allocator> idx1, idx2;
  if (useAxis1)
    idx1.resize_noinit(boxes_cnt);
  idx2.resize_noinit(boxes_cnt);
  {
    dag::Vector<AxisSortKey, framemem_allocator> keys0, keys1, keys2;
    keys0.resize_noinit(boxes_cnt);
    if (useAxis1)
      keys1.resize_noinit(boxes_cnt);
    keys2.resize_noinit(boxes_cnt);
    auto sortKeys = [&](AxisSortKey *arr) {
      if (boxes_cnt >= presort_use_radix_threshold)
      {
        dag::Vector<AxisSortKey, framemem_allocator> tmp;
        tmp.resize_noinit(boxes_cnt); // radix scatters every slot in each pass
        radixSortAxisKeys(arr, boxes_cnt, tmp.data());
      }
      else
        sortAxisKeys(arr, boxes_cnt);
    };
    for (uint32_t i = 0; i < boxes_cnt; ++i)
      keys0[i] = AxisSortKey{v_extract_x(v_add(boxes[i].bmin, boxes[i].bmax)), i};
    sortKeys(keys0.data());
    // idx1/idx2 order positions of the axis-0-sorted boxes; identity-initialized pair arrays
    // reproduce the old sortIndicesByAxis(identity) decisions exactly. Their keys are gathered
    // through the axis-0 permutation (boxes[keys0[i].pos] IS sorted position i) before the
    // boxes move -- after that only bytes move and ready-made keys are compared.
    for (uint32_t i = 0; i < boxes_cnt; ++i)
    {
      vec4f c2 = v_add(boxes[keys0[i].pos].bmin, boxes[keys0[i].pos].bmax);
      if (useAxis1)
        keys1[i] = AxisSortKey{v_extract_y(c2), i};
      keys2[i] = AxisSortKey{v_extract_z(c2), i};
    }
    {
      dag::Vector<bbox3f, framemem_allocator> sorted;
      sorted.resize_noinit(boxes_cnt);
      for (uint32_t i = 0; i < boxes_cnt; ++i)
        sorted[i] = boxes[keys0[i].pos];
      memcpy(boxes, sorted.data(), boxes_cnt * sizeof(bbox3f));
    }
    if (useAxis1)
    {
      sortKeys(keys1.data());
      for (uint32_t i = 0; i < boxes_cnt; ++i)
        idx1[i] = keys1[i].pos;
    }
    sortKeys(keys2.data());
    for (uint32_t i = 0; i < boxes_cnt; ++i)
      idx2[i] = keys2[i].pos;
  }

  SplitHelper h{nodes};
  h.maxChildrenCount = max_children_count;
  h.allowMaxChildrenCount = max_children_count; // flattened clusters honor the same fanout cap
  h.splitAxes = split_axes;
  h.boxes = boxes;
  h.idx1 = useAxis1 ? idx1.data() : nullptr;
  h.idx2 = idx2.data();

  nodes.reserve(nodes.size() + boxes_cnt * 3 / 2);
  h.area.reserve(boxes_cnt);
  h.mark.reserve(boxes_cnt);
  h.remap.reserve(boxes_cnt);
  h.tempBoxes.reserve(boxes_cnt);
  h.tempIdx.reserve(boxes_cnt);
  h.tempIdx2.reserve(boxes_cnt);

  int ret = createBVHNodeSAH(boxes, boxes + boxes_cnt, h);
  max_depth = max(max_depth, h.maxDepth);
  return ret;
}

// A triangle is encodable iff some vertex (apex) reaches the other two within the signed offset range
// [minOff, maxOff]. The range is asymmetric, so a well-centered apex covers up to (maxOff - minOff) of
// index spread -- roughly twice a single positive window -- and far fewer triangles need duplicating.
static inline bool triEncodable(long a, long b, long c, int minOff, int maxOff)
{
  auto apexFits = [minOff, maxOff](long ap, long x, long y) {
    const long ox = x - ap, oy = y - ap;
    return ox >= minOff && ox <= maxOff && oy >= minOff && oy <= maxOff;
  };
  return apexFits(a, b, c) || apexFits(b, a, c) || apexFits(c, a, b);
}

// File-local second phase of leafOrderVertexFetch (its only caller) so the prep sequence cannot be
// skipped or misordered by external callers.
template <class IdxT>
static unsigned dedupWindowDup(IdxT *idx, unsigned idxCount, int minOff, int maxOff, dag::Vector<vec4f> &outVerts);

template <class IdxT>
unsigned leafOrderVertexFetch(IdxT *idx, unsigned idxCount, const vec4f *srcVerts, unsigned srcVertCount, dag::Vector<vec4f> &outVerts,
  int minOff, int maxOff)
{
  // LEAF_OFF_DEFAULT means "default leaf-offset window": resolve to the quad-BLAS range before it reaches
  // dedupWindowDup, which derives blockCap from maxOff and must never see the sentinel. minOff is a real
  // signed bound that may legitimately be negative, so test the exact sentinel rather than minOff < 0.
  if (minOff == LEAF_OFF_DEFAULT)
    minOff = QUAD_O_MIN;
  if (maxOff == LEAF_OFF_DEFAULT)
    maxOff = QUAD_O_MAX;
  outVerts.clear();
  const unsigned faceCount = idxCount / 3u;
  if (faceCount == 0)
    return 0;

  // Triangle AABBs with the source-tri index in bmin.w (addQuadPrimitivesAABBList
  // convention) so the in-place SAH reorder stays traceable back to triangles. Default allocator: bbox3f
  // needs 16-byte alignment for the vec ops.
  dag::Vector<bbox3f> triBoxes(faceCount);
  for (unsigned t = 0; t < faceCount; ++t)
  {
    bbox3f bx;
    v_bbox3_init(bx, srcVerts[idx[t * 3 + 0]]);
    v_bbox3_add_pt(bx, srcVerts[idx[t * 3 + 1]]);
    v_bbox3_add_pt(bx, srcVerts[idx[t * 3 + 2]]);
    bx.bmin = v_perm_xyzd(bx.bmin, v_cast_vec4f(v_splatsi((int)t)));
    triBoxes[t] = bx;
  }

  // SAH reorders triBoxes in place into spatial-partition order (one prim per leaf); only the order is
  // used, so the node table is a throwaway -- framemem per dag_bvhBuild.h's transient-build contract.
  Tab<bbox3f> sahNodes(framemem_ptr());
  int maxDepth = 0;
  create_bvh_node_sah(sahNodes, triBoxes.data(), faceCount, 4, maxDepth);

  // First-reference renumber over the SAH-ordered triangles.
  dag::Vector<int, framemem_allocator> newIdx(srcVertCount);
  for (unsigned i = 0; i < srcVertCount; ++i)
    newIdx[i] = -1;
  outVerts.reserve(srcVertCount);
  for (unsigned k = 0; k < faceCount; ++k)
  {
    const unsigned t = (unsigned)v_extract_wi(v_cast_vec4i(triBoxes[k].bmin));
    for (unsigned c = 0; c < 3; ++c)
    {
      const unsigned v = idx[t * 3 + c];
      if (newIdx[v] < 0)
      {
        newIdx[v] = (int)outVerts.size();
        outVerts.push_back(srcVerts[v]);
      }
    }
  }
  for (unsigned i = 0; i < idxCount; ++i)
    idx[i] = (IdxT)newIdx[idx[i]];
  // Second phase: duplicate the residual over-spread verts so every triangle fits the leaf offset range.
  return dedupWindowDup(idx, idxCount, minOff, maxOff, outVerts);
}

template <class IdxT>
static unsigned dedupWindowDup(IdxT *idx, unsigned idxCount, int minOff, int maxOff, dag::Vector<vec4f> &outVerts)
{
  const unsigned baseCount = (unsigned)outVerts.size();
  // Dup'd verts land at consecutive tail slots, so a block this wide keeps a min-apex (offsets
  // 0..blockCap-1) inside [0, maxOff]; the over-spread gate itself uses the wider asymmetric test.
  const unsigned blockCap = (unsigned)maxOff + 1u;
  if (baseCount <= blockCap)
    return baseCount; // max possible index span < blockCap -- nothing can over-spread
  const unsigned faceCount = idxCount / 3u;
  // After the SAH-leaf-order renumber verts are spatially local, so most large nodes still have
  // every triangle encodable. Cheap scan first: skip the O(baseCount) dup state unless a triangle
  // actually over-spreads.
  bool anyOverSpread = false;
  for (unsigned t = 0; t < faceCount; ++t)
  {
    const IdxT *tri = idx + (size_t)t * 3u;
    if (!triEncodable(tri[0], tri[1], tri[2], minOff, maxOff))
    {
      anyOverSpread = true;
      break;
    }
  }
  if (!anyOverSpread)
    return baseCount;
  dag::Vector<int, framemem_allocator> slot(baseCount), stamp(baseCount);
  for (unsigned i = 0; i < baseCount; ++i)
    stamp[i] = -1;
  int blockId = 0;
  unsigned blockFill = blockCap; // force a fresh block on the first over-spread triangle
  for (unsigned t = 0; t < faceCount; ++t)
  {
    IdxT *tri = idx + (size_t)t * 3u;
    if (triEncodable(tri[0], tri[1], tri[2], minOff, maxOff))
      continue; // fits in place -- keep the base verts
    const unsigned need = (stamp[tri[0]] != blockId) + (stamp[tri[1]] != blockId) + (stamp[tri[2]] != blockId);
    if (blockFill + need > blockCap) // this tri's verts would not fit the open block -- start a new one
    {
      ++blockId;
      blockFill = 0;
    }
    for (unsigned c = 0; c < 3; ++c)
    {
      const unsigned v = tri[c];
      if (stamp[v] != blockId)
      {
        stamp[v] = blockId;
        slot[v] = (int)outVerts.size();
        const vec4f baseVert = outVerts[v]; // v is always a base vert (< baseCount); load by value so a grow can't dangle it
        outVerts.push_back(baseVert);
        ++blockFill;
      }
      tri[c] = (IdxT)slot[v];
    }
  }
  return (unsigned)outVerts.size();
}

template unsigned leafOrderVertexFetch<uint16_t>(uint16_t *, unsigned, const vec4f *, unsigned, dag::Vector<vec4f> &, int, int);
template unsigned leafOrderVertexFetch<uint32_t>(uint32_t *, unsigned, const vec4f *, unsigned, dag::Vector<vec4f> &, int, int);

}; // namespace build_bvh
