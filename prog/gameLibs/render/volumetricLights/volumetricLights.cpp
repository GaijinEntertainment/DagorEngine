#include <render/volumetricLights/volumetricLights.h>
#include "shaders/volume_lights_common_def.hlsli"

#include <shaders/dag_computeShaders.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_lockTexture.h>
#include <3d/dag_gpuConfig.h>
#include <render/viewVecs.h>
#include <3d/dag_textureIDHolder.h> // for noiseTex.h // TODO: refactor it
#include <render/noiseTex.h>
#include <render/nodeBasedShader.h>
#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <math/random/dag_halton.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>

#include <util/dag_convar.h> //DEBUG!!!!!

#define GLOBAL_VARS_LIST                                 \
  VAR(prev_globtm_psf_0)                                 \
  VAR(prev_globtm_psf_1)                                 \
  VAR(prev_globtm_psf_2)                                 \
  VAR(prev_globtm_psf_3)                                 \
  VAR(prev_world_view_pos)                               \
  VAR(globtm_psf_0)                                      \
  VAR(globtm_psf_1)                                      \
  VAR(globtm_psf_2)                                      \
  VAR(globtm_psf_3)                                      \
  VAR(prev_initial_inscatter)                            \
  VAR(prev_initial_extinction)                           \
  VAR(initial_inscatter)                                 \
  VAR(initial_extinction)                                \
  VAR(volfog_prev_range_ratio)                           \
  VAR(volfog_froxel_volume_res)                          \
  VAR(inv_volfog_froxel_volume_res)                      \
  VAR(volfog_froxel_range_params)                        \
  VAR(jitter_ray_offset)                                 \
  VAR(volfog_occlusion)                                  \
  VAR(prev_volfog_occlusion)                             \
  VAR(prev_distant_fog_inscatter)                        \
  VAR(prev_distant_fog_reconstruction_weight)            \
  VAR(distant_fog_result_inscatter)                      \
  VAR(distant_fog_raymarch_resolution)                   \
  VAR(distant_fog_reconstruction_resolution)             \
  VAR(fog_raymarch_frame_id)                             \
  VAR(distant_fog_local_view_z)                          \
  VAR(prev_distant_fog_raymarch_start_weights)           \
  VAR(volfog_media_fog_input_mul)                        \
  VAR(volfog_blended_slice_cnt)                          \
  VAR(mip_gen_input_tex)                                 \
  VAR(mip_gen_input_tex_size)                            \
  VAR(volfog_blended_slice_start_depth)                  \
  VAR(distant_fog_use_smart_raymarching_pattern)         \
  VAR(distant_fog_reconstruction_params_0)               \
  VAR(distant_fog_reconstruction_params_1)               \
  VAR(distant_fog_disable_occlusion_check)               \
  VAR(distant_fog_disable_temporal_filtering)            \
  VAR(distant_fog_disable_unfiltered_blurring)           \
  VAR(distant_fog_reconstruct_current_frame_bilaterally) \
  VAR(distant_fog_reprojection_type)                     \
  VAR(distant_fog_use_stable_filtering)                  \
  VAR(distant_fog_raymarch_params_0)                     \
  VAR(distant_fog_raymarch_params_1)                     \
  VAR(distant_fog_raymarch_params_2)                     \
  VAR(channel_swizzle_indices)                           \
  VAR(froxel_fog_dispatch_mode)                          \
  VAR(froxel_fog_fading_params)                          \
  VAR(froxel_fog_use_debug_media)                        \
  VAR(global_time_phase)

#define GLOBAL_VARS_LIST_OPT              \
  VAR(volfog_shadow_res)                  \
  VAR(prev_volfog_shadow)                 \
  VAR(volfog_shadow_prev_frame_tc_offset) \
  VAR(volfog_shadow)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST_OPT
#undef VAR

static void init_shader_vars()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST_OPT
#undef VAR
}

static constexpr int FROXEL_ONLY_BLENDED_SLICE_CNT = 4;

CONSOLE_BOOL_VAL("render", volfog_disable_node_based_input, false); // only for testing

// CONSOLE_BOOL_VAL("distant_fog", use_3x3_bilateral_kernel, false); // for testing (expensive) // TODO: temporarily disabled due to a
// Vulkan compiler bug

CONSOLE_BOOL_VAL("distant_fog", use_smart_raymarching_pattern, true);

CONSOLE_BOOL_VAL("distant_fog", disable_occlusion_check, false);
CONSOLE_BOOL_VAL("distant_fog", disable_temporal_filtering, false);
CONSOLE_BOOL_VAL("distant_fog", disable_unfiltered_blurring, false);
CONSOLE_BOOL_VAL("distant_fog", reconstruct_current_frame_bilaterally, true);
CONSOLE_BOOL_VAL("distant_fog", enable_raymarch_opt_empty_ray, true);
CONSOLE_BOOL_VAL("distant_fog", enable_raymarch_opt_ray_start, true);
CONSOLE_BOOL_VAL("distant_fog", use_stable_filtering, true);

CONSOLE_BOOL_VAL("volfog", froxel_fog_use_optimized_dispatch, true); // in time, the non-optimized version will be removed
CONSOLE_BOOL_VAL("volfog", froxel_fog_enable_pixel_shader, true);
CONSOLE_BOOL_VAL("volfog", froxel_fog_enable_linear_accumulation, true);

CONSOLE_FLOAT_VAL_MINMAX("volfog", volfog_shadow_voxel_size, 8.0f, 0.1f, 20.0f);
CONSOLE_FLOAT_VAL_MINMAX("volfog", volfog_shadow_accumulation_factor, 0.9f, 0.0f, 0.999f);


CONSOLE_FLOAT_VAL_MINMAX("volfog", froxel_fog_fading_range_ratio, 0.3f, 0.0f, 1.0f);

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


static const eastl::array<float3, 32> POISSON_SAMPLES_32 = {
#define NUM_POISSON_SAMPLES 32
#include "../shaders/poisson_256_content.hlsl"
#undef NUM_POISSON_SAMPLES
};

static const IPoint3 VOLFOG_SHADOW_VOLTEX_RES = IPoint3(32, 16, 32);


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
  if (nodeBasedFillMediaCs)
    nodeBasedFillMediaCs->closeShader();
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

  if (nodeBasedFillMediaCs)
    nodeBasedFillMediaCs->reset();
  if (nodeBasedDistantFogRaymarchCs)
    nodeBasedDistantFogRaymarchCs->reset();
  if (nodeBasedFogShadowCs)
    nodeBasedFogShadowCs->reset();
}


bool VolumeLight::isDistantFogEnabled() const
{
  bool bValidNBS = nodeBasedDistantFogRaymarchCs && nodeBasedDistantFogRaymarchCs->isValid();
  return hasDistantFog() && (bValidNBS || volfog_disable_node_based_input);
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
}

static IPoint3 calc_froxel_resolution(const IPoint3 &orig_res, bool is_hq) { return is_hq ? (orig_res * 2) : orig_res; }

bool VolumeLight::hasDistantFog() const { return !!distantFogFrameReconstruct[0]; }
bool VolumeLight::hasVolfogShadows() const { return !!volfogShadow[0]; }

void VolumeLight::onCompatibilityModeChange(bool use_compatiblity_mode) { useCompatibilityMode = use_compatiblity_mode; }

void VolumeLight::onSettingsChange(bool has_hq_fog, bool has_distant_fog, bool has_volfog_shadow)
{
  if (nodeBasedFillMediaCs)
    nodeBasedFillMediaCs->reset();
  if (nodeBasedDistantFogRaymarchCs)
    nodeBasedDistantFogRaymarchCs->reset();
  if (nodeBasedFogShadowCs)
    nodeBasedFogShadowCs->reset();

  bool bInvalidate = false;
  if (hq_volfog != has_hq_fog)
  {
    hq_volfog = has_hq_fog;
    froxelResolution = calc_froxel_resolution(froxelOrigResolution, hq_volfog);
    initFroxelFog();
    bInvalidate = true;
  }

  if (hasVolfogShadows() != has_volfog_shadow)
  {
    if (!has_volfog_shadow)
      resetVolfogShadow();
    else
      initVolfogShadow();
    bInvalidate = true;
  }

  if (hasDistantFog() != has_distant_fog)
  {
    if (!has_distant_fog)
      resetDistantFog();
    else
      initDistantFog();
    bInvalidate = true;
  }

  if (bInvalidate)
    invalidate();
}

bool VolumeLight::updateShaders(const String &shader_name, const DataBlock &shader_blk, String &out_errors)
{
  if (!nodeBasedFillMediaCs)
    nodeBasedFillMediaCs = create_node_based_shader(NodeBasedShaderFogVariant::Froxel);
  if (!nodeBasedFillMediaCs->update(shader_name, shader_blk, out_errors))
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
    nodeBasedFillMediaCs.reset();
    nodeBasedDistantFogRaymarchCs.reset();
    nodeBasedFogShadowCs.reset();
    return;
  }
  if (!nodeBasedFillMediaCs)
    nodeBasedFillMediaCs = create_node_based_shader(NodeBasedShaderFogVariant::Froxel);
  nodeBasedFillMediaCs->init(shader_blk);

  if (!nodeBasedDistantFogRaymarchCs)
    nodeBasedDistantFogRaymarchCs = create_node_based_shader(NodeBasedShaderFogVariant::Raymarch);
  nodeBasedDistantFogRaymarchCs->init(shader_blk);

  if (!nodeBasedFogShadowCs)
    nodeBasedFogShadowCs = create_node_based_shader(NodeBasedShaderFogVariant::Shadow);
  nodeBasedFogShadowCs->init(shader_blk);
}

void VolumeLight::enableOptionalShader(const String &shader_name, bool enable)
{
  if (nodeBasedFillMediaCs)
    nodeBasedFillMediaCs->enableOptionalGraph(shader_name, enable);
  if (nodeBasedDistantFogRaymarchCs)
    nodeBasedDistantFogRaymarchCs->enableOptionalGraph(shader_name, enable);
  if (nodeBasedFogShadowCs)
    nodeBasedFogShadowCs->enableOptionalGraph(shader_name, enable);
}

static bool gpuHasUMA()
{
  return d3d_get_gpu_cfg().integrated; // TODO: use a driver command for UMA instead
}

void VolumeLight::init()
{
  preferLinearAccumulation = gpuHasUMA(); // can use special optimization for igpus
  close();
  shaderHelperUtils = texture_util::get_shader_helper();

  viewResultInscatterPs.init("view_initial_inscatter_ps");

  viewResultInscatterCs.reset(new_compute_shader("view_result_inscatter_cs"));
  G_ASSERT_RETURN(viewResultInscatterCs, );
  viewInitialInscatterCs.reset(new_compute_shader("view_initial_inscatter_cs"));
  G_ASSERT_RETURN(viewInitialInscatterCs, );
  volfogOcclusionCs.reset(new_compute_shader("volfog_occlusion_cs"));
  G_ASSERT_RETURN(volfogOcclusionCs, );
  fillMediaCs.reset(new_compute_shader("fill_media_cs"));
  G_ASSERT_RETURN(fillMediaCs, );

  // volfog shadow is optional, eg. bare minimum cannot afford it
  volfogShadowCs.reset(new_compute_shader("volfog_shadow_cs", true));
  clearVolfogShadowCs.reset(new_compute_shader("clear_volfog_shadow_cs", true));

  distantFogRaymarchCs.reset(new_compute_shader("distant_fog_raymarch_cs"));
  G_ASSERT_RETURN(distantFogRaymarchCs, );
  distantFogReconstructCs.reset(new_compute_shader("distant_fog_reconstruct_cs"));
  G_ASSERT_RETURN(distantFogReconstructCs, );
  distantFogMipGenerationCs.reset(new_compute_shader("volfog_mip_gen_cs"));
  G_ASSERT_RETURN(distantFogMipGenerationCs, );
  distantFogFxMipGenerationCs.reset(new_compute_shader("volume_light_distant_mip_gen_cs"));
  G_ASSERT_RETURN(distantFogFxMipGenerationCs, );

  init_shader_vars();
  init_and_get_l8_64_noise();

  int poissonCnt = POISSON_SAMPLES_32.count; // in case of multiple arrays in the same texture, it must be the sum of them (and handled
                                             // from shader ofc)
  poissonSamples = dag::create_tex(nullptr, poissonCnt, 1, TEXFMT_R32F, 1, "volfog_poisson_samples");
  poissonSamples.getTex2D()->texaddr(TEXADDR_CLAMP);
  poissonSamples.getTex2D()->texfilter(TEXFILTER_POINT);

  int stride;
  if (auto lockedTex = lock_texture<float>(poissonSamples.getTex2D(), stride, 0, TEXLOCK_WRITE))
  {
    float *data = lockedTex.get();
    for (int i = 0, pos = 0; i < POISSON_SAMPLES_32.count; ++i)
      data[i] = POISSON_SAMPLES_32[i].z * 0.5 + 0.5;
  }

  isReady = true;
}

void VolumeLight::initVolfogShadow()
{
  for (int i = 0; i < volfogShadow.count; ++i)
  {
    String name(128, "volfog_shadow_%d", i);
    volfogShadow[i] = dag::create_voltex(VOLFOG_SHADOW_VOLTEX_RES.x, VOLFOG_SHADOW_VOLTEX_RES.y, VOLFOG_SHADOW_VOLTEX_RES.z,
      TEXFMT_L16 | TEXCF_UNORDERED, 1, name);
    volfogShadow[i].getVolTex()->texfilter(TEXFILTER_SMOOTH);
    volfogShadow[i].getVolTex()->texaddr(TEXADDR_BORDER);
    volfogShadow[i].getVolTex()->texbordercolor(0xFFFFFFFF); // no shadow outside of voxel range
    d3d::resource_barrier({volfogShadow[i].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }
}

void VolumeLight::initFroxelFog()
{
  G_ASSERT_RETURN(froxelResolution.x > 0 && froxelResolution.y > 0 && froxelResolution.z > 0, );

  const int reservedSliceCnt = 1; // last layer is special-case: no scattering

  ShaderGlobal::set_color4(volfog_froxel_volume_resVarId, froxelResolution.x, froxelResolution.y, froxelResolution.z, 0);
  // need to pass inverse too for node based shader (no preshader there)
  ShaderGlobal::set_color4(inv_volfog_froxel_volume_resVarId, 1.0f / froxelResolution.x, 1.0f / froxelResolution.y,
    1.0f / froxelResolution.z, 0);

  const uint32_t phaseFmt = TEXFMT_R16F | TEXCF_UNORDERED;
  for (int i = 0; i < initialInscatter.count; ++i)
  {
    String name_rgb(128, "view_initial_inscatter_rgb%d", i);
    String name_a(128, "view_initial_inscatter_a%d", i);
    initialInscatter[i] =
      dag::create_voltex(froxelResolution.x, froxelResolution.y, froxelResolution.z, TEXFMT_R11G11B10F | TEXCF_UNORDERED, 1, name_rgb);
    initialInscatter[i].getVolTex()->texaddr(TEXADDR_CLAMP);

    initialExtinction[i] =
      dag::create_voltex(froxelResolution.x, froxelResolution.y, froxelResolution.z, TEXFMT_R11G11B10F | TEXCF_UNORDERED, 1, name_a);
    initialExtinction[i].getVolTex()->texaddr(TEXADDR_CLAMP);
  }

  initialMedia = dag::create_voltex(froxelResolution.x, froxelResolution.y, froxelResolution.z, TEXFMT_A16B16G16R16F | TEXCF_UNORDERED,
    1, "initial_media");
  initialMedia.getVolTex()->texaddr(TEXADDR_CLAMP);
  initialMedia.setVar();

#if USE_SEPARATE_PHASE
  initialPhase = dag::create_voltex(froxelResolution.x, froxelResolution.y, froxelResolution.z, TEXFMT_R16F | TEXCF_UNORDERED, 1,
    "initial_phase"); // for local lights (if present)
#endif

  resultInscatter = dag::create_voltex(froxelResolution.x, froxelResolution.y, froxelResolution.z + reservedSliceCnt,
    TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1, "view_result_inscatter");
  resultInscatter.getVolTex()->texfilter(TEXFILTER_SMOOTH);
  resultInscatter.getVolTex()->texaddr(TEXADDR_CLAMP);
  resultInscatter.setVar();

  for (int i = 0; i < volfogOcclusion.count; ++i)
  {
    String name(128, "volfog_occlusion_%d", i);
    volfogOcclusion[i] = dag::create_tex(nullptr, froxelResolution.x, froxelResolution.y, TEXFMT_R8 | TEXCF_UNORDERED, 1, name);
    volfogOcclusion[i].getTex2D()->texfilter(TEXFILTER_POINT);
    volfogOcclusion[i].getTex2D()->texaddr(TEXADDR_CLAMP);
    d3d::resource_barrier({volfogOcclusion[i].getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  if (isVolfogShadowEnabled())
    initVolfogShadow();
}

void VolumeLight::initDistantFog()
{
  G_ASSERT_RETURN(reconstructionFrameRes.x > 0 && reconstructionFrameRes.y > 0, );

  raymarchFrameRes = DEBUG_DISTANT_FOG_DISABLE_4_WAY_RECONSTRUCTION ? reconstructionFrameRes : (reconstructionFrameRes / 2);
  ShaderGlobal::set_color4(distant_fog_raymarch_resolutionVarId, raymarchFrameRes.x, raymarchFrameRes.y, 1.0 / raymarchFrameRes.x,
    1.0 / raymarchFrameRes.y);

  distantFogFrameRaymarchInscatter = dag::create_tex(nullptr, raymarchFrameRes.x, raymarchFrameRes.y,
    TEXFMT_A2B10G10R10 | TEXCF_UNORDERED, 1, "distant_fog_raymarch_inscatter");
  distantFogFrameRaymarchInscatter.getTex2D()->texaddr(TEXADDR_CLAMP);
  distantFogFrameRaymarchInscatter.getTex2D()->texfilter(TEXFILTER_POINT);

  distantFogFrameRaymarchExtinction = dag::create_tex(nullptr, raymarchFrameRes.x, raymarchFrameRes.y, TEXFMT_R16F | TEXCF_UNORDERED,
    1, "distant_fog_raymarch_extinction");
  distantFogFrameRaymarchExtinction.getTex2D()->texaddr(TEXADDR_CLAMP);
  distantFogFrameRaymarchExtinction.getTex2D()->texfilter(TEXFILTER_POINT);

#if DEBUG_DISTANT_FOG_RAYMARCH
  distantFogFrameRaymarchDebug = dag::create_tex(nullptr, raymarchFrameRes.x, raymarchFrameRes.y,
    TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1, "distant_fog_raymarch_debug");
  distantFogFrameRaymarchDebug.getTex2D()->texaddr(TEXADDR_CLAMP);
  distantFogFrameRaymarchDebug.getTex2D()->texfilter(TEXFILTER_POINT);
#endif

  static constexpr int FX_RESULT_MIP_CNT = 1;

  for (int i = 0; i < distantFogFrameReconstruct.count; ++i)
  {
    String name(128, "distant_fog_inscatter_%d", i);
    distantFogFrameReconstruct[i] = dag::create_tex(nullptr, reconstructionFrameRes.x, reconstructionFrameRes.y,
      TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1 + FX_RESULT_MIP_CNT, name);
    distantFogFrameReconstruct[i].getTex2D()->texaddr(TEXADDR_CLAMP);
    distantFogFrameReconstruct[i].getTex2D()->texfilter(TEXFILTER_SMOOTH);
  }

  for (int i = 0; i < distantFogFrameReconstructionWeight.count; ++i)
  {
    String name(128, "distant_fog_reconstruction_weight_%d", i);
    distantFogFrameReconstructionWeight[i] =
      dag::create_tex(nullptr, reconstructionFrameRes.x, reconstructionFrameRes.y, TEXFMT_R8 | TEXCF_UNORDERED, 1, name);
    distantFogFrameReconstructionWeight[i].getTex2D()->texaddr(TEXADDR_CLAMP);
    distantFogFrameReconstructionWeight[i].getTex2D()->texfilter(TEXFILTER_POINT);
  }

  distantFogFrameReprojectionDist = dag::create_tex(nullptr, reconstructionFrameRes.x, reconstructionFrameRes.y,
    TEXFMT_R16F | TEXCF_UNORDERED, 1 + FX_RESULT_MIP_CNT, "distant_fog_reprojection_dist");
  distantFogFrameReprojectionDist.getTex2D()->texaddr(TEXADDR_CLAMP);
  distantFogFrameReprojectionDist.getTex2D()->texfilter(TEXFILTER_SMOOTH);

  distantFogFrameRaymarchDist.close();
  distantFogFrameRaymarchDist =
    dag::create_tex(nullptr, raymarchFrameRes.x, raymarchFrameRes.y, TEXFMT_R16F | TEXCF_UNORDERED, 1, "distant_fog_raymarch_dist");
  distantFogFrameRaymarchDist.getTex2D()->texaddr(TEXADDR_CLAMP);
  distantFogFrameRaymarchDist.getTex2D()->texfilter(TEXFILTER_POINT);

  for (int i = 0; i < distantFogRaymarchStartWeights.count; ++i)
  {
    String name(128, "distant_fog_raymarch_start_weights_%d", i);
    distantFogRaymarchStartWeights[i] = dag::create_tex(nullptr, raymarchFrameRes.x, raymarchFrameRes.y, TEXFMT_R8G8 | TEXCF_UNORDERED,
      DISTANT_FOG_OCCLUSION_WEIGHT_MIP_CNT, name);
    distantFogRaymarchStartWeights[i].getTex2D()->texfilter(TEXFILTER_POINT);
    distantFogRaymarchStartWeights[i].getTex2D()->texmipmap(TEXMIPMAP_LINEAR);
    distantFogRaymarchStartWeights[i].getTex2D()->texaddr(TEXADDR_BORDER);
    distantFogRaymarchStartWeights[i].getTex2D()->texbordercolor(0); // very important to avoid getting stuck in false-positive
                                                                     // non-empty rays
  }

#if DEBUG_DISTANT_FOG_RECONSTRUCT
  distantFogDebug.close();
  distantFogDebug = dag::create_tex(nullptr, reconstructionFrameRes.x, reconstructionFrameRes.y,
    TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1, "distant_fog_debug");
  distantFogDebug.getTex2D()->texaddr(TEXADDR_CLAMP);
  distantFogDebug.getTex2D()->texfilter(TEXFILTER_POINT);
  distantFogDebug.setVar();
#endif
}

void VolumeLight::setResolution(int resW, int resH, int resD, int screenW, int screenH)
{
  G_ASSERT(resW % RESULT_WARP_SIZE == 0);
  G_ASSERT(resH % RESULT_WARP_SIZE == 0);
  G_ASSERT(resD % INITIAL_WARP_SIZE_Z == 0);

  froxelOrigResolution = IPoint3(resW, resH, resD);

  IPoint3 targetFroxelResolution = calc_froxel_resolution(froxelOrigResolution, hq_volfog);

  IPoint2 targetReconstructionFrameRes = IPoint2(screenW, screenH) / 2;
  bool reinitFroxelFog = targetFroxelResolution != froxelResolution;
  bool reinitDistantFog = targetReconstructionFrameRes != reconstructionFrameRes;

  froxelResolution = targetFroxelResolution;
  reconstructionFrameRes = targetReconstructionFrameRes;

  ShaderGlobal::set_color4(distant_fog_reconstruction_resolutionVarId, reconstructionFrameRes.x, reconstructionFrameRes.y,
    1.0 / reconstructionFrameRes.x, 1.0 / reconstructionFrameRes.y);

  if (reinitFroxelFog)
    initFroxelFog();
  if (reinitDistantFog)
    initDistantFog();
  if (reinitFroxelFog || reinitDistantFog)
    invalidate();
}

void VolumeLight::invalidate()
{
  const float clear0[4] = {0};
  const float clear1[4] = {1}; // clear with maximum occlusion (255), so empty voxels are cleared with (0,0,0,1) at first
  for (int i = 0; i < initialInscatter.count; ++i)
  {
    shaderHelperUtils->clear_float4_voltex_via_cs(initialInscatter[i].getVolTex());
    d3d::resource_barrier({initialInscatter[i].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    shaderHelperUtils->clear_float4_voltex_via_cs(initialExtinction[i].getVolTex());
    d3d::resource_barrier({initialExtinction[i].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }
  shaderHelperUtils->clear_float4_voltex_via_cs(initialMedia.getVolTex());
#if USE_SEPARATE_PHASE
  shaderHelperUtils->clear_float_voltex_via_cs(initialPhase.getVolTex());
#endif
  shaderHelperUtils->clear_float_voltex_via_cs(resultInscatter.getVolTex());
  for (int i = 0; i < volfogOcclusion.count; ++i)
  {
    if (volfogOcclusion[i])
    {
      d3d::GpuAutoLock gpuLock;
      d3d::clear_rwtexf(volfogOcclusion[i].getTex2D(), clear1, 0, 0);
      d3d::resource_barrier({volfogOcclusion[i].getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    }
  }
  for (int i = 0; i < volfogShadow.count; ++i)
  {
    if (isVolfogShadowEnabled() && volfogShadow[i])
    {
      d3d::GpuAutoLock gpuLock;
      ShaderGlobal::set_color4(volfog_shadow_resVarId, VOLFOG_SHADOW_VOLTEX_RES, volfog_shadow_voxel_size);
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, 0), volfogShadow[i].getVolTex());
      clearVolfogShadowCs->dispatchThreads(VOLFOG_SHADOW_VOLTEX_RES.x, VOLFOG_SHADOW_VOLTEX_RES.y, VOLFOG_SHADOW_VOLTEX_RES.z);
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
  if (nodeBasedFillMediaCs)
    nodeBasedFillMediaCs->closeShader();
  if (nodeBasedDistantFogRaymarchCs)
    nodeBasedDistantFogRaymarchCs->closeShader();
  if (nodeBasedFogShadowCs)
    nodeBasedFogShadowCs->closeShader();
}

void VolumeLight::switchOff()
{
  ShaderGlobal::set_texture(resultInscatter.getVarId(), BAD_TEXTUREID);
  ShaderGlobal::set_texture(distant_fog_result_inscatterVarId, BAD_TEXTUREID);
  ShaderGlobal::set_color4(volfog_froxel_range_paramsVarId, 0, 0, 0, 0);
  ShaderGlobal::set_texture(volfog_shadowVarId, BAD_TEXTUREID);
}

void VolumeLight::switchOn()
{
  if (!fogIsValid)
  {
    switchOff();
    return;
  }
  resultInscatter.setVar();
  ShaderGlobal::set_color4(volfog_froxel_range_paramsVarId, currentRange, 1.0f / currentRange, 1.0f,
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
bool VolumeLight::isVolfogShadowEnabled() const { return volfogShadowCs && clearVolfogShadowCs && !useCompatibilityMode; }

bool VolumeLight::perform(const TMatrix4 &view_tm, const TMatrix4_vec4 &glob_tm, const Point3 &camera_pos)
{
  bool useNodeBasedInput = !volfog_disable_node_based_input && nodeBasedFillMediaCs && nodeBasedFillMediaCs->isValid();

  if (!isReady)
  {
    switchOff();
    return false;
  }

  ShaderGlobal::set_int(froxel_fog_use_debug_mediaVarId, volfog_disable_node_based_input);

  if (hasDistantFog() && useNodeBasedInput && !(nodeBasedDistantFogRaymarchCs && nodeBasedDistantFogRaymarchCs->isValid()))
  {
    logerr("Distant fog node based shaders are invalid!");
    resetDistantFog();
  }

  if (isDistantFogEnabled()) // distant fog is much more stable with a fixed starting depth (depends on froxel range)
    currentRange = froxel_fog_range;

  switchOn(); // set range params early for volfog passes

  TIME_D3D_PROFILE(volume_light_perform);

  set_viewvecs_to_shader();
  ShaderGlobal::set_color4(prev_globtm_psf_0VarId, Color4(prevGlobTm.current()[0]));
  ShaderGlobal::set_color4(prev_globtm_psf_1VarId, Color4(prevGlobTm.current()[1]));
  ShaderGlobal::set_color4(prev_globtm_psf_2VarId, Color4(prevGlobTm.current()[2]));
  ShaderGlobal::set_color4(prev_globtm_psf_3VarId, Color4(prevGlobTm.current()[3]));
  const int jitter_z_samples = 8;
  const int jitter_x_samples = 4;
  const int jitter_y_samples = 6;

  static const int MAX_FRAME_OFFSET = 64; // to avoid precision loss from int -> float, should be >= blue noise sample cnt
  frameId = (frameId + 1) % 64;

  Point3 posDiff = camera_pos - prevCameraPos;
  prevCameraPos = camera_pos;

  int frameRnd = frameId;
  _rnd(frameRnd);
  frameRnd = (_rnd(frameRnd) & 1023) + 1;
  float jitter_ray_x_offset = halton_sequence(frameRnd, jitter_x_samples);
  float jitter_ray_y_offset = halton_sequence(frameRnd, jitter_y_samples);
  float jitter_ray_z_offset = halton_sequence(frameRnd, jitter_z_samples);

  ShaderGlobal::set_color4(jitter_ray_offsetVarId, jitter_ray_x_offset - 0.5f, jitter_ray_y_offset - 0.5f, jitter_ray_z_offset - 0.5f,
    frameId);

  ShaderGlobal::set_real(volfog_prev_range_ratioVarId, prevRange < 0.1f ? 1.f : currentRange / prevRange);

  ShaderGlobal::set_color4(froxel_fog_fading_paramsVarId,
    (isDistantFogEnabled() || froxel_fog_fading_range_ratio <= VERY_SMALL_NUMBER)
      ? Color4(0, 1, 0, 0)
      : Color4(-1.0f / (currentRange * froxel_fog_fading_range_ratio), 1.0f / froxel_fog_fading_range_ratio, 0, 0));

  int blendedSliceCnt = getBlendedSliceCnt();
  ShaderGlobal::set_int(volfog_blended_slice_cntVarId, blendedSliceCnt);
  ShaderGlobal::set_real(volfog_blended_slice_start_depthVarId, calcBlendedStartDepth(blendedSliceCnt));

  if (!useNodeBasedInput)
    ShaderGlobal::set_real(global_time_phaseVarId, get_shader_global_time_phase(0, 0));

  // TODO: set shadervars when params change, use pullValueChange
  ShaderGlobal::set_real(volfog_media_fog_input_mulVarId, volfog_media_fog_input_mul);

  int prevChannelSwizzleFrameId = froxelChannelSwizzleFrameId;
  froxelChannelSwizzleFrameId = (froxelChannelSwizzleFrameId + 1) % 3;
  ShaderGlobal::set_int4(channel_swizzle_indicesVarId, IPoint4(froxelChannelSwizzleFrameId, prevChannelSwizzleFrameId, 0, 0));

  const int nextFrame = frameId & 1;
  const int prevFrame = 1 - nextFrame;

  const ResourceBarrier flags = RB_RO_SRV | (canUsePixelShader() ? RB_STAGE_PIXEL : RB_STAGE_COMPUTE);

  {
    TIME_D3D_PROFILE(occlusion);
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, 0), volfogOcclusion[nextFrame].getTex2D());
    volfogOcclusionCs->dispatchThreads(froxelResolution.x, froxelResolution.y, 1);

    d3d::resource_barrier({volfogOcclusion[nextFrame].getTex2D(), flags, 0, 0});
  }

  ShaderGlobal::set_texture(prev_volfog_occlusionVarId, volfogOcclusion[prevFrame].getTexId());
  ShaderGlobal::set_texture(volfog_occlusionVarId, volfogOcclusion[nextFrame].getTexId());

  {
    TIME_D3D_PROFILE(fill_media);
    IPoint3 dispatchRes =
      IPoint3(froxelResolution.x / MEDIA_WARP_SIZE_X, froxelResolution.y / MEDIA_WARP_SIZE_Y, froxelResolution.z / MEDIA_WARP_SIZE_Z);
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, 0), initialMedia.getVolTex());
    if (useNodeBasedInput)
      nodeBasedFillMediaCs->dispatch(dispatchRes.x, dispatchRes.y, dispatchRes.z);
    else
      fillMediaCs->dispatch(dispatchRes.x, dispatchRes.y, dispatchRes.z);
  }
  d3d::resource_barrier({initialMedia.getVolTex(), flags, 0, 0});

  if (hasVolfogShadows() && isVolfogShadowEnabled())
  {
    TIME_D3D_PROFILE(volfog_shadow);

    ShaderGlobal::set_color4(volfog_shadow_resVarId, VOLFOG_SHADOW_VOLTEX_RES, volfog_shadow_voxel_size);

    Point3 invTcScale = VOLFOG_SHADOW_VOLTEX_RES * (volfog_shadow_voxel_size * 2.0);
    Point3 tcDiff = Point3(posDiff.x / invTcScale.x, posDiff.y / invTcScale.y, posDiff.z / invTcScale.z);
    ShaderGlobal::set_color4(volfog_shadow_prev_frame_tc_offsetVarId, tcDiff, volfog_shadow_accumulation_factor);

    ShaderGlobal::set_texture(prev_volfog_shadowVarId, volfogShadow[prevFrame].getTexId());

    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, 0), volfogShadow[nextFrame].getVolTex());

    IPoint3 dispatchRes = IPoint3(VOLFOG_SHADOW_VOLTEX_RES.x / MEDIA_WARP_SIZE_X, VOLFOG_SHADOW_VOLTEX_RES.y / MEDIA_WARP_SIZE_Y,
      VOLFOG_SHADOW_VOLTEX_RES.z / MEDIA_WARP_SIZE_Z);
    if (useNodeBasedInput && nodeBasedFogShadowCs)
      nodeBasedFogShadowCs->dispatch(dispatchRes.x, dispatchRes.y, dispatchRes.z);
    else
      volfogShadowCs->dispatch(dispatchRes.x, dispatchRes.y, dispatchRes.z);

    d3d::resource_barrier({volfogShadow[nextFrame].getVolTex(), RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0});
    ShaderGlobal::set_texture(volfog_shadowVarId, volfogShadow[nextFrame].getTexId());
  }
  else
  {
    ShaderGlobal::set_texture(volfog_shadowVarId, BAD_TEXTUREID);
  }

  if (isDistantFogEnabled())
    performRaymarching(useNodeBasedInput, view_tm, posDiff);

  prevGlobTm.current() = glob_tm.transpose();
  prevRange = currentRange;
  return true;
}

void VolumeLight::performRaymarching(bool use_node_based_input, const TMatrix4 &view_tm, const Point3 &pos_diff)
{
  Point4 local_view_z = view_tm.getcol(2);
  ShaderGlobal::set_color4(distant_fog_local_view_zVarId, Color4(local_view_z.x, local_view_z.y, local_view_z.z, 0));

  static constexpr float RAW_DEPTH_PRE_SCALE = 0.0001f; // TODO: use non-linear params instead (needs a wrapper to avoid calling pow
                                                        // every frame)

  float texelChangePerDiff = empty_ray_occlusion_weight_mip_scale * (DEBUG_DISTANT_FOG_DISABLE_4_WAY_RECONSTRUCTION ? 2 : 4);

  float occlusionWeightSampleOffset =
    clamp(length(pos_diff) / texelChangePerDiff, 1.0f, DISTANT_FOG_OCCLUSION_WEIGHT_MAX_SAMPLE_OFFSET);
  float occlusionWeightMip = log2(occlusionWeightSampleOffset) + empty_ray_occlusion_weight_mip_bias;

  ShaderGlobal::set_color4(distant_fog_raymarch_params_0VarId, enable_raymarch_opt_empty_ray, enable_raymarch_opt_ray_start,
    raymarch_substep_start, 1.0f / raymarch_substep_mul);
  ShaderGlobal::set_color4(distant_fog_raymarch_params_1VarId, raymarch_start_volfog_slice_offset, raymarch_step_size,
    smart_raymarching_pattern_depth_bias, raymarch_occlusion_threshold * RAW_DEPTH_PRE_SCALE);
  ShaderGlobal::set_color4(distant_fog_raymarch_params_2VarId, max_range, fading_dist, occlusionWeightMip,
    occlusionWeightSampleOffset);

  ShaderGlobal::set_color4(distant_fog_reconstruction_params_0VarId, filtering_min_threshold * RAW_DEPTH_PRE_SCALE,
    filtering_max_threshold * RAW_DEPTH_PRE_SCALE, occlusion_threshold * RAW_DEPTH_PRE_SCALE, limit_pow_factor);

  ShaderGlobal::set_color4(distant_fog_reconstruction_params_1VarId,
    min<float>(max_range, MAX_RAYMARCH_FOG_DIST) - DISTANT_FOG_FX_DIST_BIAS, 0, 0, 0);

  ShaderGlobal::set_int(distant_fog_use_smart_raymarching_patternVarId, use_smart_raymarching_pattern ? 1 : 0);
  ShaderGlobal::set_int(distant_fog_disable_occlusion_checkVarId, disable_occlusion_check ? 1 : 0);
  ShaderGlobal::set_int(distant_fog_disable_temporal_filteringVarId, disable_temporal_filtering ? 1 : 0);
  ShaderGlobal::set_int(distant_fog_disable_unfiltered_blurringVarId, disable_unfiltered_blurring ? 1 : 0);
  // ShaderGlobal::set_int(distant_fog_use_3x3_bilateral_kernelVarId, use_3x3_bilateral_kernel ? 1 : 0);
  ShaderGlobal::set_int(distant_fog_reconstruct_current_frame_bilaterallyVarId, reconstruct_current_frame_bilaterally ? 1 : 0);
  ShaderGlobal::set_int(distant_fog_reprojection_typeVarId, reprojection_type);
  ShaderGlobal::set_int(distant_fog_use_stable_filteringVarId, use_stable_filtering ? 1 : 0);

  raymarchFrameId = (raymarchFrameId + 1) % 4;
  ShaderGlobal::set_int(fog_raymarch_frame_idVarId, raymarchFrameId);

  const int nextFrame = frameId & 1;
  const int prevFrame = 1 - nextFrame;

  {
    TIME_D3D_PROFILE(distant_fog_raymarch);

    ShaderGlobal::set_texture(prev_distant_fog_raymarch_start_weightsVarId, distantFogRaymarchStartWeights[prevFrame].getTexId());

    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, 0), distantFogFrameRaymarchInscatter.getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 6, VALUE, 0, 0), distantFogFrameRaymarchExtinction.getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 5, VALUE, 0, 0), distantFogFrameRaymarchDist.getTex2D());
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 4, VALUE, 0, 0), distantFogRaymarchStartWeights[nextFrame].getTex2D());

#if DEBUG_DISTANT_FOG_RAYMARCH
    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 3, VALUE, 0, 0), distantFogFrameRaymarchDebug.getTex2D());
#endif

    IPoint2 dispatchRes =
      IPoint2(raymarchFrameRes.x + RAYMARCH_WARP_SIZE - 1, raymarchFrameRes.y + RAYMARCH_WARP_SIZE - 1) / RAYMARCH_WARP_SIZE;
    if (use_node_based_input)
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
    TIME_D3D_PROFILE(distant_fog_raymarch_mip_gen);

    const UniqueTex &mipTex = distantFogRaymarchStartWeights[nextFrame];
    ShaderGlobal::set_texture(mip_gen_input_texVarId, mipTex.getTexId());

    IPoint2 mipTexSize = raymarchFrameRes;
    for (int mip = 1; mip < DISTANT_FOG_OCCLUSION_WEIGHT_MIP_CNT; ++mip)
    {
      int prevMip = mip - 1;
      mipTex.getTex2D()->texmiplevel(prevMip, prevMip);

      ShaderGlobal::set_color4(mip_gen_input_tex_sizeVarId,
        Color4(mipTexSize.x, mipTexSize.y, 1.0f / mipTexSize.x, 1.0f / mipTexSize.y));
      mipTexSize /= 2;

      d3d::resource_barrier({mipTex.getTex2D(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, (uint)prevMip, 1});

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, mip), mipTex.getTex2D());
      distantFogMipGenerationCs->dispatchThreads(mipTexSize.x, mipTexSize.y, 1);

      d3d::resource_barrier({mipTex.getTex2D(), flags, unsigned(mip), 1});
    }
    mipTex.getTex2D()->texmiplevel(0, DISTANT_FOG_OCCLUSION_WEIGHT_MIP_CNT - 1);
    d3d::resource_barrier({mipTex.getTex2D(), flags, 0, 0});
  }
}

void VolumeLight::performIntegration()
{
  const int nextFrame = frameId & 1;
  const int prevFrame = 1 - nextFrame;

  {
    TIME_D3D_PROFILE(froxel_fog);

    ShaderGlobal::set_texture(prev_initial_inscatterVarId, initialInscatter[prevFrame].getTexId());
    ShaderGlobal::set_texture(prev_initial_extinctionVarId, initialExtinction[prevFrame].getTexId());

    ShaderGlobal::set_int(froxel_fog_dispatch_modeVarId, froxel_fog_use_optimized_dispatch ? (canUseLinearAccumulation() ? 2 : 1) : 0);

    if (froxel_fog_use_optimized_dispatch)
    {
      TIME_D3D_PROFILE(combined_light_calc);
      {
        // somehow igpus (especially Intel) are much faster with fully linear accumulation, it's the opposite for dgpus
        bool usePixelShader = canUsePixelShader();
        int stage = usePixelShader ? STAGE_PS : STAGE_CS;

        STATE_GUARD_NULLPTR(d3d::set_rwtex(stage, 4, VALUE, 0, 0), resultInscatter.getVolTex());
#if USE_SEPARATE_PHASE
        STATE_GUARD_NULLPTR(d3d::set_rwtex(stage, 5, VALUE, 0, 0), initialPhase.getVolTex());
#endif
        STATE_GUARD_NULLPTR(d3d::set_rwtex(stage, 6, VALUE, 0, 0), initialInscatter[nextFrame].getVolTex());
        STATE_GUARD_NULLPTR(d3d::set_rwtex(stage, 7, VALUE, 0, 0), initialExtinction[nextFrame].getVolTex());

        if (usePixelShader)
        {
          d3d::setview(0, 0, froxelResolution.x, froxelResolution.y, 0, 1);
          viewResultInscatterPs.render();
        }
        else
        {
          viewInitialInscatterCs->dispatchThreads(1, froxelResolution.x, froxelResolution.y);
        }
#if USE_SEPARATE_PHASE
        d3d::resource_barrier({initialPhase.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
#endif
        d3d::resource_barrier({initialExtinction[nextFrame].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
        d3d::resource_barrier({initialInscatter[nextFrame].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      }
    }
    else
    {
      TIME_D3D_PROFILE(separate_light_calc);
      {
        TIME_D3D_PROFILE(initial);
#if USE_SEPARATE_PHASE
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 5, VALUE, 0, 0), initialPhase.getVolTex());
#endif
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 6, VALUE, 0, 0), initialInscatter[nextFrame].getVolTex());
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, 0), initialExtinction[nextFrame].getVolTex());

        viewInitialInscatterCs->dispatchThreads(froxelResolution.x, froxelResolution.y, froxelResolution.z);
      }

#if USE_SEPARATE_PHASE
      d3d::resource_barrier({initialPhase.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
#endif
      d3d::resource_barrier({initialExtinction[nextFrame].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      d3d::resource_barrier({initialInscatter[nextFrame].getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});

      {
        TIME_D3D_PROFILE(propagate); // and integrate scattering
        ShaderGlobal::set_texture(initial_inscatterVarId, initialInscatter[nextFrame].getTexId());
        ShaderGlobal::set_texture(initial_extinctionVarId, initialExtinction[nextFrame].getTexId());
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, 0), resultInscatter.getVolTex());
        viewResultInscatterCs->dispatchThreads(froxelResolution.x, froxelResolution.y, 1);
      }
    }
  }

  d3d::resource_barrier({resultInscatter.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_VERTEX | RB_STAGE_COMPUTE, 0, 0});

  if (isDistantFogEnabled())
  {
    {
      TIME_D3D_PROFILE(distant_fog_reconstruct);

      ShaderGlobal::set_texture(prev_distant_fog_inscatterVarId, distantFogFrameReconstruct[prevFrame].getTexId());
      ShaderGlobal::set_texture(prev_distant_fog_reconstruction_weightVarId,
        distantFogFrameReconstructionWeight[prevFrame].getTexId());

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, 0), distantFogFrameReconstruct[nextFrame].getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 6, VALUE, 0, 0), distantFogFrameReconstructionWeight[nextFrame].getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 5, VALUE, 0, 0), distantFogFrameReprojectionDist.getTex2D());
#if DEBUG_DISTANT_FOG_RECONSTRUCT
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 4, VALUE, 0, 0), distantFogDebug.getTex2D());
#endif
      distantFogReconstructCs->dispatchThreads(reconstructionFrameRes.x, reconstructionFrameRes.y, 1);
    }

    ShaderGlobal::set_texture(distant_fog_result_inscatterVarId, distantFogFrameReconstruct[nextFrame].getTexId());
    {
      TIME_D3D_PROFILE(distant_fog_fx_helper_mip_generation);

      int mip = 1;
      int prevMip = mip - 1;
      distantFogFrameReprojectionDist.getTex2D()->texmiplevel(prevMip, prevMip);
      distantFogFrameReconstruct[nextFrame].getTex2D()->texmiplevel(prevMip, prevMip);

      IPoint2 mipTexSize = IPoint2(reconstructionFrameRes.x >> 1, reconstructionFrameRes.y >> 1);
      const ResourceBarrier flags = RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE;
      d3d::resource_barrier({distantFogFrameReprojectionDist.getTex2D(), flags, (uint)prevMip, 1});
      d3d::resource_barrier({distantFogFrameReconstruct[nextFrame].getTex2D(), flags, (uint)prevMip, 1});

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, mip), distantFogFrameReconstruct[nextFrame].getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 6, VALUE, 0, mip), distantFogFrameReprojectionDist.getTex2D());

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
    ShaderGlobal::set_texture(distant_fog_result_inscatterVarId, BAD_TEXTUREID);
  }

  fogIsValid = true;
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
