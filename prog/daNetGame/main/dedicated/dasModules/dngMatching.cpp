// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dngMatching.h"


namespace bind_dascript
{
class DngMatchingModule final : public das::Module
{
public:
  DngMatchingModule() : das::Module("DngMatching")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("rapidjson"));

    das::addExtern<DAS_BIND_FUN(das_get_mode_info)>(*this, lib, "get_mode_info", das::SideEffects::accessExternal,
      "::bind_dascript::das_get_mode_info");
    das::addExtern<DAS_BIND_FUN(das_get_mode_info_max_player_mult)>(*this, lib, "get_mode_info_max_player_mult",
      das::SideEffects::accessExternal, "::bind_dascript::das_get_mode_info_max_player_mult");
    das::addExtern<DAS_BIND_FUN(das_get_mode_info_extra_params)>(*this, lib, "get_mode_info_extra_params",
      das::SideEffects::accessExternal, "::bind_dascript::das_get_mode_info_extra_params");
    das::addExtern<DAS_BIND_FUN(dedicated_matching::on_level_loaded)>(*this, lib, "on_level_loaded", das::SideEffects::modifyExternal,
      "dedicated_matching::on_level_loaded");
    das::addExtern<DAS_BIND_FUN(dedicated_matching::on_player_team_changed)>(*this, lib, "on_player_team_changed",
      das::SideEffects::modifyExternal, "dedicated_matching::on_player_team_changed");
    das::addExtern<DAS_BIND_FUN(dedicated_matching::player_kick_from_room)>(*this, lib, "player_kick_from_room",
      das::SideEffects::modifyExternal, "dedicated_matching::player_kick_from_room");
    das::addExtern<DAS_BIND_FUN(dedicated_matching::ban_player_in_room)>(*this, lib, "ban_player_in_room",
      das::SideEffects::modifyExternal, "dedicated_matching::ban_player_in_room");
    das::addExtern<DAS_BIND_FUN(dedicated_matching::get_player_req_teams_num)>(*this, lib, "get_player_req_teams_num",
      das::SideEffects::accessExternal, "dedicated_matching::get_player_req_teams_num");
    das::addExtern<DAS_BIND_FUN(dedicated_matching::get_room_members_count)>(*this, lib, "get_room_members_count",
      das::SideEffects::accessExternal, "dedicated_matching::get_room_members_count");
    das::addExtern<DAS_BIND_FUN(dedicated_matching::get_player_custom_info)>(*this, lib, "get_player_custom_info",
      das::SideEffects::accessExternal, "dedicated_matching::get_player_custom_info");
    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <main/dedicated/dasModules/dngMatching.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngMatchingModule, bind_dascript);
