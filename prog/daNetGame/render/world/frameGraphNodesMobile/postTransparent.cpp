// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include "nodes.h"

#include <render/world/global_vars.h>
#include <render/world/private_worldRenderer.h>
#include <render/world/postfxRender.h>
#include <render/world/dynModelRenderer.h>
#include <render/world/aimRender.h>

#include <render/debugGbuffer.h>
#include <render/deferredRenderer.h>
#include <render/dynamicQuality.h>
#include <render/dynamicResolution.h>
#include <render/skies.h>

#include <perfMon/dag_statDrv.h>
#include <scene/dag_occlusion.h>
#include <shaders/dag_shaderBlock.h>
#include <util/dag_convar.h>
#include <render/vertexDensityOverlay.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>

extern void wait_occlusion_async_readback_gpu();
extern ConVarT<bool, false> use_occlusion;
extern int show_shadows;
extern bool grs_draw_wire;

dafg::NodeHandle mk_occlusion_preparing_mobile_node()
{
  return dafg::register_node("occlusion_preparing_mobile", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    const auto depthHndl =
      registry.readTexture("depth_for_transparent_effects").atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    registry.requestState().allowWireframe();

    const auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

    dafg::Texture2dCreateInfo cInfo;
    cInfo.creationFlags = TEXFMT_R32F | TEXCF_RTARGET | TEXCF_UNORDERED;
    cInfo.mipLevels = wr.downsampledTexturesMipCount;
    cInfo.resolution = registry.getResolution<2>("main_view", 0.5);
    const auto tempDepthHndl = registry.createTexture2d("temporary_depth_for_occlusion", dafg::History::No, cInfo)
                                 .atStage(dafg::Stage::PS)
                                 .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                 .handle();

    const auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    return [depthHndl, tempDepthHndl, cameraHndl]() {
      Occlusion *occlusion = cameraHndl.ref().jobsMgr->getOcclusion();
      if (occlusion && use_occlusion.get())
      {
        wait_occlusion_async_readback_gpu();

        // NOTE: it is EXTREMELY important that target depth is present here.
        // in this case, the occlusion system uses it, downsamples it into
        // the temporary texture a couple of times and then generates a
        // "final" downsampled depth thing.
        // Otherwise (i.e. if target depth is not present), the occlusion
        // system expects the `depth` argument to contain far_downsampled_depth,
        // not just an empty temporary texture!!!
        occlusion->prepareNextFrame(cameraHndl.ref().noJitterPersp.zn, cameraHndl.ref().noJitterPersp.zf,
          tempDepthHndl.view().getTex2D(), depthHndl.view().getTex2D());
      }
    };
  });
}

dafg::NodeHandle mk_postfx_target_producer_mobile_node()
{
  return dafg::register_node("postfx_target_producer_mobile", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("occlusion_preparing_mobile");

    registry.renameTexture("target_after_darg_in_world_panels_trans", "postfx_input", dafg::History::No);
    registry.registerTexture("final_target", [](const dafg::multiplexing::Index) -> ManagedTexView {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      return *wr.finalTargetFrame;
    });

    return []() {};
  });
}

dafg::NodeHandle mk_postfx_mobile_node()
{
  return dafg::register_node("postfx_mobile", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readTexture("postfx_input").atStage(dafg::Stage::PS).bindToShaderVar("frame_tex");

    postfx_bind_additional_textures_from_registry(registry);

    const auto rtHndl = registry.modifyTexture("final_target").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();

    registry.requestState().allowWireframe().setFrameBlock("");

    return [rtHndl, postfx = PostFxRenderer("forward_postfx")] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      if (wr.rtControl == WorldRenderer::RtControl::BACKBUFFER)
        d3d::set_srgb_backbuffer_write(false);

      if (wr.hasFeature(FeatureRenderFlags::POSTFX))
      {
        TIME_D3D_PROFILE(forward_postfx);
        d3d::set_render_target(0, rtHndl.view().getTex2D(), 0);
        postfx.render();

        VertexDensityOverlay::draw_heatmap();
      }
    };
  });
}


dafg::NodeHandle mk_finalize_frame_mobile_node()
{
  return dafg::register_node("finalize_frame_mobile", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.renameTexture("final_target", "final_dbg_target", dafg::History::No)
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::COLOR_ATTACHMENT);

    return []() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      wr.copyClipmapUAVFeedback();

      if (auto *skies = get_daskies())
        skies->unsetSkyLightParams();
      if (wr.dynamicQuality)
        wr.dynamicQuality->onFrameEnd();
      if (wr.dynamicResolution)
        wr.dynamicResolution->endFrame();
    };
  });
}


dafg::NodeHandle mk_debug_render_mobile_node()
{
  return dafg::register_node("debug_render_mobile", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    const auto rtHndl =
      registry.modifyTexture("final_dbg_target").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();

    const auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    return [rtHndl, cameraHndl]() {
      extern void render_scene_debug(BaseTexture * target, BaseTexture * depth, const CameraParams &camera);
      render_scene_debug(rtHndl.view().getTex2D(), nullptr, cameraHndl.ref());
    };
  });
}
