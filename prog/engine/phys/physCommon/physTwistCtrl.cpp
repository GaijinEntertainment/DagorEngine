// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/twistCtrl.h>
#include <math/dag_geomTree.h>
#include <math/dag_vecMathCompatibility.h>
#include <phys/dag_physResource.h>
#include <phys/dag_physTwistCtrl.h>
#include <generic/dag_span.h>

Tab<TwistCtrlParams> make_phys_twist_ctrls(const PhysicsResource &phys_res, const GeomNodeTree &tree)
{
  Tab<TwistCtrlParams> result;
  if (!phys_res.getNodeAlignCtrl().size())
    return result;

  dag::ConstSpan<PhysicsResource::NodeAlignCtrl> c = phys_res.getNodeAlignCtrl();
  result.reserve(c.size());
  for (int i = 0; i < c.size(); i++)
  {
    TwistCtrlParams &ctrl = result.push_back();
    ctrl.node0Id = tree.findNodeIndex(c[i].node0);
    ctrl.node1Id = tree.findNodeIndex(c[i].node1);
    ctrl.angDiff = c[i].angDiff;
    for (int j = 0; j < countof(c[i].twist) && !c[i].twist[j].empty(); j++)
    {
      ctrl.twistId[j] = tree.findNodeIndex(c[i].twist[j]);
      if (!ctrl.twistId[j])
      {
        ctrl.twistCnt = 0;
        break;
      }
      ctrl.twistCnt = j + 1;
    }
    if (!ctrl.node0Id || !ctrl.node1Id || !ctrl.twistCnt)
      result.pop_back();
  }

  return result;
}

void apply_phys_twist_ctrl(GeomNodeTree &tree, const TwistCtrlParams &ctrl)
{
  apply_twist_ctrl(tree, ctrl.node0Id, ctrl.node1Id, make_span(ctrl.twistId, ctrl.twistCnt), ctrl.angDiff);
}
