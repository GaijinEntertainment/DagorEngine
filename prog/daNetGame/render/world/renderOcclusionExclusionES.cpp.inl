// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <daECS/core/entitySystem.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/slotAttach.h>
#include <shaders/dag_shaderBlock.h>

#include "global_vars.h"
#include "dynModelRenderer.h"
#include <ecs/anim/animchar_visbits.h>
#include "render/renderEvent.h"
#include <ecs/render/renderPasses.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>

template <typename Callable>
inline void render_hero_ecs_query(Callable c);
template <typename Callable>
inline void render_hero_vehicle_ecs_query(Callable c);
template <typename Callable>
inline void process_animchar_eid_ecs_query(ecs::EntityId, Callable c);

extern ShaderBlockIdHolder dynamicDepthSceneBlockId;

using namespace dynmodel_renderer;

static void process_animchar(
  DynModelRenderingState &state, DynamicRenderableSceneInstance *scene_instance, const ecs::Point4List *additional_data)
{
  state.process_animchar(ShaderMesh::STG_opaque, ShaderMesh::STG_opaque, scene_instance,
    animchar_additional_data::get_optional_data(additional_data), false);
}

static void process_animchar_eid(DynModelRenderingState &state, ecs::EntityId animchar_eid)
{
  process_animchar_eid_ecs_query(animchar_eid,
    [&](animchar_visbits_t animchar_visbits, AnimV20::AnimcharRendComponent &animchar_render, const ecs::Point4List *additional_data) {
      if (!(animchar_visbits & VISFLG_MAIN_AND_SHADOW_VISIBLE))
        return;
      process_animchar(state, animchar_render.getSceneInstance(), additional_data);
    });
}


static void render_occlusion_exclusion_es_event_handler(const OcclusionExclusion &stg)
{
  TIME_D3D_PROFILE(occlusion_exclusion_evt);

  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  render_hero_ecs_query(
    [&](ECS_REQUIRE(ecs::auto_type watchedByPlr) AnimV20::AnimcharRendComponent &animchar_render,
      const animchar_visbits_t &animchar_visbits, const ecs::EidList *attaches_list, const ecs::Point4List *additional_data) {
      // Check visibility
      if (!(animchar_visbits & VISFLG_MAIN_AND_SHADOW_VISIBLE))
        return;
      DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
      G_ASSERT_RETURN(scene != nullptr, );
      // Render
      process_animchar(state, scene, additional_data);
      if (attaches_list)
        for (ecs::EntityId attached_eid : *attaches_list)
          process_animchar_eid(state, attached_eid);
    });

  render_hero_vehicle_ecs_query([&](ECS_REQUIRE(ecs::auto_type vehicleWithWatched) AnimV20::AnimcharRendComponent &animchar_render,
                                  const animchar_visbits_t &animchar_visbits, const ecs::Point4List *additional_data) {
    // Check visibility
    if (!(animchar_visbits & VISFLG_MAIN_AND_SHADOW_VISIBLE))
      return;
    DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
    G_ASSERT_RETURN(scene != nullptr, );
    // Render
    process_animchar(state, scene, additional_data);
  });

  state.prepareForRender();
  const DynamicBufferHolder *buffer = state.requestBuffer(BufferType::OTHER);
  if (!buffer)
    return;

  if (!stg.rendered)
    d3d::clearview(CLEAR_ZBUFFER, 0, 0, 0);
  const_cast<bool &>(stg.rendered) = true;

  TMatrix vtm = stg.viewTm;
  vtm.setcol(3, 0, 0, 0);
  d3d::settm(TM_VIEW, vtm);

  state.setVars(buffer->buffer.getBufId());
  SCENE_LAYER_GUARD(dynamicDepthSceneBlockId.get());
  state.render(buffer->curOffset);
  d3d::settm(TM_VIEW, stg.viewTm);
}
