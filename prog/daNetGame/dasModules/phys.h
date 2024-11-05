// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>

#include "gamePhys/phys/physBase.h"
#include "phys/netPhys.h"

MAKE_TYPE_FACTORY(IPhysBase, IPhysBase);

namespace bind_dascript
{
inline const IPhysBase &base_phys_actor_phys(const BasePhysActor &physActor) { return physActor.getPhys(); }
} // namespace bind_dascript
