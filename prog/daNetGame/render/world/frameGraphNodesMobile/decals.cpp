// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodes.h"

#include <render/world/gbufferConsts.h>
#include <render/world/cameraParams.h>
#include <render/world/cameraViewVisibilityManager.h>

#include <daRg/dag_panelRenderer.h>
#include <render/daFrameGraph/daFG.h>
#include <render/renderEvent.h>
#include <daECS/core/entityManager.h>

#include <ui/uiRender.h>
#include <drv/3d/dag_renderPass.h>

dafg::NodeHandle mk_decals_mobile_node()
{
  return dafg::register_node("decals_mobile", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto modifyGbuf = [&registry](const char *tex_name) {
      registry.modify(tex_name).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT);
    };
    auto readGbuf = [&registry](const char *tex_name) {
      registry.read(tex_name).texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::UNKNOWN);
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

    registry.rename("gbuf_depth_for_decals", "gbuf_depth_for_resolve", dafg::History::No)
      .texture()
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE);

    registry.requestState().setFrameBlock("global_frame").allowWireframe();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto cameraHndlHistory = registry.readBlobHistory<CameraParams>("current_camera").handle();

    auto texCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [cameraHndl, cameraHndlHistory, texCtxHndl] {
      d3d::next_subpass();

      const auto &camera = cameraHndl.ref();
      const auto &prevCamera = cameraHndlHistory.ref();

      if (renderer_has_feature(FeatureRenderFlags::DECALS))
      {
        {
          TIME_D3D_PROFILE(decals_static);
          g_entity_mgr->broadcastEventImmediate(OnRenderDecals(camera.viewTm, camera.viewItm, camera.cameraWorldPos, texCtxHndl.ref(),
            camera.jobsMgr->getRiMainVisibility()));
        }
        {
          TIME_D3D_PROFILE(decals_dynamic);
          g_entity_mgr->broadcastEventImmediate(RenderDecalsOnDynamic(camera.viewTm, camera.cameraWorldPos, camera.jitterFrustum,
            camera.jobsMgr->getOcclusion(), texCtxHndl.ref()));
        }
      }

      TMatrix4_vec4 prevProjCurrentJitter = prevCamera.noJitterProjTm;
      matrix_perspective_add_jitter(prevProjCurrentJitter, camera.jitterPersp.ox, camera.jitterPersp.oy);

      auto uiScenes = uirender::get_all_scenes();
      for (darg::IGuiScene *scn : uiScenes)
        darg_panel_renderer::render_panels_in_world(*scn, darg_panel_renderer::RenderPass::GBuffer, camera.viewItm.getcol(3),
          camera.viewTm, &prevCamera.viewTm, &prevProjCurrentJitter);
    };
  });
}
