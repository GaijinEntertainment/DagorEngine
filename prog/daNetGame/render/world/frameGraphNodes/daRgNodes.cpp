// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <render/rendererFeatures.h>
#include <render/world/defaultVrsSettings.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <daRg/dag_panelRenderer.h>
#include <3d/dag_render.h>
#include <ui/uiRender.h>

#include "frameGraphNodes.h"


dabfg::NodeHandle makeOpaqueInWorldPanelsNode()
{
  auto decoNs = dabfg::root() / "opaque" / "decorations";

  return decoNs.registerNode("darg_in_world_panels_opaq_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto state = registry.requestState().allowWireframe().setFrameBlock("global_frame");

    auto pass = render_to_gbuffer(registry);

    use_default_vrs(pass, state);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [cameraHndl] {
      auto uiScenes = uirender::get_all_scenes();
      for (darg::IGuiScene *scn : uiScenes)
        darg_panel_renderer::render_panels_in_world(*scn, cameraHndl.ref().viewItm.getcol(3),
          darg_panel_renderer::RenderPass::GBuffer);
    };
  });
}

dabfg::NodeHandle makeTranslucentInWorldPanelsNode()
{
  return dabfg::register_node("darg_in_world_panels_trans_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    if (renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
      registry.rename("target_after_transparent_scene_late", "target_after_darg_in_world_panels_trans", dabfg::History::No)
        .texture()
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT);
    registry.requestState().setFrameBlock("global_frame");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [cameraHndl] {
      auto uiScenes = uirender::get_all_scenes();
      for (darg::IGuiScene *scn : uiScenes)
        darg_panel_renderer::render_panels_in_world(*scn, cameraHndl.ref().viewItm.getcol(3),
          darg_panel_renderer::RenderPass::Translucent);
    };
  });
}
