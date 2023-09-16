#include <scene/dag_buildVisTree.h>
#include <math/dag_math3d.h>
#include <util/dag_stlqsort.h>
#include <supp/dag_prefetch.h>
// #include <debug/dag_debug.h>

struct VisTreeComparator
{
  int use_side;
  inline bool operator()(const HierVisNode &o1, const HierVisNode &o2) const
  {
    float c1 = as_point3(&o1.bbox.bmin)[use_side] + as_point3(&o1.bbox.bmax)[use_side];
    float c2 = as_point3(&o2.bbox.bmin)[use_side] + as_point3(&o2.bbox.bmax)[use_side];
    float dif = (c1 - c2) * 0.5f;
    if (fabsf(dif) < 0.001f)
      return 0;
    if (dif < 0)
      return true;
    return false;
  }
} comparator;

Tab<HierVisNode> BuildVisTree::buildList(tmpmem_ptr());

void BuildVisTree::clearLastPlaneWord() {}

void BuildVisTree::build(HierVisBuildNode *leaves, int leaves_count, bool clear_lp)
{
  leavesCount = leaves_count;
  if (!leavesCount)
  {
    clear_and_shrink(nodes);
    return;
  }


  buildHierVisData(leaves, leaves_count);

  nodes = buildList;
  clear_and_shrink(buildList);
  if (clear_lp)
    clearLastPlaneWord();
}

void BuildVisTree::buildHierVisData(HierVisBuildNode *leaves, int leaves_count)
{
  buildList.reserve(leaves_count * 2 + 1);
  buildList.resize(leaves_count);
  for (int i = 0; i < leaves_count; ++i)
  {
    float sr = length(leaves[i].bbox.center() - leaves[i].bsph.c) + leaves[i].bsph.r;
    float maxr2 = lengthSq(leaves[i].bbox.width()) * 0.25;

    buildList[i].bbox.bmin = v_make_vec4f(leaves[i].bbox.lim[0].x, leaves[i].bbox.lim[0].y, leaves[i].bbox.lim[0].z,
      leaves[i].maxSubobjRad * leaves[i].maxSubobjRad);
    buildList[i].bbox.bmax =
      v_make_vec4f(leaves[i].bbox.lim[1].x, leaves[i].bbox.lim[1].y, leaves[i].bbox.lim[1].z, (sr * sr > maxr2) ? maxr2 : sr * sr);

    buildList[i].flags = leaves[i].flags;
    buildList[i]._resv = i + 1;
    buildList[i].childFirst = leaves[i].first;
    buildList[i].childLast = leaves[i].last;
  }
  HierVisNode root;
  root.childFirst = 0;
  root.childLast = buildList.size() - 1;
  buildList.push_back(root);

  recursiveBuild(buildList.size() - 1);
}

static inline void set_bbox_sr2(bbox3f &b, vec4f sr2_x, vec4f msor2_w)
{
  vec4f sr2_w = v_mul(v_splat_x(sr2_x), V_C_UNIT_0001);
  b.bmin = v_nmsub(b.bmin, V_C_UNIT_0001, v_add(b.bmin, v_mul(msor2_w, V_C_UNIT_0001)));
  b.bmax = v_nmsub(b.bmax, V_C_UNIT_0001, v_add(b.bmax, sr2_w));
}

void BuildVisTree::recursiveBuild(int node_id)
{
  static vec4f_const c16 = {16.0f, 16.0f, 16.0f, 16.0f};
  int depth = node_id >> 16;
  HierVisNode &node = buildList[node_id & 0xFFFF];
  int start = node.childFirst;
  int cnt = node.childLast - node.childFirst + 1;

  v_bbox3_init_empty(node.bbox);
  node.flags = 0;
  node._resv = 0;
  for (int i = start; i < start + cnt; i++)
  {
    v_bbox3_add_box(node.bbox, buildList[i].bbox);
    node.flags |= buildList[i].flags;
  }
  vec3f sphC = v_bbox3_center(node.bbox);
  vec4f sr2 = v_zero();
  vec4f msor2 = v_zero();
  for (int i = start; i < start + cnt; i++)
  {
    vec4f rad = v_length3_x(v_sub(sphC, v_bbox3_center(buildList[i].bbox)));
    rad = v_add_x(rad, v_sqrt_x(buildList[i].sphRad2()));
    sr2 = v_max(sr2, v_mul_x(rad, rad));
    msor2 = v_max(msor2, buildList[i].bbox.bmin);
  }
  sr2 = v_min(sr2, v_length3_sq_x(v_mul(v_bbox3_size(node.bbox), V_C_HALF)));
  if (cnt <= 8 || (cnt < 16 && v_test_vec_x_lt(v_splat_x(sr2), v_mul(v_splat_w(node.bbox.bmax), c16)))) // || boxSize < maxSz*1.21
  {
    set_bbox_sr2(node.bbox, sr2, msor2);
    // is pre-leaf
    // debug("leaves count = %d, depth=%d", cnt, depth);
    return;
  }
  set_bbox_sr2(node.bbox, sr2, msor2);
  comparator.use_side = 0;
  vec3f width = v_bbox3_size(node.bbox);
  Point3_vec4 widthS;
  v_st(&widthS.x, width);
  for (int i = 1; i < 3; ++i)
    if (widthS[comparator.use_side] < widthS[i])
      comparator.use_side = i;
  // debug("node count = %d, depth=%d", cnt, depth);

  stlsort::sort(buildList.data() + start, buildList.data() + start + cnt, comparator);
  const int maxChildsCount = 4;
  HierVisNode childs[maxChildsCount];
  int childsCount = 2;
  if (cnt >= 8)
  {
    childsCount = maxChildsCount;
    int next_side = (comparator.use_side + 1) % 3;
    for (int i = 0; i < 3; ++i)
      if (i != comparator.use_side)
        if (widthS[next_side] < widthS[i])
          next_side = i;
    // next_side = 2-comparator.use_side;
    // if (node.bbox.width()[next_side]>node.bbox.width()[comparator.use_side]*0.5)
    {
      comparator.use_side = next_side;
      stlsort::sort(buildList.data() + start, buildList.data() + start + cnt / 2, comparator);
      stlsort::sort(buildList.data() + start + cnt / 2, buildList.data() + start + cnt, comparator);
    } // else
      // debug("not re-sort, side=%d", comparator.use_side);
  }
  for (int i = 0; i < childsCount; ++i)
  {
    childs[i].childFirst = start + i * cnt / childsCount;
    childs[i].childLast = start + (i + 1) * cnt / childsCount - 1;
  }
  int first = buildList.size();
  node.childFirst = first;
  node.childLast = first + childsCount - 1;
  // node& becomes invalid here
  append_items(buildList, childsCount, childs);
  depth++;
  for (int i = 0; i < childsCount; ++i)
  {
    recursiveBuild((first + i) | (depth << 16)); // node& is invalid here
  }
}
