// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodes.h"

#include <render/world/gbufferConsts.h>
#include <render/world/cameraParams.h>

#include <daRg/dag_panelRenderer.h>
#include <render/daBfg/bfg.h>
#include <render/renderEvent.h>
#include <daECS/core/entityManager.h>

#include <ui/uiRender.h>
#include <drv/3d/dag_renderPass.h>

dabfg::NodeHandle mk_decals_mobile_node()
{
  return dabfg::register_node("decals_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto modifyGbuf = [&registry](const char *tex_name) {
      registry.modify(tex_name).texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::COLOR_ATTACHMENT);
    };
    auto readGbuf = [&registry](const char *tex_name) {
      registry.read(tex_name).texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::UNKNOWN);
    };
    if (renderer_has_feature(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS))
    {
      modifyGbuf(MOBILE_SIMPLIFIED_GBUFFER_RT_NAMES[0]);
      readGbuf(MOBILE_SIMPLIFIED_GBUFFER_RT_NAMES[1]);
    }
    else
    {
      modifyGbuf(MOBILE_GBUFFER_RT_NAMES[0]);
      modifyGbuf(MOBILE_GBUFFER_RT_NAMES[1]);
      readGbuf(MOBILE_GBUFFER_RT_NAMES[2]);
    }

    registry.rename("gbuf_depth_for_decals", "gbuf_depth_for_resolve", dabfg::History::No)
      .texture()
      .atStage(dabfg::Stage::PS)
      .useAs(dabfg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE);

    registry.requestState().setFrameBlock("global_frame").allowWireframe();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [cameraHndl] {
      d3d::next_subpass();

      if (renderer_has_feature(FeatureRenderFlags::DECALS))
      {
        {
          TIME_D3D_PROFILE(decals_static);
          g_entity_mgr->broadcastEventImmediate(
            OnRenderDecals(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm, cameraHndl.ref().cameraWorldPos));
        }
        {
          TIME_D3D_PROFILE(decals_dynamic);
          g_entity_mgr->broadcastEventImmediate(RenderDecalsOnDynamic());
        }
      }

      auto uiScenes = uirender::get_all_scenes();
      for (darg::IGuiScene *scn : uiScenes)
        darg_panel_renderer::render_panels_in_world(*scn, cameraHndl.ref().viewItm.getcol(3),
          darg_panel_renderer::RenderPass::GBuffer);
    };
  });
}
