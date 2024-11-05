// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <main/level.h>
#include <landMesh/lmeshManager.h>
#include <render/renderEvent.h>
#include <ecs/core/entityManager.h>
#include <ecs/render/updateStageRender.h>
#include <daECS/core/coreEvents.h>
#include "puddlesManager.h"
#include "puddlesManagerEvents.h"


ECS_DECLARE_RELOCATABLE_TYPE(PuddlesManager)
ECS_REGISTER_RELOCATABLE_TYPE(PuddlesManager, nullptr)

ECS_REGISTER_EVENT(RemovePuddlesInRadius)

ECS_TAG(render)
static void init_puddles_manager_es(const OnLevelLoaded &evt, PuddlesManager &puddles__manager, float puddles__bare_minimum_dist)
{
  puddles__manager.init(get_landmesh_manager(), evt.level_blk,
    renderer_has_feature(FeatureRenderFlags::HIGHRES_PUDDLES) ? -1 : puddles__bare_minimum_dist);
  puddles__manager.setPuddlesScene(get_rivers());
}

ECS_TAG(render)
static void reinit_puddles_es(const ChangeRenderFeatures &evt, PuddlesManager &puddles__manager, float puddles__bare_minimum_dist)
{
  puddles__manager.reinit_same_settings(get_landmesh_manager(),
    evt.hasFeature(FeatureRenderFlags::HIGHRES_PUDDLES) ? -1 : puddles__bare_minimum_dist);
}

ECS_TAG(render)
static void prepare_puddles_es(const BeforeDraw &evt, PuddlesManager &puddles__manager)
{
  puddles__manager.preparePuddles(evt.camPos);
}

ECS_TAG(render)
static void after_device_reset_puddles_es(const AfterDeviceReset &, PuddlesManager &puddles__manager)
{
  puddles__manager.puddlesAfterDeviceReset();
}

ECS_TAG(render)
static void remove_puddles_in_crater_es(const RemovePuddlesInRadius &evt, PuddlesManager &puddles__manager)
{
  puddles__manager.removePuddlesInCrater(evt.position, evt.radius);
}

ECS_TAG(render)
static void invalidate_puddles_es(const AfterHeightmapChange &, PuddlesManager &puddles__manager)
{
  puddles__manager.invalidatePuddles(false);
}

ECS_TAG(render)
static void unload_puddles_es(const UnloadLevel &, PuddlesManager &puddles__manager)
{
  puddles__manager.close();
  puddles__manager.setPuddlesScene(NULL);
}