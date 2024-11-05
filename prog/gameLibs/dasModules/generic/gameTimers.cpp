// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daGame/timers.h>
#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>

#include "dasModules/aotGameTimers.h"

namespace bind_dascript
{
class GameTimersModule final : public das::Module
{
public:
  GameTimersModule() : das::Module("GameTimers")
  {
    das::ModuleLibrary lib(this);

    das::addConstant(*this, "INVALID_TIMER_HANDLE", game::INVALID_TIMER_HANDLE);

    das::addExtern<DAS_BIND_FUN(bind_dascript::game_timer_set)>(*this, lib, "game_timer_set", das::SideEffects::modifyExternal,
      "bind_dascript::game_timer_set");
    das::addExtern<DAS_BIND_FUN(bind_dascript::game_timer_clear)>(*this, lib, "game_timer_clear",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::game_timer_clear");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/aotGameTimers.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(GameTimersModule, bind_dascript);
