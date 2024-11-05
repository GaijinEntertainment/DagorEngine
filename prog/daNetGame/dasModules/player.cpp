// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/player.h"
#include <daScript/daScript.h>
#include <dasModules/aotEcs.h>
#include <dasModules/actor.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/net.h>

#include "game/player.h"

MAKE_TYPE_FACTORY(Player, game::Player);

struct PlayerAnnotation final : das::ManagedStructureAnnotation<game::Player, false>
{
  PlayerAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Player", ml, " ::game::Player") {}
};

MAKE_TYPE_FACTORY(LookingAt, bind_dascript::LookingAt);

struct LookingAtAnnotation final : das::ManagedStructureAnnotation<bind_dascript::LookingAt, false>
{
  LookingAtAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("LookingAt", ml, "bind_dascript::LookingAt")
  {
    addField<DAS_BIND_MANAGED_FIELD(pos)>("pos");
    addField<DAS_BIND_MANAGED_FIELD(dir)>("dir");
  }
};

namespace bind_dascript
{
class PlayerModule final : public das::Module
{
public:
  PlayerModule() : das::Module("player")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("DngNet"));
    addBuiltinDependency(lib, require("DngActor"));

    addAnnotation(das::make_smart<PlayerAnnotation>(lib));
    addAnnotation(das::make_smart<LookingAtAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(game::get_local_player_eid)>(*this, lib, "get_local_player_eid", das::SideEffects::none,
      "game::get_local_player_eid");
    das::addExtern<DAS_BIND_FUN(game::set_local_player_eid)>(*this, lib, "set_local_player_eid", das::SideEffects::modifyExternal,
      "game::set_local_player_eid");
    das::addExtern<DAS_BIND_FUN(game::find_player_eid_for_connection)>(*this, lib, "find_player_eid_for_connection",
      das::SideEffects::accessExternal, "game::find_player_eid_for_connection");
    das::addExtern<DAS_BIND_FUN(game::find_player_for_connection)>(*this, lib, "find_player_for_connection",
      das::SideEffects::accessExternal, "game::find_player_for_connection");
    das::addExtern<DAS_BIND_FUN(bind_dascript::player_get_looking_at), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "player_get_looking_at", das::SideEffects::accessExternal, "bind_dascript::player_get_looking_at");

    using method_resetSyncData = DAS_CALL_MEMBER(game::Player::resetSyncData);
    das::addExtern<DAS_CALL_METHOD(method_resetSyncData)>(*this, lib, "resetSyncData", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(game::Player::resetSyncData));
    using method_calcControlsTickDelta = DAS_CALL_MEMBER(game::Player::calcControlsTickDelta);
    das::addExtern<DAS_CALL_METHOD(method_calcControlsTickDelta)>(*this, lib, "calcControlsTickDelta",
      das::SideEffects::modifyArgumentAndAccessExternal, DAS_CALL_MEMBER_CPP(game::Player::calcControlsTickDelta));

    das::addConstant(*this, "TEAM_UNASSIGNED", (int)TEAM_UNASSIGNED);
    das::addConstant(*this, "TEAM_SPECTATOR", (int)TEAM_SPECTATOR);

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"game/player.h\"\n";
    tw << "#include \"dasModules/player.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(PlayerModule, bind_dascript);
