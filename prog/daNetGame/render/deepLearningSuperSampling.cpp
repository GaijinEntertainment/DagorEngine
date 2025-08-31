// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "deepLearningSuperSampling.h"
#include "render/daFrameGraph/stage.h"

#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_texture.h>
#include <util/dag_convar.h>
#include <perfMon/dag_statDrv.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_log.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_mathUtils.h>
#include <util/dag_oaHashNameMap.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_textureIDHolder.h>
#include <render/daFrameGraph/daFG.h>
#include <render/resourceSlot/registerAccess.h>
#include <3d/dag_nvFeatures.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <shaders/dag_computeShaders.h>

using DLSS = DeepLearningSuperSampling;

CONSOLE_INT_VAL("render", dlss_quality, int(DLSS::Preset::QUALITY), int(DLSS::Preset::PERFORMANCE), int(DLSS::Preset::DLAA));
CONSOLE_FLOAT_VAL_MINMAX("render", dlss_mip_bias, -1.0f, -5, 4);
CONSOLE_FLOAT_VAL_MINMAX("render", dlss_mip_bias_epsilon, 0.0f, -5, 4);
CONSOLE_BOOL_VAL("render", dlss_renderpath_without_dlss,
  false); // DLSS fails to initialize when launching game from RenderDoc :(
bool dlss_toggle = true;

static nv::DLSS::Mode parse_mode()
{
  dlss_quality.set(::dgs_get_settings()->getBlockByNameEx("video")->getInt("dlssQuality", dlss_quality.get()));
  return nv::DLSS::Mode(dlss_quality.get());
}

static bool is_ray_reconstruction_enabled()
{
  nv::Streamline *streamline = nullptr;
  d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
  nv::DLSS *dlss = streamline ? streamline->getDlssFeature(0) : nullptr;
  return dlss && dlss->isRR;
}

static IPoint2 query_input_resolution(nv::DLSS::Mode mode, const IPoint2 &outputResolution)
{
  if (mode == nv::DLSS::Mode::DLAA)
    return outputResolution;

  nv::Streamline *streamline = nullptr;
  d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
  nv::DLSS *dlss = streamline ? streamline->getDlssFeature(0) : nullptr;
  if (!dlss)
    return outputResolution;

  auto optimalSettings = dlss->getOptimalSettings(mode, outputResolution);
  G_ASSERT(optimalSettings.has_value());

  debug("Rendering resolution applied by DLSS: %dx%d.", optimalSettings->renderWidth, optimalSettings->renderHeight);

  return IPoint2(optimalSettings->renderWidth, optimalSettings->renderHeight);
}

void DeepLearningSuperSampling::load_settings()
{
  dlss_quality.set(::dgs_get_settings()->getBlockByNameEx("video")->getInt("dlssQuality", dlss_quality.get()));
  dlss_renderpath_without_dlss.set(
    ::dgs_get_settings()->getBlockByNameEx("video")->getBool("dlssRenderpathWithoutDlss", dlss_renderpath_without_dlss.get()));
}

bool DeepLearningSuperSampling::is_enabled()
{
  nv::Streamline *streamline = nullptr;
  d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
  bool ready = streamline && streamline->getDlssState() == nv::DLSS::State::READY;
  return dlss_toggle && (ready || dlss_renderpath_without_dlss.get());
}

bool DeepLearningSuperSampling::isFrameGenerationEnabled() const { return frameGenerationNode.valid(); }

static int parse_generated_frame_count()
{
  const DataBlock &blkVideo = *::dgs_get_settings()->getBlockByNameEx("video");
  return blkVideo.getInt("dlssFrameGenerationCount", 0);
}

DeepLearningSuperSampling::DeepLearningSuperSampling(const IPoint2 &outputResolution) :
  AntiAliasing(query_input_resolution(parse_mode(), outputResolution), outputResolution), rrEnabled(is_ray_reconstruction_enabled())
{
  // Advised mip bias in DLSS_Programming_Guide_Release.pdf, section 3.5 is
  // DlssMipLevelBias = log2(Render XResolution / Display XResolution) + epsilon

  // TODO: create DLSS's own mip bias property
  dlss_mip_bias.set(log2(float(inputResolution.y) / float(outputResolution.y)) + dlss_mip_bias_epsilon.get());

  nv::Streamline *streamline = nullptr;
  d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
  G_ASSERT(streamline);
  nv::DLSS *dlss = streamline ? streamline->getDlssFeature(0) : nullptr;
  if (!dlss)
    return;

  dlss->setOptions(parse_mode(), outputResolution);

  if (rrEnabled)
  {
    rayReconstructionPrepareNode =
      dafg::register_node("dlss_ray_reconstruction_prepare_node", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
        read_gbuffer(registry, dafg::Stage::PS, readgbuffer::NORMAL | readgbuffer::MATERIAL);
        registry
          .createTexture2d("packed_normal_roughness", dafg::History::No,
            {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>("main_view")})
          .atStage(dafg::Stage::COMPUTE)
          .bindToShaderVar("dlss_normal_roughness");
        registry
          .createTexture2d("specular_albedo", dafg::History::No,
            {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>("main_view")})
          .atStage(dafg::Stage::COMPUTE)
          .bindToShaderVar("dlss_specular_albedo");

        return [this, renderer = ComputeShader("prepare_ray_reconstruction")] {
          renderer.dispatchThreads(inputResolution.x, inputResolution.y, 1);
        };
      });
  }

  applierNode = dafg::register_node("dlss", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
    auto opaqueFinalTargetHndl = registry.readTexture("final_target_with_motion_blur")
                                   .atStage(dafg::Stage::PS_OR_CS)
                                   .useAs(dafg::Usage::SHADER_RESOURCE)
                                   .handle();
    auto depthHndl =
      registry.readTexture("depth_after_transparency").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto motionVectorsHndl = registry.readTexture("motion_vecs_after_transparency")
                               .atStage(dafg::Stage::PS_OR_CS)
                               .useAs(dafg::Usage::SHADER_RESOURCE)
                               .handle();
    auto exposureNormFactorHndl = registry.readTexture("exposure_normalization_factor")
                                    .atStage(dafg::Stage::PS_OR_CS)
                                    .useAs(dafg::Usage::SHADER_RESOURCE)
                                    .handle();
    auto antialiasedHndl = registry
                             .createTexture2d("frame_after_aa", dafg::History::No,
                               {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>("display")})
                             .atStage(dafg::Stage::PS_OR_CS)
                             .useAs(dafg::Usage::SHADER_RESOURCE)
                             .handle();
    auto camera = registry.readBlob<CameraParams>("current_camera").handle();
    auto cameraHistory = registry.readBlobHistory<CameraParams>("current_camera").handle();

    auto albedoHndl =
      rrEnabled ? eastl::make_optional(
                    registry.read("gbuf_0").texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle())
                : eastl::nullopt;
    auto specularAlbedoHndl =
      rrEnabled
        ? eastl::make_optional(
            registry.read("specular_albedo").texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle())
        : eastl::nullopt;
    auto normalRoughnessHndl = rrEnabled ? eastl::make_optional(registry.read("packed_normal_roughness")
                                                                  .texture()
                                                                  .atStage(dafg::Stage::PS_OR_CS)
                                                                  .useAs(dafg::Usage::SHADER_RESOURCE)
                                                                  .handle())
                                         : eastl::nullopt;

    auto hitDistHndl =
      rrEnabled
        ? eastl::make_optional(
            registry.read("rtr_tex_unfiltered").texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle())
        : eastl::nullopt;

    auto frameCounterHndl = registry.readBlob<uint32_t>("monotonic_frame_counter").handle();

    auto resources = eastl::make_tuple(depthHndl, motionVectorsHndl, exposureNormFactorHndl, opaqueFinalTargetHndl, antialiasedHndl,
      camera, cameraHistory, albedoHndl, normalRoughnessHndl, specularAlbedoHndl, hitDistHndl, frameCounterHndl);

    return [this, resources = eastl::make_unique<decltype(resources)>(resources)] {
      auto [depthHndl, motionVectorsHndl, exposureNormFactorHndl, opaqueFinalTargetHndl, antialiasedHndl, camera, cameraHistory,
        albedoHndl, normalRoughnessHndl, specularAlbedoHndl, hitDistHndl, frameCounterHndl] = *resources;
      OptionalInputParams params{};
      params.depth = depthHndl.view().getTex2D();
      params.motion = motionVectorsHndl.view().getTex2D();
      params.exposure = exposureNormFactorHndl.view().getTex2D();
      if (rrEnabled)
      {
        params.albedo = albedoHndl->view().getTex2D();
        params.specularAlbedo = specularAlbedoHndl->view().getTex2D();
        params.normalRoughness = normalRoughnessHndl->view().getTex2D();
        params.hitDist = hitDistHndl->view().getTex2D();
      }
      dlss_render(opaqueFinalTargetHndl.view().getTex2D(), antialiasedHndl.view().getTex2D(), jitterOffset, params, camera.ref(),
        cameraHistory.ref(), frameCounterHndl.ref());
    };
  });

  nv::DLSSFrameGeneration *dlssg = streamline ? streamline->getDlssGFeature(0) : nullptr;
  if (dlssg)
  {
    int framesToGenerate = parse_generated_frame_count();
    int maxNumberOfGeneratedFrames = streamline ? streamline->getMaximumNumberOfGeneratedFrames() : 0;
    framesToGenerate = eastl::min(maxNumberOfGeneratedFrames, framesToGenerate);
    if (framesToGenerate > 0)
    {
      frameGenerationNode = dafg::register_node("dlss_g", DAFG_PP_NODE_SRC, [this, framesToGenerate](dafg::Registry registry) {
        registry.executionHas(dafg::SideEffects::External);
        auto beforeUINs = registry.root() / "before_ui";
        auto finalFrameHandle =
          beforeUINs.readTexture("frame_done").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        auto uiHandle = registry.readTexture("ui_tex").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        auto depthHandle =
          registry.readTexture("depth_for_postfx").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        auto motionVectorsHandle = registry.readTexture("motion_vecs_after_transparency")
                                     .atStage(dafg::Stage::PS_OR_CS)
                                     .useAs(dafg::Usage::SHADER_RESOURCE)
                                     .handle();
        auto camera = registry.readBlob<CameraParams>("current_camera").handle();
        auto cameraHistory = registry.readBlobHistory<CameraParams>("current_camera").handle();
        auto frameCounterHndl = registry.readBlob<uint32_t>("monotonic_frame_counter").handle();

        return [this, finalFrameHandle, uiHandle, motionVectorsHandle, depthHandle, camera, cameraHistory, framesToGenerate,
                 frameCounterHndl](const dafg::multiplexing::Index idx) {
          nv::Streamline *streamline = nullptr;
          d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
          nv::DLSSFrameGeneration *dlssg = streamline ? streamline->getDlssGFeature(idx.viewport) : nullptr;
          if (!dlssg)
          {
            logerr("DLSS-G feature is not available for viewport %d", idx.viewport);
            return;
          }

          nv::DlssGParams dlssGParams = {};
          dlssGParams.inHUDless = finalFrameHandle.view().getTex2D();
          dlssGParams.inUI = uiHandle.view().getTex2D();
          dlssGParams.inMotionVectors = motionVectorsHandle.view().getTex2D();
          dlssGParams.inDepth = depthHandle.view().getTex2D();
          dlssGParams.inJitterOffsetX = jitterOffset.x;
          dlssGParams.inJitterOffsetY = jitterOffset.y;
          dlssGParams.inMVScaleX = 1.f;
          dlssGParams.inMVScaleY = 1.f;
          dlssGParams.camera.projection = camera.ref().noJitterProjTm;
          dlssGParams.camera.projectionInverse = inverse44(camera.ref().noJitterProjTm);
          dlssGParams.camera.reprojection = inverse44(camera.ref().noJitterGlobtm) * cameraHistory.ref().noJitterGlobtm;
          dlssGParams.camera.reprojectionInverse = inverse44(cameraHistory.ref().noJitterGlobtm) * camera.ref().noJitterGlobtm;
          dlssGParams.camera.right = camera.ref().viewItm.getcol(0);
          dlssGParams.camera.up = camera.ref().viewItm.getcol(1);
          dlssGParams.camera.forward = camera.ref().viewItm.getcol(2);
          dlssGParams.camera.position = camera.ref().viewItm.getcol(3);
          dlssGParams.camera.nearZ = camera.ref().noJitterPersp.zn;
          dlssGParams.camera.farZ = camera.ref().noJitterPersp.zf;
          dlssGParams.camera.fov = 2 * atan(1.f / camera.ref().noJitterPersp.wk);
          dlssGParams.camera.aspect = camera.ref().noJitterPersp.hk / camera.ref().noJitterPersp.wk;
          dlssGParams.frameId = frameCounterHndl.ref();

          int viewIndex = idx.viewport;
          if (!dlssg->isEnabled())
          {
            dlssg->setEnabled(framesToGenerate);
          }
          d3d::driver_command(Drv3dCommand::EXECUTE_DLSS_G, &dlssGParams, &viewIndex);
        };
      });

      // the sole purpose of this node is to reference history of the resources used by dlss_g node in order to make these resources
      // "survive" until present()
      lifetimeExtenderNode = dafg::register_node("dlss_g_lifetime_extender", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
        registry.executionHas(dafg::SideEffects::External);
        // ordering before setup_world_rendering_node makes lifetimes as short as possible while making sure the resources are still
        // alive during present()
        registry.orderMeBefore("setup_world_rendering_node");
        registry.readTextureHistory("ui_tex").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE);
        registry.readTextureHistory("depth_for_postfx").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE);
        registry.readTextureHistory("motion_vecs_after_transparency")
          .atStage(dafg::Stage::PS_OR_CS)
          .useAs(dafg::Usage::SHADER_RESOURCE);
        return [] {};
      });
    }
    else
    {
      dlssg->setEnabled(0);
    }
  }
}

DeepLearningSuperSampling::~DeepLearningSuperSampling()
{
  nv::Streamline *streamline = nullptr;
  d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
  nv::DLSSFrameGeneration *dlssg = streamline ? streamline->getDlssGFeature(0) : nullptr;
  if (dlssg)
    dlssg->setEnabled(0);
}

float DeepLearningSuperSampling::getLodBias() const { return dlss_mip_bias.get(); }

void dlss_render(Texture *in_color,
  Texture *out_color,
  Point2 jitter_offset,
  const AntiAliasing::OptionalInputParams &params,
  const CameraParams &camera,
  const CameraParams &prev_camera,
  uint32_t frame_id)
{
  nv::DlssParams dlssParams = {};
  dlssParams.inColor = in_color;
  dlssParams.inDepth = params.depth;
  dlssParams.inMotionVectors = params.motion;
  dlssParams.inExposure = params.exposure;
  dlssParams.inAlbedo = params.albedo;
  dlssParams.inSpecularAlbedo = params.specularAlbedo;
  dlssParams.inNormalRoughness = params.normalRoughness;
  dlssParams.inHitDist = params.hitDist;
  dlssParams.inJitterOffsetX = jitter_offset.x;
  dlssParams.inJitterOffsetY = jitter_offset.y;
  dlssParams.outColor = out_color;
  dlssParams.inMVScaleX = 1.f;
  dlssParams.inMVScaleY = 1.f;
  dlssParams.camera.projection = camera.noJitterProjTm;
  dlssParams.camera.projectionInverse = inverse44(camera.noJitterProjTm);
  dlssParams.camera.reprojection = inverse44(camera.noJitterGlobtm) * prev_camera.noJitterGlobtm;
  dlssParams.camera.reprojectionInverse = inverse44(prev_camera.noJitterGlobtm) * camera.noJitterGlobtm;
  dlssParams.camera.right = camera.viewItm.getcol(0);
  dlssParams.camera.up = camera.viewItm.getcol(1);
  dlssParams.camera.forward = camera.viewItm.getcol(2);
  dlssParams.camera.position = camera.viewItm.getcol(3);
  dlssParams.camera.nearZ = camera.noJitterPersp.zn;
  dlssParams.camera.farZ = camera.noJitterPersp.zf;
  dlssParams.camera.fov = 2 * atan(1.f / camera.noJitterPersp.wk);
  dlssParams.camera.aspect = camera.noJitterPersp.hk / camera.noJitterPersp.wk;
  dlssParams.camera.worldToView = camera.viewTm;
  dlssParams.camera.viewToWorld = camera.viewItm;
  dlssParams.frameId = frame_id;

  ShaderGlobal::set_color4(get_shader_variable_id("dlss_jitter_offset", true), jitter_offset);

  {
    TIME_D3D_PROFILE(dlss_execute);
    if (dlss_renderpath_without_dlss.get())
      d3d::stretch_rect(dlssParams.inColor, dlssParams.outColor);
    else
      d3d::driver_command(Drv3dCommand::EXECUTE_DLSS, &dlssParams);
  }
}

#if DAGOR_DBGLEVEL > 0

#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <startup/dag_loadSettings.h>
#include <drv/3d/dag_resetDevice.h>
#include <render/renderer.h>

static void dlss_settings_imgui()
{
  IRenderWorld *wr = get_world_renderer();
  if (!wr)
    return;

  ImGui::Text("DLSS enabled: %s", DeepLearningSuperSampling::is_enabled() ? "true" : "false");

  static const char *dlssQualityStrings[] = {"Ultra-Performance", "Performance", "Balanced", "Quality", "DLAA"};
  int selectedDlssQuality = dlss_quality.get();
  if (ImGui::Combo("Quality preset", &selectedDlssQuality, dlssQualityStrings, IM_ARRAYSIZE(dlssQualityStrings)))
  {
    dlss_quality.set(selectedDlssQuality);

    DataBlock blk;
    blk.addBlock("video")->setInt("dlssQuality", dlss_quality.get());
    ::dgs_apply_config_blk(blk, false, false);

    FastNameMap changed;
    changed.addNameId("video/dlssQuality");
    bool applyAfterResetDevice = false;
    change_driver_reset_request(applyAfterResetDevice);
    wr->onSettingsChanged(changed, applyAfterResetDevice);
  }

  float mipBias = dlss_mip_bias.get();
  ImGui::SliderFloat("Mip bias", &mipBias, dlss_mip_bias.getMin(), dlss_mip_bias.getMax());
  dlss_mip_bias.set(mipBias);
  float mipBiasEpsilon = dlss_mip_bias_epsilon.get();
  bool needSetResolution =
    ImGui::SliderFloat("Mip bias epsilon", &mipBiasEpsilon, dlss_mip_bias_epsilon.getMin(), dlss_mip_bias_epsilon.getMax());
  dlss_mip_bias_epsilon.set(mipBiasEpsilon);

  needSetResolution |= ImGui::Checkbox("Toggle", &dlss_toggle);
  if (needSetResolution)
  {
    d3d::GpuAutoLock gpuLock;
    wr->setResolution();
  }
}

REGISTER_IMGUI_WINDOW("Render", "DLSS settings", dlss_settings_imgui);

#endif