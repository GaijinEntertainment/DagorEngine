// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_geomTree.h>
#include <math/dag_vecMathCompatibility.h>

static inline void apply_twist_ctrl(GeomNodeTree &tree, dag::Index16 node0, dag::Index16 node1, dag::Span<dag::Index16> twistNodes,
  float ang_diff)
{
  if (!twistNodes.size())
    return;

  tree.partialCalcWtm(node1);

  Point3 dir = (tree.getNodeWposRelScalar(node1) - tree.getNodeWposRelScalar(node0)) / twistNodes.size();
  float twist_ang =
    (fabsf(normalize(as_point3(&tree.getNodeTm(node1).col0)).y) < 0.9999)
      ? safe_atan2(-normalize(as_point3(&tree.getNodeTm(node1).col2)).y, normalize(as_point3(&tree.getNodeTm(node1).col1)).y)
      : 0.f;

  twist_ang = norm_s_ang(twist_ang - ang_diff);
  for (int i = 0; i < twistNodes.size(); i++)
  {
    mat44f &tn_tm = tree.getNodeTm(twistNodes[i]);
    mat44f &tn_wtm = tree.getNodeWtmRel(twistNodes[i]);
    v_mat33_make_rot_cw_x((mat33f &)tn_tm, v_splats((twist_ang * (i + 1)) / (twistNodes.size() + 1))); //-V1027
    tn_tm.col3 = v_zero();
    v_mat44_mul43(tn_wtm, tree.getNodeWtmRel(node0), tn_tm);
    if (i > 0)
      as_point3(&tn_wtm.col3) += dir * i;
    tree.recalcTm(twistNodes[i], false);

    if (i + 1 == twistNodes.size())
      tree.calcWtmForBranch(twistNodes[i]);
  }
}
