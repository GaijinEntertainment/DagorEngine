// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/physToSolverBody.h>
#include <generic/dag_span.h>
#include <gamePhys/common/loc.h>
#include <gamePhys/phys/physBase.h>

daphys::SolverBodyInfo gamephys::phys_to_solver_body(const IPhysBase *phys)
{
  DPoint3 moi = phys->calcInertia();
  gamephys::Loc loc = phys->getCurrentStateLoc();
  TMatrix tm;
  loc.toTM(tm);
  TMatrix itm = inverse(tm);
  return daphys::SolverBodyInfo(tm, itm, dpoint3(phys->getCenterOfMass()), phys->getCurrentStateVelocity(),
    phys->getCurrentStateOmega(), DPoint3(safeinv(moi.x), safeinv(moi.y), safeinv(moi.z)), safeinv(phys->getMass()));
}
