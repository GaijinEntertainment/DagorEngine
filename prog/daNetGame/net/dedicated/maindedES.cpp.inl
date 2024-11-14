// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <ioSys/dag_dataBlock.h>
#include <gameRes/dag_gameResSystem.h>
#include <util/dag_delayedAction.h>
#include "net/dedicated/matching.h"
#include <daECS/net/network.h>
#include "game/gameEvents.h"
#include "game/player.h"
#include "net/dedicated.h"
#include "net/dedicated/matching.h"
#include "net/net.h"
#include "netded.h"
#include <bindQuirrelEx/autoBind.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <quirrel/quirrel_json/jsoncpp.h>

namespace workcycle_internal
{
extern void idle_loop();
}

namespace dedicated
{

ECS_AFTER(on_gameapp_started_es)
static void dedicated_init_on_appstart_es(const EventOnGameAppStarted &)
{
  set_no_gameres_factory_fatal(false);
#if !_TARGET_PC_LINUX // On Linux using workcycle::idle_loop() is called automatically from dagor_idle_cycle()
  // as close to start of new frame as possible (time calculation is relying on that)
  register_regular_action_to_idle_cycle([](void *) { workcycle_internal::idle_loop(); }, nullptr);
#endif
  dedicated_matching::init();
  g_entity_mgr->broadcastEventImmediate(DedicatedServerEventOnInit{});
}

void update()
{
  TIME_PROFILE(dedicated_update);
  dedicated_matching::update();
}

void shutdown()
{
  g_entity_mgr->broadcastEventImmediate(DedicatedServerEventOnTerm{});
  dedicated_matching::shutdown();
}
} // namespace dedicated

static void kick_player(int64_t user_id, const char *reason)
{
  if (const game::Player *plr = game::find_player_by_userid((matching::UserId)user_id))
    if (net::IConnection *conn = plr->getConn())
    {
      logwarn("Kicked player %ld (conn=#%d) because of '%s'", user_id, (int)conn->getId(), reason);
      net_disconnect(*conn, DC_KICK_GENERIC);
    }
}

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_get_mode_info, "dedicated", sq::VM_GAME)
{
  Sqrat::Table tbl(vm);
  tbl //
    .Func("get_matching_mode_info", [vm]() { return jsoncpp_to_quirrel(vm, dedicated_matching::get_mode_info()); })
    .Func("kick_player", kick_player)
    /**/;
  return tbl;
}
