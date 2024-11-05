// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "deepLearningSuperSampling.h"

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
#include <render/daBfg/bfg.h>
#include <render/resourceSlot/registerAccess.h>
#include <3d/dag_nvFeatures.h>
#include <render/world/cameraParams.h>


using DLSS = DeepLearningSuperSampling;

CONSOLE_INT_VAL("render", dlss_quality, int(DLSS::Preset::QUALITY), int(DLSS::Preset::PERFORMANCE), int(DLSS::Preset::DLAA));
CONSOLE_FLOAT_VAL_MINMAX("render", dlss_mip_bias, -1.0f, -5, 4);
CONSOLE_FLOAT_VAL_MINMAX("render", dlss_mip_bias_epsilon, 0.0f, -5, 4);
CONSOLE_FLOAT_VAL_MINMAX("render", dlss_sharpness, 0.0f, 0.0f, 1.0f);
CONSOLE_BOOL_VAL("render", dlss_renderpath_without_dlss,
  false); // DLSS fails to initialize when launching game from RenderDoc :(
bool dlss_toggle = true;

static nv::DLSS::Mode parse_mode()
{
  dlss_quality.set(::dgs_get_settings()->getBlockByNameEx("video")->getInt("dlssQuality", dlss_quality.get()));
  return nv::DLSS::Mode(dlss_quality.get());
}

static IPoint2 query_input_resolution(nv::DLSS::Mode mode, const IPoint2 &outputResolution)
{
  if (mode == nv::DLSS::Mode::DLAA)
    return outputResolution;

  nv::DLSS *dlss{};
  d3d::driver_command(Drv3dCommand::GET_DLSS, &dlss);
  G_ASSERT(dlss);
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
  nv::DLSS *dlss{};
  d3d::driver_command(Drv3dCommand::GET_DLSS, &dlss);
  bool ready = dlss && dlss->getDlssState() == nv::DLSS::State::READY;
  return dlss_toggle && (ready || dlss_renderpath_without_dlss.get());
}

bool DeepLearningSuperSampling::isFrameGenerationEnabled() const { return frameGenerationNode.valid(); }

DeepLearningSuperSampling::DeepLearningSuperSampling(const IPoint2 &outputResolution) :
  AntiAliasing(query_input_resolution(parse_mode(), outputResolution), outputResolution)
{
  // Advised mip bias in DLSS_Programming_Guide_Release.pdf, section 3.5 is
  // DlssMipLevelBias = log2(Render XResolution / Display XResolution) + epsilon

  // TODO: create DLSS's own mip bias property
  dlss_mip_bias.set(log2(float(inputResolution.y) / float(outputResolution.y)) + dlss_mip_bias_epsilon.get());

  nv::DLSS *dlss{};
  d3d::driver_command(Drv3dCommand::GET_DLSS, &dlss);

  nv::DLSS::Mode mode = parse_mode();
  if (mode == nv::DLSS::Mode::DLAA)
  {
    dlss->setOptions(0, nv::DLSS::Mode::DLAA, outputResolution, 1.f);
  }
  else
  {
    auto optimalSettings = dlss->getOptimalSettings(mode, outputResolution);
    G_ASSERT(optimalSettings.has_value());
    dlss->setOptions(0, mode, outputResolution, optimalSettings->sharpness);
  }

  applierNode = resource_slot::register_access("dlss", DABFG_PP_NODE_SRC,
    {resource_slot::Create{"postfx_input_slot", "frame_for_postfx"}},
    [this](resource_slot::State slotsState, dabfg::Registry registry) {
      auto opaqueFinalTargetHndl = registry.readTexture("final_target_with_motion_blur")
                                     .atStage(dabfg::Stage::PS_OR_CS)
                                     .useAs(dabfg::Usage::SHADER_RESOURCE)
                                     .handle();
      auto depthHndl =
        registry.readTexture("depth_after_transparency").atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
      auto motionVectorsHndl = registry.readTexture("motion_vecs_after_transparency")
                                 .atStage(dabfg::Stage::PS_OR_CS)
                                 .useAs(dabfg::Usage::SHADER_RESOURCE)
                                 .handle();
      auto exposureNormFactorHndl = registry.readTexture("exposure_normalization_factor")
                                      .atStage(dabfg::Stage::PS_OR_CS)
                                      .useAs(dabfg::Usage::SHADER_RESOURCE)
                                      .handle();
      auto antialiasedHndl = registry
                               .createTexture2d(slotsState.resourceToCreateFor("postfx_input_slot"), dabfg::History::No,
                                 {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED | TEXCF_RTARGET, registry.getResolution<2>("display")})
                               .atStage(dabfg::Stage::PS_OR_CS)
                               .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                               .handle();
      auto camera = registry.readBlob<CameraParams>("current_camera").handle();
      auto cameraHistory = registry.readBlobHistory<CameraParams>("current_camera").handle();

      return
        [this, depthHndl, motionVectorsHndl, exposureNormFactorHndl, opaqueFinalTargetHndl, antialiasedHndl, camera, cameraHistory] {
          OptionalInputParams params;
          params.depth = depthHndl.view().getTex2D();
          params.motion = motionVectorsHndl.view().getTex2D();
          params.exposure = exposureNormFactorHndl.view().getTex2D();
          dlss_render(opaqueFinalTargetHndl.view().getTex2D(), antialiasedHndl.view().getTex2D(), jitterOffset, params, camera.ref(),
            cameraHistory.ref());
        };
    });

  if (dlss->isFrameGenerationSupported())
  {
    if (::dgs_get_settings()->getBlockByNameEx("video")->getBool("dlssFrameGeneration", false))
    {
      frameGenerationNode = dabfg::register_node("dlss_g", DABFG_PP_NODE_SRC, [dlss](dabfg::Registry registry) {
        registry.executionHas(dabfg::SideEffects::External);
        auto beforeUINs = registry.root() / "before_ui";
        auto finalFrameHandle =
          beforeUINs.readTexture("frame_done").atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
        auto uiHandle = registry.readTexture("ui_tex").atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
        auto depthHandle =
          registry.readTexture("depth_for_postfx").atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
        auto motionVectorsHandle = registry.readTexture("motion_vecs_after_transparency")
                                     .atStage(dabfg::Stage::PS_OR_CS)
                                     .useAs(dabfg::Usage::SHADER_RESOURCE)
                                     .handle();

        return [dlss, finalFrameHandle, uiHandle, motionVectorsHandle, depthHandle](const dabfg::multiplexing::Index idx) {
          nv::DlssGParams dlssGParams = {};
          dlssGParams.inHUDless = finalFrameHandle.view().getTex2D();
          dlssGParams.inUI = uiHandle.view().getTex2D();
          dlssGParams.inMotionVectors = motionVectorsHandle.view().getTex2D();
          dlssGParams.inDepth = depthHandle.view().getTex2D();
          int viewIndex = idx.viewport;
          if (!dlss->isFrameGenerationEnabled())
            dlss->enableDlssG(0); // enable only the last minute, otherwise DLSS-G is missing some textures on Present call.
          d3d::driver_command(Drv3dCommand::EXECUTE_DLSS_G, &dlssGParams, &viewIndex);
        };
      });

      // the sole purpose of this node is to reference history of the resources used by dlss_g node in order to make these resources
      // "survive" until present()
      lifetimeExtenderNode = dabfg::register_node("dlss_g_lifetime_extender", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
        registry.executionHas(dabfg::SideEffects::External);
        // ordering before setup_world_rendering_node makes lifetimes as short as possible while making sure the resources are still
        // alive during present()
        registry.orderMeBefore("setup_world_rendering_node");
        registry.readTextureHistory("ui_tex").atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE);
        registry.readTextureHistory("depth_for_postfx").atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE);
        registry.readTextureHistory("motion_vecs_after_transparency")
          .atStage(dabfg::Stage::PS_OR_CS)
          .useAs(dabfg::Usage::SHADER_RESOURCE);
        return [] {};
      });
    }
    else
    {
      dlss->disableDlssG(0);
    }
  }
}

DeepLearningSuperSampling::~DeepLearningSuperSampling()
{
  nv::DLSS *dlss{};
  d3d::driver_command(Drv3dCommand::GET_DLSS, &dlss);
  if (dlss && dlss->isDlssGSupported() == nv::SupportState::Supported)
    dlss->disableDlssG(0);
}

float DeepLearningSuperSampling::getLodBias() const { return dlss_mip_bias.get(); }

void dlss_render(Texture *in_color,
  Texture *out_color,
  Point2 jitter_offset,
  const AntiAliasing::OptionalInputParams &params,
  const CameraParams &camera,
  const CameraParams &prev_camera)
{
  nv::DlssParams dlssParams = {};
  dlssParams.inColor = in_color;
  dlssParams.inDepth = params.depth;
  dlssParams.inMotionVectors = params.motion;
  dlssParams.inExposure = params.exposure;
  dlssParams.inSharpness = dlss_sharpness.get();
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