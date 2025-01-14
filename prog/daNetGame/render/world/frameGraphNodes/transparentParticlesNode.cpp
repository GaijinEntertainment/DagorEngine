// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/deferredRenderer.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>
#include <render/world/aimRender.h>

#include "render/fx/fx.h"
#include "render/fx/fxRenderTags.h"
#include "render/world/partitionSphere.h"
#include "render/world/frameGraphHelpers.h"

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include <drv/3d/dag_renderTarget.h>

dabfg::NodeHandle makeTransparentParticlesPartitionNode()
{
  auto nodeNs = dabfg::root() / "transparent";
  return nodeNs.registerNode("particles_partition_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.executionHas(dabfg::SideEffects::External);
    registry.setPriority(dabfg::PRIO_AS_LATE_AS_POSSIBLE);
    registry.read("acesfx_update_token").blob<OrderingToken>();

    auto cullingIdHndl = registry.createBlob<dafx::CullingId>("early_particles_cullingId", dabfg::History::No).handle();

    return [cullingIdHndl, cullingIdWrapper{acesfx::UniqueCullingId(dafx::create_proxy_culling_state(acesfx::get_dafx_context()))}] {
      G_ASSERT(cullingIdWrapper.get());

      const eastl::vector<eastl::string> tags = {
        render_tags[ERT_TAG_LOWRES], render_tags[ERT_TAG_HIGHRES], render_tags[ERT_TAG_ATEST]};

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      PartitionSphere partitionSphere = wr.getTransparentPartitionSphere();
      if (partitionSphere.status == PartitionSphere::Status::CAMERA_INSIDE_SPHERE)
        dafx::partition_workers_if_outside_sphere(acesfx::get_dafx_context(), acesfx::get_cull_id(), cullingIdWrapper.get(), tags,
          partitionSphere.sphere);
      else if (partitionSphere.status == PartitionSphere::Status::CAMERA_OUTSIDE_SPHERE)
        dafx::partition_workers_if_inside_sphere(acesfx::get_dafx_context(), acesfx::get_cull_id(), cullingIdWrapper.get(), tags,
          partitionSphere.sphere);
      else
        return;

      cullingIdHndl.ref() = cullingIdWrapper.get();
    };
  });
}

static void use_particles_render(dabfg::Registry registry)
{
  registry.read("fom_shadows_sin").texture().atStage(dabfg::Stage::PRE_RASTER).bindToShaderVar("fom_shadows_sin").optional();
  registry.read("fom_shadows_cos").texture().atStage(dabfg::Stage::PRE_RASTER).bindToShaderVar("fom_shadows_cos").optional();
  registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();
  registry.requestState().setFrameBlock("global_frame");
}

dabfg::NodeHandle makeTransparentParticlesLowresPrepareNode(bool is_early)
{
  // NOTE: If there are no effects on screen or if they take too little time to render
  // then better to use fullres depth from `wr.target`. It already has rendered water
  // and this way we can save time by avoiding one downsample. We also can avoid downsample
  // if there is no water on the screen.

  auto nodeNs = dabfg::root() / "transparent" / (is_early ? "far" : "close");
  return nodeNs.registerNode("particles_lowres_prepare_node", DABFG_PP_NODE_SRC, [is_early](dabfg::Registry registry) {
    registry.allowAsyncPipelines();
    registry.read("acesfx_update_token").blob<OrderingToken>();

    auto lowResFxTexHndl = registry.create("low_res_fx_rt", dabfg::History::No)
                             .texture({TEXFMT_A16B16G16R16F | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f)})
                             .atStage(dabfg::Stage::PS)
                             .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                             .handle();

    registry.readBlob<dafx::CullingId>("early_particles_cullingId").optional();
    auto partitionCullingIdOptHndl =
      is_early ? eastl::make_optional(registry.readBlob<dafx::CullingId>("early_particles_cullingId").handle()) : eastl::nullopt;

    // TODO: this request should depend on water rendering mode, but now we don't recompile FG underwater.
    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    bool upscaleTexAvailable = wr.hasFeature(FeatureRenderFlags::UPSCALE_SAMPLING_TEX);
    auto depthWithWaterHndl = registry.read(upscaleTexAvailable ? "checkerboard_depth_with_water" : "far_downsampled_depth_with_water")
                                .texture()
                                .atStage(dabfg::Stage::PS)
                                .useAs(dabfg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
                                .handle();

    auto depthThermalOptHndl = registry.read("thermal_downsampled_depth_for_fx")
                                 .texture()
                                 .atStage(dabfg::Stage::PS)
                                 .useAs(dabfg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
                                 .optional()
                                 .handle();

    auto downsampledDepthHndl = registry.read("downsampled_depth_with_early_after_envi_water")
                                  .texture()
                                  .atStage(dabfg::Stage::PS)
                                  .useAs(dabfg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
                                  .handle();

    auto checkerboardDepthHndl =
      registry.read("checkerboard_depth").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).optional().handle();

    auto waterRenderModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();

    use_particles_render(registry);
    registry.create("effects_depth_sampler", dabfg::History::No)
      .blob<d3d::SamplerHandle>(d3d::request_sampler({}))
      .bindToShaderVar("effects_depth_tex_samplerstate");

    return [lowResFxTexHndl, depthWithWaterHndl, depthThermalOptHndl, downsampledDepthHndl, checkerboardDepthHndl, waterRenderModeHndl,
             partitionCullingIdOptHndl, effectsDepthTexVarId{::get_shader_glob_var_id("effects_depth_tex")}] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const bool thermalVisionOn = acesfx::thermal_vision_on();

      ManagedTexView depthAttachment;
      ManagedTexView depthShaderResource;
      if (waterRenderModeHndl.ref() != WaterRenderMode::LATE)
      {
        ManagedTexView downsampledDepthWithWaterTex;
        if (!thermalVisionOn)
          downsampledDepthWithWaterTex = depthWithWaterHndl.view();
        else
          downsampledDepthWithWaterTex = depthThermalOptHndl.view();

        // Uses checkerboard_depth_with_water if available, far_downsampled_depth_with_water otherwise
        depthAttachment = downsampledDepthWithWaterTex;
        depthShaderResource = downsampledDepthWithWaterTex;
      }
      else if (wr.hasFeature(FeatureRenderFlags::UPSCALE_SAMPLING_TEX))
      {
        // Cannot bind `checkerboardDepth` due to it's format but depth masking hides most false positives.
        depthAttachment = downsampledDepthHndl.view();
        depthShaderResource = checkerboardDepthHndl.view();
      }
      else
      {
        depthAttachment = downsampledDepthHndl.view();
        depthShaderResource = downsampledDepthHndl.view();
      }

      ManagedTexView lowResFxTex = lowResFxTexHndl.view();
      d3d::set_render_target({depthAttachment.getBaseTex(), 0}, DepthAccess::SampledRO, {{lowResFxTex.getBaseTex(), 0}});
      d3d::clearview(CLEAR_TARGET, 0xFF000000, 0, 0);

      ShaderGlobal::set_texture(effectsDepthTexVarId, depthShaderResource);
      acesfx::setDepthTex(depthShaderResource.getTex2D());

      dafx::CullingId cullingId = partitionCullingIdOptHndl ? partitionCullingIdOptHndl->ref() : dafx::CullingId();
      if (!thermalVisionOn)
        acesfx::renderTransLowRes(cullingId);
      else
        acesfx::renderTransThermal(cullingId);
    };
  });
}

dabfg::NodeHandle makeTransparentParticlesNode(bool is_early)
{
  auto nodeNs = dabfg::root() / "transparent" / (is_early ? "far" : "close");
  return nodeNs.registerNode("particles_node", DABFG_PP_NODE_SRC, [is_early](dabfg::Registry registry) {
    render_transparency(registry, "effects_depth_tex");
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_PARTICLES);

    const auto depthHndl =
      registry.readTexture("depth").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE).handle();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("effects_depth_tex_samplerstate");

    registry.readBlob<dafx::CullingId>("early_particles_cullingId").optional();
    auto partitionCullingIdOptHndl =
      is_early ? eastl::make_optional(registry.readBlob<dafx::CullingId>("early_particles_cullingId").handle()) : eastl::nullopt;

    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    const bool shouldHaveLowres = wr.getFxRtOverride() != FX_RT_OVERRIDE_HIGHRES;
    const bool upscaleTexAvailable = wr.hasFeature(FeatureRenderFlags::UPSCALE_SAMPLING_TEX);
    if (shouldHaveLowres)
    {
      registry.read("low_res_fx_rt").texture().atStage(dabfg::Stage::PS).bindToShaderVar("lowres_fx_source_tex");

      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("low_res_sampler", dabfg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
        .bindToShaderVar("lowres_fx_source_tex_samplerstate");

      if (upscaleTexAvailable)
        registry.read("upscale_sampling_tex").texture().atStage(dabfg::Stage::PS).bindToShaderVar("upscale_sampling_tex");
    }

    use_particles_render(registry);

    return [applyLowResFx{
              shouldHaveLowres ? PostFxRenderer(upscaleTexAvailable ? "apply_lowres_fx" : "fast_apply_lowres_fx") : PostFxRenderer()},
             depthHndl, partitionCullingIdOptHndl, shouldHaveLowres] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      const bool thermalVisionOn = acesfx::thermal_vision_on();

      if (shouldHaveLowres)
      {
        // upscale_sampling_tex has been generated based on checkerboard_depth.
        // However, using it also for checkerboard_depth_with_water gives pleasant results even on water.
        applyLowResFx.render();
      }

      dafx::CullingId cullingId = partitionCullingIdOptHndl ? partitionCullingIdOptHndl->ref() : dafx::CullingId();
      acesfx::setDepthTex(depthHndl.view().getTex2D());

      if (!thermalVisionOn)
        acesfx::renderTransHighRes(cullingId);
      else if (wr.getFxRtOverride() == FX_RT_OVERRIDE_HIGHRES)
        acesfx::renderTransThermal(cullingId);
      acesfx::renderTransSpecial(ERT_TAG_ATEST, cullingId);
    };
  });
}

dabfg::NodeHandle makeTransparentParticlesNode() { return makeTransparentParticlesNode(false); }

dabfg::NodeHandle makeTransparentParticlesEarlyNode() { return makeTransparentParticlesNode(true); }

dabfg::NodeHandle makeTransparentParticlesLowresPrepareNode() { return makeTransparentParticlesLowresPrepareNode(false); }

dabfg::NodeHandle makeTransparentParticlesLowresPrepareEarlyNode() { return makeTransparentParticlesLowresPrepareNode(true); }