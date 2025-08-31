// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/autoBind.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <quirrel/sqModules/sqModules.h>
#include <sqrat.h>
#include <ecs/scripts/sqEntity.h>
#include <ecs/scripts/sqBindEvent.h>
#include <daECS/net/netEvents.h>
#include <daECS/net/time.h>
#include <dasModules/aotEcsEvents.h>

#include "userid.h"
#include "net/netScope.h"
#include "net/protoVersion.h"

namespace net
{

/// @module net
void bind_danetgame_net(SqModules *module_mgr)
{
  // It's first declared in gameLibs, daNetGame only adds few daNet-specific functions.
  Sqrat::Object *existingModule = module_mgr->findNativeModule("net");
  auto tbl = existingModule ? Sqrat::Table(*existingModule) : Sqrat::Table(module_mgr->getVM());

  tbl //
    .Func("get_user_id", net::get_user_id)
    .Func("add_entity_in_net_scope", add_entity_in_net_scope)
    .Func("get_sync_time", get_sync_time)
    /**/;
  tbl.SetValue("NET_PROTO_VERSION", NET_PROTO_VERSION); // todo: split by game?

  if (!existingModule)
    module_mgr->addNativeModule("net", tbl);
}

} // namespace net
