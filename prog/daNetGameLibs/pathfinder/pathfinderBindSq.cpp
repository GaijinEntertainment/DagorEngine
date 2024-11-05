// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/autoBind.h>
#include <quirrel/sqPathFinder/sqPathFinder.h>

using bindquirrel::sqrat_bind_path_finder;
SQ_DEF_AUTO_BINDING_REGFUNC(sqrat_bind_path_finder);

#include <daECS/core/componentType.h>
ECS_DEF_PULL_VAR(dng_pathfinder_sq);
