// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/rendererFeatures.h>
#include <render/world/defaultVrsSettings.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <daRg/dag_panelRenderer.h>
#include <3d/dag_render.h>
#include <ui/uiRender.h>

#include "frameGraphNodes.h"


dafg::NodeHandle makeOpaqueInWorldPanelsNode()
{
  auto decoNs = dafg::root() / "opaque" / "decorations";

  return decoNs.registerNode("darg_in_world_panels_opaq_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    render_to_gbuffer(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto cameraHndlHistory = registry.readBlobHistory<CameraParams>("current_camera").handle();
    return [cameraHndl, cameraHndlHistory] {
      auto uiScenes = uirender::get_all_scenes();

      const auto &camera = cameraHndl.ref();
      const auto &prevCamera = cameraHndlHistory.ref();

      TMatrix4_vec4 prevProjCurrentJitter = prevCamera.noJitterProjTm;
      matrix_perspective_add_jitter(prevProjCurrentJitter, camera.jitterPersp.ox, camera.jitterPersp.oy);

      for (darg::IGuiScene *scn : uiScenes)
        darg_panel_renderer::render_panels_in_world(*scn, darg_panel_renderer::RenderPass::GBuffer, camera.cameraWorldPos,
          camera.viewTm, &prevCamera.viewTm, &prevProjCurrentJitter);
    };
  });
}

dafg::NodeHandle makeTranslucentInWorldPanelsNode()
{
  return dafg::register_node("darg_in_world_panels_trans_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    if (renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
      registry.rename("target_after_transparent_scene_late", "target_after_darg_in_world_panels_trans", dafg::History::No)
        .texture()
        .atStage(dafg::Stage::PS)
        .useAs(dafg::Usage::COLOR_ATTACHMENT);
    registry.requestState().setFrameBlock("global_frame");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [cameraHndl] {
      auto uiScenes = uirender::get_all_scenes();
      for (darg::IGuiScene *scn : uiScenes)
        darg_panel_renderer::render_panels_in_world(*scn, darg_panel_renderer::RenderPass::Translucent,
          cameraHndl.ref().viewItm.getcol(3), cameraHndl.ref().viewTm);
    };
  });
}
