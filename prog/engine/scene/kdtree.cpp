// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMath.h>
#include <math/dag_mathUtils.h>
#include <scene/dag_kdtree.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
namespace kdtree
{
inline void make_box(bbox3f &box, bbox3f &cbox, const bbox3f *boxes, const uint32_t *indices, int cnt)
{
  box = boxes[indices[0]];
  v_bbox3_init(cbox, v_add(box.bmin, box.bmax));
  for (int i = 1; i < cnt; ++i)
  {
    v_bbox3_add_pt(cbox, v_add(boxes[indices[i]].bmin, boxes[indices[i]].bmax));
    v_bbox3_add_box(box, boxes[indices[i]]);
  }
  cbox.bmin = v_mul(cbox.bmin, V_C_HALF);
  cbox.bmax = v_mul(cbox.bmax, V_C_HALF);
}

inline int max_axis(bbox3f_cref cbox, float &center)
{
  alignas(16) float d[4];
  v_st(d, v_sub(cbox.bmax, cbox.bmin));
  vec4f vcenter = v_bbox3_center(cbox);
  if (d[0] >= d[1] && d[0] >= d[2])
  {
    center = v_extract_x(vcenter);
    return 0;
  }
  else if (d[1] >= d[0] && d[1] >= d[2])
  {
    center = v_extract_y(vcenter);
    return 1;
  }
  else
  {
    center = v_extract_z(vcenter);
    return 2;
  }
}
inline float get_max_box_size(bbox3f_cref box)
{
  vec3f width = v_bbox3_size(box);
  width = v_max(width, v_max(v_rot_1(width), v_rot_2(width)));
  return v_extract_x(width);
}

int make_nodes(KDNode *outRoot, const bbox3f *boxes, const float *center_x, const float *center_y, const float *center_z, int from,
  int to, uint32_t *indices, dag::Vector<KDNode> &nodes, int min_to_split_geom, int max_to_split_count, float min_box_to_split_geom,
  float max_box_to_split_count)
{
  const int cnt = to - from + 1;
  if (cnt >= KDNode::INDEX_COUNT)
  {
    G_ASSERTF(0, "too much kd leaves %d >= %d", cnt, KDNode::INDEX_COUNT); // if that ever happen we can actually change distribution
                                                                           // or build two trees
    return -1;
  }
  if (from >= KDNode::INDEX_COUNT)
  {
    G_ASSERTF(0, "too big offset %d >= %d", from, KDNode::INDEX_COUNT); // if that ever happen we can actually change distribution or
                                                                        // build two trees
  }
  if (from > to)
  {
    G_ASSERTF(0, "empty_node %d", from);
    return -1;
  }
  G_STATIC_ASSERT(sizeof(KDNode) == 32);
  bbox3f box, cbox;
  make_box(box, cbox, boxes, &indices[from], cnt);
  KDNode root;
  const int nodeId = (int)(nodes.size());
  root.bmin_start = box.bmin;
  root.bmax_count = box.bmax;
  root.setStart(from);
  root.setCount(cnt);
  if (!outRoot)
    nodes.push_back();
  bool child = true;
  float maxAxisWidth = get_max_box_size(box);
  if ((cnt > min_to_split_geom && maxAxisWidth > min_box_to_split_geom) || // doesn't make sense to split very small count
      (cnt > max_to_split_count && maxAxisWidth > max_box_to_split_count)) // doesn't make sense to split very small boxes
  {
    float center;
    int axis = max_axis(cbox, /*out*/ center);
    const float *centers = (axis == 0 ? center_x : (axis == 1 ? center_y : center_z));
    int median = from;
    for (int i = from; i <= to; ++i)
    {
      if (centers[indices[i]] < center)
        median++;
    }
    if (median <= from || median >= to - 1)
      median = (from + to) / 2;

    eastl::nth_element(indices + from, indices + median, indices + to + 1,
      [centers](const uint32_t &a, const uint32_t &b) { return centers[a] < centers[b]; });

    int left = make_nodes(NULL, boxes, center_x, center_y, center_z, from, median, indices, nodes, min_to_split_geom,
      max_to_split_count, min_box_to_split_geom, max_box_to_split_count);
    if (left < 0 || left - nodeId >= KDNode::NODES_COUNT)
      return -1;
    int right = make_nodes(NULL, boxes, center_x, center_y, center_z, median + 1, to, indices, nodes, min_to_split_geom,
      max_to_split_count, min_box_to_split_geom, max_box_to_split_count);

    if (right < 0 || right - left >= KDNode::NODES_COUNT)
      return -1;

    child = false;

    if (nodes[left].isLeaf() && nodes[right].isLeaf() && // is useless subdivision. checking two leaves is still checking two boxes)
        ((nodes[left].getCount() <= 2 && v_bbox3_test_box_inside(nodes[right].getBox(), nodes[left].getBox())) ||
          (nodes[right].getCount() <= 2 && v_bbox3_test_box_inside(nodes[left].getBox(), nodes[right].getBox()))))
      child = true;


    if (!child && cnt <= (max_to_split_count + min_to_split_geom) / 2 && // we tried to split according to geom reasoning, but results
                                                                         // weren't good
        v_bbox3_test_box_intersect(nodes[left].getBox(), nodes[right].getBox()) &&
        min(get_max_box_size(nodes[left].getBox()), get_max_box_size(nodes[right].getBox())) > min_box_to_split_geom)
      child = true;

    if (child)
      nodes.erase(nodes.begin() + nodeId + 1, nodes.end());
    else
    {
      root.setLeft(left - nodeId);
      root.setRight(right - left);
    }
  }
  if (child)
    eastl::sort(indices + from, indices + to + 1); // sort for cache locality, when then checking visibility.
  if (!outRoot)
    nodes.data()[nodeId] = root;
  else
  {
    *outRoot = root;
    G_ASSERTF(nodeId == root.getLeftToParent(nodeId) || root.isLeaf(), "%d == %d", nodeId, root.getLeftToParent(0));
  }
  return nodeId;
}
} // namespace kdtree
