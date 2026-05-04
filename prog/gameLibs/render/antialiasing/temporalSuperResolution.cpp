// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/temporalSuperResolution.h>

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_computeShaders.h>
#include <math/random/dag_halton.h>
#include <math/dag_TMatrix4D.h>
#include <debug/dag_assert.h>
#include <EASTL/iterator.h>
#include <EASTL/array.h>
#include <render/daFrameGraph/daFG.h>
#include <render/antialiasing.h>
#include <gui/dag_imgui.h>
#include <generic/dag_enumerate.h>

#include <numeric>

#include "../shaders/tsr_coeffs.hlsli"
#include "drv/3d/dag_buffers.h"
#include "drv/3d/dag_consts.h"
#include "drv/3d/dag_drv3d_multi.h"
#include "memory/dag_framemem.h"

#define SHADER_VAR_LIST                     \
  VAR(tsr_input_color)                      \
  VAR(tsr_history_color)                    \
  VAR(tsr_history_confidence)               \
  VAR(tsr_reactive_mask)                    \
  VAR(tsr_jitter_offset)                    \
  VAR(tsr_input_resolution)                 \
  VAR(tsr_output_resolution)                \
  VAR(tsr_should_restart)                   \
  VAR(tsr_debug_update_override)            \
  VAR(tsr_debug_view)                       \
  VAR(tsr_input_sampling_sigma)             \
  VAR(tsr_sharpening)                       \
  VAR(tsr_resampling_loss_sigma)            \
  VAR(tsr_depth_overhang_sigma)             \
  VAR(tsr_process_loss)                     \
  VAR(tsr_process_loss_dynamic)             \
  VAR(tsr_debug)                            \
  VAR(tsr_scale_base)                       \
  VAR(tsr_scale_motion_steepness)           \
  VAR(tsr_scale_motion_max)                 \
  VAR(tsr_scale_base_dynamic)               \
  VAR(tsr_scale_motion_steepness_dynamic)   \
  VAR(tsr_scale_motion_max_dynamic)         \
  VAR(tsr_history_reconstruction)           \
  VAR(tsr_uv_transform)                     \
  VAR(tsr_vrs_mask)                         \
  VAR(tsr_quality)                          \
  VAR(tsr_phase_configuration)              \
  VAR(tsr_use_precalculated_filter_weights) \
  VAR(tsr_kernel_weights)

#define VAR(a) static int a##_var_id = -1;
SHADER_VAR_LIST
#undef VAR

static void init_shader_vars()
{
#define VAR(a) a##_var_id = ::get_shader_variable_id(#a, true);
  SHADER_VAR_LIST
#undef VAR
}

namespace
{
bool tsr = true;
bool tsr_use_bicubic_history_sampling = true;
float tsr_input_sampling_sigma = 0.47f;
float tsr_sharpening = 0.5f;
float tsr_resampling_loss_sigma = 1.0f;
float tsr_depth_overhang_sigma = 0.01f;
float tsr_process_loss = 0.99f;
float tsr_debug_update_override = 1.0f;
int tsr_debug_view = 1;
float tsr_scale_base = 1.5f;
float tsr_scale_motion_steepness = 1e2f;
float tsr_scale_motion_max = 0.1f;
bool allow_precalculated_filter_weights = true;

// Low preset only
float tsr_process_loss_dynamic = 0.93f;
float tsr_scale_base_dynamic = 1.2f;
float tsr_scale_motion_steepness_dynamic = 1e2f;
float tsr_scale_motion_max_dynamic = 1000.0f;
} // namespace

static bool is_tsr_debug_window_open();

TemporalSuperResolution::Preset TemporalSuperResolution::parse_preset(bool vr_mode)
{
  using P = TemporalSuperResolution::Preset;
  if (vr_mode)
    return P::Vr;
  else
    return P(clamp(::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("tsrQuality", eastl::to_underlying(P::High)),
      eastl::to_underlying(P::Low), eastl::to_underlying(P::Vr)));
}

namespace
{

void apply_preset(TemporalSuperResolution::Preset preset)
{
  using P = TemporalSuperResolution::Preset;
  tsr_resampling_loss_sigma = preset == P::Vr ? 2.0f : 1.0f;
  tsr_use_bicubic_history_sampling = preset == P::High;
}

float fracf(float v) { return v - floorf(v); }
struct PhaseConfiguration
{
  int Px, Qx, Py, Qy;
};

using Phases = dag::Vector<Weights, framemem_allocator>;

PhaseConfiguration calculate_phase_configuration(const IPoint2 &input_resolution, const IPoint2 &output_resolution)
{
  // for now we are using fixed configuration, but we can calculate it based on input/output resolution ratio and other parameters
  PhaseConfiguration config;
  int gcdx = std::gcd(input_resolution.x, output_resolution.x);
  int gcdy = std::gcd(input_resolution.y, output_resolution.y);
  config.Px = output_resolution.x / gcdx;
  config.Qx = input_resolution.x / gcdx;
  config.Py = output_resolution.y / gcdy;
  config.Qy = input_resolution.y / gcdy;
  return config;
}

template <int N>
constexpr eastl::array<int, N> generate_taps()
{
  static_assert(N % 2 == 1, "N must be odd");

  eastl::array<int, N> result;

  for (int i = 0; i < N; ++i)
    result[i] = i - N / 2;

  return result;
}

template <int NTaps, typename F>
void calculate_filter_params(Phases &result, const PhaseConfiguration &config, const Point2 &jitter, F kernel)
{
  const Point2 upsamplingRatio((float)config.Px / (float)config.Qx, (float)config.Py / (float)config.Qy);

  for (int py = 0; py < config.Py; py++)
  {
    // Keep this phase/tap mapping in sync with prefetchCacheColor()/reconstructInput() in tsr.dshl.
    const float ay = fracf(((float)py + 0.5f * (float)config.Qy) / (float)config.Py + jitter.y);
    const float texelCoordDiffMulY = (ay - 0.5f) * upsamplingRatio.y;
    for (int px = 0; px < config.Px; ++px)
    {
      const float ax = fracf(((float)px + 0.5f * (float)config.Qx) / (float)config.Px + jitter.x);
      const float texelCoordDiffMulX = (ax - 0.5f) * upsamplingRatio.x;

      Weights &weights = result[py * config.Px + px];
      weights.sum = 0.0f;

      constexpr eastl::array<int, NTaps> taps = generate_taps<NTaps>();

      for (auto [j, tapY] : enumerate(taps))
      {
        const float dy = -(float)tapY * upsamplingRatio.y + texelCoordDiffMulY;
        for (auto [i, tapX] : enumerate(taps))
        {
          const float dx = -(float)tapX * upsamplingRatio.x + texelCoordDiffMulX;
          const float v = kernel(Point2(dx, dy));
          weights.w[j * NTaps + i] = v;
          weights.sum += v;
        }
      }

      for (auto &tap : weights.w)
        tap /= weights.sum;
    }
  }
}

} // namespace

TemporalSuperResolution::TemporalSuperResolution(const IPoint2 &input_resolution, const IPoint2 &output_resolution, bool vr_mode) :
  inputResolution(input_resolution),
  outputResolution(output_resolution),
  render_cs(new_compute_shader("tsr_cs")),
  render_vr_cs(new_compute_shader("tsr_vr_cs", true)),
  lodBias(-log2(float(output_resolution.y) / float(input_resolution.y))),
  preset(parse_preset(vr_mode))
{
  apply_preset(preset);
  init_shader_vars();

  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = d3d::AddressMode::Clamp;
  smpInfo.filter_mode = d3d::FilterMode::Linear;
  d3d::SamplerHandle bilinearSmp = d3d::request_sampler(smpInfo);
  smpInfo.filter_mode = d3d::FilterMode::Point;
  d3d::SamplerHandle pointSmp = d3d::request_sampler(smpInfo);
  ShaderGlobal::set_sampler(get_shader_variable_id("tsr_input_color_samplerstate"), vr_mode ? bilinearSmp : pointSmp);
  ShaderGlobal::set_sampler(get_shader_variable_id("tsr_history_color_samplerstate"), bilinearSmp);
  ShaderGlobal::set_sampler(get_shader_variable_id("tsr_history_confidence_samplerstate"), bilinearSmp);
  ShaderGlobal::set_sampler(get_shader_variable_id("tsr_reactive_mask_samplerstate"), bilinearSmp);
}

void TemporalSuperResolution::releaseShaderResources()
{
  // Must be called before destruction to release ShaderGlobal's managed-res reference on
  // filterWeightsBuf and immediately evict the RMGR entry. Two steps are required:
  // 1. set_buffer(nullptr) drops ShaderGlobal's acquire ref (refCount 2->1).
  // 2. filterWeightsBuf.close() drops the owner ref (refCount 1->0) and evicts the entry.
  // Without the explicit close(), eviction is deferred to ~UniqueBuf inside ~TemporalSuperResolution.
  // By that point the render thread may have already tried to register a new "tsr_kernel_weights"
  // buffer for the next TSR instance, causing "texture id already used" (PS4/PS5 repro:
  // benchmark scene reload). The D3D_FLUSH called by reinit() before reset() ensures the GPU
  // is done with the buffer before we free it here.
  ShaderGlobal::set_buffer(tsr_kernel_weights_var_id, nullptr);
  filterWeightsBuf.close();
}

dafg::NodeHandle TemporalSuperResolution::createApplierNode(const char *input_name)
{
  return dafg::register_node("tsr", DAFG_PP_NODE_SRC, [this, input_name](dafg::Registry registry) {
    auto applyCtxHndl = registry.readBlob<render::antialiasing::ApplyContext>("aa_apply_context").handle();

    unsigned int antialiasedTexFlags = TEXFMT_A16B16G16R16F | TEXCF_UNORDERED | extraTexFlags;
    auto antialiasedHndl = registry.createTexture2d("frame_after_aa", {antialiasedTexFlags, outputResolution})
                             .withHistory(dafg::History::ClearZeroOnFirstFrame)
                             .atStage(dafg::Stage::PS_OR_CS)
                             .useAs(dafg::Usage::SHADER_RESOURCE)
                             .handle();

    // we are using frame_after_aa on postProcessing nodes, thus we can optionally have a copy of the history to have correct data
    auto antialiasedHistCopyHndl = registry.readTextureHistory("frame_after_aa_copy")
                                     .atStage(dafg::Stage::PS_OR_CS)
                                     .useAs(dafg::Usage::SHADER_RESOURCE)
                                     .optional()
                                     .handle();

    auto antialiasedHistHndl =
      registry.readTextureHistory("frame_after_aa").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    auto confidenceHndl = registry.createTexture2d("tsr_confidence", {TEXFMT_R8 | TEXCF_UNORDERED, outputResolution})
                            .withHistory(dafg::History::ClearZeroOnFirstFrame)
                            .atStage(dafg::Stage::PS_OR_CS)
                            .useAs(dafg::Usage::SHADER_RESOURCE)
                            .handle();

    auto confidenceHistHndl =
      registry.readTextureHistory("tsr_confidence").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    auto frameHndl = registry.read(input_name).texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    registry.readTexture("depth_after_transparency").atStage(dafg::Stage::COMPUTE).bindToShaderVar("depth_gbuf");

    return [this, frameHndl, applyCtxHndl, antialiasedHndl, antialiasedHistHndl, antialiasedHistCopyHndl, confidenceHndl,
             confidenceHistHndl] {
      const render::antialiasing::ApplyContext &applyCtx = applyCtxHndl.ref();

      auto historyTarget = antialiasedHistCopyHndl.get() ? antialiasedHistCopyHndl.get() : antialiasedHistHndl.get();
      apply(frameHndl.get(), antialiasedHndl.get(), historyTarget, confidenceHndl.get(), confidenceHistHndl.get(),
        applyCtx.reactiveTexture, getDebugRenderTarget().getTex2D(), applyCtx.depthTexTransform, applyCtx.resetHistory,
        applyCtx.jitterPixelOffset, applyCtx.vrVrsMask, applyCtx.inputResolution);
    };
  });
}

TextureIDPair TemporalSuperResolution::getDebugRenderTarget()
{
#if DAGOR_DBGLEVEL > 0
  const bool isDebugWindowOpen = is_tsr_debug_window_open();

  if (isDebugWindowOpen && !debugTex.getTex2D())
    debugTex = dag::create_tex(nullptr, outputResolution.x, outputResolution.y, TEXFMT_R11G11B10F | TEXCF_RTARGET | TEXCF_UNORDERED, 1,
      "tsr_debug_tex", RESTAG_AA);

  if (!isDebugWindowOpen && debugTex.getTex2D())
    debugTex.close();

  return {debugTex.getTex2D(), debugTex.getTexId()};
#else
  return {};
#endif
}

void TemporalSuperResolution::apply(Texture *in_color, Texture *out_color, Texture *history_color, Texture *out_confidence,
  Texture *history_confidence, Texture *reactive_tex, Texture *debug_texture, const Point4 &uv_transform, bool reset,
  Point2 jitterPixelOffset, Texture *vrs_mask, IPoint2 input_resolution)
{
  if (!tsr)
  {
    d3d::stretch_rect(in_color, out_color);
    return;
  }

  TIME_D3D_PROFILE(tsr);

  // TODO remove this fallback when all callers are updated to pass input resolution
  input_resolution = input_resolution.x > 0 && input_resolution.y > 0 ? input_resolution : this->inputResolution;

  ShaderGlobal::set_int(tsr_should_restart_var_id, static_cast<int>(reset));
  ShaderGlobal::set_float4(tsr_jitter_offset_var_id, jitterPixelOffset.x, jitterPixelOffset.y, 0, 0);
  ShaderGlobal::set_float4(tsr_input_resolution_var_id, input_resolution.x, input_resolution.y, 0, 0);
  ShaderGlobal::set_float4(tsr_output_resolution_var_id, outputResolution.x, outputResolution.y, 0, 0);

  ShaderGlobal::set_int(tsr_quality_var_id, eastl::to_underlying(preset));
  ShaderGlobal::set_int(tsr_debug_var_id, getDebugRenderTarget().getTex2D() ? 1 : 0);
  ShaderGlobal::set_float(tsr_debug_update_override_var_id, tsr_debug_update_override);
  ShaderGlobal::set_int(tsr_debug_view_var_id, tsr_debug_view);
  ShaderGlobal::set_float(tsr_input_sampling_sigma_var_id, tsr_input_sampling_sigma);
  ShaderGlobal::set_float(tsr_sharpening_var_id, tsr_sharpening);
  ShaderGlobal::set_float(tsr_resampling_loss_sigma_var_id, tsr_resampling_loss_sigma);
  ShaderGlobal::set_float(tsr_depth_overhang_sigma_var_id, tsr_depth_overhang_sigma);
  ShaderGlobal::set_float(tsr_process_loss_var_id, tsr_process_loss);
  ShaderGlobal::set_float(tsr_process_loss_dynamic_var_id, tsr_process_loss_dynamic);
  ShaderGlobal::set_float(tsr_scale_base_var_id, tsr_scale_base);
  ShaderGlobal::set_float(tsr_scale_motion_steepness_var_id, tsr_scale_motion_steepness);
  ShaderGlobal::set_float(tsr_scale_motion_max_var_id, tsr_scale_motion_max);
  ShaderGlobal::set_float(tsr_scale_base_dynamic_var_id, tsr_scale_base_dynamic);
  ShaderGlobal::set_float(tsr_scale_motion_steepness_dynamic_var_id, tsr_scale_motion_steepness_dynamic);
  ShaderGlobal::set_float(tsr_scale_motion_max_dynamic_var_id, tsr_scale_motion_max_dynamic);
  ShaderGlobal::set_int(tsr_history_reconstruction_var_id, static_cast<int>(tsr_use_bicubic_history_sampling));

  const PhaseConfiguration phaseConfig = calculate_phase_configuration(input_resolution, outputResolution);
  ShaderGlobal::set_int4(tsr_phase_configuration_var_id, phaseConfig.Qx, phaseConfig.Qy, phaseConfig.Px, phaseConfig.Py);

  const unsigned totalPhases = phaseConfig.Px * phaseConfig.Py;
  const bool usePrecalculatedWeights = allow_precalculated_filter_weights && totalPhases <= TSR_MAX_PHASES;
  ShaderGlobal::set_int(tsr_use_precalculated_filter_weights_var_id, usePrecalculatedWeights ? 1 : 0);

  if (usePrecalculatedWeights)
  {
    if (!filterWeightsBuf)
      filterWeightsBuf =
        dag::buffers::create_one_frame_sr_structured(sizeof(Weights), TSR_MAX_PHASES, "tsr_kernel_weights", RESTAG_AA);

    auto inputSamplingKernel = [inv_sigma = 1.0f / (tsr_input_sampling_sigma * tsr_input_sampling_sigma)](Point2 p) {
      return expf(-0.5f * (p.x * p.x + p.y * p.y) * inv_sigma);
    };

    Phases phases;
    phases.resize_noinit(totalPhases);
    calculate_filter_params<TSR_TAPS>(phases, phaseConfig, jitterPixelOffset, inputSamplingKernel);
    filterWeightsBuf->updateData(0, sizeof(Weights) * phases.size(), phases.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    ShaderGlobal::set_buffer(tsr_kernel_weights_var_id, filterWeightsBuf.getBufId());

    d3d::resource_barrier({filterWeightsBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
  }
  else
  {
    ShaderGlobal::set_buffer(tsr_kernel_weights_var_id, nullptr);
  }

  d3d::resource_barrier({in_color, RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  d3d::resource_barrier({history_color, RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  d3d::resource_barrier({history_confidence, RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});

  if (reactive_tex)
  {
    d3d::resource_barrier({reactive_tex, RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  ShaderGlobal::set_texture(tsr_input_color_var_id, in_color);
  ShaderGlobal::set_texture(tsr_history_color_var_id, history_color);
  ShaderGlobal::set_texture(tsr_history_confidence_var_id, history_confidence);
  ShaderGlobal::set_texture(tsr_reactive_mask_var_id, reactive_tex);
  ShaderGlobal::set_texture(tsr_vrs_mask_var_id, vrs_mask);

  ShaderGlobal::set_float4(tsr_uv_transform_var_id, uv_transform);

  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, 0), out_color);
  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 6, VALUE, 0, 0), out_confidence);
  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 5, VALUE, 0, 0), debug_texture);

  G_ASSERT(preset != Preset::Vr || render_vr_cs);

  (preset == Preset::Vr ? render_vr_cs : render_cs)->dispatchThreads(outputResolution.x, outputResolution.y, 1);

  ShaderGlobal::set_texture(tsr_input_color_var_id, BAD_TEXTUREID);
  ShaderGlobal::set_texture(tsr_history_color_var_id, BAD_TEXTUREID);
  ShaderGlobal::set_texture(tsr_history_confidence_var_id, BAD_TEXTUREID);
  ShaderGlobal::set_texture(tsr_reactive_mask_var_id, BAD_TEXTUREID);
  ShaderGlobal::set_texture(tsr_vrs_mask_var_id, BAD_TEXTUREID);
}

#if DAGOR_DBGLEVEL > 0

#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>

static bool is_tsr_debug_window_open() { return imgui_get_state() != ImGuiState::OFF && imgui_window_is_visible("Render", "TSR"); }

static void render_imgui()
{
  ImGui::Checkbox("Enabled", &tsr);
  ImGui::Checkbox("Use Bicubic History Sampling", &tsr_use_bicubic_history_sampling);
  ImGui::Checkbox("Allow Precalculated Filter Weights", &allow_precalculated_filter_weights);
  ImGui::SliderFloat("Input Sampling Sigma", &tsr_input_sampling_sigma, 0.0f, 10.0f);
  ImGui::SliderFloat("Sharpening", &tsr_sharpening, 0.0f, 1.0f);
  ImGui::SliderFloat("Resampling Loss Sigma", &tsr_resampling_loss_sigma, 0.0f, 10.0f);
  ImGui::SliderFloat("Depth Overhang Sigma", &tsr_depth_overhang_sigma, 0.0f, 0.1f, "%.6f");
  ImGui::SliderFloat("Process Loss", &tsr_process_loss, 0.0f, 1.0f);
  ImGui::SliderFloat("Process Loss Dynamic", &tsr_process_loss_dynamic, 0.0f, 1.0f);
  ImGui::SliderFloat("Debug Update Override", &tsr_debug_update_override, 0.0f, 1.0f);
  ImGui::SliderFloat("Scale Base", &tsr_scale_base, 0.0f, 10.0f);
  ImGui::SliderFloat("Scale Motion Steepness", &tsr_scale_motion_steepness, 0.0f, 1e6f);
  ImGui::SliderFloat("Scale Motion Max", &tsr_scale_motion_max, 0.0f, 10.0f);
  ImGui::SliderFloat("Scale Base Dynamic", &tsr_scale_base_dynamic, 0.0f, 10.0f);
  ImGui::SliderFloat("Scale Motion Steepness Dynamic", &tsr_scale_motion_steepness_dynamic, 0.0f, 1e6f);
  ImGui::SliderFloat("Scale Motion Max Dynamic", &tsr_scale_motion_max_dynamic, 0.0f, 10.0f);

  if (!tsr)
    return;

  auto *tsrInstance = render::antialiasing::get_tsr();
  if (!tsrInstance)
    return;

  TextureIDPair debugTex = tsrInstance->getDebugRenderTarget();
  if (!debugTex.getTex2D())
    return;

  static const char *const debug_view_labels[] = {
    "Confidence", "Rejection", "Tonemapped color", "Variance clipping AABB scale", "History", "Rectified history"};
  const int debug_view_max = static_cast<int>(sizeof(debug_view_labels) / sizeof(debug_view_labels[0])) - 1;
  if (tsr_debug_view < 0)
    tsr_debug_view = 0;
  else if (tsr_debug_view > debug_view_max)
    tsr_debug_view = debug_view_max;
  ImGui::SliderInt("Debug View", &tsr_debug_view, 0, debug_view_max, debug_view_labels[tsr_debug_view]);
  ImGuiDagor::Image(debugTex.getId(), debugTex.getTex2D());
}

REGISTER_IMGUI_WINDOW("Render", "TSR", render_imgui);

#endif
