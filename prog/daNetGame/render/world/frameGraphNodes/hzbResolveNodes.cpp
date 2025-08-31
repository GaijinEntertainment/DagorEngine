// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/renderEvent.h>
#include <render/deferredRenderer.h>
#include <scene/dag_occlusion.h>
#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <daECS/core/entityManager.h>

#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <render/daFrameGraph/daFG.h>
#include "frameGraphNodes.h"
#include <drv/3d/dag_renderTarget.h>
#include <render/world/dynModelRenderPass.h>

extern void wait_occlusion_async_readback_gpu();

// TODO: refactor this somehow?
bool need_hero_cockpit_flag_in_prepass = false;

namespace var
{
static ShaderVariableInfo occlusion_lens_mask{"occlusion_lens_mask", false};
static ShaderVariableInfo occlusion_lens_mask_mode{"occlusion_lens_mask_mode", false};
} // namespace var

static void set_scope_lens_mask(const auto &mask_hndl, const dafg::multiplexing::Index &index)
{
  const bool useScopeLensMask = camera_in_camera::is_lens_render_active() && mask_hndl.get();
  ShaderGlobal::set_texture(var::occlusion_lens_mask.get_var_id(), useScopeLensMask ? mask_hndl.view().getTexId() : BAD_TEXTUREID);
  ShaderGlobal::set_int(var::occlusion_lens_mask_mode, index.subCamera);
}

dafg::NodeHandle makeHzbResolveNode(bool beforeDynamics)
{
  need_hero_cockpit_flag_in_prepass = beforeDynamics;
  if (beforeDynamics)
  {
    auto ns = dafg::root() / "opaque" / "statics";
    return ns.registerNode("hzb_resolve_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      auto depthForOcclusionHndl =
        registry.create("depth_for_occlusion", dafg::History::No)
          .texture({TEXFMT_R32F | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("main_view", 0.5f), dafg::AUTO_MIP_COUNT})
          .atStage(dafg::Stage::PS)
          .useAs(dafg::Usage::COLOR_ATTACHMENT)
          .handle();

      auto depthHndl = registry.read("gbuf_depth").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

      registry.read("gbuf_2").texture().atStage(dafg::Stage::PS).bindToShaderVar("material_gbuf").optional();

      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("gbuf_sampler", dafg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
        .bindToShaderVar("material_gbuf_samplerstate");

      auto cameraHndl = read_camera_in_camera(registry).handle();
      registry.requestState().setFrameBlock("global_frame");

      auto closeupsNs = registry.root() / "opaque" / "closeups";
      auto lensMaskHndl =
        closeupsNs.readTexture("scope_lens_hzb_mask").optional().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

      registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);

      return [depthForOcclusionHndl, depthHndl, cameraHndl, lensMaskHndl,
               occlusionExclusionMaskVarId = get_shader_variable_id("occlusionExclusionMask")](
               const dafg::multiplexing::Index &multiplexing_index) {
        Occlusion *occlusion = cameraHndl.ref().jobsMgr->getOcclusion();
        if (!occlusion)
          return;

        wait_occlusion_async_readback_gpu();

        // We need to set this variable here because the downsample shader branches on it.
        // See the other branch (beforeDynamics == false) for comparison.
        ShaderGlobal::set_texture(occlusionExclusionMaskVarId, BAD_TEXTUREID);

        set_scope_lens_mask(lensMaskHndl, multiplexing_index);

        const auto &persp = cameraHndl.ref().jitterPersp;

        occlusion->prepareNextFrame(persp.zn, persp.zf, depthForOcclusionHndl.view().getTex2D(), depthHndl.view().getTex2D());
      };
    });
  }

  return dafg::register_node("hzb_resolve_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("downsample_depth_node");
    registry.orderMeBefore("prepare_lights_node");

    auto occlusionExclusionMaskHndl = registry.create("occlusionExclusionMask", dafg::History::No)
                                        .texture({TEXCF_RTARGET | TEXFMT_DEPTH16, IPoint2(256, 128)})
                                        .atStage(dafg::Stage::PS)
                                        .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                                        .handle();

    auto downsampledDepthHndl = registry.read("far_downsampled_depth")
                                  .texture()
                                  .atStage(dafg::Stage::PS_OR_CS)
                                  .bindToShaderVar("downsampled_far_depth_tex")
                                  .handle();

    registry.read("gbuf_2").texture().atStage(dafg::Stage::PS).bindToShaderVar("material_gbuf").optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("material_gbuf_samplerstate");

    auto cameraHndl = read_camera_in_camera(registry).handle();
    registry.requestState().setFrameBlock("global_frame");

    auto closeupsNs = registry.root() / "opaque" / "closeups";
    auto lensMaskHndl =
      closeupsNs.readTexture("scope_lens_hzb_mask").optional().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);

    return [occlusionExclusionMaskHndl, cameraHndl, downsampledDepthHndl, lensMaskHndl,
             dynModelRenderPassVarId = get_shader_variable_id("dyn_model_render_pass"),
             occlusionExclusionMaskVarId = get_shader_variable_id("occlusionExclusionMask")](
             const dafg::multiplexing::Index &multiplexing_index) {
      Occlusion *occlusion = cameraHndl.ref().jobsMgr->getOcclusion();
      if (!occlusion)
        return;

      wait_occlusion_async_readback_gpu();

      const auto &camera = cameraHndl.ref();

      set_scope_lens_mask(lensMaskHndl, multiplexing_index);

      {
        TIME_D3D_PROFILE(occlusion_exclusion);
        ShaderGlobal::set_int(dynModelRenderPassVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));
        SCOPE_RENDER_TARGET;
        d3d::set_render_target(nullptr, 0);
        d3d::set_depth(occlusionExclusionMaskHndl.view().getTex2D(), DepthAccess::RW);
        OcclusionExclusion evt(camera.viewTm);
        ::g_entity_mgr->broadcastEventImmediate(evt);
        ShaderGlobal::set_texture(occlusionExclusionMaskVarId,
          evt.rendered ? occlusionExclusionMaskHndl.view().getTexId() : BAD_TEXTUREID);
        ShaderGlobal::set_int(dynModelRenderPassVarId, 0);
      }

      occlusion->prepareNextFrame(camera.jitterPersp.zn, camera.jitterPersp.zf, downsampledDepthHndl.view().getTex2D(), nullptr);
    };
  });
}
