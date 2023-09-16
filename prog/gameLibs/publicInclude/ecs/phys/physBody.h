//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <phys/dag_physDecl.h>

ECS_DECLARE_BOXED_TYPE(PhysBody);

extern PhysWorld *g_phys_world; // expected to be set from outside
