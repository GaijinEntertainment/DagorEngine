// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/physToSolverBody.h>
#include <generic/dag_span.h>
#include <gamePhys/common/loc.h>
#include <gamePhys/phys/physBase.h>
#include <vecmath/dag_vecMath.h>

daphys::SolverBodyInfo gamephys::phys_to_solver_body(const IPhysBase *phys)
{
  DPoint3 moi = phys->calcInertia();
  gamephys::Loc loc = phys->getCurrentStateLoc();
  TMatrix tm;
  loc.toTM(tm); // rotation from a unit quaternion plus translation, hence orthonormal
  mat44f vtm, vitm;
  v_mat44_make_from_43cu_unsafe(vtm, tm.array);
  v_mat44_orthonormal_inverse43(vitm, vtm); // transpose, not a general inverse
  TMatrix itm;
  v_mat_43cu_from_mat44(itm.array, vitm);
  return daphys::SolverBodyInfo(tm, itm, dpoint3(phys->getCenterOfMass()), phys->getCurrentStateVelocity(),
    phys->getCurrentStateOmega(), DPoint3(safeinv(moi.x), safeinv(moi.y), safeinv(moi.z)), safeinv(phys->getMass()));
}
