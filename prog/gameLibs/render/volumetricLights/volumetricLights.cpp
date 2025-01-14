// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/volumetricLights/volumetricLights.h>
#include "shaders/volfog_common_def.hlsli"

#include <shaders/dag_computeShaders.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_lockTexture.h>
#include <3d/dag_gpuConfig.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_lock.h>
#include <render/viewVecs.h>
#include <3d/dag_textureIDHolder.h> // for noiseTex.h // TODO: refactor it
#include <render/noiseTex.h>
#include <render/nodeBasedShader.h>
#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <math/random/dag_halton.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <poisson/poisson_buffer_helper.h>

#include <util/dag_convar.h> //DEBUG!!!!!


namespace var
{
static ShaderVariableInfo prev_globtm_psf_0("prev_globtm_psf_0");
static ShaderVariableInfo prev_globtm_psf_1("prev_globtm_psf_1");
static ShaderVariableInfo prev_globtm_psf_2("prev_globtm_psf_2");
static ShaderVariableInfo prev_globtm_psf_3("prev_globtm_psf_3");
static ShaderVariableInfo prev_world_view_pos("prev_world_view_pos", true);
static ShaderVariableInfo globtm_psf_0("globtm_psf_0");
static ShaderVariableInfo globtm_psf_1("globtm_psf_1");
static ShaderVariableInfo globtm_psf_2("globtm_psf_2");
static ShaderVariableInfo globtm_psf_3("globtm_psf_3");
static ShaderVariableInfo prev_initial_inscatter("prev_initial_inscatter");
static ShaderVariableInfo prev_initial_inscatter_samplerstate("prev_initial_inscatter_samplerstate");
static ShaderVariableInfo prev_initial_extinction("prev_initial_extinction");
static ShaderVariableInfo prev_initial_extinction_samplerstate("prev_initial_extinction_samplerstate");
static ShaderVariableInfo initial_media_samplerstate("initial_media_samplerstate");
static ShaderVariableInfo volfog_prev_range_ratio("volfog_prev_range_ratio");
static ShaderVariableInfo volfog_froxel_volume_res("volfog_froxel_volume_res");
static ShaderVariableInfo inv_volfog_froxel_volume_res("inv_volfog_froxel_volume_res");
static ShaderVariableInfo volfog_froxel_range_params("volfog_froxel_range_params");
static ShaderVariableInfo jitter_ray_offset("jitter_ray_offset");
static ShaderVariableInfo volfog_occlusion("volfog_occlusion");
static ShaderVariableInfo prev_volfog_occlusion("prev_volfog_occlusion");
static ShaderVariableInfo prev_distant_fog_inscatter("prev_distant_fog_inscatter");
static ShaderVariableInfo prev_distant_fog_inscatter_samplerstate("prev_distant_fog_inscatter_samplerstate");
static ShaderVariableInfo prev_distant_fog_reconstruction_weight("prev_distant_fog_reconstruction_weight");
static ShaderVariableInfo prev_distant_fog_reconstruction_weight_samplerstate("prev_distant_fog_reconstruction_weight_samplerstate");
static ShaderVariableInfo distant_fog_result_inscatter("distant_fog_result_inscatter");
static ShaderVariableInfo distant_fog_result_inscatter_samplerstate("distant_fog_result_inscatter_samplerstate");
static ShaderVariableInfo distant_fog_raymarch_resolution("distant_fog_raymarch_resolution");
static ShaderVariableInfo distant_fog_reconstruction_resolution("distant_fog_reconstruction_resolution");
static ShaderVariableInfo fog_raymarch_frame_id("fog_raymarch_frame_id");
static ShaderVariableInfo distant_fog_local_view_z("distant_fog_local_view_z");
static ShaderVariableInfo prev_distant_fog_raymarch_start_weights("prev_distant_fog_raymarch_start_weights");
static ShaderVariableInfo prev_distant_fog_raymarch_start_weights_samplerstate("prev_distant_fog_raymarch_start_weights_samplerstate");
static ShaderVariableInfo volfog_media_fog_input_mul("volfog_media_fog_input_mul");
static ShaderVariableInfo volfog_blended_slice_cnt("volfog_blended_slice_cnt");
static ShaderVariableInfo mip_gen_input_tex("mip_gen_input_tex");
static ShaderVariableInfo mip_gen_input_tex_samplerstate("mip_gen_input_tex_samplerstate");
static ShaderVariableInfo mip_gen_input_tex_size("mip_gen_input_tex_size");
static ShaderVariableInfo volfog_blended_slice_start_depth("volfog_blended_slice_start_depth");
static ShaderVariableInfo distant_fog_use_smart_raymarching_pattern("distant_fog_use_smart_raymarching_pattern");
static ShaderVariableInfo distant_fog_reconstruction_params_0("distant_fog_reconstruction_params_0");
static ShaderVariableInfo distant_fog_reconstruction_params_1("distant_fog_reconstruction_params_1");
static ShaderVariableInfo distant_fog_disable_occlusion_check("distant_fog_disable_occlusion_check");
static ShaderVariableInfo distant_fog_disable_temporal_filtering("distant_fog_disable_temporal_filtering");
static ShaderVariableInfo distant_fog_disable_unfiltered_blurring("distant_fog_disable_unfiltered_blurring");
static ShaderVariableInfo distant_fog_reconstruct_current_frame_bilaterally("distant_fog_reconstruct_current_frame_bilaterally");
static ShaderVariableInfo distant_fog_reprojection_type("distant_fog_reprojection_type");
static ShaderVariableInfo distant_fog_use_stable_filtering("distant_fog_use_stable_filtering");
static ShaderVariableInfo distant_fog_raymarch_params_0("distant_fog_raymarch_params_0");
static ShaderVariableInfo distant_fog_raymarch_params_1("distant_fog_raymarch_params_1");
static ShaderVariableInfo distant_fog_raymarch_params_2("distant_fog_raymarch_params_2");
static ShaderVariableInfo channel_swizzle_indices("channel_swizzle_indices");
static ShaderVariableInfo froxel_fog_dispatch_mode("froxel_fog_dispatch_mode");
static ShaderVariableInfo froxel_fog_fading_params("froxel_fog_fading_params");
static ShaderVariableInfo volfog_hardcoded_input_type("volfog_hardcoded_input_type");
static ShaderVariableInfo distant_fog_use_static_shadows("distant_fog_use_static_shadows");
static ShaderVariableInfo prev_volfog_weight("prev_volfog_weight");
static ShaderVariableInfo static_shadows_cascades("static_shadows_cascades");
static ShaderVariableInfo froxel_fog_use_experimental_offscreen_reprojection("froxel_fog_use_experimental_offscreen_reprojection");
static ShaderVariableInfo volfog_occlusion_rw("volfog_occlusion_rw");
static ShaderVariableInfo volfog_occlusion_shadow_rw("volfog_occlusion_shadow_rw");
static ShaderVariableInfo volfog_gi_sampling_mode("volfog_gi_sampling_mode");
static ShaderVariableInfo global_time_phase("global_time_phase");

static ShaderVariableInfo volfog_shadow_res("volfog_shadow_res", true);
static ShaderVariableInfo prev_volfog_shadow("prev_volfog_shadow", true);
static ShaderVariableInfo prev_volfog_shadow_samplerstate("prev_volfog_shadow_samplerstate", true);
static ShaderVariableInfo volfog_shadow_accumulation_factor("volfog_shadow_accumulation_factor", true);
static ShaderVariableInfo volfog_shadow("volfog_shadow", true);
static ShaderVariableInfo volfog_shadow_samplerstate("volfog_shadow_samplerstate", true);
static ShaderVariableInfo view_result_inscatter_samplerstate("view_result_inscatter_samplerstate", true);
} // namespace var


#define VOLFOG_CONST_LIST                               \
  VAR(volfog_ff_weight_const_no)                        \
  VAR(volfog_ff_accumulated_inscatter_const_no)         \
  VAR(volfog_ff_initial_inscatter_const_no)             \
  VAR(volfog_ff_initial_extinction_const_no)            \
  VAR(volfog_df_reconstruct_inscatter_const_no)         \
  VAR(volfog_df_reconstruct_weight_const_no)            \
  VAR(volfog_df_reconstruct_reprojection_dist_const_no) \
  VAR(volfog_df_reconstruct_debug_tex_const_no)         \
  VAR(volfog_df_mipgen_start_weight_const_no)           \
  VAR(volfog_df_mipgen_inscatter_const_no)              \
  VAR(volfog_df_mipgen_reprojection_dist_const_no)      \
  VAR(volfog_ss_accumulated_shadow_const_no)            \
  VAR(volfog_df_raymarch_inscatter_const_no)            \
  VAR(volfog_df_raymarch_extinction_const_no)           \
  VAR(volfog_df_raymarch_dist_const_no)                 \
  VAR(volfog_df_raymarch_start_weights_const_no)        \
  VAR(volfog_df_raymarch_debug_tex_const_no)            \
  VAR(volfog_ff_initial_media_const_no)

#define VAR(a) static int a = -1;
VOLFOG_CONST_LIST
#undef VAR

static void init_shader_vars()
{
#define VAR(a)                             \
  {                                        \
    int tmp = get_shader_variable_id(#a);  \
    if (tmp >= 0)                          \
      a = ShaderGlobal::get_int_fast(tmp); \
  }
  VOLFOG_CONST_LIST
#undef VAR
}

static constexpr int FROXEL_ONLY_BLENDED_SLICE_CNT = 4;

// only for testing
CONSOLE_BOOL_VAL("volfog", disable_node_based_input, false);

CONSOLE_BOOL_VAL("distant_fog", use_smart_raymarching_pattern, true);

CONSOLE_BOOL_VAL("distant_fog", disable_occlusion_check, false);
CONSOLE_BOOL_VAL("distant_fog", disable_temporal_filtering, false);
CONSOLE_BOOL_VAL("distant_fog", disable_unfiltered_blurring, false);
CONSOLE_BOOL_VAL("distant_fog", reconstruct_current_frame_bilaterally, true);
CONSOLE_BOOL_VAL("distant_fog", enable_raymarch_opt_empty_ray, true);
CONSOLE_BOOL_VAL("distant_fog", enable_raymarch_opt_ray_start, true);
CONSOLE_BOOL_VAL("distant_fog", use_stable_filtering, true);

CONSOLE_BOOL_VAL("volfog", froxel_fog_enable_pixel_shader, false); // TODO: enable it after fixing visual artefacts
CONSOLE_BOOL_VAL("volfog", froxel_fog_enable_linear_accumulation, true);

CONSOLE_FLOAT_VAL_MINMAX("volfog", volfog_shadow_voxel_size, 8.0f, 0.1f, 20.0f);
CONSOLE_FLOAT_VAL_MINMAX("volfog", volfog_shadow_accumulation_factor, 0.9f, 0.0f, 0.999f);

CONSOLE_BOOL_VAL("volfog", use_fixed_froxel_fog_range, true);

CONSOLE_FLOAT_VAL_MINMAX("volfog", froxel_fog_fading_range_ratio, 0.3f, 0.0f, 1.0f);

CONSOLE_BOOL_VAL("volfog", volfog_gi_sampling_disable, false);
CONSOLE_BOOL_VAL("volfog", volfog_gi_sampling_use_filter, false);

CONSOLE_INT_VAL("distant_fog", raymarch_substep_mul, 8, 1, 10);
CONSOLE_INT_VAL("distant_fog", reprojection_type, 2, 0, 2); // {default, block_based, bilateral}
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", raymarch_substep_start, 100, 0, 200);
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", raymarch_start_volfog_slice_offset, 8.0f, 0.0f, 64.0f);
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", smart_raymarching_pattern_depth_bias, 2.0f, 0.0f, 100.0f);
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", raymarch_step_size, 32, 8, 256);
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", raymarch_occlusion_threshold, 0.1, 0, 4);
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", empty_ray_occlusion_weight_mip_bias, 0, 0, 4); // mostly for testing

// lower means better quality, but potentially more expensive with fast camera
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", empty_ray_occlusion_weight_mip_scale, 1.0f, 0.1f, 4.0f);

CONSOLE_FLOAT_VAL_MINMAX("distant_fog", filtering_min_threshold, 0.005f, 0.001f, 0.05f);
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", filtering_max_threshold, 0.05f, 0.01f, 0.5f);
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", occlusion_threshold, 0.1f, 0.001f, 1.0f);
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", limit_pow_factor, 8.0f, 1.0f, 100.0f);


CONSOLE_FLOAT_VAL_MINMAX("render", volfog_media_fog_input_mul, 1.0f, 0.0f, 2.0f); // for testing


// used only when distant fog is active:
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", froxel_fog_range, 300.0f, 1.0f, 2000.0f);
CONSOLE_INT_VAL("distant_fog", volfog_blended_slice_cnt, 12, 1, 64);

CONSOLE_FLOAT_VAL_MINMAX("distant_fog", max_range, 5000.0f, 2000.0f, 20000.0f);
CONSOLE_FLOAT_VAL_MINMAX("distant_fog", fading_dist, 600.0f, 0.01f, 5000.0f);


bool VolumeLight::IS_SUPPORTED = true;

static const eastl::array<float3, 32> POISSON_SAMPLES_32 = {
#define NUM_POISSON_SAMPLES 32
#include "../shaders/poisson_256_content.hlsl"
#undef NUM_POISSON_SAMPLES
};


static eastl::unique_ptr<NodeBasedShader> create_node_based_shader(NodeBasedShaderFogVariant variant)
{
  return eastl::move(eastl::make_unique<NodeBasedShader>(NodeBasedShaderType::Fog, String(::get_shader_name(NodeBasedShaderType::Fog)),
    String(::get_shader_suffix(NodeBasedShaderType::Fog)), static_cast<uint32_t>(variant)));
}

VolumeLight::VolumeLight() {}

VolumeLight::~VolumeLight() { close(); }

void VolumeLight::close()
{
  switchOff();
  if (isReady)
    release_l8_64_noise();
  isReady = false;
}

void VolumeLight::beforeReset()
{
  fogIsValid = false;
  switchOff();
  if (nodeBasedFroxelFogFillMediaCs)
    nodeBasedFroxelFogFillMediaCs->closeShader();
  if (nodeBasedDistantFogRaymarchCs)
    nodeBasedDistantFogRaymarchCs->closeShader();
  if (nodeBasedFogShadowCs)
    nodeBasedFogShadowCs->closeShader();
}

void VolumeLight::afterReset()
{
  fogIsValid = false;
  switchOff();
  for (int i = 0; i < initialInscatter.count; ++i)
    d3d::resource_barrier({initialInscatter[i].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  for (int i = 0; i < initialExtinction.count; ++i)
    d3d::resource_barrier({initialExtinction[i].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});

  if (hasDistantFog())
  {
    for (int i = 0; i < distantFogFrameReconstruct.count; ++i)
      d3d::resource_barrier({distantFogFrameReconstruct[i].getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    for (int i = 0; i < distantFogRaymarchStartWeights.count; ++i)
      d3d::resource_barrier({distantFogRaymarchStartWeights[i].getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({distantFogFrameRaymarchInscatter.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({distantFogFrameRaymarchExtinction.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({distantFogFrameRaymarchDist.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  if (nodeBasedFroxelFogFillMediaCs)
    nodeBasedFroxelFogFillMediaCs->reset();
  if (nodeBasedDistantFogRaymarchCs)
    nodeBasedDistantFogRaymarchCs->reset();
  if (nodeBasedFogShadowCs)
    nodeBasedFogShadowCs->reset();
}


bool VolumeLight::isDistantFogEnabled() const
{
  bool bValidNBS = nodeBasedDistantFogRaymarchCs && nodeBasedDistantFogRaymarchCs->isValid();
  return hasDistantFog() && (bValidNBS || disable_node_based_input);
}

void VolumeLight::resetDistantFog()
{
  distantFogFrameRaymarchInscatter.close();
  distantFogFrameRaymarchExtinction.close();
  distantFogFrameRaymarchDist.close();
  distantFogFrameRaymarchDebug.close();
  distantFogFrameReprojectionDist.close();
  for (int i = 0; i < FRAME_HISTORY; i++)
  {
    distantFogFrameReconstruct[i].close();
    distantFogRaymarchStartWeights[i].close();
    distantFogFrameReconstructionWeight[i].close();
  }
  distantFogDebug.close();
}

void VolumeLight::resetVolfogShadow()
{
  for (int i = 0; i < volfogShadow.count; ++i)
    volfogShadow[i].close();
  volfogShadowOcclusion.close();
}

static IPoint3 calc_froxel_resolution(const IPoint3 &orig_res, VolumeLight::VolfogQuality volfog_quality)
{
  return volfog_quality == VolumeLight::VolfogQuality::HighQuality ? (orig_res * 2) : orig_res;
}

bool VolumeLight::hasDistantFog() const { return !!distantFogFrameReconstruct[0]; }
bool VolumeLight::hasVolfogShadows() const { return !!volfogShadow[0]; }


static int get_shadervar_interval(const ShaderVariableInfo &shadervar_id)
{
  return ShaderGlobal::is_var_assumed(shadervar_id) ? ShaderGlobal::get_interval_assumed_value(shadervar_id)
                                                    : ShaderGlobal::get_int(shadervar_id);
}

static bool has_distant_fog_static_shadows()
{
  return get_shadervar_interval(var::static_shadows_cascades) == 2; // DF static shadow sampling only works with 2 cascades
}

static bool has_froxel_fog_experimental_offscreen_reprojection()
{
  return get_shadervar_interval(var::froxel_fog_use_experimental_offscreen_reprojection) >= 1;
}

static bool need_froxel_fog_experimental_offscreen_reprojection(VolumeLight::DistantFogQuality df_quality)
{
  return df_quality != VolumeLight::DistantFogQuality::DisableDistantFog; // it is tied to DF for now, for lack of better option
}

void VolumeLight::onSettingsChange(VolfogQuality volfog_quality, VolfogShadowCasting shadow_casting, DistantFogQuality df_quality)
{
  if (!canUseVolfogShadow())
    shadow_casting = VolfogShadowCasting::No;

  bool needExperimentalVolfogSettings = need_froxel_fog_experimental_offscreen_reprojection(df_quality);
  bool currentExperimentalVolfogSettings = has_froxel_fog_experimental_offscreen_reprojection();
  ShaderGlobal::set_int(var::froxel_fog_use_experimental_offscreen_reprojection, needExperimentalVolfogSettings ? 1 : 0);
  bool hasExperimentalVolfogSettingsChanged =
    currentExperimentalVolfogSettings != has_froxel_fog_experimental_offscreen_reprojection();

  bool bInvalidate = false;
  if (volfogQuality != volfog_quality || hasExperimentalVolfogSettingsChanged)
  {
    volfogQuality = volfog_quality;
    froxelResolution = calc_froxel_resolution(froxelOrigResolution, volfogQuality);
    initFroxelFog();
    bInvalidate = true;
  }

  enableVolfogShadows = shadow_casting == VolfogShadowCasting::Yes;
  if (hasVolfogShadows() != enableVolfogShadows)
  {
    resetVolfogShadow();
    if (enableVolfogShadows)
      initVolfogShadow();
    bInvalidate = true;
  }

  enableDistantFog = df_quality != DistantFogQuality::DisableDistantFog;
  if (hasDistantFog() != enableDistantFog)
  {
    resetDistantFog();
    if (enableDistantFog)
      initDistantFog();
    bInvalidate = true;
  }

  if (bInvalidate)
    invalidate();
}

bool VolumeLight::updateShaders(const String &shader_name, const DataBlock &shader_blk, String &out_errors)
{
  if (!nodeBasedFroxelFogFillMediaCs)
    nodeBasedFroxelFogFillMediaCs = create_node_based_shader(NodeBasedShaderFogVariant::Froxel);
  if (!nodeBasedFroxelFogFillMediaCs->update(shader_name, shader_blk, out_errors))
    return false;

  if (!nodeBasedDistantFogRaymarchCs)
    nodeBasedDistantFogRaymarchCs = create_node_based_shader(NodeBasedShaderFogVariant::Raymarch);
  if (!nodeBasedDistantFogRaymarchCs->update(shader_name, shader_blk, out_errors))
    return false;

  if (!nodeBasedFogShadowCs)
    nodeBasedFogShadowCs = create_node_based_shader(NodeBasedShaderFogVariant::Shadow);
  if (!nodeBasedFogShadowCs->update(shader_name, shader_blk, out_errors))
    return false;

  return true;
}

void VolumeLight::initShaders(const DataBlock &shader_blk)
{
  closeShaders();
  if (shader_blk.getBlockByName("height_fog", 0))
    logerr(
      "Deprecated \"height_fog\" entries are used in the level blk! Remove them and convert them to node based shader graph nodes!");
  if (!shader_blk.getStr("rootFogGraph", nullptr))
  {
    nodeBasedFroxelFogFillMediaCs.reset();
    nodeBasedDistantFogRaymarchCs.reset();
    nodeBasedFogShadowCs.reset();
    return;
  }
  if (!nodeBasedFroxelFogFillMediaCs)
    nodeBasedFroxelFogFillMediaCs = create_node_based_shader(NodeBasedShaderFogVariant::Froxel);
  nodeBasedFroxelFogFillMediaCs->init(shader_blk);

  if (!nodeBasedDistantFogRaymarchCs)
    nodeBasedDistantFogRaymarchCs = create_node_based_shader(NodeBasedShaderFogVariant::Raymarch);
  nodeBasedDistantFogRaymarchCs->init(shader_blk);

  if (!nodeBasedFogShadowCs)
    nodeBasedFogShadowCs = create_node_based_shader(NodeBasedShaderFogVariant::Shadow);
  nodeBasedFogShadowCs->init(shader_blk);
}

void VolumeLight::enableOptionalShader(const String &shader_name, bool enable)
{
  if (nodeBasedFroxelFogFillMediaCs)
    nodeBasedFroxelFogFillMediaCs->enableOptionalGraph(shader_name, enable);
  if (nodeBasedDistantFogRaymarchCs)
    nodeBasedDistantFogRaymarchCs->enableOptionalGraph(shader_name, enable);
  if (nodeBasedFogShadowCs)
    nodeBasedFogShadowCs->enableOptionalGraph(shader_name, enable);
}

void VolumeLight::volfogMediaInjectionStart(uint32_t shader_stage)
{
  d3d::set_rwtex(shader_stage, volfog_ff_initial_media_const_no, initialMedia.getVolTex(), 0, 0);
}

void VolumeLight::volfogMediaInjectionEnd(uint32_t shader_stage)
{
  d3d::set_rwtex(shader_stage, volfog_ff_initial_media_const_no, nullptr, 0, 0);
  d3d::resource_barrier({initialMedia.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
}

static bool gpuHasUMA()
{
  return d3d_get_gpu_cfg().integrated; // TODO: use a driver command for UMA instead
}

void VolumeLight::init()
{
  init_shader_vars();

  preferLinearAccumulation = gpuHasUMA(); // can use special optimization for igpus
  close();

  constexpr uint32_t FROXEL_FOG_DITHERING_SEED = 666 * 666;
  generate_poission_points(froxelFogDitheringSamples, FROXEL_FOG_DITHERING_SEED, VOLFOG_DITHERING_POISSON_SAMPLE_CNT,
    "froxel_fog_dithering_samples_buf", "froxel_fog_dithering_samples_buf", Point2::ONE, Point2(-0.5f, -0.5f));

  froxelFogLightCalcPropagatePs.init("froxel_fog_light_calc_propagate_ps");

  froxelFogLightCalcPropagateCs.reset(new_compute_shader("froxel_fog_light_calc_propagate_cs"));
  G_ASSERT_RETURN(froxelFogLightCalcPropagateCs, );
  froxelFogOcclusionCs.reset(new_compute_shader("froxel_fog_occlusion_cs"));
  G_ASSERT_RETURN(froxelFogOcclusionCs, );
  froxelFogFillMediaCs.reset(new_compute_shader("froxel_fog_fill_media_cs"));
  G_ASSERT_RETURN(froxelFogFillMediaCs, );

  // volfog shadow is optional, eg. bareMinimum cannot afford it
  volfogShadowCs.reset(new_compute_shader("volfog_shadow_cs", true));

  distantFogRaymarchCs.reset(new_compute_shader("distant_fog_raymarch_cs"));
  G_ASSERT_RETURN(distantFogRaymarchCs, );
  distantFogReconstructCs.reset(new_compute_shader("distant_fog_reconstruct_cs"));
  G_ASSERT_RETURN(distantFogReconstructCs, );
  distantFogStartWeightMipGenerationCs.reset(new_compute_shader("volfog_df_start_weight_mipgen_cs"));
  G_ASSERT_RETURN(distantFogStartWeightMipGenerationCs, );
  distantFogFxMipGenerationCs.reset(new_compute_shader("volfog_df_fx_mipgen_cs"));
  G_ASSERT_RETURN(distantFogFxMipGenerationCs, );

  init_and_get_l8_64_noise();

  int poissonCnt = POISSON_SAMPLES_32.count; // in case of multiple arrays in the same texture, it must be the sum of them (and handled
                                             // from shader ofc)
  poissonSamples = dag::create_tex(nullptr, poissonCnt, 1, TEXFMT_R32F, 1, "volfog_poisson_samples");
  poissonSamples.getTex2D()->texaddr(TEXADDR_CLAMP);
  poissonSamples.getTex2D()->texfilter(TEXFILTER_POINT);

  if (auto lockedTex = lock_texture<float>(poissonSamples.getTex2D(), 0, TEXLOCK_WRITE))
  {
    for (int i = 0, pos = 0; i < POISSON_SAMPLES_32.count; ++i)
      lockedTex.at(i, 0) = POISSON_SAMPLES_32[i].z * 0.5 + 0.5;
  }

  isReady = true;

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    var::prev_volfog_shadow_samplerstate.set_sampler(sampler);
    var::volfog_shadow_samplerstate.set_sampler(sampler);
    var::prev_distant_fog_inscatter_samplerstate.set_sampler(sampler);
    var::distant_fog_result_inscatter_samplerstate.set_sampler(sampler);
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    var::prev_initial_inscatter_samplerstate.set_sampler(sampler);
    var::prev_initial_extinction_samplerstate.set_sampler(sampler);
    var::initial_media_samplerstate.set_sampler(sampler);
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
    smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack; // very important to avoid getting stuck in false-positive
                                                                      // non-empty rays
    smpInfo.mip_map_mode = d3d::MipMapMode::Linear;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    var::prev_distant_fog_raymarch_start_weights_samplerstate.set_sampler(sampler);
    var::mip_gen_input_tex_samplerstate.set_sampler(sampler);
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    var::prev_distant_fog_reconstruction_weight_samplerstate.set_sampler(sampler);
  }
}

void VolumeLight::initVolfogShadow()
{
  IPoint3 volfogShadowRes = getVolfogShadowRes();
  for (int i = 0; i < volfogShadow.count; ++i)
  {
    String name(128, "volfog_shadow_%d", i);
    volfogShadow[i] =
      dag::create_voltex(volfogShadowRes.x, volfogShadowRes.y, volfogShadowRes.z, TEXFMT_L16 | TEXCF_UNORDERED, 1, name);
    volfogShadow[i]->disableSampler();
    d3d::resource_barrier({volfogShadow[i].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  volfogShadowOcclusion =
    dag::create_tex(nullptr, volfogShadowRes.x, volfogShadowRes.y, TEXFMT_R8 | TEXCF_UNORDERED, 1, "volfog_shadow_occlusion");
  volfogShadowOcclusion.getTex2D()->texfilter(TEXFILTER_POINT);
  volfogShadowOcclusion.getTex2D()->texaddr(TEXADDR_CLAMP);
  d3d::resource_barrier({volfogShadowOcclusion.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
}

void VolumeLight::initFroxelFog()
{
  G_ASSERT_RETURN(froxelResolution.x > 0 && froxelResolution.y > 0 && froxelResolution.z > 0, );

  const int reservedSliceCnt = 1; // last layer is special-case: no scattering

  ShaderGlobal::set_color4(var::volfog_froxel_volume_res, froxelResolution.x, froxelResolution.y, froxelResolution.z, 0);
  // need to pass inverse too for node based shader (no preshader there)
  ShaderGlobal::set_color4(var::inv_volfog_froxel_volume_res, 1.0f / froxelResolution.x, 1.0f / froxelResolution.y,
    1.0f / froxelResolution.z, 0);

  for (int i = 0; i < initialInscatter.count; ++i)
  {
    String name_rgb(128, "view_initial_inscatter_rgb%d", i);
    String name_a(128, "view_initial_inscatter_a%d", i);
    initialInscatter[i] =
      dag::create_voltex(froxelResolution.x, froxelResolution.y, froxelResolution.z, TEXFMT_R11G11B10F | TEXCF_UNORDERED, 1, name_rgb);
    initialInscatter[i]->disableSampler();

    initialExtinction[i] =
      dag::create_voltex(froxelResolution.x, froxelResolution.y, froxelResolution.z, TEXFMT_R11G11B10F | TEXCF_UNORDERED, 1, name_a);
    initialExtinction[i]->disableSampler();
  }

  initialMedia = dag::create_voltex(froxelResolution.x, froxelResolution.y, froxelResolution.z, TEXFMT_A16B16G16R16F | TEXCF_UNORDERED,
    1, "initial_media");
  initialMedia->disableSampler();
  initialMedia.setVar();

  resultInscatter = dag::create_voltex(froxelResolution.x, froxelResolution.y, froxelResolution.z + reservedSliceCnt,
    TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1, "view_result_inscatter");
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    var::view_result_inscatter_samplerstate.set_sampler(d3d::request_sampler(smpInfo));
  }
  resultInscatter->disableSampler();
  resultInscatter.setVar();

  for (int i = 0; i < volfogOcclusion.count; ++i)
  {
    String name(128, "volfog_occlusion_%d", i);
    volfogOcclusion[i] = dag::create_tex(nullptr, froxelResolution.x, froxelResolution.y, TEXFMT_R8 | TEXCF_UNORDERED, 1, name);
    volfogOcclusion[i].getTex2D()->texfilter(TEXFILTER_POINT);
    volfogOcclusion[i].getTex2D()->texaddr(TEXADDR_CLAMP);
    d3d::resource_barrier({volfogOcclusion[i].getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  for (int i = 0; i < volfogWeight.count; ++i)
    volfogWeight[i].close();
  if (has_froxel_fog_experimental_offscreen_reprojection())
  {
    for (int i = 0; i < volfogWeight.count; ++i)
    {
      String name(128, "volfog_weight%d", i);
      volfogWeight[i] =
        dag::create_voltex(froxelResolution.x, froxelResolution.y, froxelResolution.z, TEXFMT_R8 | TEXCF_UNORDERED, 1, name);
      volfogWeight[i].getVolTex()->texaddr(TEXADDR_CLAMP);
      d3d::resource_barrier({volfogWeight[i].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    }
  }
}

void VolumeLight::initDistantFog()
{
  G_ASSERT_RETURN(reconstructionFrameRes.x > 0 && reconstructionFrameRes.y > 0, );

  raymarchFrameRes = DEBUG_DISTANT_FOG_DISABLE_4_WAY_RECONSTRUCTION ? reconstructionFrameRes : (reconstructionFrameRes / 2);
  ShaderGlobal::set_color4(var::distant_fog_raymarch_resolution, raymarchFrameRes.x, raymarchFrameRes.y, 1.0 / raymarchFrameRes.x,
    1.0 / raymarchFrameRes.y);

  d3d::SamplerHandle clampPointSampler;
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    clampPointSampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(get_shader_variable_id("distant_fog_raymarch_inscatter_samplerstate", true), clampPointSampler);
    ShaderGlobal::set_sampler(get_shader_variable_id("distant_fog_raymarch_extinction_samplerstate", true), clampPointSampler);
  }
  distantFogFrameRaymarchInscatter = dag::create_tex(nullptr, raymarchFrameRes.x, raymarchFrameRes.y,
    TEXFMT_A2B10G10R10 | TEXCF_UNORDERED, 1, "distant_fog_raymarch_inscatter");
  distantFogFrameRaymarchInscatter->disableSampler();

  distantFogFrameRaymarchExtinction = dag::create_tex(nullptr, raymarchFrameRes.x, raymarchFrameRes.y, TEXFMT_R16F | TEXCF_UNORDERED,
    1, "distant_fog_raymarch_extinction");
  distantFogFrameRaymarchExtinction->disableSampler();

#if DEBUG_DISTANT_FOG_RAYMARCH
  distantFogFrameRaymarchDebug = dag::create_tex(nullptr, raymarchFrameRes.x, raymarchFrameRes.y,
    TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1, "distant_fog_raymarch_debug");
  ShaderGlobal::set_sampler(get_shader_variable_id("distant_fog_raymarch_debug_samplerstate", true), clampPointSampler);
  distantFogFrameRaymarchDebug->disableSampler();
#endif

  static constexpr int FX_RESULT_MIP_CNT = 1;

  for (int i = 0; i < distantFogFrameReconstruct.count; ++i)
  {
    String name(128, "distant_fog_inscatter_%d", i);
    distantFogFrameReconstruct[i] = dag::create_tex(nullptr, reconstructionFrameRes.x, reconstructionFrameRes.y,
      TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1 + FX_RESULT_MIP_CNT, name);
    distantFogFrameReconstruct[i]->disableSampler();
  }

  for (int i = 0; i < distantFogFrameReconstructionWeight.count; ++i)
  {
    String name(128, "distant_fog_reconstruction_weight_%d", i);
    distantFogFrameReconstructionWeight[i] =
      dag::create_tex(nullptr, reconstructionFrameRes.x, reconstructionFrameRes.y, TEXFMT_R8 | TEXCF_UNORDERED, 1, name);
    distantFogFrameReconstructionWeight[i]->disableSampler();
  }

  distantFogFrameReprojectionDist = dag::create_tex(nullptr, reconstructionFrameRes.x, reconstructionFrameRes.y,
    TEXFMT_R16F | TEXCF_UNORDERED, 1 + FX_RESULT_MIP_CNT, "distant_fog_reprojection_dist");
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(get_shader_variable_id("distant_fog_reprojection_dist_samplerstate", true),
      d3d::request_sampler(smpInfo));
  }
  distantFogFrameReprojectionDist->disableSampler();

  distantFogFrameRaymarchDist.close();
  distantFogFrameRaymarchDist =
    dag::create_tex(nullptr, raymarchFrameRes.x, raymarchFrameRes.y, TEXFMT_R16F | TEXCF_UNORDERED, 1, "distant_fog_raymarch_dist");
  ShaderGlobal::set_sampler(get_shader_variable_id("distant_fog_raymarch_dist_samplerstate", true), clampPointSampler);
  distantFogFrameRaymarchDist->disableSampler();

  for (int i = 0; i < distantFogRaymarchStartWeights.count; ++i)
  {
    String name(128, "distant_fog_raymarch_start_weights_%d", i);
    distantFogRaymarchStartWeights[i] = dag::create_tex(nullptr, raymarchFrameRes.x, raymarchFrameRes.y, TEXFMT_R8G8 | TEXCF_UNORDERED,
      DISTANT_FOG_OCCLUSION_WEIGHT_MIP_CNT, name);
    distantFogRaymarchStartWeights[i]->disableSampler();
  }

#if DEBUG_DISTANT_FOG_RECONSTRUCT
  distantFogDebug.close();
  distantFogDebug = dag::create_tex(nullptr, reconstructionFrameRes.x, reconstructionFrameRes.y,
    TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1, "distant_fog_debug");
  ShaderGlobal::set_sampler(get_shader_variable_id("distant_fog_debug_samplerstate", true), clampPointSampler);
  distantFogDebug->disableSampler();
  distantFogDebug.setVar();
#endif
}

void VolumeLight::setResolution(int resW, int resH, int resD, int screenW, int screenH)
{
  G_ASSERT(resW % RESULT_WARP_SIZE == 0);
  G_ASSERT(resH % RESULT_WARP_SIZE == 0);
  G_ASSERT(resD % INITIAL_WARP_SIZE_Z == 0);

  froxelOrigResolution = IPoint3(resW, resH, resD);

  IPoint3 targetFroxelResolution = calc_froxel_resolution(froxelOrigResolution, volfogQuality);

  IPoint2 targetReconstructionFrameRes = IPoint2(screenW, screenH) / 2;
  bool reinitFroxelFog = targetFroxelResolution != froxelResolution;
  bool reinitVolfogShadows = enableVolfogShadows && reinitFroxelFog;
  bool reinitDistantFog = enableDistantFog && targetReconstructionFrameRes != reconstructionFrameRes;

  froxelResolution = targetFroxelResolution;
  reconstructionFrameRes = targetReconstructionFrameRes;

  ShaderGlobal::set_color4(var::distant_fog_reconstruction_resolution, reconstructionFrameRes.x, reconstructionFrameRes.y,
    1.0 / reconstructionFrameRes.x, 1.0 / reconstructionFrameRes.y);

  if (reinitFroxelFog)
    initFroxelFog();
  if (reinitVolfogShadows)
    initVolfogShadow();
  if (reinitDistantFog)
    initDistantFog();
  if (reinitFroxelFog || reinitVolfogShadows || reinitDistantFog)
    invalidate();
}

void VolumeLight::invalidate()
{
  const float clear0[4] = {0};
  const float clear1[4] = {1}; // clear with maximum occlusion (255), so empty voxels are cleared with (0,0,0,1) at first
  for (int i = 0; i < initialInscatter.count; ++i)
  {
    d3d::clear_rwtexf(initialInscatter[i].getVolTex(), clear0, 0, 0);
    d3d::resource_barrier({initialInscatter[i].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    d3d::clear_rwtexf(initialExtinction[i].getVolTex(), clear0, 0, 0);
    d3d::resource_barrier({initialExtinction[i].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }
  d3d::clear_rwtexf(initialMedia.getVolTex(), clear0, 0, 0);
  d3d::clear_rwtexf(resultInscatter.getVolTex(), clear0, 0, 0);
  for (int i = 0; i < volfogOcclusion.count; ++i)
  {
    if (volfogOcclusion[i])
    {
      d3d::GpuAutoLock gpuLock;
      d3d::clear_rwtexf(volfogOcclusion[i].getTex2D(), clear1, 0, 0);
      d3d::resource_barrier({volfogOcclusion[i].getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    }
  }
  if (volfogShadowOcclusion)
  {
    d3d::GpuAutoLock gpuLock;
    d3d::clear_rwtexf(volfogShadowOcclusion.getTex2D(), clear1, 0, 0);
    d3d::resource_barrier({volfogShadowOcclusion.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }
  for (int i = 0; i < volfogWeight.count; ++i)
  {
    if (volfogWeight[i])
    {
      d3d::GpuAutoLock gpuLock;
      d3d::clear_rwtexf(volfogWeight[i].getVolTex(), clear0, 0, 0);
      d3d::resource_barrier({volfogWeight[i].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    }
  }
  for (int i = 0; i < volfogShadow.count; ++i)
  {
    if (volfogShadow[i])
    {
      d3d::GpuAutoLock gpuLock;
      d3d::clear_rwtexf(volfogShadow[i].getVolTex(), clear1, 0, 0);
      d3d::resource_barrier({volfogShadow[i].getVolTex(), RB_RO_SRV | RB_STAGE_ALL_SHADERS, 0, 0});
    }
  }
  if (hasDistantFog())
  {
    for (int i = 0; i < initialInscatter.count; ++i)
    {
      d3d::clear_rwtexf(distantFogFrameReconstruct[i].getTex2D(), clear0, 0, 0);
      d3d::resource_barrier({distantFogFrameReconstruct[i].getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      d3d::clear_rwtexf(distantFogRaymarchStartWeights[i].getTex2D(), clear0, 0, 0);
      d3d::resource_barrier({distantFogRaymarchStartWeights[i].getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    }
    d3d::clear_rwtexf(distantFogFrameReprojectionDist.getTex2D(), clear0, 0, 0);
    d3d::resource_barrier({distantFogFrameReprojectionDist.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    d3d::clear_rwtexf(distantFogFrameRaymarchInscatter.getTex2D(), clear0, 0, 0);
    d3d::resource_barrier({distantFogFrameRaymarchInscatter.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    d3d::clear_rwtexf(distantFogFrameRaymarchExtinction.getTex2D(), clear0, 0, 0);
    d3d::resource_barrier({distantFogFrameRaymarchExtinction.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    d3d::clear_rwtexf(distantFogFrameRaymarchDist.getTex2D(), clear0, 0, 0);
    d3d::resource_barrier({distantFogFrameRaymarchDist.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
#if DEBUG_DISTANT_FOG_RAYMARCH
    d3d::clear_rwtexf(distantFogFrameRaymarchDebug.getTex2D(), clear0, 0, 0);
    d3d::resource_barrier({distantFogFrameRaymarchDebug.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
#endif
  }
}

void VolumeLight::setRange(float range) { currentRange = max(0.001f, range); }

void VolumeLight::closeShaders()
{
  if (nodeBasedFroxelFogFillMediaCs)
    nodeBasedFroxelFogFillMediaCs->closeShader();
  if (nodeBasedDistantFogRaymarchCs)
    nodeBasedDistantFogRaymarchCs->closeShader();
  if (nodeBasedFogShadowCs)
    nodeBasedFogShadowCs->closeShader();
}

void VolumeLight::switchOff()
{
  ShaderGlobal::set_texture(resultInscatter.getVarId(), BAD_TEXTUREID);
  ShaderGlobal::set_texture(var::distant_fog_result_inscatter, BAD_TEXTUREID);
  ShaderGlobal::set_color4(var::volfog_froxel_range_params, 0, 0, 0, 0);
  ShaderGlobal::set_texture(var::volfog_shadow, BAD_TEXTUREID);
}

void VolumeLight::switchOn()
{
  if (!fogIsValid)
  {
    switchOff();
    return;
  }
  resultInscatter.setVar();
  ShaderGlobal::set_color4(var::volfog_froxel_range_params, currentRange, 1.0f / currentRange, 1.0f,
    static_cast<float>(isDistantFogEnabled()));
}

int VolumeLight::getBlendedSliceCnt() const
{
  return min(isDistantFogEnabled() ? volfog_blended_slice_cnt : FROXEL_ONLY_BLENDED_SLICE_CNT, froxelResolution.z);
}
float VolumeLight::calcBlendedStartDepth(int blended_slice_cnt) const
{
  if (froxelResolution.z <= 0)
    return 0.0001f;
  float startSlice = 1.0f - (float)blended_slice_cnt / froxelResolution.z;
  return (startSlice * startSlice) * currentRange;
}

bool VolumeLight::canUseLinearAccumulation() const { return froxel_fog_enable_linear_accumulation && preferLinearAccumulation; }
bool VolumeLight::canUsePixelShader() const { return canUseLinearAccumulation() && froxel_fog_enable_pixel_shader; }
bool VolumeLight::canUseVolfogShadow() const { return !!volfogShadowCs; }

bool VolumeLight::performStartFrame(const TMatrix4 &view_tm, const TMatrix4 &proj_tm, const TMatrix4_vec4 &glob_tm,
  const Point3 &camera_pos)
{
  // view vecs are stored here for all volfog passes
  set_viewvecs_to_shader(froxelFogViewVecLT, froxelFogViewVecRT, froxelFogViewVecLB, froxelFogViewVecRB, tmatrix(view_tm), proj_tm);

  useNodeBasedInput = !disable_node_based_input && nodeBasedFroxelFogFillMediaCs && nodeBasedFroxelFogFillMediaCs->isValid();

  if (!isReady)
  {
    switchOff();
    return false;
  }

  if (hasDistantFog() && useNodeBasedInput && !(nodeBasedDistantFogRaymarchCs && nodeBasedDistantFogRaymarchCs->isValid()))
  {
    logerr("Distant fog node based shaders are invalid!");
    resetDistantFog();
  }

  if (use_fixed_froxel_fog_range || isDistantFogEnabled()) // distant fog is much more stable with a fixed starting depth (depends on
                                                           // froxel range)
    currentRange = froxel_fog_range;

  switchOn(); // set range params early for volfog passes

  Point4 local_view_z = view_tm.getcol(2);
  ShaderGlobal::set_color4(var::distant_fog_local_view_z, Color4(local_view_z.x, local_view_z.y, local_view_z.z, 0));

  ShaderGlobal::set_color4(var::prev_globtm_psf_0, Color4(prevGlobTm.current()[0]));
  ShaderGlobal::set_color4(var::prev_globtm_psf_1, Color4(prevGlobTm.current()[1]));
  ShaderGlobal::set_color4(var::prev_globtm_psf_2, Color4(prevGlobTm.current()[2]));
  ShaderGlobal::set_color4(var::prev_globtm_psf_3, Color4(prevGlobTm.current()[3]));
  const int jitter_z_samples = 8;
  const int jitter_x_samples = 4;
  const int jitter_y_samples = 6;

  static const int MAX_FRAME_OFFSET = 64; // to avoid precision loss from int -> float, should be >= blue noise sample cnt
  frameId = (frameId + 1) % MAX_FRAME_OFFSET;

  distantFogOcclusionPosDiff = camera_pos - prevCameraPos;
  prevCameraPos = camera_pos;

  int frameRnd = frameId;
  _rnd(frameRnd);
  frameRnd = (_rnd(frameRnd) & 1023) + 1;
  float jitter_ray_x_offset = halton_sequence(frameRnd, jitter_x_samples);
  float jitter_ray_y_offset = halton_sequence(frameRnd, jitter_y_samples);
  float jitter_ray_z_offset = halton_sequence(frameRnd, jitter_z_samples);

  ShaderGlobal::set_color4(var::jitter_ray_offset, jitter_ray_x_offset - 0.5f, jitter_ray_y_offset - 0.5f, jitter_ray_z_offset - 0.5f,
    frameId);

  ShaderGlobal::set_real(var::volfog_prev_range_ratio, prevRange < 0.1f ? 1.f : currentRange / prevRange);

  ShaderGlobal::set_color4(var::froxel_fog_fading_params,
    (isDistantFogEnabled() || froxel_fog_fading_range_ratio <= VERY_SMALL_NUMBER)
      ? Color4(0, 1, 0, 0)
      : Color4(-1.0f / (currentRange * froxel_fog_fading_range_ratio), 1.0f / froxel_fog_fading_range_ratio, 0, 0));

  int blendedSliceCnt = getBlendedSliceCnt();
  ShaderGlobal::set_int(var::volfog_blended_slice_cnt, blendedSliceCnt);
  ShaderGlobal::set_real(var::volfog_blended_slice_start_depth, calcBlendedStartDepth(blendedSliceCnt));

  if (!useNodeBasedInput)
    ShaderGlobal::set_real(var::global_time_phase, get_shader_global_time_phase(0, 0));

  // TODO: set shadervars when params change, use pullValueChange
  ShaderGlobal::set_real(var::volfog_media_fog_input_mul, volfog_media_fog_input_mul);

  prevGlobTm.current() = glob_tm.transpose();
  prevRange = currentRange;
  return true;
}

void VolumeLight::performFroxelFogOcclusion()
{
  if (!isReady)
    return;

  set_viewvecs_to_shader(froxelFogViewVecLT, froxelFogViewVecRT, froxelFogViewVecLB, froxelFogViewVecRB);

  const int nextFrame = frameId & 1;
  const int prevFrame = 1 - nextFrame;
  const ResourceBarrier flags = RB_RO_SRV | (canUsePixelShader() ? RB_STAGE_PIXEL : RB_STAGE_COMPUTE);
  {
    TIME_D3D_PROFILE(volfog_ff_occlusion);
    STATE_GUARD(ShaderGlobal::set_texture(var::volfog_occlusion_rw, VALUE), volfogOcclusion[nextFrame].getTexId(), BAD_TEXTUREID);
    STATE_GUARD(ShaderGlobal::set_texture(var::volfog_occlusion_shadow_rw, VALUE), volfogShadowOcclusion.getTexId(), BAD_TEXTUREID);
    froxelFogOcclusionCs->dispatchThreads(froxelResolution.x, froxelResolution.y, 1);

    d3d::resource_barrier({volfogOcclusion[nextFrame].getTex2D(), flags, 0, 0});
    if (volfogShadowOcclusion)
      d3d::resource_barrier({volfogShadowOcclusion.getTex2D(), flags, 0, 0});
  }

  ShaderGlobal::set_texture(var::prev_volfog_occlusion, volfogOcclusion[prevFrame].getTexId());
  ShaderGlobal::set_texture(var::volfog_occlusion, volfogOcclusion[nextFrame].getTexId());
}

void VolumeLight::performFroxelFogFillMedia()
{
  if (!isReady)
    return;

  ShaderGlobal::set_int(var::volfog_hardcoded_input_type, disable_node_based_input ? 0 : -1);

  set_viewvecs_to_shader(froxelFogViewVecLT, froxelFogViewVecRT, froxelFogViewVecLB, froxelFogViewVecRB);

  const ResourceBarrier flags = RB_RO_SRV | (canUsePixelShader() ? RB_STAGE_PIXEL : RB_STAGE_COMPUTE);
  {
    TIME_D3D_PROFILE(volfog_ff_fill_media);
    IPoint3 dispatchRes =
      IPoint3(froxelResolution.x / MEDIA_WARP_SIZE_X, froxelResolution.y / MEDIA_WARP_SIZE_Y, froxelResolution.z / MEDIA_WARP_SIZE_Z);
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_ff_initial_media_const_no, VALUE, 0, 0), initialMedia.getVolTex());
    if (useNodeBasedInput)
      nodeBasedFroxelFogFillMediaCs->dispatch(dispatchRes.x, dispatchRes.y, dispatchRes.z);
    else
      froxelFogFillMediaCs->dispatch(dispatchRes.x, dispatchRes.y, dispatchRes.z);
  }
  d3d::resource_barrier({initialMedia.getVolTex(), flags, 0, 0});
}

void VolumeLight::performVolfogShadow()
{
  if (!isReady)
    return;

  if (hasVolfogShadows())
  {
    const int nextFrame = frameId & 1;
    const int prevFrame = 1 - nextFrame;

    set_viewvecs_to_shader(froxelFogViewVecLT, froxelFogViewVecRT, froxelFogViewVecLB, froxelFogViewVecRB);

    TIME_D3D_PROFILE(volfog_shadow);

    IPoint3 volfogShadowRes = getVolfogShadowRes();
    ShaderGlobal::set_color4(var::volfog_shadow_res, volfogShadowRes, volfog_shadow_voxel_size);

    ShaderGlobal::set_real(var::volfog_shadow_accumulation_factor, volfog_shadow_accumulation_factor);

    ShaderGlobal::set_texture(var::prev_volfog_shadow, volfogShadow[prevFrame].getTexId());

    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_ss_accumulated_shadow_const_no, VALUE, 0, 0),
      volfogShadow[nextFrame].getVolTex());

    IPoint3 dispatchRes = IPoint3(div_ceil(volfogShadowRes.x, MEDIA_WARP_SIZE_X), div_ceil(volfogShadowRes.y, MEDIA_WARP_SIZE_Y),
      div_ceil(volfogShadowRes.z, MEDIA_WARP_SIZE_Z));
    if (useNodeBasedInput && nodeBasedFogShadowCs)
      nodeBasedFogShadowCs->dispatch(dispatchRes.x, dispatchRes.y, dispatchRes.z);
    else
      volfogShadowCs->dispatch(dispatchRes.x, dispatchRes.y, dispatchRes.z);

    d3d::resource_barrier({volfogShadow[nextFrame].getVolTex(), RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0});
    ShaderGlobal::set_texture(var::volfog_shadow, volfogShadow[nextFrame].getTexId());
  }
  else
  {
    ShaderGlobal::set_texture(var::volfog_shadow, BAD_TEXTUREID);
  }
}

void VolumeLight::performDistantFogRaymarch()
{
  if (!isReady)
    return;

  if (!isDistantFogEnabled())
    return;

  ShaderGlobal::set_int(var::distant_fog_use_static_shadows, has_distant_fog_static_shadows());

  static constexpr float RAW_DEPTH_PRE_SCALE = 0.0001f; // TODO: use non-linear params instead (needs a wrapper to avoid calling pow
                                                        // every frame)

  float texelChangePerDiff = empty_ray_occlusion_weight_mip_scale * (DEBUG_DISTANT_FOG_DISABLE_4_WAY_RECONSTRUCTION ? 2 : 4);

  float occlusionWeightSampleOffset =
    clamp(length(distantFogOcclusionPosDiff) / texelChangePerDiff, 1.0f, DISTANT_FOG_OCCLUSION_WEIGHT_MAX_SAMPLE_OFFSET);
  float occlusionWeightMip = log2(occlusionWeightSampleOffset) + empty_ray_occlusion_weight_mip_bias;

  ShaderGlobal::set_color4(var::distant_fog_raymarch_params_0, enable_raymarch_opt_empty_ray, enable_raymarch_opt_ray_start,
    raymarch_substep_start, 1.0f / raymarch_substep_mul);
  ShaderGlobal::set_color4(var::distant_fog_raymarch_params_1, raymarch_start_volfog_slice_offset, raymarch_step_size,
    smart_raymarching_pattern_depth_bias, raymarch_occlusion_threshold * RAW_DEPTH_PRE_SCALE);
  ShaderGlobal::set_color4(var::distant_fog_raymarch_params_2, max_range, fading_dist, occlusionWeightMip,
    occlusionWeightSampleOffset);

  ShaderGlobal::set_color4(var::distant_fog_reconstruction_params_0, filtering_min_threshold * RAW_DEPTH_PRE_SCALE,
    filtering_max_threshold * RAW_DEPTH_PRE_SCALE, occlusion_threshold * RAW_DEPTH_PRE_SCALE, limit_pow_factor);

  ShaderGlobal::set_color4(var::distant_fog_reconstruction_params_1,
    min<float>(max_range, MAX_RAYMARCH_FOG_DIST) - DISTANT_FOG_FX_DIST_BIAS, 0, 0, 0);

  ShaderGlobal::set_int(var::distant_fog_use_smart_raymarching_pattern, use_smart_raymarching_pattern ? 1 : 0);
  ShaderGlobal::set_int(var::distant_fog_disable_occlusion_check, disable_occlusion_check ? 1 : 0);
  ShaderGlobal::set_int(var::distant_fog_disable_temporal_filtering, disable_temporal_filtering ? 1 : 0);
  ShaderGlobal::set_int(var::distant_fog_disable_unfiltered_blurring, disable_unfiltered_blurring ? 1 : 0);
  ShaderGlobal::set_int(var::distant_fog_reconstruct_current_frame_bilaterally, reconstruct_current_frame_bilaterally ? 1 : 0);
  ShaderGlobal::set_int(var::distant_fog_reprojection_type, reprojection_type);
  ShaderGlobal::set_int(var::distant_fog_use_stable_filtering, use_stable_filtering ? 1 : 0);

  raymarchFrameId = (raymarchFrameId + 1) % 4;
  ShaderGlobal::set_int(var::fog_raymarch_frame_id, raymarchFrameId);

  const int nextFrame = frameId & 1;
  const int prevFrame = 1 - nextFrame;

  {
    TIME_D3D_PROFILE(volfog_df_raymarch);

    ShaderGlobal::set_texture(var::prev_distant_fog_raymarch_start_weights, distantFogRaymarchStartWeights[prevFrame].getTexId());

    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_raymarch_inscatter_const_no, VALUE, 0, 0),
      distantFogFrameRaymarchInscatter.getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_raymarch_extinction_const_no, VALUE, 0, 0),
      distantFogFrameRaymarchExtinction.getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_raymarch_dist_const_no, VALUE, 0, 0),
      distantFogFrameRaymarchDist.getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_raymarch_start_weights_const_no, VALUE, 0, 0),
      distantFogRaymarchStartWeights[nextFrame].getTex2D());

#if DEBUG_DISTANT_FOG_RAYMARCH
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_raymarch_debug_tex_const_no, VALUE, 0, 0),
      distantFogFrameRaymarchDebug.getTex2D());
#endif

    IPoint2 dispatchRes =
      IPoint2(raymarchFrameRes.x + RAYMARCH_WARP_SIZE - 1, raymarchFrameRes.y + RAYMARCH_WARP_SIZE - 1) / RAYMARCH_WARP_SIZE;
    if (useNodeBasedInput)
      nodeBasedDistantFogRaymarchCs->dispatch(dispatchRes.x, dispatchRes.y, 1);
    else
      distantFogRaymarchCs->dispatch(dispatchRes.x, dispatchRes.y, 1);
  }

  const ResourceBarrier flags = RB_RO_SRV | RB_STAGE_COMPUTE;

  d3d::resource_barrier({distantFogFrameRaymarchInscatter.getTex2D(), flags, 0, 0});
  d3d::resource_barrier({distantFogFrameRaymarchExtinction.getTex2D(), flags, 0, 0});
  d3d::resource_barrier({distantFogFrameRaymarchDist.getTex2D(), flags, 0, 0});
#if DEBUG_DISTANT_FOG_RAYMARCH
  d3d::resource_barrier({distantFogFrameRaymarchDebug.getTex2D(), flags, 0, 0});
#endif

  {
    TIME_D3D_PROFILE(volfog_df_raymarch_mip_gen);

    const UniqueTex &mipTex = distantFogRaymarchStartWeights[nextFrame];
    ShaderGlobal::set_texture(var::mip_gen_input_tex, mipTex.getTexId());

    IPoint2 mipTexSize = raymarchFrameRes;
    for (int mip = 1; mip < DISTANT_FOG_OCCLUSION_WEIGHT_MIP_CNT; ++mip)
    {
      int prevMip = mip - 1;
      mipTex.getTex2D()->texmiplevel(prevMip, prevMip);

      ShaderGlobal::set_color4(var::mip_gen_input_tex_size,
        Color4(mipTexSize.x, mipTexSize.y, 1.0f / mipTexSize.x, 1.0f / mipTexSize.y));
      mipTexSize /= 2;

      d3d::resource_barrier({mipTex.getTex2D(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, (uint)prevMip, 1});

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_mipgen_start_weight_const_no, VALUE, 0, mip), mipTex.getTex2D());
      distantFogStartWeightMipGenerationCs->dispatchThreads(mipTexSize.x, mipTexSize.y, 1);

      d3d::resource_barrier({mipTex.getTex2D(), flags, unsigned(mip), 1});
    }
    mipTex.getTex2D()->texmiplevel(0, DISTANT_FOG_OCCLUSION_WEIGHT_MIP_CNT - 1);
    d3d::resource_barrier({mipTex.getTex2D(), flags, 0, 0});
  }
}

void VolumeLight::performFroxelFogPropagate()
{
  if (!isReady)
    return;

  const int nextFrame = frameId & 1;
  const int prevFrame = 1 - nextFrame;

  set_viewvecs_to_shader(froxelFogViewVecLT, froxelFogViewVecRT, froxelFogViewVecLB, froxelFogViewVecRB);

  int prevChannelSwizzleFrameId = froxelChannelSwizzleFrameId;
  froxelChannelSwizzleFrameId = (froxelChannelSwizzleFrameId + 1) % 3;
  ShaderGlobal::set_int4(var::channel_swizzle_indices, IPoint4(froxelChannelSwizzleFrameId, prevChannelSwizzleFrameId, 0, 0));
  ShaderGlobal::set_texture(var::prev_initial_inscatter, initialInscatter[prevFrame].getTexId());

  ShaderGlobal::set_int(var::volfog_gi_sampling_mode, volfog_gi_sampling_disable ? 1 : (volfog_gi_sampling_use_filter ? 2 : 0));

  ShaderGlobal::set_texture(var::prev_initial_extinction, initialExtinction[prevFrame].getTexId());
  if (volfogWeight[prevFrame])
    ShaderGlobal::set_texture(var::prev_volfog_weight, volfogWeight[prevFrame].getTexId());

  ShaderGlobal::set_int(var::froxel_fog_dispatch_mode, canUseLinearAccumulation() ? 1 : 0);

  {
    TIME_D3D_PROFILE(volfog_ff_combined_light_calc);
    {
      // somehow igpus (especially Intel) are much faster with fully linear accumulation, it's the opposite for dgpus
      bool usePixelShader = canUsePixelShader();
      int stage = usePixelShader ? STAGE_PS : STAGE_CS;

      STATE_GUARD_NULLPTR(d3d::set_rwtex(stage, volfog_ff_weight_const_no, VALUE, 0, 0), volfogWeight[nextFrame].getVolTex());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(stage, volfog_ff_accumulated_inscatter_const_no, VALUE, 0, 0), resultInscatter.getVolTex());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(stage, volfog_ff_initial_inscatter_const_no, VALUE, 0, 0),
        initialInscatter[nextFrame].getVolTex());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_ff_initial_extinction_const_no, VALUE, 0, 0),
        initialExtinction[nextFrame].getVolTex());

      if (usePixelShader)
      {
        SCOPE_RENDER_TARGET;
        d3d::set_render_target({}, DepthAccess::RW, {});
        d3d::setview(0, 0, froxelResolution.x, froxelResolution.y, 0, 1);
        froxelFogLightCalcPropagatePs.render();
      }
      else
      {
        froxelFogLightCalcPropagateCs->dispatchThreads(1, froxelResolution.x, froxelResolution.y);
      }
      d3d::resource_barrier({initialExtinction[nextFrame].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      d3d::resource_barrier({initialInscatter[nextFrame].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      if (volfogWeight[nextFrame])
        d3d::resource_barrier({volfogWeight[nextFrame].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    }
  }

  d3d::resource_barrier({resultInscatter.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX | RB_STAGE_COMPUTE, 0, 0});

  fogIsValid = true;
}

void VolumeLight::performDistantFogReconstruct()
{
  if (isReady && isDistantFogEnabled())
  {
    const int nextFrame = frameId & 1;
    const int prevFrame = 1 - nextFrame;

    {
      TIME_D3D_PROFILE(volfog_df_reconstruct);

      ShaderGlobal::set_texture(var::prev_distant_fog_inscatter, distantFogFrameReconstruct[prevFrame].getTexId());
      ShaderGlobal::set_texture(var::prev_distant_fog_reconstruction_weight,
        distantFogFrameReconstructionWeight[prevFrame].getTexId());

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_reconstruct_inscatter_const_no, VALUE, 0, 0),
        distantFogFrameReconstruct[nextFrame].getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_reconstruct_weight_const_no, VALUE, 0, 0),
        distantFogFrameReconstructionWeight[nextFrame].getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_reconstruct_reprojection_dist_const_no, VALUE, 0, 0),
        distantFogFrameReprojectionDist.getTex2D());
#if DEBUG_DISTANT_FOG_RECONSTRUCT
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_reconstruct_debug_tex_const_no, VALUE, 0, 0), distantFogDebug.getTex2D());
#endif

      distantFogReconstructCs->dispatchThreads(reconstructionFrameRes.x, reconstructionFrameRes.y, 1);
    }

    ShaderGlobal::set_texture(var::distant_fog_result_inscatter, distantFogFrameReconstruct[nextFrame].getTexId());
    {
      TIME_D3D_PROFILE(volfog_df_recontruct_fx_mip_gen);

      int mip = 1;
      int prevMip = mip - 1;
      distantFogFrameReprojectionDist.getTex2D()->texmiplevel(prevMip, prevMip);
      distantFogFrameReconstruct[nextFrame].getTex2D()->texmiplevel(prevMip, prevMip);

      IPoint2 mipTexSize = IPoint2(reconstructionFrameRes.x >> 1, reconstructionFrameRes.y >> 1);
      const ResourceBarrier flags = RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE;
      d3d::resource_barrier({distantFogFrameReprojectionDist.getTex2D(), flags, (uint)prevMip, 1});
      d3d::resource_barrier({distantFogFrameReconstruct[nextFrame].getTex2D(), flags, (uint)prevMip, 1});

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_mipgen_inscatter_const_no, VALUE, 0, mip),
        distantFogFrameReconstruct[nextFrame].getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, volfog_df_mipgen_reprojection_dist_const_no, VALUE, 0, mip),
        distantFogFrameReprojectionDist.getTex2D());

      distantFogFxMipGenerationCs->dispatchThreads(mipTexSize.x, mipTexSize.y, 1);

      distantFogFrameReprojectionDist.getTex2D()->texmiplevel(0, mip);
      distantFogFrameReconstruct[nextFrame].getTex2D()->texmiplevel(0, mip);
    }

    const ResourceBarrier flags = RB_STAGE_PIXEL | RB_STAGE_VERTEX | RB_RO_SRV;
    d3d::resource_barrier({distantFogFrameReconstruct[nextFrame].getTex2D(), flags, 0, 0});
    d3d::resource_barrier({distantFogFrameReconstructionWeight[nextFrame].getTex2D(), flags, 0, 0});
    d3d::resource_barrier({distantFogFrameReprojectionDist.getTex2D(), flags, 0, 0});
#if DEBUG_DISTANT_FOG_RECONSTRUCT
    d3d::resource_barrier({distantFogDebug.getTex2D(), flags, 0, 0});
#endif
  }
  else
  {
    ShaderGlobal::set_texture(var::distant_fog_result_inscatter, BAD_TEXTUREID);
  }
}

void VolumeLight::setCurrentView(int view) { prevGlobTm.setCurrentView(view); }


Frustum VolumeLight::calcFrustum(const TMatrix4 &view_tm, const Driver3dPerspective &persp) const
{
  float zf = clamp(currentRange, persp.zn + 1e-5f, persp.zf); // this may use the prev frame range, but it shouldn't be a problem
  TMatrix4 projTm = matrix_perspective_reverse(persp.wk, persp.hk, persp.zn, zf);
  projTm(2, 0) += persp.ox;
  projTm(2, 1) += persp.oy;
  return Frustum(view_tm * projTm);
}
