// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>

#include <render/renderEvent.h>
#include "render/renderLibsAllowed.h"
#include <ecs/render/updateStageRender.h>

#include "puddleQueryManager.h"

ECS_TAG(render)
ECS_ON_EVENT(OnLevelLoaded)
static void puddle_query_on_level_loaded_es(const ecs::Event &)
{
  if (is_render_lib_allowed("puddle_query"))
    init_puddle_query_mgr();
}

ECS_TAG(render)
ECS_ON_EVENT(UnloadLevel)
static void puddle_query_on_level_unloaded_es(const ecs::Event &) { close_puddle_query_mgr(); }

ECS_TAG(render)
ECS_ON_EVENT(UpdateStageInfoBeforeRender)
static void puddle_query_update_es(const ecs::Event &)
{
  if (get_puddle_query_mgr())
  {
    TIME_D3D_PROFILE(puddle_query_update)
    get_puddle_query_mgr()->update();
  }
}
