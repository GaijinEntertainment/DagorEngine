// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/daBfg/bfg.h>
#include <3d/dag_texStreamingContext.h>
#include <daECS/core/entityManager.h>

#include <render/renderEvent.h>
#include <render/world/wrDispatcher.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphNodes/prevFrameTexRequests.h>
#include <render/world/frameGraphHelpers.h>

dabfg::NodeHandle makeTransparentSceneLateNode()
{
  return dabfg::register_node("transparent_scene_late_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    if (!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
    {
      registry.requestRenderPass().color({"target_for_transparency"}).depthRo("depth_for_transparency");
      use_current_camera(registry);
    }
    else
      registry.requestRenderPass()
        .color({registry.rename("target_after_under_water_fog", "target_after_transparent_scene_late", dabfg::History::No).texture()})
        .depthRo("depth_for_transparent_effects");

    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    auto prevFrameTexHndl = read_prev_frame_tex(registry);
    auto currentTexCtxHndl = registry.read("tex_ctx").blob<TexStreamingContext>().handle();
    auto cameraHndl = registry.read("current_camera").blob<CameraParams>().handle();

    return [prevFrameTexHndl, currentTexCtxHndl, cameraHndl] {
      if (!g_entity_mgr)
        return;

      // Note: we use CSM inside this node and used to explicitly call
      // CascadeShadows::setCascadesToShader, but this node is always
      // run after some other type of transparent stuff, which already
      // calls that function, so it's probably ok to omit calling it here.

      auto prevFrameTex = prevFrameTexHndl.has_value() ? prevFrameTexHndl.value().view() : ManagedTexView{};
      g_entity_mgr->broadcastEventImmediate(
        RenderLateTransEvent(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos, currentTexCtxHndl.ref(), prevFrameTex));
    };
  });
}

extern void render_mainhero_trans(const TMatrix &view_tm);

dabfg::NodeHandle makeMainHeroTransNode()
{
  return dabfg::register_node("main_hero_trans", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("transparent_scene_late_node");
    if (!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
    {
      registry.requestRenderPass().color({"target_for_transparency"}).depthRo("depth_for_transparency");
      registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
      registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    }
    else
      registry.requestRenderPass().color({"target_after_transparent_scene_late"}).depthRo("depth_for_transparent_effects");
    registry.allowAsyncPipelines();

    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    return [cameraHndl] { render_mainhero_trans(cameraHndl.ref().viewTm); };
  });
}
