//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityId.h>
#include <EASTL/string.h>
#include <EASTL/optional.h>
#include <EASTL/fixed_function.h>
#include <generic/dag_span.h>

namespace bind_dascript
{
struct DasEvent;
}
class SqModules;

namespace net
{
class IConnection;
void register_dasevents(SqModules *module_mgr);
void send_dasevent(ecs::EntityManager *, bool broadcast, const ecs::EntityId eid, bind_dascript::DasEvent *evt, const char *event_name,
  eastl::optional<dag::ConstSpan<net::IConnection *>> explicitConnections,
  eastl::fixed_function<sizeof(void *), eastl::string()> debug_msg);
} // namespace net
