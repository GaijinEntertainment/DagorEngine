//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gamePhys/collision/solverBodyInfo.h>

class IPhysBase;

namespace gamephys
{
daphys::SolverBodyInfo phys_to_solver_body(const IPhysBase *phys);
};
