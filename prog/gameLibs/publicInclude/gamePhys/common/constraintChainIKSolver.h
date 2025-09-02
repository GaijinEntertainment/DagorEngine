//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gamePhys/common/chainIKSolver.h>
#include <EASTL/vector.h>

namespace chain_ik
{

// stops first point from rotation by chainIK
// makes last point move along first point, like piston
struct Constraint
{
  // readonly fields
  eastl::vector<Chain *> chainsToAffect;
  const Point2 offsetLimits;

  Constraint(const Point2 offset_limits) : offsetLimits(offset_limits) {}

  void apply(Chain *chain);
};

class ConstraintChainIKSolver
{
protected:
  eastl::vector<Constraint> constraints;

public:
  void solve();
  ConstraintChainIKSolver(const DataBlock *blk, const GeomNodeTree *tree, ChainIKSolver &chain_solver, const char *class_name);
};
} // namespace chain_ik