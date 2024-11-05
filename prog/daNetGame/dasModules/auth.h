// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "net/authEvents.h"
#include <dasModules/dasModulesCommon.h>
#include <ecs/core/entityManager.h>

namespace bind_dascript
{
inline const char *get_country_code(das::Context *context, das::LineInfoArg *at)
{
  eastl::string country{};
  g_entity_mgr->broadcastEventImmediate(NetAuthGetCountryCodeEvent{&country});
  return context->allocateString(country, at);
}
} // namespace bind_dascript
