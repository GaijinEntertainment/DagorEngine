// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_Point2.h>
class DataBlock;
class PhysVars;

struct AnimVar
{
  int id;
  Point2 range;
  AnimVar() : id(-1), range(ZERO<Point2>()) {}

  void load(
    PhysVars &phys_vars, const DataBlock *blk, const char *var_name, const char *node_name, const Point2 &range_def = ZERO<Point2>());
  void load(PhysVars &phys_vars, const DataBlock *blk, const char *name, const Point2 &range_def = ZERO<Point2>());
  void load(PhysVars &phys_vars, const DataBlock *blk, const char *name, int index, const Point2 &range_def = ZERO<Point2>());
  float loop(float val) const;
  void animate(PhysVars &phys_vars, float val) const;
  void animateSmooth(PhysVars &phys_vars, float val, float dt, float smoothness) const;
  void animateLooped(PhysVars &phys_vars, float val) const;
};