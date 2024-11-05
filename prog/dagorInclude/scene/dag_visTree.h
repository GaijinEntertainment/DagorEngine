//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>
#include <vecmath/dag_vecMath.h>
#include <supp/dag_prefetch.h>
#include <util/dag_stdint.h>
#include <scene/dag_visibility.h>
#include <scene/dag_occlusion.h>

struct HierVisNode
{
  uint32_t flags;
  uint32_t childFirst, childLast;
  uint32_t _resv;
  bbox3f bbox;

  vec4f sphRad2() const { return v_splat_w(bbox.bmax); }
  vec4f sphMaxSubRad2() const { return v_splat_w(bbox.bmin); }
  static constexpr int INVALID_NODE = 0xFFFF;

  HierVisNode &operator=(const HierVisNode &n)
  {
    memcpy(this, &n, sizeof(n));
    return *this;
  }
};

class VisTree
{
public:
  VisTree() : leavesCount(0), leavesVisibility(0) {}

  // template class Ex must have
  //   static void mark_visible(void *obj, uint16_t childFirst, uint16_t childLast);
  template <class Ex>
  void prepareVisibility(void *leaves_visibility, int render_type, unsigned flags, const VisibilityFinder &vf)
  {
    G_ASSERT(nodes.size());
    if (render_type > 10)
      render_type = 10;
    leavesVisibility = leaves_visibility;
    current_flags = flags;
    visibility_recursive<Ex>(nodes[leavesCount], false, vf);
  }

  dag::Span<HierVisNode> nodes;
  int leavesCount; // number of leaves - first N of nodes are leaves

protected:
  unsigned current_flags;
  void *leavesVisibility;

  template <class Ex>
  void set_visibility(HierVisNode &node, bool is_obj, const VisibilityFinder &vf)
  {
    if (!(node.flags & current_flags))
      return;
    if (!vf.isScreenRatioVisible(v_bbox3_center(node.bbox), node.sphRad2(), node.sphMaxSubRad2()))
      return;
    const Occlusion *occlusion = vf.getOcclusion();
    if (occlusion != nullptr && occlusion->isOccludedBox(node.bbox))
      return;
    if (!is_obj)
    {
      bool child_objes = node.childFirst < leavesCount;
      HierVisNode *__restrict childs = &nodes[node.childFirst], *__restrict childe = &nodes[node.childLast];
      for (; childs <= childe; childs++)
        set_visibility<Ex>(*childs, child_objes, vf);
    }
    else
      Ex::mark_visible(leavesVisibility, node.childFirst, node.childLast, &node - nodes.data(), node.bbox);
  }

  template <class Ex>
  void visibility_recursive(HierVisNode &node, bool is_obj, const VisibilityFinder &vf)
  {
    if (!(node.flags & current_flags))
      return;
    if (!is_obj)
    {
      unsigned visResult = vf.isVisibleBoxV(node.bbox.bmin, node.bbox.bmax);
      bool child_objes = node.childFirst < leavesCount;
      HierVisNode *__restrict childs = &nodes[node.childFirst], *__restrict childe = &nodes[node.childLast];
      if (VisibilityFinder::isInside(visResult))
      {
        for (; childs <= childe; childs++)
          set_visibility<Ex>(*childs, child_objes, vf);
      }
      else if (VisibilityFinder::isIntersect(visResult))
      {
        for (; childs <= childe; childs++)
          visibility_recursive<Ex>(*childs, child_objes, vf);
      }
    }
    else
    {
      if (vf.isVisibleBoxVLeaf(node.bbox.bmin, node.bbox.bmax))
        Ex::mark_visible(leavesVisibility, node.childFirst, node.childLast, &node - nodes.data(), node.bbox);
    }
  }
};
