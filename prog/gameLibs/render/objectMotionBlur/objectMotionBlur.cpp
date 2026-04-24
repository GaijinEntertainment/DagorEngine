// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/objectMotionBlur.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_tex3d.h>
#include <math/dag_color.h>
#include <math/integer/dag_IPoint2.h>
#include <gameRes/dag_gameResSystem.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_computeShaders.h>
#include <3d/dag_resPtr.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>
#include <util/dag_convar.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>

#if DAGOR_DBGLEVEL > 0
#define MOTION_BLUR_DEBUG_WINDOW 1
#endif

namespace objectmotionblur
{
struct Context
{
  Context();
  ~Context() = default;

  eastl::unique_ptr<ComputeShaderElement> tileMaxCs;
  eastl::unique_ptr<ComputeShaderElement> neighborMaxCs;
  eastl::unique_ptr<ComputeShaderElement> motionBlurCs;

  ShaderVariableInfo inTexSmpVarId = ShaderVariableInfo("object_motion_blur_in_tex_samplerstate");
  ShaderVariableInfo inTexVarId = ShaderVariableInfo("object_motion_blur_in_tex");
  ShaderVariableInfo velocityMulVarId = ShaderVariableInfo("object_motion_blur_velocity_mul");
  ShaderVariableInfo maxVelPxVarId = ShaderVariableInfo("object_motion_blur_max_vel_px");
  ShaderVariableInfo maxSamplesVarId = ShaderVariableInfo("object_motion_blur_max_samples");
  ShaderVariableInfo frameRateMulVarId = ShaderVariableInfo("object_motion_blur_frame_rate_mul");
  ShaderVariableInfo rampingPowVarId = ShaderVariableInfo("object_motion_blur_pow");
  ShaderVariableInfo vignetteStrengthVarId = ShaderVariableInfo("object_motion_blur_vignette_strength");
  ShaderVariableInfo useAllSamplesVarId = ShaderVariableInfo("object_motion_blur_use_all_samples");
  ShaderVariableInfo cancelCameraMotionVarId = ShaderVariableInfo("object_motion_blur_cancel_camera_motion");

  bool debugEnabled = true;
  int maxBlurPx = 32;
  float blurAmount = 1.0f;
  int maxSamples = 16;
  float rampStrength = 1.0f;
  float rampCutoff = 1.0f;
  float vignetteStrength = 0.0f;
  bool frozenInPause = false;
  bool useAllSamples = false;
  bool cancelCameraMotion = false;

  const float target_frame_rate = 60.0f;
};

static eastl::unique_ptr<Context> g_ctx;
static MotionBlurSettings settingsFromConfig;

Context::Context()
{
  tileMaxCs.reset(new_compute_shader("object_motion_blur_tile_max"));
  if (!tileMaxCs)
  {
    return;
  }

  neighborMaxCs.reset(new_compute_shader("object_motion_blur_neighbor_max"));
  if (!neighborMaxCs)
  {
    return;
  }

  motionBlurCs.reset(new_compute_shader("object_motion_blur"));
  if (!motionBlurCs)
  {
    return;
  }

  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  smpInfo.filter_mode = d3d::FilterMode::Linear;
  ShaderGlobal::set_sampler(inTexSmpVarId, d3d::request_sampler(smpInfo));
}

static void update_context_settings()
{
  if (g_ctx)
  {
    g_ctx->maxBlurPx = settingsFromConfig.maxBlurPx;
    g_ctx->maxSamples = settingsFromConfig.maxSamples;
    g_ctx->rampCutoff = settingsFromConfig.rampCutoff;
    g_ctx->rampStrength = settingsFromConfig.rampStrength;
    g_ctx->vignetteStrength = settingsFromConfig.vignetteStrength;
    g_ctx->blurAmount = settingsFromConfig.strength;
    g_ctx->cancelCameraMotion = settingsFromConfig.cancelCameraMotion;
    g_ctx->useAllSamples = settingsFromConfig.useAllSamples;
  }
}

static void initialize()
{
  if (!is_enabled())
  {
    g_ctx.reset();
    return;
  }

  if (!g_ctx)
  {
    g_ctx = eastl::make_unique<Context>();
  }

  if (!g_ctx || !g_ctx->tileMaxCs || !g_ctx->neighborMaxCs || !g_ctx->motionBlurCs)
  {
    logerr("Failed to initialize object motion blur shaders!");
    g_ctx.reset();
    return;
  }
}

static bool should_apply()
{
  if (!g_ctx || !g_ctx->debugEnabled || !g_ctx->tileMaxCs || !g_ctx->neighborMaxCs || !g_ctx->motionBlurCs)
  {
    return false;
  }

  return true;
}

static bool (*save_config_callback)(const MotionBlurSettings &settings) = nullptr;
void set_save_config_callback(bool (*fn_save_config)(const MotionBlurSettings &settings)) { save_config_callback = fn_save_config; }

bool is_enabled() { return settingsFromConfig.strength > 0.0f; }
void on_settings_changed(const MotionBlurSettings &settings)
{
  bool wasEnabled = is_enabled();
  settingsFromConfig = settings;
  if (wasEnabled != is_enabled())
  {
    initialize();
  }
  update_context_settings();
}

bool is_frozen_in_pause()
{
  if (!is_enabled())
  {
    return false;
  }

  return g_ctx->frozenInPause;
}

bool needs_transparent_gbuffer() { return is_enabled() && g_ctx && g_ctx->blurAmount > 0.95f; }

void teardown()
{
  settingsFromConfig = MotionBlurSettings();
  g_ctx.reset();
}

void apply(BaseTexture *source_target_ptr, BaseTexture *resultTex, float current_frame_rate)
{
  if (!should_apply())
  {
    return;
  }

  float frame_time_mul = current_frame_rate / g_ctx->target_frame_rate;
  frame_time_mul = min(frame_time_mul, 3.0f); // If the frame rate is too low, we don't want to blur too much

  TIME_D3D_PROFILE(objectmotionblur::apply);

  ShaderGlobal::set_texture(g_ctx->inTexVarId, source_target_ptr);
  ShaderGlobal::set_float(g_ctx->velocityMulVarId, g_ctx->blurAmount);
  ShaderGlobal::set_float(g_ctx->maxVelPxVarId, g_ctx->maxBlurPx);
  ShaderGlobal::set_int(g_ctx->maxSamplesVarId, ((g_ctx->maxSamples + 3) / 4) * 4);
  ShaderGlobal::set_float(g_ctx->frameRateMulVarId, frame_time_mul);
  ShaderGlobal::set_float4(g_ctx->rampingPowVarId, g_ctx->rampStrength, g_ctx->rampCutoff);
  ShaderGlobal::set_float(g_ctx->vignetteStrengthVarId, g_ctx->vignetteStrength);
  ShaderGlobal::set_float(g_ctx->useAllSamplesVarId, g_ctx->useAllSamples ? 1.0f : 0.0f);
  ShaderGlobal::set_int(g_ctx->cancelCameraMotionVarId, g_ctx->cancelCameraMotion ? 1 : 0);

  TextureInfo resultTi{};
  if (resultTex)
  {
    resultTex->getinfo(resultTi);
  }

  {
    TIME_D3D_PROFILE(objectmotionblur::tileMax);
    g_ctx->tileMaxCs->dispatchThreads(resultTi.w, resultTi.h, 1);
  }
  {
    TIME_D3D_PROFILE(objectmotionblur::neighborMax);
    int tileCountX = ceilf((float)resultTi.w / TILE_SIZE);
    int tileCountY = ceilf((float)resultTi.h / TILE_SIZE);
    g_ctx->neighborMaxCs->dispatch(tileCountX, tileCountY, 1);
  }
  {
    TIME_D3D_PROFILE(objectmotionblur::motionBlur);
    g_ctx->motionBlurCs->dispatchThreads(resultTi.w, resultTi.h, 1);
  }

  ShaderGlobal::set_texture(g_ctx->inTexVarId, nullptr);
}
} // namespace objectmotionblur

#if MOTION_BLUR_DEBUG_WINDOW
#include <startup/dag_loadSettings.h>
#include <memory/dag_framemem.h>

namespace objectmotionblur
{
static void imguiWindow()
{
  if (!g_ctx)
  {
    ImGui::Text(
      "Motion Blur may be disabled from settings, or failed to initialize.\nSet graphics.enableMotionBlur:b=yes in a config blk!");
    return;
  }

  ImGui::Checkbox("Enabled", &g_ctx->debugEnabled);
  ImGui::SliderInt("Samples", &g_ctx->maxSamples, 1, 100, "%d", ImGuiSliderFlags_Logarithmic);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("Maximum samples used for one pixel. Actually used sample count depends on how close the pixel velocity is to "
                      "the max blur px setting.");
  }

  ImGui::SliderFloat("Blur amount", &g_ctx->blurAmount, 0, 10, "%0.2f", ImGuiSliderFlags_Logarithmic);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("Simulates exposure time. Values less than 1 are realistic. Use higher for artistic effect.");
  }

  ImGui::SliderInt("Max blur px", &g_ctx->maxBlurPx, 0, 400, "%d", ImGuiSliderFlags_Logarithmic);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("Only pixels reaching this velocity this will use all samples, so setting this too high will decrease quality.");
  }

  ImGui::SliderFloat("Ramp Cutoff", &g_ctx->rampCutoff, 1, 100, "%.1f", ImGuiSliderFlags_Logarithmic);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("Velocities greater than this will be increased, smaller will be decreased.");
  }
  ImGui::SliderFloat("Ramp Strength", &g_ctx->rampStrength, 1, 10, "%0.1f", ImGuiSliderFlags_Logarithmic);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("Value of 1 disables the ramping.");
  }
  ImGui::SliderFloat("Vignette Strength", &g_ctx->vignetteStrength, 0, 1, "%0.1f", ImGuiSliderFlags_None);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("Value of 1 disables the ramping.");
  }

  ImGui::Checkbox("Cancel camera motion", &g_ctx->cancelCameraMotion);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("Blur resulting from the camera's motion is reduced.");
  }

  ImGui::Checkbox("Force Use All Samples", &g_ctx->useAllSamples);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("Max number of samples are used for all pixels. This will decrease performance.");
  }

  ImGui::Checkbox("Frozen in Pause", &g_ctx->frozenInPause);
  if (ImGui::IsItemHovered())
  {
    ImGui::SetTooltip("Pausing the game will maintain the motion vectors, so the frozen frame will have motion blur applied");
  }

  if (ImGui::Button("Reset settings"))
  {
    settingsFromConfig = MotionBlurSettings();
    update_context_settings();
  }

  if (save_config_callback)
  {
    static double saveResultTime = -1.0;
    static bool lastSaveResult = false;
    if (ImGui::Button("Save settings"))
    {
      MotionBlurSettings settingsToSave;
      settingsToSave.maxBlurPx = g_ctx->maxBlurPx;
      settingsToSave.maxSamples = g_ctx->maxSamples;
      settingsToSave.strength = g_ctx->blurAmount;
      settingsToSave.rampCutoff = g_ctx->rampCutoff;
      settingsToSave.rampStrength = g_ctx->rampStrength;
      settingsToSave.vignetteStrength = g_ctx->vignetteStrength;
      settingsToSave.cancelCameraMotion = g_ctx->cancelCameraMotion;
      settingsToSave.useAllSamples = g_ctx->useAllSamples;

      lastSaveResult = save_config_callback(settingsToSave);
      saveResultTime = ImGui::GetTime();
    }

    if (saveResultTime > 0.0f && ImGui::GetTime() - saveResultTime < 2.0f)
    {
      ImGui::SameLine();
      ImGui::Text("Settings saved to config blk");
    }
    else
      saveResultTime = -1.0;
  }
}

REGISTER_IMGUI_WINDOW("Render", "Motion Blur", imguiWindow);
} // namespace objectmotionblur
#endif // MOTION_BLUR_DEBUG_WINDOW