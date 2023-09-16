//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <landMesh/biomeQuery.h>

MAKE_TYPE_FACTORY(BiomeQueryResult, BiomeQueryResult);


namespace bind_dascript
{
inline int biome_query_start(Point3 world_pos, float radius) { return biome_query::query(world_pos, radius); }
} // namespace bind_dascript
