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

namespace objectmotionblur
{
struct ResContext
{
  UniqueTexHolder tileMaxTex;
  UniqueTexHolder neightborMaxTex;
  UniqueTexHolder resultTex;
  UniqueTexHolder flattenedVelocityTex;
};

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

  eastl::unique_ptr<ResContext> resCtx;
};

static eastl::unique_ptr<Context> g_ctx;
static MotionBlurSettings settingsFromConfig;
static IPoint2 renderingResolution = IPoint2(0, 0);
static int targetFormat = 0;

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
  }
}

static void initialize()
{
  if (!is_enabled() || (!settingsFromConfig.externalTextures && renderingResolution == IPoint2(0, 0)))
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

  if (!settingsFromConfig.externalTextures)
  {
    if (!g_ctx->resCtx)
      g_ctx->resCtx = eastl::make_unique<ResContext>();

    int width = renderingResolution.x;
    int height = renderingResolution.y;

    int tileCountX = ceilf((float)width / TILE_SIZE);
    int tileCountY = ceilf((float)height / TILE_SIZE);

    if (g_ctx->resCtx->tileMaxTex)
      g_ctx->resCtx->tileMaxTex.close();

    if (g_ctx->resCtx->neightborMaxTex)
      g_ctx->resCtx->neightborMaxTex.close();

    if (g_ctx->resCtx->resultTex)
      g_ctx->resCtx->resultTex.close();

    if (g_ctx->resCtx->flattenedVelocityTex)
      g_ctx->resCtx->flattenedVelocityTex.close();

    g_ctx->resCtx->tileMaxTex =
      dag::create_tex(nullptr, tileCountX, tileCountY, TEXCF_UNORDERED | TEXFMT_G16R16F, 1, "object_motion_blur_tile_max_tex");
    g_ctx->resCtx->neightborMaxTex =
      dag::create_tex(nullptr, tileCountX, tileCountY, TEXCF_UNORDERED | TEXFMT_G16R16F, 1, "object_motion_blur_neighbor_max");
    g_ctx->resCtx->resultTex =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | targetFormat, 1, "object_motion_blur_out_tex");

    g_ctx->resCtx->flattenedVelocityTex =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R11G11B10F, 1, "object_motion_blur_flattened_vel_tex");
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      ShaderGlobal::set_sampler(::get_shader_variable_id("object_motion_blur_flattened_vel_tex_samplerstate", true),
        d3d::request_sampler(smpInfo));
    }

    if (!g_ctx->resCtx->tileMaxTex || !g_ctx->resCtx->neightborMaxTex || !g_ctx->resCtx->resultTex ||
        !g_ctx->resCtx->flattenedVelocityTex)
    {
      logerr("Failed to create object motion blur textures!");
      g_ctx.reset();
      return;
    }
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

void on_render_resolution_changed(const IPoint2 &rendering_resolution, int format)
{
  renderingResolution = rendering_resolution;
  targetFormat = format;
  initialize();
  update_context_settings();
}

void apply(Texture *source_tex, TEXTUREID source_id, float current_frame_rate)
{
  if (!g_ctx || !g_ctx->resCtx || !source_tex)
    return;

  if (g_ctx->resCtx->resultTex)
  {
    TextureInfo sourceTextureInfo;
    source_tex->getinfo(sourceTextureInfo);

    TextureInfo resultTextureInfo;
    g_ctx->resCtx->resultTex->getinfo(resultTextureInfo);
    if (sourceTextureInfo.w != resultTextureInfo.w || sourceTextureInfo.h != resultTextureInfo.h ||
        (sourceTextureInfo.cflg & TEXFMT_MASK) != (resultTextureInfo.cflg & TEXFMT_MASK))
    {
      // Ensure that the result texture has the same size and format as the source texture
      targetFormat = sourceTextureInfo.cflg & TEXFMT_MASK;
      renderingResolution = IPoint2(sourceTextureInfo.w, sourceTextureInfo.h);

      initialize();
    }
  }

  apply(source_tex, source_id, g_ctx->resCtx->resultTex, current_frame_rate);
}

void apply(Texture *source_tex, TEXTUREID source_id, ManagedTexView resultTex, float current_frame_rate)
{
  if (!should_apply())
  {
    return;
  }

  float frame_time_mul = current_frame_rate / g_ctx->target_frame_rate;
  frame_time_mul = min(frame_time_mul, 3.0f); // If the frame rate is too low, we don't want to blur too much

  TIME_D3D_PROFILE(objectmotionblur::apply);

  ShaderGlobal::set_texture(g_ctx->inTexVarId, source_id);
  ShaderGlobal::set_real(g_ctx->velocityMulVarId, g_ctx->blurAmount);
  ShaderGlobal::set_real(g_ctx->maxVelPxVarId, g_ctx->maxBlurPx);
  ShaderGlobal::set_int(g_ctx->maxSamplesVarId, ((g_ctx->maxSamples + 3) / 4) * 4);
  ShaderGlobal::set_real(g_ctx->frameRateMulVarId, frame_time_mul);
  ShaderGlobal::set_color4(g_ctx->rampingPowVarId, g_ctx->rampStrength, g_ctx->rampCutoff);
  ShaderGlobal::set_real(g_ctx->vignetteStrengthVarId, g_ctx->vignetteStrength);
  ShaderGlobal::set_real(g_ctx->useAllSamplesVarId, g_ctx->useAllSamples ? 1.0f : 0.0f);
  ShaderGlobal::set_int(g_ctx->cancelCameraMotionVarId, g_ctx->cancelCameraMotion ? 1 : 0);

  TextureInfo resultTi;
  resultTex->getinfo(resultTi);

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

  if (source_tex && resultTex)
    source_tex->update(resultTex.getTex2D());
}

#if DAGOR_DBGLEVEL > 0
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
}

REGISTER_IMGUI_WINDOW("Render", "Motion Blur", imguiWindow);
#endif // DAGOR_DBGLEVEL > 0

} // namespace objectmotionblur