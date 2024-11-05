//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gamePhys/collision/solverBodyInfo.h>

class IPhysBase;

namespace gamephys
{
daphys::SolverBodyInfo phys_to_solver_body(const IPhysBase *phys);
};
