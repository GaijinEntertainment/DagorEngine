// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <ecs/render/updateStageRender.h>
#include <render/daFrameGraph/daFG.h>
#include <util/dag_convar.h>
#include <render/rendererFeatures.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/cameraViewVisibilityManager.h>

#include "render/world/cameraParams.h"

extern ConVarT<bool, false> async_animchars_main;

dafg::NodeHandle makeAsyncAnimcharRenderingStartNode(bool has_motion_vectors)
{
  return dafg::register_node("async_animchar_rendering_start_node", DAFG_PP_NODE_SRC, [has_motion_vectors](dafg::Registry registry) {
    registry.setPriority(dafg::PRIO_AS_EARLY_AS_POSSIBLE);

    registry.read("prepare_hero_cockpit_token").blob<OrderingToken>().optional();

    // The job we launch here touches global state of dynmodels, so
    // running it concurrently with the hiding node would cause data races.
    // Another optional token for closeups was added for similar reasons.
    registry.read("hidden_animchar_nodes_token").blob<OrderingToken>();
    (registry.root() / "opaque" / "closeups").read("dynmodel_global_state_access_optional_token").blob<OrderingToken>().optional();


    registry.create("async_animchar_rendering_started_token").blob<OrderingToken>();

    auto cameraHndl = use_camera_in_camera(registry).handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [cameraHndl, strmCtxHndl, has_motion_vectors] {
      G_ASSERT(async_animchars_main.get());

      uint8_t mainHints =
        UpdateStageInfoRender::RENDER_COLOR | UpdateStageInfoRender::RENDER_DEPTH | UpdateStageInfoRender::RENDER_MAIN;
      if (has_motion_vectors)
        mainHints |= UpdateStageInfoRender::RENDER_MOTION_VECS;

      cameraHndl.ref().jobsMgr->startAsyncAnimcharMainRender(cameraHndl.ref().noJitterFrustum, mainHints, strmCtxHndl.ref());
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_async_animchar_rendering_start_nodes_es(const OnCameraNodeConstruction &evt)
{
  if (async_animchars_main.get())
    evt.nodes->push_back(makeAsyncAnimcharRenderingStartNode(evt.hasMotionVectors));
}
