//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector_map.h>
#include <daECS/core/entityId.h>

namespace ecs // link time resolved dependency on async resources
{
typedef eastl::vector_map<eastl::string /*res_name*/, unsigned /*res_class_id*/> gameres_list_t;
bool load_gameres_list(const gameres_list_t &reslist);
// Return true if resulted filtered reslist is not empty
bool filter_out_loaded_gameres(gameres_list_t &reslist, unsigned spins = 1024);
void place_gameres_request(eastl::vector<ecs::EntityId> &&, gameres_list_t &&);
}; // namespace ecs
