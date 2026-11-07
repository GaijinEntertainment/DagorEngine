//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <phys/dag_physics.h>

// ECS probes sizeof(PhysBody) here, so the backend class must be complete.
ECS_DECLARE_BOXED_TYPE(PhysBody);

extern PhysWorld *g_phys_world; // expected to be set from outside
