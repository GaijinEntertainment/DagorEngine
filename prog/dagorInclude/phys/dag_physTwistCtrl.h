//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_index16.h>

class PhysicsResource;
class GeomNodeTree;

struct TwistCtrlParams
{
  dag::Index16 node0Id, node1Id, twistId[3];
  int16_t twistCnt = 0;
  float angDiff = 0;
};

Tab<TwistCtrlParams> make_phys_twist_ctrls(const PhysicsResource &phys_res, const GeomNodeTree &tree);
void apply_phys_twist_ctrl(GeomNodeTree &tree, const TwistCtrlParams &ctrl);
