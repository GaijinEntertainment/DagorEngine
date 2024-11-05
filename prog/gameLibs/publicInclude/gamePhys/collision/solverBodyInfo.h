//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathBase.h>

#include <generic/dag_span.h>

namespace daphys
{
struct SolverBodyInfo
{
  TMatrix tm = TMatrix::IDENT;
  TMatrix itm = TMatrix::IDENT;

  DPoint3 cog = DPoint3(0, 0, 0);

  DPoint3 vel = DPoint3(0, 0, 0);
  DPoint3 omega = DPoint3(0, 0, 0);

  DPoint3 addVel = DPoint3(0, 0, 0);
  DPoint3 addOmega = DPoint3(0, 0, 0);

  DPoint3 pseudoVel = DPoint3(0, 0, 0);
  DPoint3 pseudoOmega = DPoint3(0, 0, 0);

  DPoint3 invMoi = DPoint3(0, 0, 0);
  double invMass = 0.0;

  double commonImpulseLimit = VERY_BIG_NUMBER;
  dag::Span<double> impulseLimits = {};

  SolverBodyInfo(const TMatrix &in_tm, const TMatrix &in_itm, const DPoint3 &center_of_grav, const DPoint3 &in_vel,
    const DPoint3 &in_omega, const DPoint3 &inv_moi, double inv_mass) :
    tm(in_tm), itm(in_itm), cog(center_of_grav), vel(in_vel), omega(in_omega), invMoi(inv_moi), invMass(inv_mass)
  {}
};
}; // namespace daphys
