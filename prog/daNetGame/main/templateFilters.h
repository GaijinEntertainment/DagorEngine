// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/vector_map.h>
class DataBlock;

namespace ecs
{
class EntityManager;
} // namespace ecs

typedef eastl::vector_map<eastl::string, eastl::pair<eastl::string, eastl::string>> ComponentToFilterAndPath;

void apply_component_filters(const ecs::EntityManager &mgr, const ComponentToFilterAndPath &);
void apply_replicated_component_client_modify_blacklist(const ecs::EntityManager &mgr, const ComponentToFilterAndPath &);

void read_component_filters(const DataBlock &blk, ComponentToFilterAndPath &to);
void read_replicated_component_client_modify_blacklist(const DataBlock &blk, ComponentToFilterAndPath &to);
