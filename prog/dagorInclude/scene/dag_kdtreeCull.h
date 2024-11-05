//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <scene/dag_kdtree.h>
#include <math/dag_frustum.h>
#include <scene/dag_occlusion.h>

namespace kdtree
{

struct VisibleLeaf
{
  uint32_t start = 0, count = 0;
  VisibleLeaf(uint32_t s, uint32_t c) : start(s), count(c) {}
};

template <class T, int N>
class FastStack // fast stack based stack
{
public:
  int size() const { return num; }
  const T *top() const
  {
    G_FAST_ASSERT(num < N);
    return data + num - 1;
  } // not check, intentionally
  void pop()
  {
    G_FAST_ASSERT(num > 0);
    --num;
  }
  void push(const T &var)
  {
    G_FAST_ASSERT(num < N);
    data[num++] = var;
  }
  const T &topAndPop() { return data[--num]; } // not check, intentionally
protected:
  T data[N];
  int num = 0;
};

template <class T>
inline auto top_and_pop(T &t)
{
  auto v = *t.top();
  t.pop();
  return v;
}
template <class T, int N>
inline auto top_and_pop(FastStack<T, N> &t)
{
  return t.topAndPop();
} // overload

struct AlwaysVisible
{
  __forceinline bool operator()(bool, uint32_t, bbox3f_cref) const { return true; }
};

struct CheckNotOccluded
{
  Occlusion *occlusion = 0;
  CheckNotOccluded(Occlusion *o) : occlusion(o) {}
  __forceinline bool operator()(bool fully_inside, uint32_t, bbox3f_cref box) const
  {
    if (!fully_inside && v_bbox3_test_pt_inside(box, occlusion->getCurViewPos())) // do not check for occlusion if viewpos is inside
                                                                                  // box
      return true;
    return occlusion->isVisibleBox(box.bmin, box.bmax);
  }
};

//
struct DistFastCheck
{
  const float *distances = 0;
  vec3f vpos_scale;
  DistFastCheck(const float *d, vec4f vp_sc) : distances(d), vpos_scale(vp_sc) {}
  __forceinline bool operator()(bool, uint32_t node, bbox3f_cref box) const
  {
    return v_test_vec_x_lt(v_mul(v_splat_w(vpos_scale), v_distance_sq_to_bbox_x(box.bmin, box.bmax, vpos_scale)),
      v_splats(distances[node]));
  }
};

static const uint32_t fully_inside_node_flag = 0x80000000;

template <bool allowEarly, class Visible, typename FastCheckNode, typename CheckVisible, class WorkingSet>
__forceinline void kd_frustum_visibility(WorkingSet &set, vec3f plane03X, vec3f plane03Y, vec3f plane03Z, const vec3f &plane03W,
  const vec3f &plane47X, const vec3f &plane47Y, const vec3f &plane47Z, const vec3f &plane47W, const kdtree::KDNode *nodes,
  const FastCheckNode &fast_check_node, // fast check, such as flags and distance, or box
  const CheckVisible &check_visible, Visible &visible)
{
  for (; set.size();)
  {
    uint32_t node = top_and_pop(set); // faster for FastStack
    uint32_t fully_inside = node & fully_inside_node_flag;
    node &= ~fully_inside_node_flag;
    bbox3f box;
    box.bmin = nodes[node].bmin_start;
    box.bmax = nodes[node].bmax_count;
    // todo: continue recursive dist check if min distance to box is bigger than distances[node]
    if (!fast_check_node(fully_inside, node, box))
      continue;
    if (!fully_inside)
    {
      // int vis = frustum.testBox(box.bmin, box.bmax);
      vec3f center = v_bbox3_center(box);
      const int vis = v_box_frustum_intersect_extent2(center, v_sub(box.bmax, center), plane03X, plane03Y, plane03Z, plane03W,
        plane47X, plane47Y, plane47Z, plane47W);
      if (vis == Frustum::OUTSIDE)
        continue;
      if (vis == Frustum::INSIDE)
        fully_inside = fully_inside_node_flag;
    }

    if (!check_visible(fully_inside, node, box))
      continue;
    if (nodes[node].isLeaf() || (allowEarly && fully_inside)) // leaf
    {
      if (visible.size()) // optimize reallocation by collapsing node
      {
        const int start = nodes[node].getStart();
        auto &last = visible.back();
        const int lastCount = last.count & (~fully_inside_node_flag);
        if ((last.count & fully_inside_node_flag) == fully_inside && last.start + lastCount == start)
        {
          last.count = (lastCount + nodes[node].getCount()) | fully_inside;
          continue;
        }
      }
      visible.emplace_back(nodes[node].getStart(), (fully_inside | nodes[node].getCount()));
      continue;
    }
    const int left = nodes[node].getLeftToParent(node);
    set.push(nodes[node].getRightToLeft(left) | fully_inside);
    set.push(left | fully_inside);
  }
}

template <int MaxDepth, bool allowEarly, class Visible, typename FastCheckNode, typename CheckVisible>
inline void kd_frustum_visibility(uint32_t node, mat44f_cref globtm, const kdtree::KDNode *nodes,
  const FastCheckNode &fast_check_node, // fast check, such as flags and distance, or box
  const CheckVisible &check_visible,    // slow check
  Visible &visible)
{
  vec3f plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W;
  v_construct_camplanes(globtm, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y);
  v_mat44_transpose(plane03X, plane03Y, plane03Z, plane03W);
  plane47Z = plane47X, plane47W = plane47Y; // we can use some useful planes instead of replicating
  v_mat44_transpose(plane47X, plane47Y, plane47Z, plane47W);

  FastStack<uint32_t, MaxDepth + 1> workingSet;
  workingSet.push(node);
  kd_frustum_visibility<allowEarly>(workingSet, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W, nodes,
    fast_check_node, check_visible, visible);
}

}; // namespace kdtree
