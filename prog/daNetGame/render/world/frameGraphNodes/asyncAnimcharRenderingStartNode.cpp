// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/render/updateStageRender.h>
#include <render/daBfg/bfg.h>
#include <util/dag_convar.h>
#include <render/rendererFeatures.h>
#include <render/world/frameGraphHelpers.h>

#include "render/world/cameraParams.h"

extern ConVarT<bool, false> async_animchars_main;

void start_async_animchar_main_render(const Frustum &fr, uint32_t hints, TexStreamingContext texCtx);

dabfg::NodeHandle makeAsyncAnimcharRenderingStartNode(bool has_motion_vectors)
{
  return dabfg::register_node("async_animchar_rendering_start_node", DABFG_PP_NODE_SRC,
    [has_motion_vectors](dabfg::Registry registry) {
      registry.setPriority(dabfg::PRIO_AS_EARLY_AS_POSSIBLE);

      // The job we launch here touches global state of dynmodels, so
      // running it concurrently with the hiding node would cause data races.
      // Another optional token for closeups was added for similar reasons.
      if (!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
      {
        registry.read("hidden_animchar_nodes_token").blob<OrderingToken>();
        (registry.root() / "opaque" / "closeups").read("dynmodel_global_state_access_optional_token").blob<OrderingToken>().optional();
      }

      registry.create("async_animchar_rendering_started_token", dabfg::History::No).blob<OrderingToken>();

      auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
      auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
      return [cameraHndl, strmCtxHndl, has_motion_vectors] {
        G_ASSERT(async_animchars_main.get());

        uint8_t mainHints =
          UpdateStageInfoRender::RENDER_COLOR | UpdateStageInfoRender::RENDER_DEPTH | UpdateStageInfoRender::RENDER_MAIN;
        if (has_motion_vectors)
          mainHints |= UpdateStageInfoRender::RENDER_MOTION_VECS;

        start_async_animchar_main_render(cameraHndl.ref().noJitterFrustum, mainHints, strmCtxHndl.ref());
      };
    });
}
