// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>

das::Module *register_RebuildNavMeshModule() { return nullptr; } // Note: dummy editor pathfinder's dep

void pull_game_das() { NEED_MODULE(PuddleQueryManagerModule); }
