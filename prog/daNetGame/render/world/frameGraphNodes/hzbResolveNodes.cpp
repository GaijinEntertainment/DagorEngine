// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/renderEvent.h>
#include <render/deferredRenderer.h>
#include <scene/dag_occlusion.h>
#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <daECS/core/entityManager.h>

#include <render/world/cameraParams.h>
#include <render/daBfg/bfg.h>
#include "frameGraphNodes.h"
#include <drv/3d/dag_renderTarget.h>

extern void wait_occlusion_async_readback_gpu();

// TODO: refactor this somehow?
bool need_hero_cockpit_flag_in_prepass = false;

dabfg::NodeHandle makeHzbResolveNode(bool beforeDynamics)
{
  need_hero_cockpit_flag_in_prepass = beforeDynamics;
  if (beforeDynamics)
  {
    auto ns = dabfg::root() / "opaque" / "statics";
    return ns.registerNode("hzb_resolve_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
      auto depthForOcclusionHndl = registry.create("depth_for_occlusion", dabfg::History::No)
                                     .texture({TEXFMT_R32F | TEXCF_RTARGET | TEXCF_UNORDERED,
                                       registry.getResolution<2>("main_view", 0.5f), dabfg::AUTO_MIP_COUNT})
                                     .atStage(dabfg::Stage::PS)
                                     .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                     .handle();

      auto depthHndl = registry.read("gbuf_depth").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

      registry.read("gbuf_2").texture().atStage(dabfg::Stage::PS).bindToShaderVar("material_gbuf").optional();

      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("gbuf_sampler", dabfg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
        .bindToShaderVar("material_gbuf_samplerstate");

      auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
      registry.requestState().setFrameBlock("global_frame");
      return [depthForOcclusionHndl, depthHndl, cameraHndl,
               occlusionExclusionMaskVarId = get_shader_variable_id("occlusionExclusionMask")] {
        if (!current_occlusion)
          return;

        wait_occlusion_async_readback_gpu();

        // We need to set this variable here because the downsample shader branches on it.
        // See the other branch (beforeDynamics == false) for comparison.
        ShaderGlobal::set_texture(occlusionExclusionMaskVarId, BAD_TEXTUREID);

        const auto &persp = cameraHndl.ref().jitterPersp;

        current_occlusion->prepareNextFrame(persp.zn, persp.zf, depthForOcclusionHndl.view().getTex2D(), depthHndl.view().getTex2D());
      };
    });
  }

  return dabfg::register_node("hzb_resolve_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("downsample_depth_node");
    registry.orderMeBefore("prepare_lights_node");

    auto occlusionExclusionMaskHndl = registry.create("occlusionExclusionMask", dabfg::History::No)
                                        .texture({TEXCF_RTARGET | TEXFMT_DEPTH16, IPoint2(256, 128)})
                                        .atStage(dabfg::Stage::PS)
                                        .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                                        .handle();

    auto downsampledDepthHndl = registry.read("far_downsampled_depth")
                                  .texture()
                                  .atStage(dabfg::Stage::PS_OR_CS)
                                  .bindToShaderVar("downsampled_far_depth_tex")
                                  .handle();

    registry.read("gbuf_2").texture().atStage(dabfg::Stage::PS).bindToShaderVar("material_gbuf").optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("material_gbuf_samplerstate");

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.requestState().setFrameBlock("global_frame");
    return [occlusionExclusionMaskHndl, cameraHndl, downsampledDepthHndl,
             dynModelRenderPassVarId = get_shader_variable_id("dyn_model_render_pass"),
             occlusionExclusionMaskVarId = get_shader_variable_id("occlusionExclusionMask")] {
      if (!current_occlusion)
        return;

      wait_occlusion_async_readback_gpu();

      const auto &camera = cameraHndl.ref();

      {
        TIME_D3D_PROFILE(occlusion_exclusion);
        ShaderGlobal::set_int(dynModelRenderPassVarId, 1);
        SCOPE_RENDER_TARGET;
        d3d::set_render_target(nullptr, 0);
        d3d::set_depth(occlusionExclusionMaskHndl.view().getTex2D(), DepthAccess::RW);
        OcclusionExclusion evt(camera.viewTm);
        ::g_entity_mgr->broadcastEventImmediate(evt);
        ShaderGlobal::set_texture(occlusionExclusionMaskVarId,
          evt.rendered ? occlusionExclusionMaskHndl.view().getTexId() : BAD_TEXTUREID);
        ShaderGlobal::set_int(dynModelRenderPassVarId, 0);
      }

      current_occlusion->prepareNextFrame(camera.jitterPersp.zn, camera.jitterPersp.zf, downsampledDepthHndl.view().getTex2D(),
        nullptr);
    };
  });
}
