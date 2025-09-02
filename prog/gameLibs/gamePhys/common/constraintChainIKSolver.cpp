// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/common/constraintChainIKSolver.h>
#include <math/dag_geomTree.h>
#include <math/dag_mathUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <regExp/regExp.h>


namespace chain_ik
{

void Constraint::apply(Chain *chain)
{
  const ParticlePoint &front = chain->points.front();
  ParticlePoint &back = chain->points.back();

  const Point3 posFrom = front.getPos();
  const Point3 direction = front.tm % normalizeDef(inverse(front.initialTm) * back.initialTm.getcol(3), Point3::ZERO);

  const float originalLength = (back.getOriginalPos() - front.getOriginalPos()).length();
  const Point3 posA = posFrom + (originalLength - offsetLimits.x) * direction;
  const Point3 posB = posFrom + (originalLength + offsetLimits.y) * direction;

  const Point3 posOnEdge = back.getPos();
  float t = 0.f;
  Point3 ptOnEdge = posOnEdge;
  distanceToSeg(posOnEdge, posA, posB, t, ptOnEdge);

  back.tm.setcol(3, ptOnEdge);
}

void ConstraintChainIKSolver::solve()
{
  for (Constraint &constraint : constraints)
  {
    for (Chain *chain : constraint.chainsToAffect)
      constraint.apply(chain);
  }
}

ConstraintChainIKSolver::ConstraintChainIKSolver(const DataBlock *blk, const GeomNodeTree *tree, ChainIKSolver &chain_solver,
  const char *class_name)
{
  G_UNUSED(class_name);
  const int constraintsCount = blk->blockCount();
  constraints.reserve(constraintsCount);

  for (int i = 0; i < constraintsCount; ++i)
  {
    const DataBlock *constrBlk = blk->getBlock(i);
    const char *chainTemplate = constrBlk->getStr("chainTemplate", nullptr);
    if (!chainTemplate)
      continue;

    RegExp constrRe;
    constrRe.compile(chainTemplate, "");

    const Point2 offsetLimits = constrBlk->getPoint2("offsetLimits", Point2::ZERO);
    Constraint constraint(offsetLimits);

    for (Chain &chain : chain_solver.chains)
    {
      dag::Index16 nodeId = chain.points.front().nodeId;
      if (constrRe.test(tree->getNodeName(nodeId)))
      {
        chain.needRotateFirst = false;
        constraint.chainsToAffect.push_back(&chain);
      }
    }
    if (!constraint.chainsToAffect.empty())
      constraints.push_back(constraint);
    else
      G_ASSERT_LOG(false, "ConstraintChainIKSolver: found no chains, suitable for '%s' template, unit %s", chainTemplate, class_name);
  }
}

} // namespace chain_ik