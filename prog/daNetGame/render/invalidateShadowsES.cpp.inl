// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/updateStage.h>
#include <3d/dag_texMgrTags.h>

#include "main/gameLoad.h"
#include <daECS/core/coreEvents.h>
#include <ecs/rendInst/riExtra.h>
#include <gamePhys/phys/rendinstDestr.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstAccess.h>

#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"
#include <ecs/render/updateStageRender.h>

ECS_TAG(render)
void invalidate_ri_shadows_es(const UpdateStageInfoBeforeRender &, int &ri_texture_gen)
{
  // this should not be called at all in game. It is just for menu!
  if (get_world_renderer() && ri_texture_gen != interlocked_relaxed_load(textag_get_info(TEXTAG_RENDINST).loadGeneration))
  {
    ri_texture_gen = interlocked_acquire_load(textag_get_info(TEXTAG_RENDINST).loadGeneration);
    ((WorldRenderer *)get_world_renderer())->invalidateAllShadows();
  }
}

ECS_TAG(render)
ECS_BEFORE(ri_extra_destroyed_es)
ECS_REQUIRE(ecs::Tag isRendinstDestr)
ECS_ON_EVENT(on_disappear)
void invalidate_shadows_es_event_handler(const ecs::Event &, const RiExtraComponent &ri_extra, const TMatrix &transform)
{
  if (!sceneload::unload_in_progress && rendinst::isRiGenExtraValid(ri_extra.handle))
  {
    BBox3 bbox = rendinst::getRIGenBBox(rendinst::RendInstDesc(ri_extra.handle));
    bbox = BBox3(bbox.center() * transform, bbox.width().length());
    ((WorldRenderer *)get_world_renderer())->shadowsInvalidate(bbox);
  }
}
