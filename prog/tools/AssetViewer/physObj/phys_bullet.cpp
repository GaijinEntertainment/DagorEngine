// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define USE_BULLET_PHYSICS 1

#include <phys/dag_physDecl.h>
inline constexpr PhysWorld *get_external_phys_world() { return nullptr; }

#include "phys.inc.cpp"
IMPLEMENT_PHYS_API(bullet)
