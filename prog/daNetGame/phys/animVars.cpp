// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "phys/animVars.h"
#include <gamePhys/phys/physVars.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <math/dag_mathUtils.h>

void AnimVar::load(PhysVars &phys_vars, const DataBlock *blk, const char *var_name, const char *node_name, const Point2 &range_def)
{
  id = phys_vars.registerVar(var_name, 0.f);
  range = blk->getPoint2(node_name, range_def);
}

void AnimVar::load(PhysVars &phys_vars, const DataBlock *blk, const char *name, const Point2 &range_def)
{
  String varName(64, "p_%s", name);
  load(phys_vars, blk, varName.str(), name, range_def);
}

void AnimVar::load(PhysVars &phys_vars, const DataBlock *blk, const char *name, int index, const Point2 &range_def)
{
  String nodeName(64, name, index > 0 ? String(8, "%d", index).str() : "");
  String varName(64, "p_%s", nodeName.str());
  load(phys_vars, blk, varName.str(), nodeName.str(), range_def);
}

float AnimVar::loop(float val) const
{
  return range.y - range.x > VERY_SMALL_NUMBER ? range.x + fmodf(val - range.x, range.y - range.x) : range.x;
}

void AnimVar::animate(PhysVars &phys_vars, float val) const { phys_vars.setVar(id, cvt(val, range.x, range.y, 0.0f, 1.0f)); }

void AnimVar::animateSmooth(PhysVars &phys_vars, float val, float dt, float smoothness) const
{
  const float curValue = phys_vars.getVar(id);
  const float desiredValue = cvt(val, range.x, range.y, 0.0f, 1.0f);
  const float newValue = move_to(curValue, desiredValue, dt, smoothness);
  phys_vars.setVar(id, newValue);
}

void AnimVar::animateLooped(PhysVars &phys_vars, float val) const
{
  const float loopedVar = loop(val);
  phys_vars.setVar(id, cvt(loopedVar, range.x, range.y, 0.0f, 1.0f));
}