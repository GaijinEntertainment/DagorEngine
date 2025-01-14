// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "temporalSuperResolution.h"

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <render/temporalAA.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_hash.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/tsr_inc.hlsli>
#include <render/world/frameGraphHelpers.h>
#include <render/resourceSlot/registerAccess.h>
#include <render/viewVecs.h>
#include <util/dag_parseResolution.h>

#define SHADER_VAR_LIST                    \
  VAR(tsr_input_color)                     \
  VAR(tsr_input_color_samplerstate)        \
  VAR(tsr_history_color)                   \
  VAR(tsr_history_color_samplerstate)      \
  VAR(tsr_history_confidence)              \
  VAR(tsr_history_confidence_samplerstate) \
  VAR(tsr_jitter_offset)                   \
  VAR(tsr_input_resolution)                \
  VAR(tsr_output_resolution)               \
  VAR(tsr_should_restart)                  \
  VAR(tsr_debug_update_override)           \
  VAR(tsr_input_sampling_sigma)            \
  VAR(tsr_sharpening)                      \
  VAR(tsr_resampling_loss_sigma)           \
  VAR(tsr_depth_overhang_sigma)            \
  VAR(tsr_process_loss)                    \
  VAR(tsr_process_loss_dynamic)            \
  VAR(tsr_debug)                           \
  VAR(tsr_scale_base)                      \
  VAR(tsr_scale_motion_steepness)          \
  VAR(tsr_scale_motion_max)                \
  VAR(tsr_scale_base_dynamic)              \
  VAR(tsr_scale_motion_steepness_dynamic)  \
  VAR(tsr_scale_motion_max_dynamic)        \
  VAR(tsr_history_reconstruction)          \
  VAR(tsr_static_dynamic_mismatch_loss)

#define VAR(a) static int a##_var_id = -1;
SHADER_VAR_LIST
#undef VAR

static void init_shader_vars()
{
#define VAR(a) a##_var_id = ::get_shader_variable_id(#a, true);
  SHADER_VAR_LIST
#undef VAR
}

CONSOLE_BOOL_VAL("render", tsr, true);
CONSOLE_BOOL_VAL("render", tsr_debug, false);
CONSOLE_BOOL_VAL("render", tsr_use_bicubic_history_sampling, true);
CONSOLE_BOOL_VAL("render", tsr_allow_reactive, true);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_input_sampling_sigma, 0.47f, 0.f, 10.f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_sharpening, 0.15f, 0.f, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_resampling_loss_sigma, 1.f, 0.f, 10.f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_depth_overhang_sigma, 0.0004f, 0.f, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_process_loss, 0.99f, 0.f, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_process_loss_dynamic, 0.99f, 0.f, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_debug_update_override, 1.f, 0.f, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_scale_base, 1.5f, 0.f, 10.f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_scale_motion_steepness, 1e2f, 0.f, 1e6f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_scale_motion_max, 0.1f, 0.f, 10.f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_scale_base_dynamic, 1.5f, 0.f, 10.f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_scale_motion_steepness_dynamic, 1e2f, 0.f, 1e6f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_scale_motion_max_dynamic, 3.f, 0.f, 10.f);
CONSOLE_FLOAT_VAL_MINMAX("render", tsr_static_dynamic_mismatch_loss, 0.5f, 0.f, 1.f);

void TemporalSuperResolution::load_settings() {} // kept around for consistency's sake

bool TemporalSuperResolution::is_enabled() { return tsr.get(); }

bool TemporalSuperResolution::needMotionVectors() const { return preset == Preset::High; }

bool TemporalSuperResolution::supportsReactiveMask() const { return preset == Preset::High && tsr_allow_reactive.get(); }

TemporalSuperResolution::Preset TemporalSuperResolution::parse_preset()
{
  using P = TemporalSuperResolution::Preset;
  return P(clamp(::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("tsrQuality", eastl::to_underlying(P::High)),
    eastl::to_underlying(P::Low), eastl::to_underlying(P::High)));
}

static void apply_preset(TemporalSuperResolution::Preset preset)
{
  using P = TemporalSuperResolution::Preset;
  tsr_scale_base_dynamic.set(preset == P::High ? 1.5f : 1.2f);
  tsr_scale_motion_max_dynamic.set(preset == P::High ? 0.1f : 1000.0f);
  tsr_process_loss_dynamic.set(preset == P::High ? 0.99f : 0.93f);
  tsr_depth_overhang_sigma.set(preset == P::High ? 0.0004f : 0.00006f);
  tsr_use_bicubic_history_sampling.set(preset == P::High);
  tsr_static_dynamic_mismatch_loss.set(preset == P::High ? 0.9f : 0.0f);
}

TemporalSuperResolution::TemporalSuperResolution(const IPoint2 &output_resolution) :
  AntiAliasing(parse_preset() == Preset::Low ? output_resolution : computeInputResolution(output_resolution), output_resolution),
  preset(parse_preset())
{
  apply_preset(preset);

  init_shader_vars();

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(tsr_input_color_samplerstate_var_id, sampler);
    ShaderGlobal::set_sampler(tsr_history_color_samplerstate_var_id, sampler);
    ShaderGlobal::set_sampler(tsr_history_confidence_samplerstate_var_id, sampler);
  }

  computeRenderer.reset(new_compute_shader("tsr_cs"));

  applierNode = dabfg::register_node("tsr", DABFG_PP_NODE_SRC, [this, output_resolution](dabfg::Registry registry) {
    auto opaqueFinalTargetHndl = registry.readTexture("final_target_with_motion_blur")
                                   .atStage(dabfg::Stage::PS_OR_CS)
                                   .useAs(dabfg::Usage::SHADER_RESOURCE)
                                   .handle();
    read_gbuffer(registry, dabfg::Stage::PS_OR_CS);
    registry.readTexture("motion_vecs_after_transparency")
      .atStage(dabfg::Stage::PS_OR_CS)
      .bindToShaderVar("resolved_motion_vectors")
      .optional();
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("resolved_motion_vectors_samplerstate").optional();
    registry.readTexture("depth_after_transparency").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("depth_gbuf");
    registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");

    auto antialiasedHndl = registry
                             .createTexture2d("frame_after_aa", dabfg::History::ClearZeroOnFirstFrame,
                               {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED | TEXCF_RTARGET, output_resolution})
                             .atStage(dabfg::Stage::PS_OR_CS)
                             .useAs(dabfg::Usage::SHADER_RESOURCE)
                             .handle();

    auto antialiasedHistHndl =
      registry.readTextureHistory("frame_after_aa").atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    auto confidenceHndl = registry
                            .createTexture2d("tsr_confidence", dabfg::History::ClearZeroOnFirstFrame,
                              {TEXFMT_R8 | TEXCF_UNORDERED | TEXCF_RTARGET, output_resolution})
                            .atStage(dabfg::Stage::PS_OR_CS)
                            .useAs(dabfg::Usage::SHADER_RESOURCE)
                            .handle();

    auto confidenceHistHndl =
      registry.readTextureHistory("tsr_confidence").atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    registry.readTexture("reactive_mask").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("tsr_reactive_mask").optional();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    registry.readBlob<OrderingToken>("motion_vector_access_token");

    return [this, opaqueFinalTargetHndl, antialiasedHndl, antialiasedHistHndl, confidenceHndl, confidenceHistHndl, cameraHndl] {
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      tsr_render(TextureIDPair{opaqueFinalTargetHndl.view().getTex2D(), opaqueFinalTargetHndl.view().getTexId()},
        antialiasedHndl.view(), antialiasedHistHndl.view(), confidenceHndl.view(), confidenceHistHndl.view(), getDebugRenderTarget(),
        computeRenderer.get(), outputResolution);
    };
  });
}

int TemporalSuperResolution::getTemporalFrameCount() const
{
  if (!is_enabled())
    return 1;

  return AntiAliasing::getTemporalFrameCount();
}

Point2 TemporalSuperResolution::update(Driver3dPerspective &perspective)
{
  if (!is_enabled())
    return Point2(0, 0);

  Point2 frameJitterOffset = AntiAliasing::update(perspective);

  ShaderGlobal::set_color4(tsr_jitter_offset_var_id, jitterOffset.x, jitterOffset.y, 0, 0);
  ShaderGlobal::set_color4(tsr_input_resolution_var_id, inputResolution.x, inputResolution.y, 0, 0);
  ShaderGlobal::set_color4(tsr_output_resolution_var_id, outputResolution.x, outputResolution.y, 0, 0);

  const bool shouldRestart = frameCounter == 0;

  ShaderGlobal::set_int(tsr_should_restart_var_id, static_cast<int>(shouldRestart));
  ShaderGlobal::set_real(tsr_debug_update_override_var_id, tsr_debug_update_override.get());
  ShaderGlobal::set_real(tsr_input_sampling_sigma_var_id, tsr_input_sampling_sigma.get());
  ShaderGlobal::set_real(tsr_sharpening_var_id, tsr_sharpening.get());
  ShaderGlobal::set_real(tsr_resampling_loss_sigma_var_id, tsr_resampling_loss_sigma.get());
  ShaderGlobal::set_real(tsr_depth_overhang_sigma_var_id, tsr_depth_overhang_sigma.get());
  ShaderGlobal::set_real(tsr_process_loss_var_id, tsr_process_loss.get());
  ShaderGlobal::set_real(tsr_process_loss_dynamic_var_id, tsr_process_loss_dynamic.get());
  ShaderGlobal::set_int(tsr_debug_var_id, tsr_debug.get());
  ShaderGlobal::set_real(tsr_scale_base_var_id, tsr_scale_base.get());
  ShaderGlobal::set_real(tsr_scale_motion_steepness_var_id, tsr_scale_motion_steepness.get());
  ShaderGlobal::set_real(tsr_scale_motion_max_var_id, tsr_scale_motion_max.get());
  ShaderGlobal::set_real(tsr_scale_base_dynamic_var_id, tsr_scale_base_dynamic.get());
  ShaderGlobal::set_real(tsr_scale_motion_steepness_dynamic_var_id, tsr_scale_motion_steepness_dynamic.get());
  ShaderGlobal::set_real(tsr_scale_motion_max_dynamic_var_id, tsr_scale_motion_max_dynamic.get());
  ShaderGlobal::set_int(tsr_history_reconstruction_var_id, tsr_use_bicubic_history_sampling.get());
  ShaderGlobal::set_real(tsr_static_dynamic_mismatch_loss_var_id, tsr_static_dynamic_mismatch_loss.get());

  return frameJitterOffset;
}

Texture *TemporalSuperResolution::getDebugRenderTarget()
{
  if (tsr_debug.get() && !debugTex.getTex2D())
    debugTex = dag::create_tex(nullptr, outputResolution.x, outputResolution.y, TEXFMT_R11G11B10F | TEXCF_RTARGET | TEXCF_UNORDERED, 1,
      "tsr_debug_tex");

  if (!tsr_debug.get() && debugTex.getTex2D())
    debugTex.close();

  return debugTex.getTex2D();
}

void tsr_render(const TextureIDPair &in_color,
  ManagedTexView out_color,
  ManagedTexView history_color,
  ManagedTexView out_confidence,
  ManagedTexView history_confidence,
  Texture *debug_texture,
  const ComputeShaderElement *compute_shader,
  IPoint2 output_resolution)
{

  if (!TemporalSuperResolution::is_enabled())
  {
    d3d::stretch_rect(in_color.getTex2D(), out_color.getTex2D());
    return;
  }

  TIME_D3D_PROFILE(tsr);

  d3d::resource_barrier({in_color.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({history_color.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({history_confidence.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  ShaderGlobal::set_texture(tsr_input_color_var_id, in_color.getId());
  ShaderGlobal::set_texture(tsr_history_color_var_id, history_color.getTexId());
  ShaderGlobal::set_texture(tsr_history_confidence_var_id, history_confidence.getTexId());

  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, 0), out_color.getTex2D());
  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 6, VALUE, 0, 0), out_confidence.getTex2D());
  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 5, VALUE, 0, 0), debug_texture);
  compute_shader->dispatchThreads(output_resolution.x, output_resolution.y, 1);
}
