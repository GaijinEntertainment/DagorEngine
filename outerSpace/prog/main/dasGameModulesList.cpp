// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>
#include <package_physobj/pullDas.h>
#include <phys_kind/pullDas.h>

das::Module *das_pull_RebuildNavMeshModule() { return nullptr; } // Note: dummy editor pathfinder's dep

void pull_game_das()
{
  NEED_MODULES_PACKAGE_PHYSOBJ()
  NEED_MODULES_PHYS_KIND()
}
