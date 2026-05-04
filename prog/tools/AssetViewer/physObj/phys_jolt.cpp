// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define USE_JOLT_PHYSICS 1

#include <gamePhys/collision/collisionLib.h>
#include "../av_appwnd.h"

// DNG already creates and owns Jolt physworld
inline PhysWorld *get_external_phys_world() { return get_app().dngBasedSceneRenderUsed() ? dacoll::get_phys_world() : nullptr; }

#include "phys.inc.cpp"
IMPLEMENT_PHYS_API(jolt)
