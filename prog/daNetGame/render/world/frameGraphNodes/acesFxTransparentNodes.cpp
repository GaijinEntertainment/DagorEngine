// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/deferredRenderer.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderBlock.h>
#include <render/world/aimRender.h>

#include "render/fx/fx.h"
#include "render/fx/fxRenderTags.h"
#include "render/world/partitionSphere.h"
#include "render/world/frameGraphHelpers.h"
#include <render/antiAliasing.h>
#include <render/world/wrDispatcher.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include <drv/3d/dag_renderTarget.h>

dafg::NodeHandle makeAcesFxTransparentPartitionNode()
{
  auto nodeNs = dafg::root() / "transparent";
  return nodeNs.registerNode("acesfx_transparent_partition_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.setPriority(dafg::PRIO_AS_LATE_AS_POSSIBLE);
    registry.read("acesfx_update_token").blob<OrderingToken>();

    auto cullingIdHndl = registry.createBlob<dafx::CullingId>("early_particles_cullingId", dafg::History::No).handle();

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

static void use_particles_render(dafg::Registry registry)
{
  registry.read("fom_shadows_sin").texture().atStage(dafg::Stage::PRE_RASTER).bindToShaderVar("fom_shadows_sin").optional();
  registry.read("fom_shadows_cos").texture().atStage(dafg::Stage::PRE_RASTER).bindToShaderVar("fom_shadows_cos").optional();
  registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();
  registry.requestState().setFrameBlock("global_frame");
}

eastl::fixed_vector<dafg::NodeHandle, 2, false> makeAcesFxLowresTransparentNodes(bool is_early)
{
  // NOTE: If there are no effects on screen or if they take too little time to render
  // then better to use fullres depth from `wr.target`. It already has rendered water
  // and this way we can save time by avoiding one downsample. We also can avoid downsample
  // if there is no water on the screen.

  auto nodeNs = dafg::root() / "transparent" / (is_early ? "far" : "close");

  eastl::fixed_vector<dafg::NodeHandle, 2, false> nodes;

  const char *prepareNodeName = "acesfx_prepare_lowres_transparent_node";
  nodes.emplace_back(nodeNs.registerNode(prepareNodeName, DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.create("low_res_fx_rt", dafg::History::No)
      .texture({TEXFMT_A16B16G16R16F | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f)});

    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    AntiAliasing *antiAliasing = wr.getAntialiasing();
    bool needReactiveMask = antiAliasing && antiAliasing->supportsReactiveMask();

    if (needReactiveMask)
      registry.create("low_res_fx_reactive_mask_rt", dafg::History::No)
        .texture({TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f)});

    registry.create("effects_depth_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler({}));
  }));

  const char *renderNodeName = "acesfx_render_lowres_transparent_node";
  nodes.emplace_back(nodeNs.registerNode(renderNodeName, DAFG_PP_NODE_SRC, [is_early](dafg::Registry registry) {
    registry.allowAsyncPipelines();
    registry.read("acesfx_update_token").blob<OrderingToken>();

    auto lowResFxTexHndl =
      registry.modifyTexture("low_res_fx_rt").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto cameraHndl = read_camera_in_camera(registry).handle();

    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

    auto lowResFxReactiveMaskHndl = registry.modifyTexture("low_res_fx_reactive_mask_rt")
                                      .atStage(dafg::Stage::PS)
                                      .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                      .optional()
                                      .handle();

    registry.readBlob<dafx::CullingId>("early_particles_cullingId").optional();
    auto partitionCullingIdOptHndl =
      is_early ? eastl::make_optional(registry.readBlob<dafx::CullingId>("early_particles_cullingId").handle()) : eastl::nullopt;

    // TODO: this request should depend on water rendering mode, but now we don't recompile FG underwater.
    bool upscaleTexAvailable = wr.hasFeature(FeatureRenderFlags::UPSCALE_SAMPLING_TEX);
    auto depthWithWaterHndl = registry.read(upscaleTexAvailable ? "checkerboard_depth_with_water" : "far_downsampled_depth_with_water")
                                .texture()
                                .atStage(dafg::Stage::PS)
                                .useAs(dafg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
                                .handle();

    auto depthThermalOptHndl = registry.read("thermal_downsampled_depth_for_fx")
                                 .texture()
                                 .atStage(dafg::Stage::PS)
                                 .useAs(dafg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
                                 .optional()
                                 .handle();

    auto downsampledDepthHndl = registry.read("downsampled_depth_with_early_after_envi_water")
                                  .texture()
                                  .atStage(dafg::Stage::PS)
                                  .useAs(dafg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
                                  .handle();

    auto checkerboardDepthHndl =
      registry.read("checkerboard_depth").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();

    auto waterRenderModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();

    auto hasAnyDynamicLightsHndl = registry.readBlob<bool>("has_any_dynamic_lights").handle();

    use_particles_render(registry);
    registry.readBlob<d3d::SamplerHandle>("effects_depth_sampler").bindToShaderVar("effects_depth_tex_samplerstate");

    auto resources =
      eastl::make_tuple(lowResFxTexHndl, depthWithWaterHndl, depthThermalOptHndl, downsampledDepthHndl, checkerboardDepthHndl,
        waterRenderModeHndl, partitionCullingIdOptHndl, lowResFxReactiveMaskHndl, cameraHndl, hasAnyDynamicLightsHndl);
    int effectsDepthTexVarId = ::get_shader_glob_var_id("effects_depth_tex");

    return [resources = eastl::make_unique<decltype(resources)>(resources), effectsDepthTexVarId](
             const dafg::multiplexing::Index &multiplexing_index) {
      auto [lowResFxTexHndl, depthWithWaterHndl, depthThermalOptHndl, downsampledDepthHndl, checkerboardDepthHndl, waterRenderModeHndl,
        partitionCullingIdOptHndl, lowResFxReactiveMaskHndl, cameraHndl, hasAnyDynamicLightsHndl] = *resources;

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const bool thermalVisionOn = acesfx::thermal_vision_on();

      ClusteredLights &lights = WRDispatcher::getClusteredLights();
      if (lights.hasClusteredLights() && hasAnyDynamicLightsHndl.ref())
        lights.setBuffersToShader();

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
      d3d::set_render_target({depthAttachment.getBaseTex(), 0}, DepthAccess::SampledRO,
        {{lowResFxTex.getBaseTex(), 0}, {lowResFxReactiveMaskHndl.get()}});

      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      const CameraParams &camera = cameraHndl.ref();
      acesfx::set_dafx_globaldata(camera.jitterGlobtm, camera.viewItm, camera.viewItm.getcol(3));

      if (multiplexing_index.subCamera == 0)
        d3d::clearview(CLEAR_TARGET, 0xFF000000, 0, 0);

      ShaderGlobal::set_texture(effectsDepthTexVarId, depthShaderResource);
      acesfx::setDepthTex(depthShaderResource.getTex2D());

      dafx::CullingId cullingId = partitionCullingIdOptHndl ? partitionCullingIdOptHndl->ref() : dafx::CullingId();
      if (!thermalVisionOn)
        acesfx::renderTransLowRes(cullingId);
      else
        acesfx::renderTransThermal(cullingId);
    };
  }));

  return nodes;
}

dafg::NodeHandle makeAcesFxTransparentNode(bool is_early)
{
  auto nodeNs = dafg::root() / "transparent" / (is_early ? "far" : "close");
  return nodeNs.registerNode("acesfx_transparent_node", DAFG_PP_NODE_SRC, [is_early](dafg::Registry registry) {
    auto cameraHndl = request_common_transparent_state(registry, "effects_depth_tex");
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_PARTICLES);
    registry.create("transparent_particles_token", dafg::History::No).blob<OrderingToken>();

    const auto depthHndl =
      registry.readTexture("depth").atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE).handle();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("effects_depth_tex_samplerstate");

    registry.readBlob<dafx::CullingId>("early_particles_cullingId").optional();
    auto partitionCullingIdOptHndl =
      is_early ? eastl::make_optional(registry.readBlob<dafx::CullingId>("early_particles_cullingId").handle()) : eastl::nullopt;

    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    const bool shouldHaveLowres = wr.getFxRtOverride() != FX_RT_OVERRIDE_HIGHRES;
    const bool upscaleTexAvailable = wr.hasFeature(FeatureRenderFlags::UPSCALE_SAMPLING_TEX);
    if (shouldHaveLowres)
    {
      registry.read("low_res_fx_rt").texture().atStage(dafg::Stage::PS).bindToShaderVar("lowres_fx_source_tex");
      registry.read("low_res_fx_reactive_mask_rt")
        .texture()
        .atStage(dafg::Stage::PS)
        .bindToShaderVar("lowres_fx_reactive_mask_tex")
        .optional();

      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("low_res_sampler", dafg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
        .bindToShaderVar("lowres_fx_source_tex_samplerstate");

      if (upscaleTexAvailable)
        registry.read("upscale_sampling_tex").texture().atStage(dafg::Stage::PS).bindToShaderVar("upscale_sampling_tex");
    }

    use_particles_render(registry);

    return [applyLowResFx{
              shouldHaveLowres ? PostFxRenderer(upscaleTexAvailable ? "apply_lowres_fx" : "fast_apply_lowres_fx") : PostFxRenderer()},
             depthHndl, partitionCullingIdOptHndl, shouldHaveLowres, cameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};
      const CameraParams &camera = cameraHndl.ref();
      acesfx::set_dafx_globaldata(camera.jitterGlobtm, camera.viewItm, camera.viewItm.getcol(3));

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
      acesfx::renderTransSpecial(ERT_TAG_XRAY, cullingId);
    };
  });
}

dafg::NodeHandle makeAcesFxTransparentNode() { return makeAcesFxTransparentNode(false); }

dafg::NodeHandle makeAcesFxEarlyTransparentNode() { return makeAcesFxTransparentNode(true); }

eastl::fixed_vector<dafg::NodeHandle, 2, false> makeAcesFxLowresTransparentNodes() { return makeAcesFxLowresTransparentNodes(false); }

eastl::fixed_vector<dafg::NodeHandle, 2, false> makeAcesFxEarlyLowresTransparentNodes()
{
  return makeAcesFxLowresTransparentNodes(true);
}