//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <EASTL/string.h>
#include <daECS/core/entityManager.h>
#include <dasModules/aotEcs.h>

extern void auth_get_country_code(ecs::EntityManager &mgr, eastl::string &country);

namespace bind_dascript
{

inline const char *get_country_code(das::Context *context, das::LineInfoArg *at)
{
  eastl::string country{};
  ecs::EntityManager *mgr = cast_es_context(context)->mgr;
  auth_get_country_code(mgr ? *mgr : *g_entity_mgr, country);
  return context->allocateString(country, at);
}

} // namespace bind_dascript
