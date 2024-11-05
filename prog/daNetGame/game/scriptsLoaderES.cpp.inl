// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <ecs/scripts/scripts.h>
#include <ecs/scripts/dasEs.h>

#include <util/dag_delayedAction.h>

#include "main/level.h"
#include "game/gameScripts.h"
#include "game/scriptsLoader.h"
#include "net/time.h"
#include "main/gameLoad.h"

ECS_REGISTER_EVENT(EventGameModeLoaded);

struct LoadGameModeScriptsAction final : public DelayedAction
{
  ecs::string dasScript;

  LoadGameModeScriptsAction(ecs::string das_script) : dasScript(eastl::move(das_script)) {}

  void performAction() override
  {
    start_es_loading(); // prevents calls to es_reset_order during es registrations

    gamescripts::run_game_mode_entry_scripts(dasScript.c_str());

    end_es_loading(); // call es_reset_order during es registrations

    g_entity_mgr->broadcastEvent(EventGameModeLoaded());
  }
};

ECS_ON_EVENT(on_appear)
void game_mode_scripts_loader_es_event_handler(const ecs::Event &, ecs::string &game_mode_loader__dasScript)
{
  if (sceneload::is_user_game_mod())
  {
    logerr("Can't use script in ugm mode!");
    return;
  }

  add_delayed_action(new LoadGameModeScriptsAction(game_mode_loader__dasScript));
}

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::string game_mode_loader__dasScript)
void game_mode_scripts_unloader_es_event_handler(const ecs::Event &)
{
  if (sceneload::is_user_game_mod())
  {
    logerr("Can't use script in ugm mode!");
    return;
  }

  delayed_call([]() { gamescripts::unload_game_mode_scripts(); });
}
