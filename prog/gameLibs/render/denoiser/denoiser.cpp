// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/denoiser/denoiser_api.h>
#include <shaders/dag_computeShaders.h>
#include <image/dag_texPixel.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/algorithm.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/tuple.h>
#include <EASTL/string.h>
#include <util/dag_string.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_resource.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>

#include "shaders/common_consts.hlsli"

#include "sobol_256_4d.h"
#include "scrambling_ranking_128x128_2d_1spp.h"

#if defined(_MSC_VER)
// MSVC: Disable warning C4100 (unused parameter)
#pragma warning(push)
#pragma warning(disable : 4100)
#elif defined(__clang__)
// Clang: Disable -Wunused-parameter
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#ifdef __ARM_NEON
namespace ml
{
struct float16_t_arm
{
  uint16_t x;

  inline float16_t_arm() = default;
  inline float16_t_arm(const float16_t_arm &) = default;
  inline float16_t_arm &operator=(const float16_t_arm &) = default;

  // Conversion

  inline float16_t_arm(float v);
  inline operator float() const;
};
} // namespace ml
#define float16_t float16_t_arm
#endif


#define ML_NAMESPACE
#undef RadToDeg
#undef DegToRad
#include "ml.h"
#include "ml.hlsli"

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

using ml::float2;
using ml::float3;
using ml::float4;
using ml::float4x4;
using ml::int2;
using ml::int3;
using ml::int4;
using ml::uint2;
using ml::uint3;
using ml::uint4;

namespace denoiser
{

static ComputeShaderElement *prepareCS = nullptr;

static ComputeShaderElement *sigmaClassifyTilesCS = nullptr;
static ComputeShaderElement *sigmaSmoothTilesCS = nullptr;
static ComputeShaderElement *sigmaBlurCS = nullptr;
static ComputeShaderElement *sigmaPostBlurCS = nullptr;
static ComputeShaderElement *sigmaStabilizeCS = nullptr;

static ComputeShaderElement *reblurValidation = nullptr;

static ComputeShaderElement *reblurClassifyTiles = nullptr;

static ComputeShaderElement *reblurDiffuseOcclusionTemporalAccumulation = nullptr;
static ComputeShaderElement *reblurDiffuseOcclusionHistoryFix = nullptr;
static ComputeShaderElement *reblurDiffuseOcclusionBlur = nullptr;
static ComputeShaderElement *reblurDiffuseOcclusionPostBlurNoTemporalStabilization = nullptr;
static ComputeShaderElement *reblurDiffuseOcclusionPerfTemporalAccumulation = nullptr;
static ComputeShaderElement *reblurDiffuseOcclusionPerfHistoryFix = nullptr;
static ComputeShaderElement *reblurDiffuseOcclusionPerfBlur = nullptr;
static ComputeShaderElement *reblurDiffuseOcclusionPerfPostBlurNoTemporalStabilization = nullptr;

static ComputeShaderElement *reblurDiffusePrepass = nullptr;
static ComputeShaderElement *reblurDiffuseTemporalAccumulation = nullptr;
static ComputeShaderElement *reblurDiffuseHistoryFix = nullptr;
static ComputeShaderElement *reblurDiffuseBlur = nullptr;
static ComputeShaderElement *reblurDiffusePostBlur = nullptr;
static ComputeShaderElement *reblurDiffuseTemporalStabilization = nullptr;
static ComputeShaderElement *reblurDiffusePerfPrepass = nullptr;
static ComputeShaderElement *reblurDiffusePerfTemporalAccumulation = nullptr;
static ComputeShaderElement *reblurDiffusePerfHistoryFix = nullptr;
static ComputeShaderElement *reblurDiffusePerfBlur = nullptr;
static ComputeShaderElement *reblurDiffusePerfPostBlur = nullptr;
static ComputeShaderElement *reblurDiffusePerfTemporalStabilization = nullptr;
static ComputeShaderElement *reblurDiffuseBehindHistory = nullptr;

static ComputeShaderElement *reblurSpecularPrepass = nullptr;
static ComputeShaderElement *reblurSpecularTemporalAccumulation = nullptr;
static ComputeShaderElement *reblurSpecularHistoryFix = nullptr;
static ComputeShaderElement *reblurSpecularBlur = nullptr;
static ComputeShaderElement *reblurSpecularPostBlur = nullptr;
static ComputeShaderElement *reblurSpecularTemporalStabilization = nullptr;
static ComputeShaderElement *reblurSpecularPerfPrepass = nullptr;
static ComputeShaderElement *reblurSpecularPerfTemporalAccumulation = nullptr;
static ComputeShaderElement *reblurSpecularPerfHistoryFix = nullptr;
static ComputeShaderElement *reblurSpecularPerfBlur = nullptr;
static ComputeShaderElement *reblurSpecularPerfPostBlur = nullptr;
static ComputeShaderElement *reblurSpecularPerfTemporalStabilization = nullptr;

static ComputeShaderElement *relaxValidation = nullptr;

static ComputeShaderElement *relaxClassifyTiles = nullptr;

static ComputeShaderElement *relaxSpecularPrepass = nullptr;
static ComputeShaderElement *relaxSpecularTemporalAccumulation = nullptr;
static ComputeShaderElement *relaxSpecularHistoryFix = nullptr;
static ComputeShaderElement *relaxSpecularHistoryClamping = nullptr;
static ComputeShaderElement *relaxSpecularATorusSmem = nullptr;
static ComputeShaderElement *relaxSpecularATorus = nullptr;
static ComputeShaderElement *relaxSpecularAntiFirefly = nullptr;
static ComputeShaderElement *relaxSpecularCopy = nullptr;

static ComputeShaderElement *reflectionBlurCS = nullptr;
static ComputeShaderElement *reflectionMipFirstCS = nullptr;
static ComputeShaderElement *reflectionMipCS = nullptr;

static d3d::SamplerHandle samplerNearestClamp = d3d::INVALID_SAMPLER_HANDLE;
static d3d::SamplerHandle samplerNearestMirror = d3d::INVALID_SAMPLER_HANDLE;
static d3d::SamplerHandle samplerLinearClamp = d3d::INVALID_SAMPLER_HANDLE;
static d3d::SamplerHandle samplerLinearMirror = d3d::INVALID_SAMPLER_HANDLE;

static UniqueTexWithShaderVar scramblingRankingTexture;
static UniqueTexWithShaderVar sobolTexture;

static UniqueBuf reblurSharedCb_ao;
static UniqueBuf reblurSharedCb_gi;
static UniqueBuf reblurSharedCb_reflection;

static int denoiser_resolutionVarId = -1;
static int denoiser_prepare_close_depthVarId = -1;
static int rtr_depth_modeVarId = -1;
static int denoiser_gi_second_chanceVarId = -1;
static int half_selected_depth_rtr_samplerstateVarId = -1;
static int denoiser_camera_attached_radiusVarId = -1;

static int rtsm_renderedVarId = -1;
static int rtao_renderedVarId = -1;
static int ptgi_renderedVarId = -1;
static int rtr_renderedVarId = -1;
static int csm_bindless_slotVarId = -1;
static int csm_sampler_bindless_slotVarId = -1;
static int vsm_bindless_slotVarId = -1;
static int vsm_sampler_bindless_slotVarId = -1;
static int rtr_bindless_sampler_slotVarId = -1;
static int rtr_normal_roughness_bindless_sampler_slotVarId = -1;
static int ptgi_bindless_sampler_slotVarId = -1;
static int ptgi_depth_bindless_sampler_slotVarId = -1;
static int rtao_bindless_sampler_slotVarId = -1;
static int ray_reconstruction_activeVarId = -1;
static int blue_noise_frame_indexVarId = -1;
static int denoiser_blur_srcVarId = -1;
static int denoiser_blur_dstVarId = -1;
static int denoiser_blur_view_zVarId = -1;
static int denoiser_blur_nrVarId = -1;
static int denoiser_blur_verticalVarId = -1;
static int denoiser_blur_last_passVarId = -1;
static int denoiser_blur_max_radiusVarId = -1;
static int denoiser_blur_max_roughnessVarId = -1;
static int denoiser_blur_texels_per_unitVarId = -1;
static int denoiser_blur_checkerboardVarId = -1;
static int denoiser_reflection_mip_dst_sizeVarId = -1;
static int denoiser_reflection_mip_src_sizeVarId = -1;

static int bindless_range = -1;

enum
{
  csm_bindless_index = 0,
  vsm_bindless_index,

  bindless_texture_count
};

Config resolution_config;

static uint32_t frame_counter = 0;

static float4x4 world_to_view;
static float4x4 view_to_clip;
static float4x4 view_to_world;
static float4x4 world_to_clip;
static float4x4 prev_world_to_view;
static float4x4 prev_view_to_clip;
static float4x4 prev_view_to_world;
static float4x4 prev_world_to_clip;

static float3 view_pos;
static float3 prev_view_pos;

static float3 view_dir;
static float3 prev_view_dir;

static float2 jitter;
static float2 prev_jitter;

static float3 motion_multiplier;

static float projectY;
static float4 frustum;
static float4 frustum_prev;

static float4 rotator_pre;
static float4 rotator;
static float4 rotator_post;

static int max_accumulated_frame_num = 30;
static int max_fast_accumulated_frame_num = 6;

static float checkerboard_resolve_accum_speed = 1;
static float frame_rate_scale = 1;
static float frame_rate_scale_relax = 1;
static float time_delta_ms = 1;
static float smooth_time_delta_ms = 1;

static float jitter_delta = 0;

static bool reset_history = false;

struct SigmaSharedConstants
{
  float4x4 worldToView;
  float4x4 viewToClip;
  float4x4 worldToClipPrev;
  float4x4 worldToViewPrev;
  float4 rotator;
  float4 rotatorPost;
  float4 viewVectorWorld;
  float4 lightDirectionView;
  float4 frustum;
  float4 frustumPrev;
  float4 cameraDelta;
  float4 mvScale;
  float2 resourceSizeInv;
  float2 resourceSizeInvPrev;
  float2 rectSize;
  float2 rectSizeInv;
  float2 rectSizePrev;
  float2 resolutionScale;
  float2 rectOffset;
  uint2 printfAt;
  uint2 rectOrigin;
  int2 rectSizeMinusOne;
  int2 tilesSizeMinusOne;
  float orthoMode;
  float unproject;
  float denoisingRange;
  float planeDistSensitivity;
  float stabilizationStrength;
  float debug;
  float splitScreen;
  float viewZScale;
  float gMinRectDimMulUnproject;
  uint32_t frameIndex;
  uint32_t isRectChanged;

  uint32_t pad1;
  uint32_t pad2;
  uint32_t pad3;
};

static_assert(sizeof(SigmaSharedConstants) % (4 * sizeof(float)) == 0,
  "SigmaSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");

struct ReblurSharedConstants
{
  float4x4 worldToClip;
  float4x4 viewToClip;
  float4x4 viewToWorld;
  float4x4 worldToViewPrev;
  float4x4 worldToClipPrev;
  float4x4 worldPrevToWorld;
  float4 rotatorPre;
  float4 rotator;
  float4 rotatorPost;
  float4 frustum;
  float4 frustumPrev;
  float4 cameraDelta;
  float4 hitDistSettings;
  float4 viewVectorWorld;
  float4 viewVectorWorldPrev;
  float4 mvScale;
  float4 convergenceSettings;
  float2 antilagSettings;
  float2 resourceSize;
  float2 resourceSizeInv;
  float2 resourceSizeInvPrev;
  float2 rectSize;
  float2 rectSizeInv;
  float2 rectSizePrev;
  float2 resolutionScale;
  float2 resolutionScalePrev;
  float2 rectOffset;
  float2 jitter;
  uint2 printfAt;
  uint2 rectOrigin;
  int2 rectSizeMinusOne;
  float disocclusionThreshold;
  float disocclusionThresholdAlternate;
  float cameraAttachedReflectionMaterialID;
  float strandMaterialID;
  float strandThickness;
  float stabilizationStrength;
  float debug;
  float orthoMode;
  float unproject;
  float denoisingRange;
  float planeDistSensitivity;
  float framerateScale;
  float minBlurRadius;
  float maxBlurRadius;
  float diffPrepassBlurRadius;
  float specPrepassBlurRadius;
  float maxAccumulatedFrameNum;
  float maxFastAccumulatedFrameNum;
  float antiFirefly;
  float lobeAngleFraction;
  float roughnessFraction;
  float historyFixFrameNum;
  float historyFixBasePixelStride;
  float historyFixAlternatePixelStride;
  float historyFixAlternatePixelStrideMaterialID;
  float fastHistoryClampingSigmaScale;
  float minRectDimMulUnproject;
  float usePrepassNotOnlyForSpecularMotionEstimation;
  float splitScreen;
  float splitScreenPrev;
  float checkerboardResolveAccumSpeed;
  float viewZScale;
  float fireflySuppressorMinRelativeScale;
  float minHitDistanceWeight;
  float diffMinMaterial;
  float specMinMaterial;
  float responsiveAccumulationInvRoughnessThreshold;
  uint32_t responsiveAccumulationMinAccumulatedFrameNum;
  uint32_t hasHistoryConfidence;
  uint32_t hasDisocclusionThresholdMix;
  uint32_t diffCheckerboard;
  uint32_t specCheckerboard;
  uint32_t frameIndex;
  uint32_t isRectChanged;
  uint32_t resetHistory;
  uint32_t returnHistoryLengthInsteadOfOcclusion;
  // Validation layer
  uint32_t hasDiffuse;
  uint32_t hasSpecular;

  uint32_t hasBehindHistory;
  uint32_t secondChanceDebug;
};

static_assert(sizeof(ReblurSharedConstants) % (4 * sizeof(float)) == 0,
  "ReblurSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");

struct RelaxSharedConstants
{
  float4x4 worldToClip;
  float4x4 worldToClipPrev;
  float4x4 worldToViewPrev;
  float4x4 worldPrevToWorld;
  float4 rotatorPre;
  float4 frustumRight;
  float4 frustumUp;
  float4 frustumForward;
  float4 prevFrustumRight;
  float4 prevFrustumUp;
  float4 prevFrustumForward;
  float4 cameraDelta;
  float4 mvScale;
  float2 jitter;
  float2 resolutionScale;
  float2 rectOffset;
  float2 resourceSizeInv;
  float2 resourceSize;
  float2 rectSizeInv;
  float2 rectSizePrev;
  float2 resourceSizeInvPrev;
  uint2 printfAt;
  uint2 rectOrigin;
  int2 rectSize;
  float specMaxAccumulatedFrameNum;
  float specMaxFastAccumulatedFrameNum;
  float diffMaxAccumulatedFrameNum;
  float diffMaxFastAccumulatedFrameNum;
  float disocclusionThreshold;
  float disocclusionThresholdAlternate;
  float cameraAttachedReflectionMaterialID;
  float strandMaterialID;
  float strandThickness;
  float roughnessFraction;
  float specVarianceBoost;
  float splitScreen;
  float diffBlurRadius;
  float specBlurRadius;
  float depthThreshold;
  float lobeAngleFraction;
  float specLobeAngleSlack;
  float historyFixEdgeStoppingNormalPower;
  float roughnessEdgeStoppingRelaxation;
  float normalEdgeStoppingRelaxation;
  float fastHistoryClampingSigmaScale;
  float historyAccelerationAmount;
  float historyResetTemporalSigmaScale;
  float historyResetSpatialSigmaScale;
  float historyResetAmount;
  float denoisingRange;
  float specPhiLuminance;
  float diffPhiLuminance;
  float diffMaxLuminanceRelativeDifference;
  float specMaxLuminanceRelativeDifference;
  float luminanceEdgeStoppingRelaxation;
  float confidenceDrivenRelaxationMultiplier;
  float confidenceDrivenLuminanceEdgeStoppingRelaxation;
  float confidenceDrivenNormalEdgeStoppingRelaxation;
  float debug;
  float orthoMode;
  float unproject;
  float framerateScale;
  float checkerboardResolveAccumSpeed;
  float jitterDelta;
  float historyFixFrameNum;
  float historyFixBasePixelStride;
  float historyFixAlternatePixelStride;
  float historyFixAlternatePixelStrideMaterialID;
  float historyThreshold;
  float viewZScale;
  float minHitDistanceWeight;
  float diffMinMaterial;
  float specMinMaterial;
  uint32_t roughnessEdgeStoppingEnabled;
  uint32_t frameIndex;
  uint32_t diffCheckerboard;
  uint32_t specCheckerboard;
  uint32_t hasHistoryConfidence;
  uint32_t hasDisocclusionThresholdMix;
  uint32_t resetHistory;

  uint32_t stepSize;
  uint32_t isLastPass;

  uint32_t pad1;
  uint32_t pad2;
};

static_assert(sizeof(RelaxSharedConstants) % (4 * sizeof(float)) == 0,
  "RelaxSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");


static int divide_up(int x, int y) { return (x + y - 1) / y; }

static Sbuffer *create_and_update_reblur_cbuffer_if_needed(UniqueBuf &reblur_cb, const ReblurSharedConstants &reblur_shared_constants,
  const char *name)
{
  if (!reblur_cb)
    reblur_cb = dag::buffers::create_one_frame_cb(dag::buffers::cb_struct_reg_count<ReblurSharedConstants>(), name, RESTAG_DENOISER);

  reblur_cb->updateData(0, sizeof(ReblurSharedConstants), &reblur_shared_constants, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  return reblur_cb.getBuf();
}

void init_blue_noise()
{

  if (!scramblingRankingTexture)
  {
    static_assert(sizeof(scrambling_ranking_128x128_2d_1spp) == 128 * 128 * 4);
    TexImage32 *image = TexImage32::create(128, 128, tmpmem);
    memcpy(image->getPixels(), scrambling_ranking_128x128_2d_1spp, sizeof(scrambling_ranking_128x128_2d_1spp));
    scramblingRankingTexture = dag::create_tex(image, 128, 128, TEXFMT_R8G8B8A8, 1, "scrambling_ranking_texture", RESTAG_DENOISER);
    scramblingRankingTexture.setVar();
    memfree(image, tmpmem);
  }

  if (!sobolTexture)
  {
    static_assert(sizeof(sobol_256_4d) == 256 * 1 * 4);
    TexImage32 *image = TexImage32::create(256, 1, tmpmem);
    memcpy(image->getPixels(), sobol_256_4d, sizeof(sobol_256_4d));
    sobolTexture = dag::create_tex(image, 256, 1, TEXFMT_R8G8B8A8, 1, "sobol_texture", RESTAG_DENOISER);
    sobolTexture.setVar();
    memfree(image, tmpmem);
  }
}

void close_blue_noise()
{
  sobolTexture.close();
  scramblingRankingTexture.close();
}


void initialize(bool use_ray_reconstruction, bool ptgi_use_ray_reconstruction)
{
  const bool initialized = bindless_range >= 0;
  G_ASSERT(!initialized || (use_ray_reconstruction == resolution_config.useRayReconstruction &&
                             ptgi_use_ray_reconstruction == resolution_config.ptgiUseRayReconstruction));
  if (initialized)
    return;

  resolution_config.init(use_ray_reconstruction, ptgi_use_ray_reconstruction);

  if (!prepareCS)
    prepareCS = new_compute_shader("denoiser_prepare");

  if (!sigmaClassifyTilesCS)
    sigmaClassifyTilesCS = new_compute_shader("nrd_sigma_shadow_classify_tiles");
  if (!sigmaSmoothTilesCS)
    sigmaSmoothTilesCS = new_compute_shader("nrd_sigma_shadow_smooth_tiles");
  if (!sigmaBlurCS)
    sigmaBlurCS = new_compute_shader("nrd_sigma_shadow_blur");
  if (!sigmaPostBlurCS)
    sigmaPostBlurCS = new_compute_shader("nrd_sigma_shadow_post_blur");
  if (!sigmaStabilizeCS)
    sigmaStabilizeCS = new_compute_shader("nrd_sigma_shadow_stabilize");

  if (!reblurValidation)
    reblurValidation = new_compute_shader("nrd_reblur_validation");
  if (!reblurClassifyTiles)
    reblurClassifyTiles = new_compute_shader("nrd_reblur_classify_tiles");

  if (!reblurDiffuseOcclusionTemporalAccumulation)
    reblurDiffuseOcclusionTemporalAccumulation = new_compute_shader("nrd_reblur_diffuse_occlusion_temporal_accumulation");
  if (!reblurDiffuseOcclusionHistoryFix)
    reblurDiffuseOcclusionHistoryFix = new_compute_shader("nrd_reblur_diffuse_occlusion_history_fix");
  if (!reblurDiffuseOcclusionBlur)
    reblurDiffuseOcclusionBlur = new_compute_shader("nrd_reblur_diffuse_occlusion_blur");
  if (!reblurDiffuseOcclusionPostBlurNoTemporalStabilization)
    reblurDiffuseOcclusionPostBlurNoTemporalStabilization =
      new_compute_shader("nrd_reblur_diffuse_occlusion_postblur_no_temporal_stabilization");
  if (!reblurDiffuseOcclusionPerfTemporalAccumulation)
    reblurDiffuseOcclusionPerfTemporalAccumulation = new_compute_shader("nrd_reblur_perf_diffuse_occlusion_temporal_accumulation");
  if (!reblurDiffuseOcclusionPerfHistoryFix)
    reblurDiffuseOcclusionPerfHistoryFix = new_compute_shader("nrd_reblur_perf_diffuse_occlusion_history_fix");
  if (!reblurDiffuseOcclusionPerfBlur)
    reblurDiffuseOcclusionPerfBlur = new_compute_shader("nrd_reblur_perf_diffuse_occlusion_blur");
  if (!reblurDiffuseOcclusionPerfPostBlurNoTemporalStabilization)
    reblurDiffuseOcclusionPerfPostBlurNoTemporalStabilization =
      new_compute_shader("nrd_reblur_perf_diffuse_occlusion_postblur_no_temporal_stabilization");

  if (!reblurDiffusePrepass)
    reblurDiffusePrepass = new_compute_shader("nrd_reblur_diffuse_prepass");
  if (!reblurDiffuseTemporalAccumulation)
    reblurDiffuseTemporalAccumulation = new_compute_shader("nrd_reblur_diffuse_temporal_accumulation");
  if (!reblurDiffuseHistoryFix)
    reblurDiffuseHistoryFix = new_compute_shader("nrd_reblur_diffuse_history_fix");
  if (!reblurDiffuseBlur)
    reblurDiffuseBlur = new_compute_shader("nrd_reblur_diffuse_blur");
  if (!reblurDiffusePostBlur)
    reblurDiffusePostBlur = new_compute_shader("nrd_reblur_diffuse_postblur");
  if (!reblurDiffuseTemporalStabilization)
    reblurDiffuseTemporalStabilization = new_compute_shader("nrd_reblur_diffuse_temporal_stabilization");
  if (!reblurDiffusePerfPrepass)
    reblurDiffusePerfPrepass = new_compute_shader("nrd_reblur_perf_diffuse_prepass");
  if (!reblurDiffusePerfTemporalAccumulation)
    reblurDiffusePerfTemporalAccumulation = new_compute_shader("nrd_reblur_perf_diffuse_temporal_accumulation");
  if (!reblurDiffusePerfHistoryFix)
    reblurDiffusePerfHistoryFix = new_compute_shader("nrd_reblur_perf_diffuse_history_fix");
  if (!reblurDiffusePerfBlur)
    reblurDiffusePerfBlur = new_compute_shader("nrd_reblur_perf_diffuse_blur");
  if (!reblurDiffusePerfPostBlur)
    reblurDiffusePerfPostBlur = new_compute_shader("nrd_reblur_perf_diffuse_postblur");
  if (!reblurDiffusePerfTemporalStabilization)
    reblurDiffusePerfTemporalStabilization = new_compute_shader("nrd_reblur_perf_diffuse_temporal_stabilization");
  if (!reblurDiffuseBehindHistory)
    reblurDiffuseBehindHistory = new_compute_shader("nrd_reblur_diffuse_behind_history");

  if (!reblurSpecularPrepass)
    reblurSpecularPrepass = new_compute_shader("nrd_reblur_specular_prepass");
  if (!reblurSpecularTemporalAccumulation)
    reblurSpecularTemporalAccumulation = new_compute_shader("nrd_reblur_specular_temporal_accumulation");
  if (!reblurSpecularHistoryFix)
    reblurSpecularHistoryFix = new_compute_shader("nrd_reblur_specular_history_fix");
  if (!reblurSpecularBlur)
    reblurSpecularBlur = new_compute_shader("nrd_reblur_specular_blur");
  if (!reblurSpecularPostBlur)
    reblurSpecularPostBlur = new_compute_shader("nrd_reblur_specular_postblur");
  if (!reblurSpecularTemporalStabilization)
    reblurSpecularTemporalStabilization = new_compute_shader("nrd_reblur_specular_temporal_stabilization");
  if (!reblurSpecularPerfPrepass)
    reblurSpecularPerfPrepass = new_compute_shader("nrd_reblur_perf_specular_prepass");
  if (!reblurSpecularPerfTemporalAccumulation)
    reblurSpecularPerfTemporalAccumulation = new_compute_shader("nrd_reblur_perf_specular_temporal_accumulation");
  if (!reblurSpecularPerfHistoryFix)
    reblurSpecularPerfHistoryFix = new_compute_shader("nrd_reblur_perf_specular_history_fix");
  if (!reblurSpecularPerfBlur)
    reblurSpecularPerfBlur = new_compute_shader("nrd_reblur_perf_specular_blur");
  if (!reblurSpecularPerfPostBlur)
    reblurSpecularPerfPostBlur = new_compute_shader("nrd_reblur_perf_specular_postblur");
  if (!reblurSpecularPerfTemporalStabilization)
    reblurSpecularPerfTemporalStabilization = new_compute_shader("nrd_reblur_perf_specular_temporal_stabilization");

  if (!relaxValidation)
    relaxValidation = new_compute_shader("nrd_relax_validation");
  if (!relaxClassifyTiles)
    relaxClassifyTiles = new_compute_shader("nrd_relax_classify_tiles");
  if (!relaxSpecularPrepass)
    relaxSpecularPrepass = new_compute_shader("nrd_relax_specular_prepass");
  if (!relaxSpecularTemporalAccumulation)
    relaxSpecularTemporalAccumulation = new_compute_shader("nrd_relax_specular_temporal_accumulation");
  if (!relaxSpecularHistoryFix)
    relaxSpecularHistoryFix = new_compute_shader("nrd_relax_specular_history_fix");
  if (!relaxSpecularHistoryClamping)
    relaxSpecularHistoryClamping = new_compute_shader("nrd_relax_specular_history_clamping");
  if (!relaxSpecularATorusSmem)
    relaxSpecularATorusSmem = new_compute_shader("nrd_relax_specular_atorus_smem");
  if (!relaxSpecularATorus)
    relaxSpecularATorus = new_compute_shader("nrd_relax_specular_atorus");
  if (!relaxSpecularAntiFirefly)
    relaxSpecularAntiFirefly = new_compute_shader("nrd_relax_specular_anti_firefly");
  if (!relaxSpecularCopy)
    relaxSpecularCopy = new_compute_shader("nrd_relax_specular_copy");

  if (samplerNearestClamp == d3d::INVALID_SAMPLER_HANDLE)
  {
    d3d::SamplerInfo info;
    info.filter_mode = d3d::FilterMode::Point;
    info.mip_map_mode = d3d::MipMapMode::Point;
    info.address_mode_u = d3d::AddressMode::Clamp;
    info.address_mode_v = d3d::AddressMode::Clamp;
    samplerNearestClamp = d3d::request_sampler(info);
  }
  if (samplerNearestMirror == d3d::INVALID_SAMPLER_HANDLE)
  {
    d3d::SamplerInfo info;
    info.filter_mode = d3d::FilterMode::Point;
    info.mip_map_mode = d3d::MipMapMode::Point;
    info.address_mode_u = d3d::AddressMode::Mirror;
    info.address_mode_v = d3d::AddressMode::Mirror;
    samplerNearestMirror = d3d::request_sampler(info);
  }
  if (samplerLinearClamp == d3d::INVALID_SAMPLER_HANDLE)
  {
    d3d::SamplerInfo info;
    info.filter_mode = d3d::FilterMode::Linear;
    info.mip_map_mode = d3d::MipMapMode::Linear;
    info.address_mode_u = d3d::AddressMode::Clamp;
    info.address_mode_v = d3d::AddressMode::Clamp;
    samplerLinearClamp = d3d::request_sampler(info);
  }
  if (samplerLinearMirror == d3d::INVALID_SAMPLER_HANDLE)
  {
    d3d::SamplerInfo info;
    info.filter_mode = d3d::FilterMode::Linear;
    info.mip_map_mode = d3d::MipMapMode::Linear;
    info.address_mode_u = d3d::AddressMode::Mirror;
    info.address_mode_v = d3d::AddressMode::Mirror;
    samplerLinearMirror = d3d::request_sampler(info);
  }
  init_blue_noise();

  denoiser_resolutionVarId = get_shader_variable_id("denoiser_resolution");
  denoiser_prepare_close_depthVarId = get_shader_variable_id("denoiser_prepare_close_depth", true);
  rtr_depth_modeVarId = get_shader_variable_id("rtr_depth_mode", true);
  denoiser_gi_second_chanceVarId = get_shader_variable_id("denoiser_gi_second_chance", true);
  half_selected_depth_rtr_samplerstateVarId = get_shader_variable_id("half_selected_depth_rtr_samplerstate", true);
  denoiser_camera_attached_radiusVarId = get_shader_variable_id("denoiser_camera_attached_radius");

  rtsm_renderedVarId = get_shader_variable_id("rtsm_rendered");
  rtao_renderedVarId = get_shader_variable_id("rtao_rendered");
  ptgi_renderedVarId = get_shader_variable_id("ptgi_rendered");
  rtr_renderedVarId = get_shader_variable_id("rtr_rendered");
  csm_bindless_slotVarId = get_shader_variable_id("csm_bindless_slot", true);
  csm_sampler_bindless_slotVarId = get_shader_variable_id("csm_sampler_bindless_slot", true);
  vsm_bindless_slotVarId = get_shader_variable_id("vsm_bindless_slot", true);
  vsm_sampler_bindless_slotVarId = get_shader_variable_id("vsm_sampler_bindless_slot", true);
  rtr_bindless_sampler_slotVarId = get_shader_variable_id("rtr_bindless_sampler_slot");
  rtr_normal_roughness_bindless_sampler_slotVarId = get_shader_variable_id("rtr_normal_roughness_bindless_sampler_slot");
  ptgi_bindless_sampler_slotVarId = get_shader_variable_id("ptgi_bindless_sampler_slot");
  ptgi_depth_bindless_sampler_slotVarId = get_shader_variable_id("ptgi_depth_bindless_sampler_slot");
  rtao_bindless_sampler_slotVarId = get_shader_variable_id("rtao_bindless_sampler_slot");
  ray_reconstruction_activeVarId = get_shader_variable_id("ray_reconstruction_active");
  blue_noise_frame_indexVarId = get_shader_variable_id("blue_noise_frame_index");
  denoiser_blur_srcVarId = get_shader_variable_id("denoiser_blur_src", true);
  denoiser_blur_dstVarId = get_shader_variable_id("denoiser_blur_dst", true);
  denoiser_blur_view_zVarId = get_shader_variable_id("denoiser_blur_view_z", true);
  denoiser_blur_nrVarId = get_shader_variable_id("denoiser_blur_nr", true);
  denoiser_blur_verticalVarId = get_shader_variable_id("denoiser_blur_vertical", true);
  denoiser_blur_last_passVarId = get_shader_variable_id("denoiser_blur_last_pass", true);
  denoiser_blur_max_radiusVarId = get_shader_variable_id("denoiser_blur_max_radius", true);
  denoiser_blur_max_roughnessVarId = get_shader_variable_id("denoiser_blur_max_roughness", true);
  denoiser_blur_texels_per_unitVarId = get_shader_variable_id("denoiser_blur_texels_per_unit", true);
  denoiser_blur_checkerboardVarId = get_shader_variable_id("denoiser_blur_checkerboard", true);
  denoiser_reflection_mip_dst_sizeVarId = get_shader_variable_id("denoiser_reflection_mip_dst_size", true);
  denoiser_reflection_mip_src_sizeVarId = get_shader_variable_id("denoiser_reflection_mip_src_size", true);

  bindless_range = d3d::allocate_bindless_resource_range(D3DResourceType::TEX, bindless_texture_count);
}

inline void clear_texture(Texture *tex)
{
  if (!tex)
    return;

  TextureInfo ti;
  tex->getinfo(ti);

  uint32_t zeroI[] = {0, 0, 0, 0};
  float zeroF[] = {0, 0, 0, 0};

  auto &desc = get_tex_format_desc(ti.cflg);
  if (desc.mainChannelsType == ChannelDType::SFLOAT || desc.mainChannelsType == ChannelDType::UFLOAT ||
      desc.mainChannelsType == ChannelDType::UNORM)
    d3d::clear_rwtexf(tex, zeroF, 0, 0);
  else
    d3d::clear_rwtexi(tex, zeroI, 0, 0);
};

template <typename T>
inline void safe_delete(T *&ptr)
{
  if (ptr)
  {
    delete ptr;
    ptr = nullptr;
  }
}

bool is_ray_reconstruction_enabled() { return resolution_config.useRayReconstruction; }
bool is_ptgi_ray_reconstruction_enabled() { return resolution_config.ptgiUseRayReconstruction; }

void teardown()
{
  ShaderGlobal::set_int(rtr_depth_modeVarId, 0); // no prepare produces the rtr set now
  safe_delete(prepareCS);
  safe_delete(sigmaClassifyTilesCS);
  safe_delete(sigmaSmoothTilesCS);
  safe_delete(sigmaBlurCS);
  safe_delete(sigmaPostBlurCS);
  safe_delete(sigmaStabilizeCS);
  safe_delete(reblurValidation);
  safe_delete(reblurClassifyTiles);
  safe_delete(reblurDiffuseOcclusionTemporalAccumulation);
  safe_delete(reblurDiffuseOcclusionHistoryFix);
  safe_delete(reblurDiffuseOcclusionBlur);
  safe_delete(reblurDiffuseOcclusionPostBlurNoTemporalStabilization);
  safe_delete(reblurDiffuseOcclusionPerfTemporalAccumulation);
  safe_delete(reblurDiffuseOcclusionPerfHistoryFix);
  safe_delete(reblurDiffuseOcclusionPerfBlur);
  safe_delete(reblurDiffuseOcclusionPerfPostBlurNoTemporalStabilization);
  safe_delete(reblurDiffusePrepass);
  safe_delete(reblurDiffuseTemporalAccumulation);
  safe_delete(reblurDiffuseHistoryFix);
  safe_delete(reblurDiffuseBlur);
  safe_delete(reblurDiffusePostBlur);
  safe_delete(reblurDiffuseTemporalStabilization);
  safe_delete(reblurDiffusePerfPrepass);
  safe_delete(reblurDiffusePerfTemporalAccumulation);
  safe_delete(reblurDiffusePerfHistoryFix);
  safe_delete(reblurDiffusePerfBlur);
  safe_delete(reblurDiffusePerfPostBlur);
  safe_delete(reblurDiffusePerfTemporalStabilization);
  safe_delete(reblurDiffuseBehindHistory);
  safe_delete(reblurSpecularPrepass);
  safe_delete(reblurSpecularTemporalAccumulation);
  safe_delete(reblurSpecularHistoryFix);
  safe_delete(reblurSpecularBlur);
  safe_delete(reblurSpecularPostBlur);
  safe_delete(reblurSpecularTemporalStabilization);
  safe_delete(reblurSpecularPerfPrepass);
  safe_delete(reblurSpecularPerfTemporalAccumulation);
  safe_delete(reblurSpecularPerfHistoryFix);
  safe_delete(reblurSpecularPerfBlur);
  safe_delete(reblurSpecularPerfPostBlur);
  safe_delete(reblurSpecularPerfTemporalStabilization);
  safe_delete(relaxValidation);
  safe_delete(relaxClassifyTiles);
  safe_delete(relaxSpecularPrepass);
  safe_delete(relaxSpecularTemporalAccumulation);
  safe_delete(relaxSpecularHistoryFix);
  safe_delete(relaxSpecularHistoryClamping);
  safe_delete(relaxSpecularATorusSmem);
  safe_delete(relaxSpecularATorus);
  safe_delete(relaxSpecularAntiFirefly);
  safe_delete(relaxSpecularCopy);
  safe_delete(reflectionBlurCS);
  safe_delete(reflectionMipFirstCS);
  safe_delete(reflectionMipCS);

  close_blue_noise();

  reblurSharedCb_ao.close();
  reblurSharedCb_gi.close();
  reblurSharedCb_reflection.close();

  resolution_config.closeAll();
  frame_counter = 0;

  if (bindless_range >= 0)
  {
    d3d::free_bindless_resource_range(D3DResourceType::TEX, bindless_range, bindless_texture_count);
    bindless_range = -1;
  }
}

inline float4x4 convertx(const TMatrix4 &m)
{
  return float4x4(m._11, m._12, m._13, m._14, m._21, m._22, m._23, m._24, m._31, m._32, m._33, m._34, m._41, m._42, m._43, m._44);
}
inline float4x4 convert(const TMatrix4 &m)
{
  return float4x4(m._11, m._21, m._31, m._41, m._12, m._22, m._32, m._42, m._13, m._23, m._33, m._43, m._14, m._24, m._34, m._44);
}

// the frame state every pass reads, false when nothing denoises (ray reconstruction everywhere)
static bool prepare_frame(const PrepareParams &params, const IPoint2 &resolution)
{
  frame_counter++;

  ShaderGlobal::set_int(blue_noise_frame_indexVarId, frame_counter);
  ShaderGlobal::set_float(denoiser_camera_attached_radiusVarId, params.denoiserCameraAttachedRadius);

  ShaderGlobal::set_int(ray_reconstruction_activeVarId, resolution_config.useRayReconstruction ? 1 : 0);

  if (resolution_config.useRayReconstruction && resolution_config.ptgiUseRayReconstruction)
    return false;

  reset_history = params.reset;

  // This part is mandatory needed to preserve precision by making matrices camera relative
  Point3 translationDelta = params.prevViewPos - params.viewPos;

  view_to_world = convert(params.viewItm);
  prev_view_to_world = convert(params.prevViewItm);

  view_to_world.SetTranslation(float3::Zero());
  prev_view_to_world.SetTranslation(float3(translationDelta.x, translationDelta.y, translationDelta.z));

  world_to_view = view_to_world;
  world_to_view.InvertOrtho();

  prev_world_to_view = prev_view_to_world;
  prev_world_to_view.InvertOrtho();

  world_to_clip = view_to_clip * world_to_view;
  prev_world_to_clip = prev_view_to_clip * prev_world_to_view;

  view_to_clip = convert(params.projTm);
  prev_view_to_clip = convert(params.prevProjTm);

  view_pos = float3(params.viewPos.x, params.viewPos.y, params.viewPos.z);
  prev_view_pos = float3(params.prevViewPos.x, params.prevViewPos.y, params.prevViewPos.z);

  view_dir = float3(-params.viewDir.x, -params.viewDir.y, -params.viewDir.z);
  prev_view_dir = float3(-params.prevViewDir.x, -params.prevViewDir.y, -params.prevViewDir.z);

  jitter = float2(params.jitter.x, params.jitter.y);
  prev_jitter = float2(params.prevJitter.x, params.prevJitter.y);

  motion_multiplier = float3(params.motionMultiplier.x, params.motionMultiplier.y, params.motionMultiplier.z);

  float project[3];
  ml::DecomposeProjection(ml::STYLE_D3D, ml::STYLE_D3D, view_to_clip, nullptr, nullptr, nullptr, &frustum.x, project, nullptr);
  ml::DecomposeProjection(ml::STYLE_D3D, ml::STYLE_D3D, prev_view_to_clip, nullptr, nullptr, nullptr, &frustum_prev.x, nullptr,
    nullptr);
  projectY = project[1];

  float angle1 = ml::Sequence::Weyl1D(0.5f, frame_counter) * ml::radians(90.0f);
  rotator_pre = ml::Geometry::GetRotator(angle1);

  float a0 = ml::Sequence::Weyl1D(0.0f, frame_counter * 2) * ml::radians(90.0f);
  float a1 = ml::Sequence::Bayer4x4(uint2(0, 0), frame_counter * 2) * ml::radians(360.0f);
  rotator = ml::Geometry::CombineRotators(ml::Geometry::GetRotator(a0), ml::Geometry::GetRotator(a1));

  float a2 = ml::Sequence::Weyl1D(0.0f, frame_counter * 2 + 1) * ml::radians(90.0f);
  float a3 = ml::Sequence::Bayer4x4(uint2(0, 0), frame_counter * 2 + 1) * ml::radians(360.0f);
  rotator_post = ml::Geometry::CombineRotators(ml::Geometry::GetRotator(a2), ml::Geometry::GetRotator(a3));

  Point2 scaledJitter = Point2(jitter.x * resolution.x, jitter.y * resolution.y);
  Point2 scaledPrevJitter = Point2(prev_jitter.x * resolution.x, prev_jitter.y * resolution.y);

  float jitterDx = abs(scaledJitter.x - scaledPrevJitter.x);
  float jitterDy = abs(scaledJitter.y - scaledPrevJitter.y);
  jitter_delta = max(jitterDx, jitterDy);

  static uint64_t lastTime = ref_time_ticks();
  uint64_t currentTime = ref_time_ticks();
  uint64_t dt_usec = ref_time_delta_to_usec(currentTime - lastTime);
  lastTime = currentTime;

  time_delta_ms = max(float(dt_usec) / 1000, 0.1f);

  float relativeDelta = fabsf(time_delta_ms - smooth_time_delta_ms) / min(time_delta_ms, smooth_time_delta_ms) + 1e-7f;
  float f = relativeDelta / (1.0f + relativeDelta);
  smooth_time_delta_ms = smooth_time_delta_ms + (time_delta_ms - smooth_time_delta_ms) * max(f, 1.0f / 32.0f);

  frame_rate_scale = max(33.333f / time_delta_ms, 1.0f);
  frame_rate_scale_relax = clamp(16.666f / time_delta_ms, 0.25f, 4.0f);

  float fps = frame_rate_scale * 30.0f;
  float nonLinearAccumSpeed = fps * 0.25f / (1.0f + fps * 0.25f);
  checkerboard_resolve_accum_speed = lerp(nonLinearAccumSpeed, 0.5f, jitter_delta);

  constexpr bool adaptiveAccumulation = false;
  if (adaptiveAccumulation) //-V547
  {
    constexpr int reblurMaxHistoryFrameNum = 63;
    constexpr int relaxMaxHistoryFrameNum = 255;
    constexpr float accumulationTime = 0.5;
    constexpr int maxHistoryFrameNum = min(60, min(reblurMaxHistoryFrameNum, relaxMaxHistoryFrameNum));

    bool isFastHistoryEnabled = max_accumulated_frame_num > max_fast_accumulated_frame_num;

    float fps = 1000.0f / smooth_time_delta_ms;
    float maxAccumulatedFrameNum = clamp(accumulationTime * fps, 5.0f, float(maxHistoryFrameNum));
    float maxFastAccumulatedFrameNum = isFastHistoryEnabled ? (maxAccumulatedFrameNum / 5.0f) : float(maxHistoryFrameNum);

    max_accumulated_frame_num = maxAccumulatedFrameNum + 0.5f;
    max_fast_accumulated_frame_num = maxFastAccumulatedFrameNum + 0.5f;
  }

  ShaderGlobal::set_int4(denoiser_resolutionVarId, resolution.x, resolution.y, 0, 0);
  return true;
}

static float randuf() { return rand() / float(RAND_MAX); }
static float randsf() { return 2 * randuf() - 1; }
static Point4 rand4uf() { return Point4(randuf(), randuf(), randuf(), randuf()); }
static Point4 rand4sf() { return Point4(randsf(), randsf(), randsf(), randsf()); }

// a cross frame texture of a pass: the current frame is written, the last frame is read
struct StateTexture
{
  Texture *current = nullptr;
  Texture *history = nullptr;
};

struct ShadowTextures
{
  Texture *value = nullptr;
  Texture *viewZ = nullptr;
  Texture *normalRoughness = nullptr;
  Texture *motionVectors = nullptr;
  StateTexture denoised; // the output, and the history of the stabilization
  StateTexture historyLength;
  Texture *tiles = nullptr;
  Texture *smoothTiles = nullptr;
  Texture *data1 = nullptr;
  Texture *data2 = nullptr;
  Texture *temp1 = nullptr;
  Texture *temp2 = nullptr;
};

struct AOTextures
{
  Texture *unfiltered = nullptr;
  Texture *viewZ = nullptr;
  Texture *normalRoughness = nullptr;
  Texture *motionVectors = nullptr;
  Texture *denoised = nullptr; // the output, scratch until the last pass
  StateTexture prevViewZ;
  StateTexture prevNormalRoughness;
  StateTexture prevInternalData;
  StateTexture diffHistory;
  StateTexture diffFastHistory;
  Texture *tiles = nullptr;
  Texture *diffTemp2 = nullptr;
  Texture *diffFastHistoryTemp = nullptr;
  Texture *data1 = nullptr;
};

struct GITextures
{
  Texture *unfiltered = nullptr;
  Texture *viewZ = nullptr;
  Texture *normalRoughness = nullptr;
  Texture *motionVectors = nullptr;
  Texture *denoised = nullptr; // the output, scratch until the last pass
  Texture *validation = nullptr;
  StateTexture prevViewZ;
  StateTexture prevNormalRoughness;
  StateTexture prevInternalData;
  StateTexture diffHistory;
  StateTexture diffFastHistory;
  StateTexture diffHistoryStabilized;
  StateTexture behindViewZ;
  StateTexture behindInternalData;
  StateTexture behindDiffHistory;
  StateTexture behindDiffFastHistory;
  Texture *data1 = nullptr;
  Texture *data2 = nullptr;
  Texture *diffTemp2 = nullptr;
  Texture *diffFastHistoryTemp = nullptr;
  Texture *tiles = nullptr;
};

struct ReflectionTextures
{
  Texture *unfiltered = nullptr;
  Texture *viewZ = nullptr;
  Texture *normalRoughness = nullptr;
  Texture *motionVectors = nullptr;
  Texture *denoised = nullptr; // the output, scratch until the last pass
  Texture *validation = nullptr;
  struct Reblur
  {
    StateTexture prevViewZ;
    StateTexture prevNormalRoughness;
    StateTexture prevInternalData;
    StateTexture history;
    StateTexture fastHistory;
    StateTexture historyStabilized;
    StateTexture hitDistForTracking;
    Texture *data1 = nullptr;
    Texture *data2 = nullptr;
    Texture *hitDistForTrackingTemp = nullptr;
    Texture *tmp2 = nullptr;
    Texture *fastHistoryTemp = nullptr;
    Texture *tiles = nullptr;
  } reblur;
  struct Relax
  {
    StateTexture illum;
    StateTexture illumResponsive;
    StateTexture reflectionHitT;
    StateTexture historyLength;
    StateTexture normalRoughness;
    StateTexture materialId;
    StateTexture viewZ;
    Texture *tiles = nullptr;
    Texture *historyLengthTemp = nullptr;
    Texture *reprojectionConfidence = nullptr;
    Texture *illumPing = nullptr;
    Texture *illumPong = nullptr;
  } relax;
  Texture *blurTemp = nullptr;
};

static void set_shadow_maps_bindless(Texture *csm_texture, d3d::SamplerHandle csm_sampler, Texture *vsm_texture,
  d3d::SamplerHandle vsm_sampler)
{
  if (csm_texture)
  {
    d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + csm_bindless_index, csm_texture);
    ShaderGlobal::set_int(csm_bindless_slotVarId, bindless_range + csm_bindless_index);
    auto samplerIx = d3d::register_bindless_sampler(csm_sampler);
    ShaderGlobal::set_int(csm_sampler_bindless_slotVarId, samplerIx);
  }
  else
    ShaderGlobal::set_int(csm_bindless_slotVarId, -1);

  if (vsm_texture)
  {
    d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + vsm_bindless_index, vsm_texture);
    ShaderGlobal::set_int(vsm_bindless_slotVarId, bindless_range + vsm_bindless_index);
    auto samplerIx = d3d::register_bindless_sampler(vsm_sampler);
    ShaderGlobal::set_int(vsm_sampler_bindless_slotVarId, samplerIx);
  }
}

static void denoise_shadow(const ShadowParams &params, const ShadowTextures &tex)
{
  set_shadow_maps_bindless(params.csmTexture, params.csmSampler, params.vsmTexture, params.vsmSampler);

  if (resolution_config.useRayReconstruction)
    return;

  TIME_D3D_PROFILE(denoise_shadow);
  // the textures are the rect, the history keeps the size of its frame
  const IPoint2 resolution = texture_size(tex.value);
  int width = resolution.x, height = resolution.y;
  const IPoint2 prevResolution = texture_size(tex.denoised.history);
  int prevWidth = prevResolution.x, prevHeight = prevResolution.y;

  int tilesW = divide_up(width, 16);
  int tilesH = divide_up(height, 16);

  float3 lightDirectionView =
    ml::Rotate(world_to_view, float3(-params.lightDirection.x, -params.lightDirection.y, -params.lightDirection.z));

  float3 cameraDelta = prev_view_pos - view_pos;

  uint32_t frameNum = min(params.maxStabilizedFrameNum, ShadowParams::MAX_HISTORY_FRAME_NUM);
  float stabilizationStrength = frameNum / (1.0f + frameNum);

  float unproject = 1.0f / (0.5f * height * abs(projectY));

  SigmaSharedConstants sigmaSharedConstants;
  sigmaSharedConstants.worldToView = world_to_view;
  sigmaSharedConstants.viewToClip = view_to_clip;
  sigmaSharedConstants.worldToClipPrev = prev_world_to_clip;
  sigmaSharedConstants.worldToViewPrev = prev_world_to_view;
  sigmaSharedConstants.rotator = rotator;
  sigmaSharedConstants.rotatorPost = rotator_post;
  sigmaSharedConstants.viewVectorWorld = float4(view_dir, 0);
  sigmaSharedConstants.lightDirectionView = float4(lightDirectionView, 0);
  sigmaSharedConstants.frustum = frustum;
  sigmaSharedConstants.frustumPrev = frustum_prev;
  sigmaSharedConstants.cameraDelta = float4(cameraDelta, 0);
  sigmaSharedConstants.mvScale = float4(motion_multiplier, 0);
  sigmaSharedConstants.resourceSizeInv = float2(1.0 / width, 1.0 / height);
  sigmaSharedConstants.resourceSizeInvPrev = float2(1.0 / prevWidth, 1.0 / prevHeight);
  sigmaSharedConstants.rectSize = float2(width, height);
  sigmaSharedConstants.rectSizeInv = float2(1.0 / width, 1.0 / height);
  sigmaSharedConstants.rectSizePrev = float2(prevWidth, prevHeight);
  sigmaSharedConstants.resolutionScale = float2(1, 1);
  sigmaSharedConstants.rectOffset = float2(0, 0);
  sigmaSharedConstants.printfAt = uint2(0, 0);
  sigmaSharedConstants.rectOrigin = uint2(0, 0);
  sigmaSharedConstants.rectSizeMinusOne = int2(width - 1, height - 1);
  sigmaSharedConstants.tilesSizeMinusOne = int2(tilesW - 1, tilesH - 1);
  sigmaSharedConstants.orthoMode = 0;
  sigmaSharedConstants.unproject = unproject;
  sigmaSharedConstants.denoisingRange = DENOISING_RANGE;
  sigmaSharedConstants.planeDistSensitivity = 0.02;
  sigmaSharedConstants.stabilizationStrength = reset_history ? 0 : stabilizationStrength;
  sigmaSharedConstants.debug = 0;
  sigmaSharedConstants.splitScreen = 0;
  sigmaSharedConstants.viewZScale = 1;
  sigmaSharedConstants.gMinRectDimMulUnproject = min(width, height) * unproject;
  sigmaSharedConstants.frameIndex = frame_counter;
  sigmaSharedConstants.isRectChanged = 0;

  bool stabilizationEnabled = params.maxStabilizedFrameNum > 0 && !reset_history;

  {
    TIME_D3D_PROFILE(sigma::classify_tiles);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(sigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.viewZ);
    d3d::set_tex(STAGE_CS, 1, tex.value);
    d3d::set_rwtex(STAGE_CS, 0, tex.tiles, 0, 0);

    sigmaClassifyTilesCS->dispatch(tilesW, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(sigma::smooth_tiles);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.tiles);
    d3d::set_rwtex(STAGE_CS, 0, tex.smoothTiles, 0, 0);

    sigmaSmoothTilesCS->dispatchThreads(tilesW, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(sigma::blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.viewZ);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.value);
    d3d::set_tex(STAGE_CS, 3, tex.smoothTiles);
    d3d::set_rwtex(STAGE_CS, 0, tex.data1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.temp1, 0, 0);

    sigmaBlurCS->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(sigma::post_blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.viewZ);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.data1);
    d3d::set_tex(STAGE_CS, 3, tex.smoothTiles);
    d3d::set_tex(STAGE_CS, 4, tex.temp1);
    d3d::set_rwtex(STAGE_CS, 0, tex.data2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, stabilizationEnabled ? tex.temp2 : tex.denoised.current, 0, 0);

    sigmaPostBlurCS->dispatch(tilesW * 2, tilesH, 1);
  }

  if (!stabilizationEnabled)
    clear_texture(tex.historyLength.current); // the next frame reads it as a history of no frames

  if (stabilizationEnabled)
  {
    TIME_D3D_PROFILE(sigma::temporal_stabilization);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.viewZ);
    d3d::set_tex(STAGE_CS, 1, tex.motionVectors);
    d3d::set_tex(STAGE_CS, 2, tex.data2);
    d3d::set_tex(STAGE_CS, 3, tex.temp2);
    // NRD copies its history to a temp first because it writes the same resource here, the ring does not need that
    d3d::set_tex(STAGE_CS, 4, tex.denoised.history);
    d3d::set_tex(STAGE_CS, 5, tex.historyLength.history);
    d3d::set_tex(STAGE_CS, 6, tex.smoothTiles);
    d3d::set_rwtex(STAGE_CS, 0, tex.denoised.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.historyLength.current, 0, 0);

    sigmaStabilizeCS->dispatch(tilesW * 2, tilesH, 1);
  }
}

static float getCheckerboardResolveAccumSpeed(bool need_override) { return need_override ? 0.95f : checkerboard_resolve_accum_speed; }

// the textures are the rect, the history keeps the size of its frame
static ReblurSharedConstants make_reblur_shared_constants(const Point4 &hit_dist_params, const Point2 &antilag_settings,
  bool use_history_confidence, const IPoint2 &resolution, const IPoint2 &prev_resolution, int max_stabilized_frame_num,
  bool checkerboard, bool occlusion, bool antiFirefly, bool override_checker_accum)
{
  int width = resolution.x, height = resolution.y;
  int prevWidth = prev_resolution.x, prevHeight = prev_resolution.y;

  float disocclusionThresholdBonus = (1.0f + jitter_delta) / float(height);
  float disocclusionThreshold = 0.01 + disocclusionThresholdBonus;
  float disocclusionThresholdAlternate = 0.05 + disocclusionThresholdBonus;

  float3 cameraDelta = prev_view_pos - view_pos;

  uint32_t frameNum =
    min(max_stabilized_frame_num, occlusion ? AOParams::MAX_HISTORY_FRAME_NUM : ReflectionParams::REBLUR_MAX_HISTORY_FRAME_NUM);
  float stabilizationStrength = frameNum / (1.0f + frameNum);

  float unproject = 1.0f / (0.5f * height * abs(projectY));

  float lobeAngleFraction = occlusion ? 0.5f : 0.15f;

  float minBlurRadius = 1;
  float maxBlurRadius = 30;

  float fastHistoryClampingSigmaScale = 2; // It is suggested to be 1.5
  fastHistoryClampingSigmaScale =
    ml::lerp(3.0f, fastHistoryClampingSigmaScale, ml::saturate(ml::max(maxBlurRadius, minBlurRadius) / 2.0f));

  ReblurSharedConstants reblurSharedConstants;
  // float4x4
  reblurSharedConstants.worldToClip = world_to_clip;
  reblurSharedConstants.viewToClip = view_to_clip;
  reblurSharedConstants.viewToWorld = view_to_world;
  reblurSharedConstants.worldToViewPrev = prev_world_to_view;
  reblurSharedConstants.worldToClipPrev = prev_world_to_clip;
  reblurSharedConstants.worldPrevToWorld = float4x4::Identity();

  // float4
  reblurSharedConstants.rotatorPre = rotator_pre;
  reblurSharedConstants.rotator = rotator;
  reblurSharedConstants.rotatorPost = rotator_post;
  reblurSharedConstants.frustum = frustum;
  reblurSharedConstants.frustumPrev = frustum_prev;
  reblurSharedConstants.cameraDelta = float4(cameraDelta, 0);
  reblurSharedConstants.hitDistSettings = float4(hit_dist_params.x, hit_dist_params.y, hit_dist_params.z, 0);
  reblurSharedConstants.viewVectorWorld = float4(view_dir, 0);
  reblurSharedConstants.viewVectorWorldPrev = float4(prev_view_dir, 0);
  reblurSharedConstants.mvScale = float4(motion_multiplier, 0);
  reblurSharedConstants.convergenceSettings = float4(1.0, 0.2, 0.8, 0);

  // float2
  reblurSharedConstants.antilagSettings = float2(antilag_settings.x, antilag_settings.y);
  reblurSharedConstants.resourceSize = float2(width, height);
  reblurSharedConstants.resourceSizeInv = float2(1.0 / width, 1.0 / height);
  reblurSharedConstants.resourceSizeInvPrev = float2(1.0 / prevWidth, 1.0 / prevHeight);
  reblurSharedConstants.rectSize = float2(width, height);
  reblurSharedConstants.rectSizeInv = float2(1.0 / width, 1.0 / height);
  reblurSharedConstants.rectSizePrev = float2(prevWidth, prevHeight);
  reblurSharedConstants.resolutionScale = float2(1, 1);
  reblurSharedConstants.resolutionScalePrev = float2(1, 1);
  reblurSharedConstants.rectOffset = float2(0, 0);
  reblurSharedConstants.jitter = float2(jitter.x * width, jitter.y * height);

  // uint2
  reblurSharedConstants.printfAt = uint2(0, 0);
  reblurSharedConstants.rectOrigin = uint2(0, 0);

  // int2
  reblurSharedConstants.rectSizeMinusOne = int2(width - 1, height - 1);

  // float
  reblurSharedConstants.disocclusionThreshold = disocclusionThreshold;
  reblurSharedConstants.disocclusionThresholdAlternate = disocclusionThresholdAlternate;
  reblurSharedConstants.cameraAttachedReflectionMaterialID = CAMERA_ATTACHED_REFLECTION_MATERIAL_ID;
  reblurSharedConstants.strandMaterialID = STRAND_MATERIAL_ID;
  reblurSharedConstants.strandThickness = 80e-6f;
  reblurSharedConstants.stabilizationStrength = reset_history ? 0 : stabilizationStrength;
  reblurSharedConstants.debug = 0;
  reblurSharedConstants.orthoMode = 0;
  reblurSharedConstants.unproject = unproject;
  reblurSharedConstants.denoisingRange = DENOISING_RANGE;
  reblurSharedConstants.planeDistSensitivity = 0.02;
  reblurSharedConstants.framerateScale = frame_rate_scale;
  reblurSharedConstants.minBlurRadius = minBlurRadius;
  reblurSharedConstants.maxBlurRadius = ml::max(maxBlurRadius, minBlurRadius);
  reblurSharedConstants.diffPrepassBlurRadius = 30;
  reblurSharedConstants.specPrepassBlurRadius = 50;
  reblurSharedConstants.maxAccumulatedFrameNum = reset_history ? 0 : max_accumulated_frame_num;
  reblurSharedConstants.maxFastAccumulatedFrameNum = reset_history ? 0 : max_fast_accumulated_frame_num;
  reblurSharedConstants.antiFirefly = antiFirefly ? 1 : 0;
  reblurSharedConstants.lobeAngleFraction = lobeAngleFraction * lobeAngleFraction;
  reblurSharedConstants.roughnessFraction = 0.15;
  reblurSharedConstants.historyFixFrameNum = occlusion ? 2 : 3;
  reblurSharedConstants.historyFixBasePixelStride = 14;
  reblurSharedConstants.historyFixAlternatePixelStride = 14;
  reblurSharedConstants.historyFixAlternatePixelStrideMaterialID = 999.0f;
  reblurSharedConstants.fastHistoryClampingSigmaScale = fastHistoryClampingSigmaScale;
  reblurSharedConstants.minRectDimMulUnproject = (float)min(width, height) * unproject;
  reblurSharedConstants.usePrepassNotOnlyForSpecularMotionEstimation = 1;
  reblurSharedConstants.splitScreen = 0;
  reblurSharedConstants.splitScreenPrev = 0;
  reblurSharedConstants.checkerboardResolveAccumSpeed = getCheckerboardResolveAccumSpeed(override_checker_accum);
  reblurSharedConstants.viewZScale = 1;
  reblurSharedConstants.fireflySuppressorMinRelativeScale = 2;
  reblurSharedConstants.minHitDistanceWeight = 0.1f;
  reblurSharedConstants.diffMinMaterial = 0;
  reblurSharedConstants.specMinMaterial = 0;
  reblurSharedConstants.responsiveAccumulationInvRoughnessThreshold = 1.0f / 1e-3f;

  // uint32_t
  reblurSharedConstants.responsiveAccumulationMinAccumulatedFrameNum = 3;
  reblurSharedConstants.hasHistoryConfidence = use_history_confidence ? 1 : 0;
  reblurSharedConstants.hasDisocclusionThresholdMix = 0;
  reblurSharedConstants.diffCheckerboard = checkerboard ? 1 : 2;
  reblurSharedConstants.specCheckerboard = checkerboard ? 0 : 2;
  reblurSharedConstants.frameIndex = frame_counter;
  reblurSharedConstants.isRectChanged = 0;
  reblurSharedConstants.resetHistory = reset_history ? 1 : 0;

  // Validation
  reblurSharedConstants.hasDiffuse = 0;
  reblurSharedConstants.hasSpecular = 1;

  reblurSharedConstants.hasBehindHistory = 0;
  reblurSharedConstants.secondChanceDebug = 0;

  return reblurSharedConstants;
}

static void denoise_ao(const AOParams &params, const AOTextures &tex)
{
  TIME_D3D_PROFILE(denoise_ao);

  const IPoint2 resolution = texture_size(tex.unfiltered);
  int width = resolution.x, height = resolution.y, tilesWidth = divide_up(width, 16), tilesHeight = divide_up(height, 16);
  Texture *diffTemp1 = tex.denoised;

  ReblurSharedConstants reblurSharedConstants = make_reblur_shared_constants(params.hitDistParams, params.antilagSettings, false,
    resolution, texture_size(tex.prevViewZ.history), params.maxStabilizedFrameNum, params.checkerboard, true, false, false);

  Sbuffer *reblurSharedCb =
    create_and_update_reblur_cbuffer_if_needed(reblurSharedCb_ao, reblurSharedConstants, "ReblurSharedConstantsCB_ao");

  ShaderGlobal::set_int4(denoiser_resolutionVarId, width, height, 0, 0);

  {
    TIME_D3D_PROFILE(reblur::classify_tiles);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.viewZ);
    d3d::set_rwtex(STAGE_CS, 0, tex.tiles, 0, 0);

    reblurClassifyTiles->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_accumulation);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);
    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.viewZ);
    d3d::set_tex(STAGE_CS, 3, tex.motionVectors);
    d3d::set_tex(STAGE_CS, 4, tex.prevViewZ.history);
    d3d::set_tex(STAGE_CS, 5, tex.prevNormalRoughness.history);
    d3d::set_tex(STAGE_CS, 6, tex.prevInternalData.history);
    d3d::set_tex(STAGE_CS, 7, tex.viewZ); // DUMMY
    d3d::set_tex(STAGE_CS, 8, tex.viewZ); // DUMMY
    d3d::set_tex(STAGE_CS, 9, tex.unfiltered);
    d3d::set_tex(STAGE_CS, 10, tex.diffHistory.history);
    d3d::set_tex(STAGE_CS, 11, tex.diffFastHistory.history);
    d3d::set_rwtex(STAGE_CS, 0, tex.data1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.diffTemp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, tex.diffFastHistoryTemp, 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseOcclusionTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::history_fix);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.data1);
    d3d::set_tex(STAGE_CS, 3, tex.viewZ);
    d3d::set_tex(STAGE_CS, 4, tex.diffTemp2);
    d3d::set_tex(STAGE_CS, 5, tex.diffFastHistoryTemp);
    d3d::set_rwtex(STAGE_CS, 0, diffTemp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.diffFastHistory.current, 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseOcclusionHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.viewZ);
    d3d::set_tex(STAGE_CS, 3, tex.data1);
    d3d::set_tex(STAGE_CS, 4, diffTemp1);
    d3d::set_rwtex(STAGE_CS, 0, tex.prevViewZ.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.diffTemp2, 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseOcclusionBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::post_blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.data1);
    d3d::set_tex(STAGE_CS, 3, tex.prevViewZ.current);
    d3d::set_tex(STAGE_CS, 4, tex.diffTemp2);
    d3d::set_rwtex(STAGE_CS, 0, tex.prevNormalRoughness.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.diffHistory.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, tex.prevInternalData.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, tex.denoised, 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfPostBlurNoTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseOcclusionPostBlurNoTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  d3d::set_const_buffer(STAGE_CS, 0, nullptr);
}

static void denoise_gi(const GIParams &params, const GITextures &tex)
{
  TIME_D3D_PROFILE(denoise_gi);

  const IPoint2 resolution = texture_size(tex.unfiltered);
  int width = resolution.x, height = resolution.y, tilesWidth = divide_up(width, 16), tilesHeight = divide_up(height, 16);
  Texture *diffTemp1 = tex.denoised;

  ReblurSharedConstants reblurSharedConstants =
    make_reblur_shared_constants(params.hitDistParams, params.antilagSettings, false, resolution, texture_size(tex.prevViewZ.history),
      params.maxStabilizedFrameNum, params.checkerboard, true, params.antiFirefly, false);

  // Only the validation layer reads these; the default is specular only.
  reblurSharedConstants.hasDiffuse = 1;
  reblurSharedConstants.hasSpecular = 0;

  const bool secondChance = tex.behindViewZ.current && reblurDiffuseBehindHistory;
  ShaderGlobal::set_int(denoiser_gi_second_chanceVarId, secondChance ? 1 : 0);
  reblurSharedConstants.hasBehindHistory = secondChance && !reset_history ? 1 : 0;
  reblurSharedConstants.secondChanceDebug = params.secondChanceDebug ? 1 : 0;

  Sbuffer *reblurSharedCb =
    create_and_update_reblur_cbuffer_if_needed(reblurSharedCb_gi, reblurSharedConstants, "ReblurSharedConstantsCB_gi");

  ShaderGlobal::set_int4(denoiser_resolutionVarId, width, height, 0, 0);

  {
    TIME_D3D_PROFILE(reblur::classify_tiles);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.viewZ);
    d3d::set_rwtex(STAGE_CS, 0, tex.tiles, 0, 0);

    reblurClassifyTiles->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::prepass);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.viewZ);
    d3d::set_tex(STAGE_CS, 3, tex.unfiltered);
    d3d::set_rwtex(STAGE_CS, 0, diffTemp1, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfPrepass->dispatch(tilesWidth, tilesHeight, 1);
    else
      reblurDiffusePrepass->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_accumulation);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);
    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.viewZ);
    d3d::set_tex(STAGE_CS, 3, tex.motionVectors);
    d3d::set_tex(STAGE_CS, 4, tex.prevViewZ.history);
    d3d::set_tex(STAGE_CS, 5, tex.prevNormalRoughness.history);
    d3d::set_tex(STAGE_CS, 6, tex.prevInternalData.history);
    d3d::set_tex(STAGE_CS, 7, tex.viewZ); // DUMMY
    d3d::set_tex(STAGE_CS, 8, tex.viewZ); // DUMMY
    d3d::set_tex(STAGE_CS, 9, diffTemp1);
    d3d::set_tex(STAGE_CS, 10, tex.diffHistory.history);
    d3d::set_tex(STAGE_CS, 11, tex.diffFastHistory.history);
    d3d::set_tex(STAGE_CS, 14, tex.behindViewZ.history);
    d3d::set_tex(STAGE_CS, 15, tex.behindInternalData.history);
    d3d::set_tex(STAGE_CS, 16, tex.behindDiffHistory.history);
    d3d::set_tex(STAGE_CS, 17, tex.behindDiffFastHistory.history);
    d3d::set_rwtex(STAGE_CS, 0, tex.data1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.diffTemp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, tex.diffFastHistoryTemp, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, tex.data2, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);

    for (int slot = 14; slot <= 17; ++slot)
      d3d::set_tex(STAGE_CS, slot, nullptr);
  }

  {
    TIME_D3D_PROFILE(reblur::history_fix);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.data1);
    d3d::set_tex(STAGE_CS, 3, tex.viewZ);
    d3d::set_tex(STAGE_CS, 4, tex.diffTemp2);
    d3d::set_tex(STAGE_CS, 5, tex.diffFastHistoryTemp);
    d3d::set_rwtex(STAGE_CS, 0, diffTemp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.diffFastHistory.current, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.viewZ);
    d3d::set_tex(STAGE_CS, 3, tex.data1);
    d3d::set_tex(STAGE_CS, 4, diffTemp1);
    d3d::set_rwtex(STAGE_CS, 0, tex.prevViewZ.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.diffTemp2, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::post_blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.data1);
    d3d::set_tex(STAGE_CS, 3, tex.prevViewZ.current);
    d3d::set_tex(STAGE_CS, 4, tex.diffTemp2);
    d3d::set_rwtex(STAGE_CS, 0, tex.prevNormalRoughness.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.diffHistory.current, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfPostBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffusePostBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_stabilization);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.prevViewZ.current);
    d3d::set_tex(STAGE_CS, 3, tex.data1);
    d3d::set_tex(STAGE_CS, 4, tex.data2);
    d3d::set_tex(STAGE_CS, 5, tex.diffHistory.current);
    d3d::set_tex(STAGE_CS, 6, tex.diffHistoryStabilized.history);
    d3d::set_tex(STAGE_CS, 13, tex.motionVectors);
    d3d::set_rwtex(STAGE_CS, 1, tex.prevInternalData.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, tex.denoised, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, tex.diffHistoryStabilized.current, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  if (tex.validation)
  {
    TIME_D3D_PROFILE(reblur::validation);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 1, tex.viewZ);
    d3d::set_tex(STAGE_CS, 2, tex.motionVectors);
    d3d::set_tex(STAGE_CS, 3, tex.data1);
    d3d::set_tex(STAGE_CS, 4, tex.data2);
    d3d::set_tex(STAGE_CS, 5, tex.unfiltered);
    d3d::set_tex(STAGE_CS, 6, tex.unfiltered);
    d3d::set_rwtex(STAGE_CS, 0, tex.validation, 0, 0);

    // The validation shader runs 16x16 threads, one group covers a whole tile.
    reblurValidation->dispatch(tilesWidth, tilesHeight, 1);
  }

  if (secondChance)
  {
    TIME_D3D_PROFILE(reblur::behind_history);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.prevViewZ.current);
    d3d::set_tex(STAGE_CS, 1, tex.prevInternalData.current);
    d3d::set_tex(STAGE_CS, 2, tex.diffHistory.current);
    d3d::set_tex(STAGE_CS, 3, tex.diffFastHistory.current);
    d3d::set_tex(STAGE_CS, 4, tex.behindViewZ.history);
    d3d::set_tex(STAGE_CS, 5, tex.behindInternalData.history);
    d3d::set_tex(STAGE_CS, 6, tex.behindDiffHistory.history);
    d3d::set_tex(STAGE_CS, 7, tex.behindDiffFastHistory.history);
    d3d::set_tex(STAGE_CS, 8, tex.prevViewZ.history);
    d3d::set_tex(STAGE_CS, 9, tex.prevInternalData.history);
    d3d::set_tex(STAGE_CS, 10, tex.diffHistory.history);
    d3d::set_tex(STAGE_CS, 11, tex.diffFastHistory.history);
    d3d::set_rwtex(STAGE_CS, 0, tex.behindViewZ.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.behindInternalData.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, tex.behindDiffHistory.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, tex.behindDiffFastHistory.current, 0, 0);

    reblurDiffuseBehindHistory->dispatchThreads(width, height, 1);
  }

  d3d::set_const_buffer(STAGE_CS, 0, nullptr);
}

static void denoise_reflection_reblur(const ReflectionParams &params, const ReflectionTextures &tex, bool use_smart_depth)
{
  TIME_D3D_PROFILE(denoise_reflection_reblur);

  const IPoint2 resolution = texture_size(tex.unfiltered);
  const IPoint2 tilesResolution = IPoint2(divide_up(resolution.x, 16), divide_up(resolution.y, 16));

  int width = resolution.x, height = resolution.y, tilesWidth = tilesResolution.x, tilesHeight = tilesResolution.y;
  Texture *tmp1 = tex.denoised;

  ReblurSharedConstants reblurSharedConstants = make_reblur_shared_constants(params.hitDistParams, params.antilagSettings, false,
    resolution, texture_size(tex.reblur.prevViewZ.history), params.maxStabilizedFrameNum, params.checkerboard, false,
    params.antiFirefly, use_smart_depth);

  Sbuffer *reblurSharedCb =
    create_and_update_reblur_cbuffer_if_needed(reblurSharedCb_reflection, reblurSharedConstants, "ReblurSharedConstantsCB_reflection");

  ShaderGlobal::set_int4(denoiser_resolutionVarId, width, height, 0, 0);

  {
    TIME_D3D_PROFILE(reblur::classify_tiles);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.viewZ);
    d3d::set_rwtex(STAGE_CS, 0, tex.reblur.tiles, 0, 0);

    reblurClassifyTiles->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::prepass);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.reblur.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.viewZ);
    d3d::set_tex(STAGE_CS, 3, tex.unfiltered);
    d3d::set_rwtex(STAGE_CS, 0, tmp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.reblur.hitDistForTrackingTemp, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfPrepass->dispatch(tilesWidth, tilesHeight, 1);
    else
      reblurSpecularPrepass->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_accumulation);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.reblur.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.viewZ);
    d3d::set_tex(STAGE_CS, 3, tex.motionVectors);
    d3d::set_tex(STAGE_CS, 4, tex.reblur.prevViewZ.history);
    d3d::set_tex(STAGE_CS, 5, tex.reblur.prevNormalRoughness.history);
    d3d::set_tex(STAGE_CS, 6, tex.reblur.prevInternalData.history);
    d3d::set_tex(STAGE_CS, 7, tex.viewZ); // dummy
    d3d::set_tex(STAGE_CS, 8, tex.viewZ); // dummy
    d3d::set_tex(STAGE_CS, 9, tmp1);
    d3d::set_tex(STAGE_CS, 10, tex.reblur.history.history);
    d3d::set_tex(STAGE_CS, 11, tex.reblur.fastHistory.history);
    d3d::set_tex(STAGE_CS, 12, tex.reblur.hitDistForTracking.history);
    d3d::set_tex(STAGE_CS, 13, tex.reblur.hitDistForTrackingTemp);
    d3d::set_rwtex(STAGE_CS, 0, tex.reblur.data1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.reblur.tmp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, tex.reblur.fastHistoryTemp, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, tex.reblur.hitDistForTracking.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 4, tex.reblur.data2, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurSpecularTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::history_fix);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.reblur.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.reblur.data1);
    d3d::set_tex(STAGE_CS, 3, tex.viewZ);
    d3d::set_tex(STAGE_CS, 4, tex.reblur.tmp2);
    d3d::set_tex(STAGE_CS, 5, tex.reblur.fastHistoryTemp);
    d3d::set_tex(STAGE_CS, 6, tex.reblur.hitDistForTracking.current);
    d3d::set_rwtex(STAGE_CS, 0, tmp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.reblur.fastHistory.current, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurSpecularHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.reblur.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.viewZ);
    d3d::set_tex(STAGE_CS, 3, tex.reblur.data1);
    d3d::set_tex(STAGE_CS, 4, tmp1);
    d3d::set_rwtex(STAGE_CS, 0, tex.reblur.prevViewZ.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.reblur.tmp2, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurSpecularBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::post_blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.reblur.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.reblur.data1);
    d3d::set_tex(STAGE_CS, 3, tex.reblur.prevViewZ.current);
    d3d::set_tex(STAGE_CS, 4, tex.reblur.tmp2);
    d3d::set_rwtex(STAGE_CS, 0, tex.reblur.prevNormalRoughness.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.reblur.history.current, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfPostBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurSpecularPostBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_stabilization);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.reblur.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.reblur.prevViewZ.current);
    d3d::set_tex(STAGE_CS, 3, tex.reblur.data1);
    d3d::set_tex(STAGE_CS, 4, tex.reblur.data2);
    d3d::set_tex(STAGE_CS, 5, tex.reblur.hitDistForTracking.current);
    d3d::set_tex(STAGE_CS, 6, tex.reblur.history.current);
    d3d::set_tex(STAGE_CS, 7, tex.reblur.historyStabilized.history);
    d3d::set_tex(STAGE_CS, 13, tex.motionVectors);
    d3d::set_rwtex(STAGE_CS, 1, tex.reblur.prevInternalData.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, tex.denoised, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, tex.reblur.historyStabilized.current, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurSpecularTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  if (tex.validation)
  {
    TIME_D3D_PROFILE(reblur::validation);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb);
    d3d::set_tex(STAGE_CS, 0, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 1, tex.viewZ);
    d3d::set_tex(STAGE_CS, 2, tex.motionVectors);
    d3d::set_tex(STAGE_CS, 3, tex.reblur.data1);
    d3d::set_tex(STAGE_CS, 4, tex.reblur.data2);
    d3d::set_tex(STAGE_CS, 5, tex.unfiltered);
    d3d::set_tex(STAGE_CS, 6, tex.unfiltered);
    d3d::set_rwtex(STAGE_CS, 0, tex.validation, 0, 0);

    reblurValidation->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  d3d::set_const_buffer(STAGE_CS, 0, nullptr);
}

// From the NRD library
inline float3 RELAX_GetFrustumForward(const float4x4 &viewToWorld, const float4 &frustum)
{
  float4 frustumForwardView =
    float4(0.5f, 0.5f, 1.0f, 0.0f) * float4(frustum.z, frustum.w, 1.0f, 0.0f) + float4(frustum.x, frustum.y, 0.0f, 0.0f);
  float3 frustumForwardWorld = (viewToWorld * frustumForwardView).xyz;

  // Vector is not normalized for non-symmetric projections, it has to have .z = 1.0 to correctly reconstruct world position in shaders
  return frustumForwardWorld;
}

// the textures are the rect, the history keeps the size of its frame
static RelaxSharedConstants make_relax_shared_constants(const IPoint2 &resolution, const IPoint2 &prev_resolution, bool checkerboard,
  int max_stabilized_frame_num, int max_fast_stabilized_frame_num, bool override_checker_accum)
{
  int width = resolution.x, height = resolution.y;
  int prevWidth = prev_resolution.x, prevHeight = prev_resolution.y;

  float unproject = 1.0f / (0.5f * height * abs(projectY));

  float tanHalfFov = 1.0f / view_to_clip.a00;
  float aspect = view_to_clip.a00 / view_to_clip.a11;
  float3 frustumRight = float3(world_to_view.Row(0).xyz) * tanHalfFov;
  float3 frustumUp = float3(world_to_view.Row(1).xyz) * tanHalfFov * aspect;
  float3 frustumForward = RELAX_GetFrustumForward(view_to_world, frustum);

  float prevTanHalfFov = 1.0f / prev_view_to_clip.a00;
  float prevAspect = prev_view_to_clip.a00 / prev_view_to_clip.a11;
  float3 prevFrustumRight = float3(prev_world_to_view.Row(0).xyz) * prevTanHalfFov;
  float3 prevFrustumUp = float3(prev_world_to_view.Row(1).xyz) * prevTanHalfFov * prevAspect;
  float3 prevFrustumForward = RELAX_GetFrustumForward(prev_view_to_world, frustum_prev);

  float3 cameraDelta = prev_view_pos - view_pos;

  float specMaxAccumulatedFrameNum = reset_history ? 0 : min(max_stabilized_frame_num, ReflectionParams::RELAX_MAX_HISTORY_FRAME_NUM);
  float specMaxFastAccumulatedFrameNum =
    reset_history ? 0 : min(max_fast_stabilized_frame_num, ReflectionParams::RELAX_MAX_HISTORY_FRAME_NUM);
  float diffMaxAccumulatedFrameNum = reset_history ? 0 : min(30, ReflectionParams::RELAX_MAX_HISTORY_FRAME_NUM);
  float diffMaxFastAccumulatedFrameNum = reset_history ? 0 : min(6, ReflectionParams::RELAX_MAX_HISTORY_FRAME_NUM);

  float disocclusionThresholdBonus = (1.0f + jitter_delta) / float(height);
  float disocclusionThreshold = 0.01 + disocclusionThresholdBonus;
  float disocclusionThresholdAlternate = 0.05 + disocclusionThresholdBonus;

  float maxDiffuseLuminanceRelativeDifference = -log(0);
  float maxSpecularLuminanceRelativeDifference = -log(0);

  RelaxSharedConstants relaxSharedConstants;

  // float4x4
  relaxSharedConstants.worldToClip = world_to_clip;
  relaxSharedConstants.worldToClipPrev = prev_world_to_clip;
  relaxSharedConstants.worldToViewPrev = prev_world_to_view;
  relaxSharedConstants.worldPrevToWorld = float4x4::Identity();

  // float4
  relaxSharedConstants.rotatorPre = rotator_pre;
  relaxSharedConstants.frustumRight = float4(frustumRight, 0);
  relaxSharedConstants.frustumUp = float4(frustumUp, 0);
  relaxSharedConstants.frustumForward = float4(frustumForward, 0);
  relaxSharedConstants.prevFrustumRight = float4(prevFrustumRight, 0);
  relaxSharedConstants.prevFrustumUp = float4(prevFrustumUp, 0);
  relaxSharedConstants.prevFrustumForward = float4(prevFrustumForward, 0);
  relaxSharedConstants.cameraDelta = float4(cameraDelta, 0);
  relaxSharedConstants.mvScale = float4(motion_multiplier, 0);

  // float2
  relaxSharedConstants.jitter = float2(jitter.x * width, jitter.y * height);
  relaxSharedConstants.resolutionScale = float2(1, 1);
  relaxSharedConstants.rectOffset = float2(0, 0);
  relaxSharedConstants.resourceSizeInv = float2(1.0 / width, 1.0 / height);
  relaxSharedConstants.resourceSize = float2(width, height);
  relaxSharedConstants.rectSizeInv = float2(1.0 / width, 1.0 / height);
  relaxSharedConstants.rectSizePrev = float2(prevWidth, prevHeight);
  relaxSharedConstants.resourceSizeInvPrev = float2(1.0 / prevWidth, 1.0 / prevHeight);

  // uint2
  relaxSharedConstants.printfAt = uint2(0, 0);
  relaxSharedConstants.rectOrigin = uint2(0, 0);

  // int2
  relaxSharedConstants.rectSize = int2(width, height);

  // float
  relaxSharedConstants.specMaxAccumulatedFrameNum = specMaxAccumulatedFrameNum;
  relaxSharedConstants.specMaxFastAccumulatedFrameNum = specMaxFastAccumulatedFrameNum;
  relaxSharedConstants.diffMaxAccumulatedFrameNum = diffMaxAccumulatedFrameNum;
  relaxSharedConstants.diffMaxFastAccumulatedFrameNum = diffMaxFastAccumulatedFrameNum;
  relaxSharedConstants.disocclusionThreshold = disocclusionThreshold;
  relaxSharedConstants.disocclusionThresholdAlternate = disocclusionThresholdAlternate;
  relaxSharedConstants.cameraAttachedReflectionMaterialID = CAMERA_ATTACHED_REFLECTION_MATERIAL_ID;
  relaxSharedConstants.strandMaterialID = STRAND_MATERIAL_ID;
  relaxSharedConstants.strandThickness = 80e-6f;
  relaxSharedConstants.roughnessFraction = 0.15;
  relaxSharedConstants.specVarianceBoost = 0;
  relaxSharedConstants.splitScreen = 0;
  relaxSharedConstants.diffBlurRadius = 30;
  relaxSharedConstants.specBlurRadius = 50;
  relaxSharedConstants.depthThreshold = 0.003;
  relaxSharedConstants.lobeAngleFraction = 0.5;
  relaxSharedConstants.specLobeAngleSlack = ml::Math::DegToRad(0.15);
  relaxSharedConstants.historyFixEdgeStoppingNormalPower = 8;
  relaxSharedConstants.roughnessEdgeStoppingRelaxation = 1;
  relaxSharedConstants.normalEdgeStoppingRelaxation = 0.3;
  relaxSharedConstants.fastHistoryClampingSigmaScale = 2;
  relaxSharedConstants.historyAccelerationAmount = 0.3;
  relaxSharedConstants.historyResetTemporalSigmaScale = 0.5;
  relaxSharedConstants.historyResetSpatialSigmaScale = 4.5;
  relaxSharedConstants.historyResetAmount = 0.5;
  relaxSharedConstants.denoisingRange = DENOISING_RANGE;
  relaxSharedConstants.specPhiLuminance = 1;
  relaxSharedConstants.diffPhiLuminance = 2;
  relaxSharedConstants.diffMaxLuminanceRelativeDifference = maxDiffuseLuminanceRelativeDifference;
  relaxSharedConstants.specMaxLuminanceRelativeDifference = maxSpecularLuminanceRelativeDifference;
  relaxSharedConstants.luminanceEdgeStoppingRelaxation = 1;
  relaxSharedConstants.confidenceDrivenRelaxationMultiplier = 0;
  relaxSharedConstants.confidenceDrivenLuminanceEdgeStoppingRelaxation = 0;
  relaxSharedConstants.confidenceDrivenNormalEdgeStoppingRelaxation = 0;
  relaxSharedConstants.debug = 0;
  relaxSharedConstants.orthoMode = 0;
  relaxSharedConstants.unproject = unproject;
  relaxSharedConstants.framerateScale = frame_rate_scale_relax;
  relaxSharedConstants.checkerboardResolveAccumSpeed = getCheckerboardResolveAccumSpeed(override_checker_accum);
  relaxSharedConstants.jitterDelta = jitter_delta;
  relaxSharedConstants.historyFixFrameNum = 4;
  relaxSharedConstants.historyFixBasePixelStride = 14;
  relaxSharedConstants.historyFixAlternatePixelStride = 14;
  relaxSharedConstants.historyFixAlternatePixelStrideMaterialID = 999;
  relaxSharedConstants.historyThreshold = 3;
  relaxSharedConstants.viewZScale = 1;
  relaxSharedConstants.minHitDistanceWeight = 0.2;
  relaxSharedConstants.diffMinMaterial = 0;
  relaxSharedConstants.specMinMaterial = 0;

  // uint32_t
  relaxSharedConstants.roughnessEdgeStoppingEnabled = 1;
  relaxSharedConstants.frameIndex = frame_counter;
  relaxSharedConstants.diffCheckerboard = checkerboard ? 1 : 2;
  relaxSharedConstants.specCheckerboard = checkerboard ? 0 : 2;
  relaxSharedConstants.hasHistoryConfidence = 0;
  relaxSharedConstants.hasDisocclusionThresholdMix = 0;
  relaxSharedConstants.resetHistory = reset_history ? 1 : 0;

  relaxSharedConstants.stepSize = 0;
  relaxSharedConstants.isLastPass = 0;

  return relaxSharedConstants;
}

static void denoise_reflection_relax(const ReflectionParams &params, const ReflectionTextures &tex, bool use_smart_depth)
{
  TIME_D3D_PROFILE(denoise_reflection_relax);

  const IPoint2 resolution = texture_size(tex.unfiltered);
  const IPoint2 tilesResolution = IPoint2(divide_up(resolution.x, 16), divide_up(resolution.y, 16));

  int width = resolution.x, height = resolution.y, tilesWidth = tilesResolution.x, tilesHeight = tilesResolution.y;

  RelaxSharedConstants relaxSharedConstants = make_relax_shared_constants(resolution, texture_size(tex.relax.viewZ.history),
    params.checkerboard, params.maxStabilizedFrameNum, params.maxFastStabilizedFrameNum, use_smart_depth);

  ShaderGlobal::set_int4(denoiser_resolutionVarId, width, height, 0, 0);

  {
    TIME_D3D_PROFILE(relax::classify_tiles);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.viewZ);
    d3d::set_rwtex(STAGE_CS, 0, tex.relax.tiles, 0, 0);

    relaxClassifyTiles->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(relax::prepass);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.relax.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 2, tex.viewZ);
    d3d::set_tex(STAGE_CS, 3, tex.unfiltered);
    d3d::set_rwtex(STAGE_CS, 0, tex.denoised, 0, 0);

    relaxSpecularPrepass->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(relax::temporal_accumulation);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.relax.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.motionVectors);
    d3d::set_tex(STAGE_CS, 2, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 3, tex.viewZ);
    d3d::set_tex(STAGE_CS, 4, tex.viewZ); // DUMMY
    d3d::set_tex(STAGE_CS, 5, tex.relax.normalRoughness.history);
    d3d::set_tex(STAGE_CS, 6, tex.relax.viewZ.history);
    d3d::set_tex(STAGE_CS, 7, tex.relax.historyLength.history);
    d3d::set_tex(STAGE_CS, 8, tex.relax.materialId.history);
    d3d::set_tex(STAGE_CS, 9, tex.denoised);
    d3d::set_tex(STAGE_CS, 10, tex.relax.illumResponsive.history);
    d3d::set_tex(STAGE_CS, 11, tex.relax.illum.history);
    d3d::set_tex(STAGE_CS, 12, tex.relax.reflectionHitT.history);
    d3d::set_tex(STAGE_CS, 13, tex.viewZ); // DUMMY
    d3d::set_rwtex(STAGE_CS, 0, tex.relax.historyLengthTemp, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.relax.illumPing, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, tex.relax.illumPong, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, tex.relax.reflectionHitT.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 4, tex.relax.reprojectionConfidence, 0, 0);

    relaxSpecularTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(relax::history_fix);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.relax.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.relax.historyLengthTemp);
    d3d::set_tex(STAGE_CS, 2, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 3, tex.viewZ);
    d3d::set_tex(STAGE_CS, 4, tex.relax.illumPing);
    d3d::set_rwtex(STAGE_CS, 0, tex.relax.illumPong, 0, 0);

    relaxSpecularHistoryFix->dispatch(tilesWidth * 2, tilesHeight * 2, 1);
  }

  {
    TIME_D3D_PROFILE(relax::history_clamping);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.relax.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.viewZ);
    d3d::set_tex(STAGE_CS, 2, tex.relax.historyLengthTemp);
    d3d::set_tex(STAGE_CS, 3, tex.denoised);
    d3d::set_tex(STAGE_CS, 4, tex.relax.illumPing);
    d3d::set_tex(STAGE_CS, 5, tex.relax.illumPong);
    d3d::set_rwtex(STAGE_CS, 0, tex.relax.historyLength.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, tex.relax.illum.current, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, tex.relax.illumResponsive.current, 0, 0);

    relaxSpecularHistoryClamping->dispatch(tilesWidth * 2, tilesHeight * 2, 1);
  }

  relaxSharedConstants.stepSize = 1;
  relaxSharedConstants.isLastPass = 0;

  for (int iter = 0; iter < 5; ++iter)
  {
    TIME_D3D_PROFILE(relax::a - torus);

    relaxSharedConstants.stepSize = 1 << iter;
    relaxSharedConstants.isLastPass = (iter == 4) ? 1 : 0;

    bool isSmem = iter == 0;
    bool isEven = iter % 2 == 0;
    bool isLast = iter == 4;

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.relax.tiles);
    d3d::set_tex(STAGE_CS, 1, tex.relax.historyLengthTemp);
    d3d::set_tex(STAGE_CS, 2, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 3, tex.viewZ);
    if (isSmem)
      d3d::set_tex(STAGE_CS, 4, tex.relax.illum.current);
    else if (isEven)
      d3d::set_tex(STAGE_CS, 4, tex.relax.illumPong);
    else
      d3d::set_tex(STAGE_CS, 4, tex.relax.illumPing);
    d3d::set_tex(STAGE_CS, 5, tex.relax.reprojectionConfidence);
    d3d::set_tex(STAGE_CS, 6, tex.viewZ); // hasConfidenceInputs

    if (isLast)
      d3d::set_rwtex(STAGE_CS, 0, tex.denoised, 0, 0);
    else if (isEven)
      d3d::set_rwtex(STAGE_CS, 0, tex.relax.illumPing, 0, 0);
    else
      d3d::set_rwtex(STAGE_CS, 0, tex.relax.illumPong, 0, 0);

    if (isSmem)
    {
      d3d::set_rwtex(STAGE_CS, 1, tex.relax.normalRoughness.current, 0, 0);
      d3d::set_rwtex(STAGE_CS, 2, tex.relax.materialId.current, 0, 0);
      d3d::set_rwtex(STAGE_CS, 3, tex.relax.viewZ.current, 0, 0);
    }

    if (isSmem)
      relaxSpecularATorusSmem->dispatch(tilesWidth * 2, tilesHeight * 2, 1);
    else
      relaxSpecularATorus->dispatch(tilesWidth, tilesHeight, 1);
  }

  if (tex.validation)
  {
    TIME_D3D_PROFILE(relax::validation);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tex.normalRoughness);
    d3d::set_tex(STAGE_CS, 1, tex.viewZ);
    d3d::set_tex(STAGE_CS, 2, tex.motionVectors);
    d3d::set_tex(STAGE_CS, 3, tex.relax.historyLengthTemp);
    d3d::set_rwtex(STAGE_CS, 0, tex.validation, 0, 0);

    relaxValidation->dispatch(tilesWidth * 2, tilesHeight, 1);
  }
}

static void blur_reflection(const ReflectionParams &params, const ReflectionTextures &tex)
{
  TIME_D3D_PROFILE(blur_reflection);

  // made on the first use: the other methods run with a shader dump that lacks it
  if (!reflectionBlurCS)
    reflectionBlurCS = new_compute_shader("denoiser_reflection_blur");
  if (!reflectionBlurCS)
    return;

  const IPoint2 resolution = texture_size(tex.unfiltered);
  const float radius = params.blurRadius * resolution.y;
  const float texelsPerUnit = 0.5f * resolution.y * fabsf(projectY); // the inverse of the unproject of NRD
  // a checkerboard trace shoots the texels where x + y has the parity of the frame, the first pass fills the others. 2 is a
  // full source, as in NRD
  const int checkerboard = params.checkerboard ? int(frame_counter & 1) : 2;
  ShaderGlobal::set_int4(denoiser_resolutionVarId, resolution.x, resolution.y, 0, 0);
  ShaderGlobal::set_float(denoiser_blur_max_radiusVarId, radius);
  ShaderGlobal::set_float(denoiser_blur_max_roughnessVarId, params.blurMaxRoughness);
  ShaderGlobal::set_float(denoiser_blur_texels_per_unitVarId, texelsPerUnit);
  ShaderGlobal::set_texture(denoiser_blur_view_zVarId, tex.viewZ);
  ShaderGlobal::set_texture(denoiser_blur_nrVarId, tex.normalRoughness);

  auto pass = [&](Texture *src, Texture *dst, bool vertical, bool last) {
    ShaderGlobal::set_texture(denoiser_blur_srcVarId, src);
    ShaderGlobal::set_texture(denoiser_blur_dstVarId, dst);
    ShaderGlobal::set_int(denoiser_blur_verticalVarId, vertical ? 1 : 0);
    ShaderGlobal::set_int(denoiser_blur_last_passVarId, last ? 1 : 0);
    ShaderGlobal::set_int(denoiser_blur_checkerboardVarId, last ? 2 : checkerboard);
    reflectionBlurCS->dispatchThreads(resolution.x, resolution.y, 1);
  };
  pass(tex.unfiltered, tex.blurTemp, false, false);
  pass(tex.blurTemp, tex.denoised, true, true);

  ShaderGlobal::set_texture(denoiser_blur_srcVarId, nullptr);
  ShaderGlobal::set_texture(denoiser_blur_dstVarId, nullptr);
  ShaderGlobal::set_texture(denoiser_blur_view_zVarId, nullptr);
  ShaderGlobal::set_texture(denoiser_blur_nrVarId, nullptr);

  // the pyramid for the lobes wider than the blur
  if (!reflectionMipFirstCS)
    reflectionMipFirstCS = new_compute_shader("denoiser_reflection_mip_first");
  if (!reflectionMipCS)
    reflectionMipCS = new_compute_shader("denoiser_reflection_mip");
  if (!reflectionMipFirstCS || !reflectionMipCS)
    return;
  TextureInfo info;
  tex.denoised->getinfo(info);
  for (int mip = 1; mip < info.mipLevels; ++mip)
  {
    TextureInfo srcInfo, dstInfo;
    tex.denoised->getinfo(srcInfo, mip - 1);
    tex.denoised->getinfo(dstInfo, mip);
    ShaderGlobal::set_int4(denoiser_reflection_mip_dst_sizeVarId, dstInfo.w, dstInfo.h, 0, 0);
    ShaderGlobal::set_int4(denoiser_reflection_mip_src_sizeVarId, srcInfo.w, srcInfo.h, 0, 0);
    d3d::resource_barrier({tex.denoised, RB_RO_SRV | RB_STAGE_COMPUTE, unsigned(mip - 1), 1});
    tex.denoised->texmiplevel(mip - 1, mip - 1);
    d3d::set_tex(STAGE_CS, 0, tex.denoised);
    d3d::set_rwtex(STAGE_CS, 0, tex.denoised, 0, mip);
    (mip == 1 ? reflectionMipFirstCS : reflectionMipCS)->dispatchThreads(dstInfo.w, dstInfo.h, 1);
  }
  d3d::set_tex(STAGE_CS, 0, nullptr);
  d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  tex.denoised->texmiplevel(-1, -1);
  d3d::resource_barrier({tex.denoised, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
}

static void denoise_reflection(const ReflectionParams &params, const ReflectionTextures &tex, ReflectionMethod method,
  bool use_smart_depth)
{
  switch (method)
  {
    case ReflectionMethod::Reblur: denoise_reflection_reblur(params, tex, use_smart_depth); break;
    case ReflectionMethod::Relax: denoise_reflection_relax(params, tex, use_smart_depth); break;
    case ReflectionMethod::Blur: blur_reflection(params, tex); break;
    default: G_ASSERTF(false, "Reflection denoiser not implemented!");
  }
}

int get_frame_number() { return frame_counter; }

// --- daFG nodes --------------------------------------------------------------------------------------------

using OrderingToken = eastl::monostate; // the type of an ordering blob, as the games name it

// set at the registration of the prepare node and of the reflection node, read when a trace declares its view z
// and when a reader declares the prepare outputs an effect publishes through bindless
static bool shadow_trace_in_graph = false;
static eastl::optional<Inputs> reflection_inputs_in_graph;
// the auto resolution of the prepare node, every texture of the lib follows it
static eastl::string resolution_name;

using RwTex = eastl::optional<dafg::VirtualResourceHandle<BaseTexture, true, false>>;
using RoTex = eastl::optional<dafg::VirtualResourceHandle<const BaseTexture, true, false>>;

template <typename H>
static Texture *ptr(const eastl::optional<H> &handle)
{
  return handle ? handle->get() : nullptr;
}

static const char *view_z_name(Inputs inputs)
{
  return inputs == Inputs::Full ? "denoiser_view_z" : inputs == Inputs::Half ? "denoiser_half_view_z" : "denoiser_half_view_z_rtr";
}

static const char *normal_roughness_name(Inputs inputs)
{
  return inputs == Inputs::Full ? "denoiser_nr" : inputs == Inputs::Half ? "denoiser_half_nr" : "denoiser_half_nr_rtr";
}

Inputs reflection_inputs(bool checkerboard_depth, bool smart_depth)
{
  return !resolution_config.rtrHalfRes ? Inputs::Full : (checkerboard_depth || smart_depth) ? Inputs::HalfRtr : Inputs::Half;
}

static const char *reflection_view_z_name(Inputs inputs)
{
  return inputs == Inputs::Full   ? "denoiser_view_z_reflection"
         : inputs == Inputs::Half ? "denoiser_half_view_z_reflection"
                                  : "denoiser_half_view_z_rtr_reflection";
}

// the name of a view z after the traces that mark it, for a reader after them
static const char *final_view_z_name(Inputs inputs)
{
  if (reflection_inputs_in_graph.has_value() && *reflection_inputs_in_graph == inputs)
    return reflection_view_z_name(inputs);
  if (inputs == Inputs::Full && shadow_trace_in_graph)
    return "denoiser_view_z_shadow";
  return view_z_name(inputs);
}

dafg::NodeHandle make_gi_view_z_node()
{
  return dafg::root().registerNode("denoiser_gi_view_z", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    const Config &cfg = resolution_config;
    if (!cfg.ptgiUseRayReconstruction)
      registry.renameTexture(final_view_z_name(cfg.ptgiHalfRes ? Inputs::Half : Inputs::Full), "denoiser_gi_view_z");
  });
}

dafg::NodeHandle make_reflection_normal_roughness_node()
{
  return dafg::root().registerNode("denoiser_reflection_normal_roughness", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    if (reflection_inputs_in_graph)
      registry.renameTexture(normal_roughness_name(*reflection_inputs_in_graph), "denoiser_reflection_normal_roughness");
  });
}

static int bindless_sampler(bool linear) { return d3d::register_bindless_sampler(linear ? samplerLinearClamp : samplerNearestClamp); }

ParamsHandle<PrepareParams> create_prepare_params(dafg::Registry registry)
{
  return registry.createBlob<PrepareParams>("denoiser_prepare_params").handle();
}
ParamsHandle<ShadowParams> create_shadow_params(dafg::Registry registry)
{
  return registry.createBlob<ShadowParams>("denoiser_shadow_params").handle();
}
ParamsHandle<AOParams> create_ao_params(dafg::Registry registry)
{
  return registry.createBlob<AOParams>("denoiser_ao_params").handle();
}
ParamsHandle<GIParams> create_gi_params(dafg::Registry registry)
{
  return registry.createBlob<GIParams>("denoiser_gi_params").handle();
}
ParamsHandle<ReflectionParams> create_reflection_params(dafg::Registry registry)
{
  return registry.createBlob<ReflectionParams>("denoiser_reflection_params").handle();
}

OutputRwHandle create_shadow_trace_output(dafg::Registry registry, const char *shader_var)
{
  const uint32_t format = resolution_config.useRayReconstruction ? TEXFMT_R8 : TEXFMT_R16F;
  return registry.create("rtsm_value")
    .texture({format | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str())})
    .atStage(dafg::Stage::CS)
    .bindToShaderVar(shader_var)
    .handle();
}

OutputRwHandle create_ao_trace_output(dafg::Registry registry, const char *shader_var)
{
  const bool halfRes = resolution_config.rtaoHalfRes;
  return registry.create("rtao_tex_unfiltered")
    .texture({TEXFMT_L16 | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str(), halfRes ? 0.5f : 1.f)})
    .atStage(dafg::Stage::CS)
    .bindToShaderVar(shader_var)
    .handle();
}

OutputRwHandle create_gi_trace_output(dafg::Registry registry, const char *shader_var)
{
  const bool halfRes = resolution_config.ptgiHalfRes;
  return registry.create("ptgi_tex_unfiltered")
    .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str(), halfRes ? 0.5f : 1.f)})
    .atStage(dafg::Stage::CS)
    .bindToShaderVar(shader_var)
    .handle();
}

OutputRwHandle create_reflection_trace_output(dafg::Registry registry, const char *shader_var)
{
  const Config &cfg = resolution_config;
  const bool halfRes = cfg.rtrHalfRes;
  if (cfg.useRayReconstruction)
    return registry.create("rtr_tex_unfiltered")
      .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str(), halfRes ? 0.5f : 1.f)})
      .clear(make_clear_value(0.f, 0.f, 0.f, 1.f))
      .atStage(dafg::Stage::CS)
      .bindToShaderVar(shader_var)
      .handle();
  return registry.create("rtr_tex_unfiltered")
    .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str(), halfRes ? 0.5f : 1.f)})
    .atStage(dafg::Stage::CS)
    .bindToShaderVar(shader_var)
    .handle();
}

OutputRwHandle create_reflection_output(dafg::Registry registry, const char *shader_var, ReflectionMethod method)
{
  if (resolution_config.useRayReconstruction)
    return eastl::nullopt;
  const bool halfRes = resolution_config.rtrHalfRes;
  const uint32_t mipLevels = method == ReflectionMethod::Blur ? dafg::AUTO_MIP_COUNT : 1;
  return registry.create("rtr_tex")
    .texture(
      {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str(), halfRes ? 0.5f : 1.f), mipLevels})
    .clear(make_clear_value(0.f, 0.f, 0.f, 0.f))
    .atStage(dafg::Stage::CS)
    .bindToShaderVar(shader_var)
    .handle();
}

dafg::AutoResolutionRequest<2> reflection_resolution(dafg::Registry registry)
{
  return registry.getResolution<2>(resolution_name.c_str(), resolution_config.rtrHalfRes ? 0.5f : 1.f);
}

void read_normal_roughness(dafg::Registry registry, const char *shader_var, Inputs inputs)
{
  if (shader_var)
    registry.root().readTexture(normal_roughness_name(inputs)).atStage(dafg::Stage::PS_OR_CS).bindToShaderVar(shader_var).optional();
  else
    registry.root()
      .readTexture(normal_roughness_name(inputs))
      .atStage(dafg::Stage::PS_OR_CS)
      .useAs(dafg::Usage::SHADER_RESOURCE)
      .optional();
}

void read_selected_depth(dafg::Registry registry, const char *shader_var)
{
  if (shader_var)
    registry.root().readTexture("half_selected_depth_rtr").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar(shader_var).optional();
  else
    registry.root()
      .readTexture("half_selected_depth_rtr")
      .atStage(dafg::Stage::PS_OR_CS)
      .useAs(dafg::Usage::SHADER_RESOURCE)
      .optional();
}

void order_after_prepare(dafg::Registry registry) { registry.root().readBlob<OrderingToken>("denoiser_prepared"); }

void modify_view_z_for_shadow(dafg::Registry registry, const char *shader_var)
{
  if (resolution_config.useRayReconstruction)
    return;
  registry.renameTexture("denoiser_view_z", "denoiser_view_z_shadow").atStage(dafg::Stage::CS).bindToShaderVar(shader_var);
}

void modify_view_z_for_reflection(dafg::Registry registry, const char *shader_var, Inputs inputs)
{
  if (resolution_config.useRayReconstruction)
    return;
  const char *from =
    inputs == Inputs::Full ? (shadow_trace_in_graph ? "denoiser_view_z_shadow" : "denoiser_view_z") : view_z_name(inputs);
  registry.renameTexture(from, reflection_view_z_name(inputs)).atStage(dafg::Stage::CS).bindToShaderVar(shader_var);
}

OutputHandle read_shadow_output(dafg::Registry registry)
{
  if (!d3d::get_driver_desc().caps.hasBindless)
    return registry.root()
      .readTexture("rtsm_shadows_denoised")
      .atStage(dafg::Stage::PS_OR_CS)
      .useAs(dafg::Usage::SHADER_RESOURCE)
      .optional()
      .handle();
  return registry.root()
    .readTexture("rtsm_shadows_denoised")
    .atStage(dafg::Stage::PS_OR_CS)
    .bindlessShaderVar("rtsm_bindless_slot")
    .optional()
    .handle();
}

OutputHandle read_ao_output(dafg::Registry registry)
{
  if (!d3d::get_driver_desc().caps.hasBindless)
    return registry.root()
      .readTexture("denoised_ao")
      .atStage(dafg::Stage::PS_OR_CS)
      .useAs(dafg::Usage::SHADER_RESOURCE)
      .optional()
      .handle();
  return registry.root()
    .readTexture("denoised_ao")
    .atStage(dafg::Stage::PS_OR_CS)
    .bindlessShaderVar("rtao_bindless_slot")
    .optional()
    .handle();
}

OutputHandle read_gi_output(dafg::Registry registry)
{
  // the denoise publishes its view z through bindless too, the reader keeps it alive
  if (!d3d::get_driver_desc().caps.hasBindless)
  {
    registry.root().readTexture("denoiser_gi_view_z").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).optional();
    return registry.root()
      .readTexture("denoised_gi")
      .atStage(dafg::Stage::PS_OR_CS)
      .useAs(dafg::Usage::SHADER_RESOURCE)
      .optional()
      .handle();
  }
  registry.root()
    .readTexture("denoiser_gi_view_z")
    .atStage(dafg::Stage::PS_OR_CS)
    .bindlessShaderVar("ptgi_depth_bindless_slot")
    .optional();
  return registry.root()
    .readTexture("denoised_gi")
    .atStage(dafg::Stage::PS_OR_CS)
    .bindlessShaderVar("ptgi_bindless_slot")
    .optional()
    .handle();
}

OutputHandle read_reflection_output(dafg::Registry registry)
{
  // the denoise publishes its normal roughness through bindless too, the reader keeps it alive
  if (!d3d::get_driver_desc().caps.hasBindless)
  {
    registry.root()
      .readTexture("denoiser_reflection_normal_roughness")
      .atStage(dafg::Stage::PS_OR_CS)
      .useAs(dafg::Usage::SHADER_RESOURCE)
      .optional();
    return registry.root()
      .readTexture("rtr_tex")
      .atStage(dafg::Stage::PS_OR_CS)
      .useAs(dafg::Usage::SHADER_RESOURCE)
      .optional()
      .handle();
  }
  registry.root()
    .readTexture("denoiser_reflection_normal_roughness")
    .atStage(dafg::Stage::PS_OR_CS)
    .bindlessShaderVar("rtr_normal_roughness_bindless_slot")
    .optional();
  return registry.root()
    .readTexture("rtr_tex")
    .atStage(dafg::Stage::PS_OR_CS)
    .bindlessShaderVar("rtr_bindless_slot")
    .optional()
    .handle();
}

OutputHandle read_reflection_trace_output(dafg::Registry registry)
{
  if (resolution_config.useRayReconstruction)
    return eastl::nullopt;
  return registry.root()
    .readTexture("rtr_tex_unfiltered")
    .atStage(dafg::Stage::PS_OR_CS)
    .useAs(dafg::Usage::SHADER_RESOURCE)
    .optional()
    .handle();
}

OutputHandle read_gi_validation(dafg::Registry registry)
{
  return registry.root()
    .readTexture("gi_validation")
    .atStage(dafg::Stage::PS_OR_CS)
    .useAs(dafg::Usage::SHADER_RESOURCE)
    .optional()
    .handle();
}

OutputHandle read_reflection_validation(dafg::Registry registry)
{
  return registry.root()
    .readTexture("rtr_validation")
    .atStage(dafg::Stage::PS_OR_CS)
    .useAs(dafg::Usage::SHADER_RESOURCE)
    .optional()
    .handle();
}

// The first traced frame after frames without a trace starts the history anew: the state the ring holds is stale
struct ResetAfterGap
{
  bool saved;
  explicit ResetAfterGap(bool gap) : saved(reset_history) { reset_history = reset_history || gap; }
  ~ResetAfterGap() { reset_history = saved; }
};

struct RenderedFlag
{
  const int &varId;
  bool lastFrame = false;
  explicit RenderedFlag(const int &var_id) : varId(var_id) {}
  ~RenderedFlag() { ShaderGlobal::set_int(varId, 0); }
};

#if DAGOR_DBGLEVEL > 0
Texture *fit_debug_image(DebugImage &dst, const IPoint2 &rect, uint32_t format, const char *name)
{
  const int back = dst.shown ^ 1;
  UniqueTex &tex = dst.tex[back];
  IPoint2 size = rect;
  if (tex)
  {
    TextureInfo di;
    tex->getinfo(di);
    if (di.w < rect.x || di.h < rect.y || (di.cflg & TEXFMT_MASK) != (format & TEXFMT_MASK))
    {
      size = IPoint2(max(size.x, int(di.w)), max(size.y, int(di.h)));
      tex.close();
    }
  }
  if (!tex && size.x > 0 && size.y > 0)
    tex = dag::create_tex(nullptr, size.x, size.y, format, 1, String(0, "%s_%d", name, back));
  dst.rect[back] = rect;
  dst.shown = back;
  return tex.getTex2D();
}

void copy_for_debug(BaseTexture *src, DebugImage &dst, const char *name)
{
  if (!src)
    return;
  TextureInfo si;
  src->getinfo(si);
  if (Texture *tex = fit_debug_image(dst, IPoint2(si.w, si.h), (si.cflg & TEXFMT_MASK) | TEXCF_UPDATE_DESTINATION, name))
    d3d::update_sub_region(src, 0, 0, 0, 0, si.w, si.h, 1, tex, 0, 0, 0, 0);
}
#endif

dafg::NodeHandle make_prepare_node(const PrepareNodeParams &params)
{
  shadow_trace_in_graph = params.shadowTrace;
  reflection_inputs_in_graph.reset();
  resolution_name = params.resolutionName;
  eastl::shared_ptr<const PrepareNodeParams> shared = eastl::make_shared<PrepareNodeParams>(params);
  return dafg::root().registerNode("denoiser_prepare", DAFG_PP_NODE_SRC, [shared](dafg::Registry registry) {
    if (shared->declareInputs)
      shared->declareInputs(registry);
    auto paramsHndl = registry.readBlob<PrepareParams>("denoiser_prepare_params").handle();
    registry.createBlob<OrderingToken>("denoiser_prepared");
    const Config &cfg = resolution_config;
    if (!(cfg.useRayReconstruction && cfg.ptgiUseRayReconstruction))
    {
      registry.create("denoiser_nr")
        .texture({TEXFMT_A2R10G10B10 | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str())})
        .atStage(dafg::Stage::CS)
        .bindToShaderVar("denoiser_nr");
      registry.create("denoiser_view_z")
        .texture({TEXFMT_R32F | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str())})
        .atStage(dafg::Stage::CS)
        .bindToShaderVar("denoiser_view_z");
      if (shared->halfResCloseSet)
      {
        registry.create("denoiser_half_nr")
          .texture({TEXFMT_A2R10G10B10 | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str(), 0.5f)})
          .atStage(dafg::Stage::CS)
          .bindToShaderVar("denoiser_half_nr");
        registry.create("denoiser_half_view_z")
          .texture({TEXFMT_R32F | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str(), 0.5f)})
          .atStage(dafg::Stage::CS)
          .bindToShaderVar("denoiser_half_view_z");
      }
      if (shared->checkerboardSet || shared->smartDepthSet)
      {
        registry.create("denoiser_half_nr_rtr")
          .texture({TEXFMT_A2R10G10B10 | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str(), 0.5f)})
          .atStage(dafg::Stage::CS)
          .bindToShaderVar("denoiser_half_nr_rtr");
        registry.create("denoiser_half_view_z_rtr")
          .texture({TEXFMT_R32F | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str(), 0.5f)})
          .atStage(dafg::Stage::CS)
          .bindToShaderVar("denoiser_half_view_z_rtr");
        registry.create("denoiser_half_motion_vectors_rtr")
          .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str(), 0.5f)})
          .atStage(dafg::Stage::CS)
          .bindToShaderVar("denoiser_half_motion_vectors_rtr");
      }
      if (shared->smartDepthSet)
        registry.create("half_selected_depth_rtr")
          .texture({TEXFMT_R32F | TEXCF_UNORDERED, registry.getResolution<2>(resolution_name.c_str(), 0.5f)})
          .atStage(dafg::Stage::CS)
          .bindToShaderVar("half_selected_depth_rtr");
    }
    return [shared, paramsHndl, resolution = registry.getResolution<2>(resolution_name.c_str())]() {
      const PrepareParams &params = paramsHndl.ref();
      const IPoint2 res = resolution.get();
      const bool prepared = params.render && prepare_frame(params, res);
      // the readers of the rtr set select their depth by it, so it follows what the prepare produced in the frame
      ShaderGlobal::set_int(rtr_depth_modeVarId, !prepared ? 0 : shared->smartDepthSet ? 2 : shared->checkerboardSet ? 1 : 0);
      if (!prepared)
        return;
      TIME_D3D_PROFILE(denoiser::prepare);
      ShaderGlobal::set_int(denoiser_prepare_close_depthVarId, shared->halfResCloseSet ? 1 : 0);
      if (shared->smartDepthSet)
        ShaderGlobal::set_sampler(half_selected_depth_rtr_samplerstateVarId, samplerNearestClamp);
      if (shared->setShaderState)
        shared->setShaderState();
      prepareCS->dispatchThreads(res.x, res.y, 1);
    };
  });
}

dafg::NodeHandle make_shadow_node(const ShadowNodeParams &params)
{
  G_ASSERT_RETURN(resolution_config.useRayReconstruction || params.motionVectorsName, {});
  return dafg::root().registerNode("denoise_shadow", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    auto paramsHndl = registry.readBlob<ShadowParams>("denoiser_shadow_params").handle();
    RoTex valueHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, denoisedHistHndl, historyLengthHistHndl;
    RwTex denoisedHndl, historyLengthHndl, tilesHndl, smoothTilesHndl, data1Hndl, data2Hndl, temp1Hndl, temp2Hndl;
    if (resolution_config.useRayReconstruction)
      registry.renameTexture("rtsm_value", "rtsm_shadows_denoised");
    else
    {
      valueHndl = registry.readTexture("rtsm_value").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      viewZHndl = registry.readTexture("denoiser_view_z_shadow").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      normalRoughnessHndl = registry.readTexture("denoiser_nr").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      motionVectorsHndl =
        registry.readTexture(params.motionVectorsName).atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      const auto resolution = registry.getResolution<2>(resolution_name.c_str());
      const auto tilesResolution = registry.getResolution<2>(resolution_name.c_str(), 1.f / 16).roundingUp();
      denoisedHndl = registry.create("rtsm_shadows_denoised")
                       .texture({TEXFMT_R8 | TEXCF_UNORDERED, resolution})
                       .withHistory(dafg::History::ClearZeroOnFirstFrame)
                       .clear(make_clear_value(1.f, 0.f, 0.f, 0.f))
                       .atStage(dafg::Stage::CS)
                       .useAs(dafg::Usage::SHADER_RESOURCE)
                       .handle();
      denoisedHistHndl =
        registry.readTextureHistory("rtsm_shadows_denoised").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      historyLengthHndl = registry.create("rtsm_history_length")
                            .texture({TEXFMT_R32UI | TEXCF_UNORDERED, resolution})
                            .withHistory(dafg::History::ClearZeroOnFirstFrame)
                            // water and too distant texels are never written, don't leave garbage there
                            .clear(ResourceClearValue{})
                            .atStage(dafg::Stage::CS)
                            .useAs(dafg::Usage::SHADER_RESOURCE)
                            .handle();
      historyLengthHistHndl =
        registry.readTextureHistory("rtsm_history_length").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      tilesHndl = registry.create("rtsm_tiles")
                    .texture({TEXFMT_A8R8G8B8 | TEXCF_UNORDERED, tilesResolution})
                    .atStage(dafg::Stage::CS)
                    .useAs(dafg::Usage::SHADER_RESOURCE)
                    .handle();
      smoothTilesHndl = registry.create("rtsm_smooth_tiles")
                          .texture({TEXFMT_R8G8 | TEXCF_UNORDERED, tilesResolution})
                          .atStage(dafg::Stage::CS)
                          .useAs(dafg::Usage::SHADER_RESOURCE)
                          .handle();
      data1Hndl = registry.create("rtsm_data1")
                    .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                    .atStage(dafg::Stage::CS)
                    .useAs(dafg::Usage::SHADER_RESOURCE)
                    .handle();
      data2Hndl = registry.create("rtsm_data2")
                    .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                    .atStage(dafg::Stage::CS)
                    .useAs(dafg::Usage::SHADER_RESOURCE)
                    .handle();
      temp1Hndl = registry.create("rtsm_temp1")
                    .texture({TEXFMT_R8 | TEXCF_UNORDERED, resolution})
                    .atStage(dafg::Stage::CS)
                    .useAs(dafg::Usage::SHADER_RESOURCE)
                    .handle();
      temp2Hndl = registry.create("rtsm_temp2")
                    .texture({TEXFMT_R8 | TEXCF_UNORDERED, resolution})
                    .atStage(dafg::Stage::CS)
                    .useAs(dafg::Usage::SHADER_RESOURCE)
                    .handle();
    }
    auto resources = eastl::make_tuple(valueHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, denoisedHndl, denoisedHistHndl,
      historyLengthHndl, historyLengthHistHndl, tilesHndl, smoothTilesHndl, data1Hndl, data2Hndl, temp1Hndl, temp2Hndl);
    return [paramsHndl, resources = eastl::make_unique<decltype(resources)>(resources),
             rendered = eastl::make_unique<RenderedFlag>(rtsm_renderedVarId)]() {
      const auto &[valueHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, denoisedHndl, denoisedHistHndl, historyLengthHndl,
        historyLengthHistHndl, tilesHndl, smoothTilesHndl, data1Hndl, data2Hndl, temp1Hndl, temp2Hndl] = *resources;
      const ShadowParams &params = paramsHndl.ref();
      ShaderGlobal::set_int(rtsm_renderedVarId, params.render ? 1 : 0);
      if (!params.render)
      {
        rendered->lastFrame = false;
        return;
      }
      ResetAfterGap reset(!rendered->lastFrame);
      rendered->lastFrame = true;
      ShadowTextures tex;
      tex.value = ptr(valueHndl);
      tex.viewZ = ptr(viewZHndl);
      tex.normalRoughness = ptr(normalRoughnessHndl);
      tex.motionVectors = ptr(motionVectorsHndl);
      tex.denoised = {ptr(denoisedHndl), ptr(denoisedHistHndl)};
      tex.historyLength = {ptr(historyLengthHndl), ptr(historyLengthHistHndl)};
      tex.tiles = ptr(tilesHndl);
      tex.smoothTiles = ptr(smoothTilesHndl);
      tex.data1 = ptr(data1Hndl);
      tex.data2 = ptr(data2Hndl);
      tex.temp1 = ptr(temp1Hndl);
      tex.temp2 = ptr(temp2Hndl);
      denoise_shadow(params, tex);
    };
  });
}

dafg::NodeHandle make_ao_node(const AONodeParams &params)
{
  G_ASSERT_RETURN(resolution_config.useRayReconstruction ||
                    (resolution_config.rtaoHalfRes ? params.halfMotionVectorsName : params.motionVectorsName),
    {});
  return dafg::root().registerNode("denoise_ao", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    auto paramsHndl = registry.readBlob<AOParams>("denoiser_ao_params").handle();
    const Config &cfg = resolution_config;
    const bool halfRes = cfg.rtaoHalfRes;
    ShaderGlobal::set_int(rtao_bindless_sampler_slotVarId, bindless_sampler(halfRes));
    RoTex unfilteredHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, prevViewZHistHndl, prevNormalRoughnessHistHndl,
      prevInternalDataHistHndl, diffHistoryHistHndl, diffFastHistoryHistHndl;
    RwTex denoisedHndl, prevViewZHndl, prevNormalRoughnessHndl, prevInternalDataHndl, diffHistoryHndl, diffFastHistoryHndl, tilesHndl,
      diffTemp2Hndl, diffFastHistoryTempHndl, data1Hndl;
    if (cfg.useRayReconstruction)
      registry.renameTexture("rtao_tex_unfiltered", "denoised_ao");
    else
    {
      unfilteredHndl =
        registry.readTexture("rtao_tex_unfiltered").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      const Inputs inputs = halfRes ? Inputs::Half : Inputs::Full;
      viewZHndl = registry.readTexture(view_z_name(inputs)).atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      normalRoughnessHndl =
        registry.readTexture(normal_roughness_name(inputs)).atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      motionVectorsHndl = registry.readTexture(halfRes ? params.halfMotionVectorsName : params.motionVectorsName)
                            .atStage(dafg::Stage::CS)
                            .useAs(dafg::Usage::SHADER_RESOURCE)
                            .handle();
      const auto resolution = registry.getResolution<2>(resolution_name.c_str(), halfRes ? 0.5f : 1.f);
      const auto tilesResolution = registry.getResolution<2>(resolution_name.c_str(), (halfRes ? 0.5f : 1.f) / 16).roundingUp();
      denoisedHndl = registry.create("denoised_ao")
                       .texture({TEXFMT_L16 | TEXCF_UNORDERED, resolution})
                       .clear(make_clear_value(1.f, 0.f, 0.f, 0.f))
                       .atStage(dafg::Stage::CS)
                       .useAs(dafg::Usage::SHADER_RESOURCE)
                       .handle();
      prevViewZHndl = registry.create("rtao_prev_view_z")
                        .texture({TEXFMT_R32F | TEXCF_UNORDERED, resolution})
                        .withHistory(dafg::History::ClearZeroOnFirstFrame)
                        .clear(ResourceClearValue{})
                        .atStage(dafg::Stage::CS)
                        .useAs(dafg::Usage::SHADER_RESOURCE)
                        .handle();
      prevViewZHistHndl =
        registry.readTextureHistory("rtao_prev_view_z").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      prevNormalRoughnessHndl = registry.create("rtao_prev_normal_roughness")
                                  .texture({TEXFMT_A2R10G10B10 | TEXCF_UNORDERED, resolution})
                                  .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                  .clear(ResourceClearValue{})
                                  .atStage(dafg::Stage::CS)
                                  .useAs(dafg::Usage::SHADER_RESOURCE)
                                  .handle();
      prevNormalRoughnessHistHndl = registry.readTextureHistory("rtao_prev_normal_roughness")
                                      .atStage(dafg::Stage::CS)
                                      .useAs(dafg::Usage::SHADER_RESOURCE)
                                      .handle();
      prevInternalDataHndl = registry.create("rtao_prev_internal_data")
                               .texture({TEXFMT_R16UI | TEXCF_UNORDERED, resolution})
                               .withHistory(dafg::History::ClearZeroOnFirstFrame)
                               .clear(ResourceClearValue{})
                               .atStage(dafg::Stage::CS)
                               .useAs(dafg::Usage::SHADER_RESOURCE)
                               .handle();
      prevInternalDataHistHndl =
        registry.readTextureHistory("rtao_prev_internal_data").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      diffHistoryHndl = registry.create("rtao_diff_history")
                          .texture({TEXFMT_L16 | TEXCF_UNORDERED, resolution})
                          .withHistory(dafg::History::ClearZeroOnFirstFrame)
                          .clear(ResourceClearValue{})
                          .atStage(dafg::Stage::CS)
                          .useAs(dafg::Usage::SHADER_RESOURCE)
                          .handle();
      diffHistoryHistHndl =
        registry.readTextureHistory("rtao_diff_history").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      diffFastHistoryHndl = registry.create("rtao_diff_fast_history")
                              .texture({TEXFMT_L16 | TEXCF_UNORDERED, resolution})
                              .withHistory(dafg::History::ClearZeroOnFirstFrame)
                              .clear(ResourceClearValue{})
                              .atStage(dafg::Stage::CS)
                              .useAs(dafg::Usage::SHADER_RESOURCE)
                              .handle();
      diffFastHistoryHistHndl =
        registry.readTextureHistory("rtao_diff_fast_history").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      tilesHndl = registry.create("rtao_tiles")
                    .texture({TEXFMT_R8 | TEXCF_UNORDERED, tilesResolution})
                    .atStage(dafg::Stage::CS)
                    .useAs(dafg::Usage::SHADER_RESOURCE)
                    .handle();
      diffTemp2Hndl = registry.create("rtao_diff_temp2")
                        .texture({TEXFMT_L16 | TEXCF_UNORDERED, resolution})
                        .atStage(dafg::Stage::CS)
                        .useAs(dafg::Usage::SHADER_RESOURCE)
                        .handle();
      diffFastHistoryTempHndl = registry.create("rtao_diff_fast_history_temp")
                                  .texture({TEXFMT_L16 | TEXCF_UNORDERED, resolution})
                                  .atStage(dafg::Stage::CS)
                                  .useAs(dafg::Usage::SHADER_RESOURCE)
                                  .handle();
      data1Hndl = registry.create("rtao_data1")
                    .texture({TEXFMT_R8 | TEXCF_UNORDERED, resolution})
                    .atStage(dafg::Stage::CS)
                    .useAs(dafg::Usage::SHADER_RESOURCE)
                    .handle();
    }
    auto resources = eastl::make_tuple(unfilteredHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, denoisedHndl, prevViewZHndl,
      prevViewZHistHndl, prevNormalRoughnessHndl, prevNormalRoughnessHistHndl, prevInternalDataHndl, prevInternalDataHistHndl,
      diffHistoryHndl, diffHistoryHistHndl, diffFastHistoryHndl, diffFastHistoryHistHndl, tilesHndl, diffTemp2Hndl,
      diffFastHistoryTempHndl, data1Hndl);
    return [paramsHndl, resources = eastl::make_unique<decltype(resources)>(resources),
             rendered = eastl::make_unique<RenderedFlag>(rtao_renderedVarId)]() {
      const auto &[unfilteredHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, denoisedHndl, prevViewZHndl, prevViewZHistHndl,
        prevNormalRoughnessHndl, prevNormalRoughnessHistHndl, prevInternalDataHndl, prevInternalDataHistHndl, diffHistoryHndl,
        diffHistoryHistHndl, diffFastHistoryHndl, diffFastHistoryHistHndl, tilesHndl, diffTemp2Hndl, diffFastHistoryTempHndl,
        data1Hndl] = *resources;
      const AOParams &params = paramsHndl.ref();
      ShaderGlobal::set_int(rtao_renderedVarId, params.render ? 1 : 0);
      if (!params.render || resolution_config.useRayReconstruction)
      {
        rendered->lastFrame = false;
        return;
      }
      ResetAfterGap reset(!rendered->lastFrame);
      rendered->lastFrame = true;
      AOTextures tex;
      tex.unfiltered = ptr(unfilteredHndl);
      tex.viewZ = ptr(viewZHndl);
      tex.normalRoughness = ptr(normalRoughnessHndl);
      tex.motionVectors = ptr(motionVectorsHndl);
      tex.denoised = ptr(denoisedHndl);
      tex.prevViewZ = {ptr(prevViewZHndl), ptr(prevViewZHistHndl)};
      tex.prevNormalRoughness = {ptr(prevNormalRoughnessHndl), ptr(prevNormalRoughnessHistHndl)};
      tex.prevInternalData = {ptr(prevInternalDataHndl), ptr(prevInternalDataHistHndl)};
      tex.diffHistory = {ptr(diffHistoryHndl), ptr(diffHistoryHistHndl)};
      tex.diffFastHistory = {ptr(diffFastHistoryHndl), ptr(diffFastHistoryHistHndl)};
      tex.tiles = ptr(tilesHndl);
      tex.diffTemp2 = ptr(diffTemp2Hndl);
      tex.diffFastHistoryTemp = ptr(diffFastHistoryTempHndl);
      tex.data1 = ptr(data1Hndl);
      denoise_ao(params, tex);
    };
  });
}

dafg::NodeHandle make_gi_node(const GINodeParams &params)
{
  G_ASSERT_RETURN(resolution_config.ptgiUseRayReconstruction ||
                    (resolution_config.ptgiHalfRes ? params.halfMotionVectorsName : params.motionVectorsName),
    {});
  return dafg::root().registerNode("denoise_gi", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    auto paramsHndl = registry.readBlob<GIParams>("denoiser_gi_params").handle();
    const Config &cfg = resolution_config;
    const bool halfRes = cfg.ptgiHalfRes;
    ShaderGlobal::set_int(ptgi_bindless_sampler_slotVarId, bindless_sampler(!cfg.ptgiUseRayReconstruction));
    RoTex unfilteredHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, prevViewZHistHndl, prevNormalRoughnessHistHndl,
      prevInternalDataHistHndl, diffHistoryHistHndl, diffFastHistoryHistHndl, diffHistoryStabilizedHistHndl, behindViewZHistHndl,
      behindInternalDataHistHndl, behindDiffHistoryHistHndl, behindDiffFastHistoryHistHndl;
    RwTex denoisedHndl, validationHndl, prevViewZHndl, prevNormalRoughnessHndl, prevInternalDataHndl, diffHistoryHndl,
      diffFastHistoryHndl, diffHistoryStabilizedHndl, behindViewZHndl, behindInternalDataHndl, behindDiffHistoryHndl,
      behindDiffFastHistoryHndl, data1Hndl, data2Hndl, diffTemp2Hndl, diffFastHistoryTempHndl, tilesHndl;
    if (cfg.ptgiUseRayReconstruction)
      registry.renameTexture("ptgi_tex_unfiltered", "denoised_gi");
    else
    {
      ShaderGlobal::set_int(ptgi_depth_bindless_sampler_slotVarId, bindless_sampler(true));
      unfilteredHndl =
        registry.readTexture("ptgi_tex_unfiltered").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      const Inputs inputs = halfRes ? Inputs::Half : Inputs::Full;
      viewZHndl = registry.readTexture(view_z_name(inputs)).atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      normalRoughnessHndl =
        registry.readTexture(normal_roughness_name(inputs)).atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      motionVectorsHndl = registry.readTexture(halfRes ? params.halfMotionVectorsName : params.motionVectorsName)
                            .atStage(dafg::Stage::CS)
                            .useAs(dafg::Usage::SHADER_RESOURCE)
                            .handle();
      const auto resolution = registry.getResolution<2>(resolution_name.c_str(), halfRes ? 0.5f : 1.f);
      const auto tilesResolution = registry.getResolution<2>(resolution_name.c_str(), (halfRes ? 0.5f : 1.f) / 16).roundingUp();
      denoisedHndl = registry.create("denoised_gi")
                       .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, resolution})
                       .clear(make_clear_value(0.f, 0.f, 0.f, 0.f))
                       .atStage(dafg::Stage::CS)
                       .useAs(dafg::Usage::SHADER_RESOURCE)
                       .handle();
      prevViewZHndl = registry.create("ptgi_prev_view_z")
                        .texture({TEXFMT_R32F | TEXCF_UNORDERED, resolution})
                        .withHistory(dafg::History::ClearZeroOnFirstFrame)
                        .clear(ResourceClearValue{})
                        .atStage(dafg::Stage::CS)
                        .useAs(dafg::Usage::SHADER_RESOURCE)
                        .handle();
      prevViewZHistHndl =
        registry.readTextureHistory("ptgi_prev_view_z").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      prevNormalRoughnessHndl = registry.create("ptgi_prev_normal_roughness")
                                  .texture({TEXFMT_A2R10G10B10 | TEXCF_UNORDERED, resolution})
                                  .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                  .clear(ResourceClearValue{})
                                  .atStage(dafg::Stage::CS)
                                  .useAs(dafg::Usage::SHADER_RESOURCE)
                                  .handle();
      prevNormalRoughnessHistHndl = registry.readTextureHistory("ptgi_prev_normal_roughness")
                                      .atStage(dafg::Stage::CS)
                                      .useAs(dafg::Usage::SHADER_RESOURCE)
                                      .handle();
      prevInternalDataHndl = registry.create("ptgi_prev_internal_data")
                               .texture({TEXFMT_R16UI | TEXCF_UNORDERED, resolution})
                               .withHistory(dafg::History::ClearZeroOnFirstFrame)
                               .clear(ResourceClearValue{})
                               .atStage(dafg::Stage::CS)
                               .useAs(dafg::Usage::SHADER_RESOURCE)
                               .handle();
      prevInternalDataHistHndl =
        registry.readTextureHistory("ptgi_prev_internal_data").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      diffHistoryHndl = registry.create("ptgi_diff_history")
                          .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, resolution})
                          .withHistory(dafg::History::ClearZeroOnFirstFrame)
                          .clear(ResourceClearValue{})
                          .atStage(dafg::Stage::CS)
                          .useAs(dafg::Usage::SHADER_RESOURCE)
                          .handle();
      diffHistoryHistHndl =
        registry.readTextureHistory("ptgi_diff_history").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      diffFastHistoryHndl = registry.create("ptgi_diff_fast_history")
                              .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                              .withHistory(dafg::History::ClearZeroOnFirstFrame)
                              .clear(ResourceClearValue{})
                              .atStage(dafg::Stage::CS)
                              .useAs(dafg::Usage::SHADER_RESOURCE)
                              .handle();
      diffFastHistoryHistHndl =
        registry.readTextureHistory("ptgi_diff_fast_history").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      diffHistoryStabilizedHndl = registry.create("ptgi_diff_history_stabilized")
                                    .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                                    .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                    .clear(ResourceClearValue{})
                                    .atStage(dafg::Stage::CS)
                                    .useAs(dafg::Usage::SHADER_RESOURCE)
                                    .handle();
      diffHistoryStabilizedHistHndl = registry.readTextureHistory("ptgi_diff_history_stabilized")
                                        .atStage(dafg::Stage::CS)
                                        .useAs(dafg::Usage::SHADER_RESOURCE)
                                        .handle();
      if (params.secondChanceHistory)
      {
        behindViewZHndl = registry.create("ptgi_behind_view_z")
                            .texture({TEXFMT_R32F | TEXCF_UNORDERED, resolution})
                            .withHistory(dafg::History::ClearZeroOnFirstFrame)
                            .atStage(dafg::Stage::CS)
                            .useAs(dafg::Usage::SHADER_RESOURCE)
                            .handle();
        behindViewZHistHndl =
          registry.readTextureHistory("ptgi_behind_view_z").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        behindInternalDataHndl = registry.create("ptgi_behind_internal_data")
                                   .texture({TEXFMT_R16UI | TEXCF_UNORDERED, resolution})
                                   .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                   .atStage(dafg::Stage::CS)
                                   .useAs(dafg::Usage::SHADER_RESOURCE)
                                   .handle();
        behindInternalDataHistHndl = registry.readTextureHistory("ptgi_behind_internal_data")
                                       .atStage(dafg::Stage::CS)
                                       .useAs(dafg::Usage::SHADER_RESOURCE)
                                       .handle();
        behindDiffHistoryHndl = registry.create("ptgi_behind_diff_history")
                                  .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, resolution})
                                  .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                  .atStage(dafg::Stage::CS)
                                  .useAs(dafg::Usage::SHADER_RESOURCE)
                                  .handle();
        behindDiffHistoryHistHndl = registry.readTextureHistory("ptgi_behind_diff_history")
                                      .atStage(dafg::Stage::CS)
                                      .useAs(dafg::Usage::SHADER_RESOURCE)
                                      .handle();
        behindDiffFastHistoryHndl = registry.create("ptgi_behind_diff_fast_history")
                                      .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                                      .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                      .atStage(dafg::Stage::CS)
                                      .useAs(dafg::Usage::SHADER_RESOURCE)
                                      .handle();
        behindDiffFastHistoryHistHndl = registry.readTextureHistory("ptgi_behind_diff_fast_history")
                                          .atStage(dafg::Stage::CS)
                                          .useAs(dafg::Usage::SHADER_RESOURCE)
                                          .handle();
      }
      data1Hndl = registry.create("ptgi_data1")
                    .texture({TEXFMT_R8 | TEXCF_UNORDERED, resolution})
                    .atStage(dafg::Stage::CS)
                    .useAs(dafg::Usage::SHADER_RESOURCE)
                    .handle();
      data2Hndl = registry.create("ptgi_data2")
                    .texture({TEXFMT_R8UI | TEXCF_UNORDERED, resolution})
                    .atStage(dafg::Stage::CS)
                    .useAs(dafg::Usage::SHADER_RESOURCE)
                    .handle();
      diffTemp2Hndl = registry.create("ptgi_diff_temp2")
                        .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, resolution})
                        .atStage(dafg::Stage::CS)
                        .useAs(dafg::Usage::SHADER_RESOURCE)
                        .handle();
      diffFastHistoryTempHndl = registry.create("ptgi_diff_fast_history_temp")
                                  .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                                  .atStage(dafg::Stage::CS)
                                  .useAs(dafg::Usage::SHADER_RESOURCE)
                                  .handle();
      tilesHndl = registry.create("ptgi_tiles")
                    .texture({TEXFMT_R8 | TEXCF_UNORDERED, tilesResolution})
                    .atStage(dafg::Stage::CS)
                    .useAs(dafg::Usage::SHADER_RESOURCE)
                    .handle();
      if (params.validation)
        validationHndl = registry.create("gi_validation")
                           .texture({TEXFMT_A8R8G8B8 | TEXCF_UNORDERED, resolution})
                           .atStage(dafg::Stage::CS)
                           .useAs(dafg::Usage::SHADER_RESOURCE)
                           .handle();
    }
    auto resources = eastl::make_tuple(unfilteredHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, denoisedHndl, validationHndl,
      prevViewZHndl, prevViewZHistHndl, prevNormalRoughnessHndl, prevNormalRoughnessHistHndl, prevInternalDataHndl,
      prevInternalDataHistHndl, diffHistoryHndl, diffHistoryHistHndl, diffFastHistoryHndl, diffFastHistoryHistHndl,
      diffHistoryStabilizedHndl, diffHistoryStabilizedHistHndl, behindViewZHndl, behindViewZHistHndl, behindInternalDataHndl,
      behindInternalDataHistHndl, behindDiffHistoryHndl, behindDiffHistoryHistHndl, behindDiffFastHistoryHndl,
      behindDiffFastHistoryHistHndl, data1Hndl, data2Hndl, diffTemp2Hndl, diffFastHistoryTempHndl, tilesHndl);
    return [paramsHndl, resources = eastl::make_unique<decltype(resources)>(resources),
             rendered = eastl::make_unique<RenderedFlag>(ptgi_renderedVarId)]() {
      const auto &[unfilteredHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, denoisedHndl, validationHndl, prevViewZHndl,
        prevViewZHistHndl, prevNormalRoughnessHndl, prevNormalRoughnessHistHndl, prevInternalDataHndl, prevInternalDataHistHndl,
        diffHistoryHndl, diffHistoryHistHndl, diffFastHistoryHndl, diffFastHistoryHistHndl, diffHistoryStabilizedHndl,
        diffHistoryStabilizedHistHndl, behindViewZHndl, behindViewZHistHndl, behindInternalDataHndl, behindInternalDataHistHndl,
        behindDiffHistoryHndl, behindDiffHistoryHistHndl, behindDiffFastHistoryHndl, behindDiffFastHistoryHistHndl, data1Hndl,
        data2Hndl, diffTemp2Hndl, diffFastHistoryTempHndl, tilesHndl] = *resources;
      const GIParams &params = paramsHndl.ref();
      ShaderGlobal::set_int(ptgi_renderedVarId, params.render ? 1 : 0);
      if (!params.render || resolution_config.ptgiUseRayReconstruction)
      {
        rendered->lastFrame = false;
        return;
      }
      ResetAfterGap reset(!rendered->lastFrame);
      rendered->lastFrame = true;
      GITextures tex;
      tex.unfiltered = ptr(unfilteredHndl);
      tex.viewZ = ptr(viewZHndl);
      tex.normalRoughness = ptr(normalRoughnessHndl);
      tex.motionVectors = ptr(motionVectorsHndl);
      tex.denoised = ptr(denoisedHndl);
      tex.validation = ptr(validationHndl);
      tex.prevViewZ = {ptr(prevViewZHndl), ptr(prevViewZHistHndl)};
      tex.prevNormalRoughness = {ptr(prevNormalRoughnessHndl), ptr(prevNormalRoughnessHistHndl)};
      tex.prevInternalData = {ptr(prevInternalDataHndl), ptr(prevInternalDataHistHndl)};
      tex.diffHistory = {ptr(diffHistoryHndl), ptr(diffHistoryHistHndl)};
      tex.diffFastHistory = {ptr(diffFastHistoryHndl), ptr(diffFastHistoryHistHndl)};
      tex.diffHistoryStabilized = {ptr(diffHistoryStabilizedHndl), ptr(diffHistoryStabilizedHistHndl)};
      tex.behindViewZ = {ptr(behindViewZHndl), ptr(behindViewZHistHndl)};
      tex.behindInternalData = {ptr(behindInternalDataHndl), ptr(behindInternalDataHistHndl)};
      tex.behindDiffHistory = {ptr(behindDiffHistoryHndl), ptr(behindDiffHistoryHistHndl)};
      tex.behindDiffFastHistory = {ptr(behindDiffFastHistoryHndl), ptr(behindDiffFastHistoryHistHndl)};
      tex.data1 = ptr(data1Hndl);
      tex.data2 = ptr(data2Hndl);
      tex.diffTemp2 = ptr(diffTemp2Hndl);
      tex.diffFastHistoryTemp = ptr(diffFastHistoryTempHndl);
      tex.tiles = ptr(tilesHndl);
      denoise_gi(params, tex);
    };
  });
}

dafg::NodeHandle make_reflection_node(const ReflectionNodeParams &params)
{
  const Inputs inputs = reflection_inputs(params.checkerboardDepth, params.useSmartDepth);
  G_ASSERT_RETURN(resolution_config.useRayReconstruction || params.method == ReflectionMethod::Blur ||
                    (inputs == Inputs::Full ? params.motionVectorsName != nullptr
                                            : inputs == Inputs::HalfRtr || params.halfMotionVectorsName != nullptr),
    {});
  if (!resolution_config.useRayReconstruction)
    reflection_inputs_in_graph = inputs;
  return dafg::root().registerNode("denoise_reflection", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    auto paramsHndl = registry.readBlob<ReflectionParams>("denoiser_reflection_params").handle();
    const Config &cfg = resolution_config;
    const bool halfRes = cfg.rtrHalfRes;
    const bool linear = halfRes || params.method == ReflectionMethod::Blur; // the reader of a Blur output samples its mips
    ShaderGlobal::set_int(rtr_bindless_sampler_slotVarId, bindless_sampler(linear));
    RoTex unfilteredHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, reblurPrevViewZHistHndl,
      reblurPrevNormalRoughnessHistHndl, reblurPrevInternalDataHistHndl, reblurHistoryHistHndl, reblurFastHistoryHistHndl,
      reblurHistoryStabilizedHistHndl, reblurHitDistForTrackingHistHndl, relaxIllumHistHndl, relaxIllumResponsiveHistHndl,
      relaxReflectionHitTHistHndl, relaxHistoryLengthHistHndl, relaxNormalRoughnessHistHndl, relaxMaterialIdHistHndl,
      relaxViewZHistHndl;
    RwTex denoisedHndl, validationHndl, reblurPrevViewZHndl, reblurPrevNormalRoughnessHndl, reblurPrevInternalDataHndl,
      reblurHistoryHndl, reblurFastHistoryHndl, reblurHistoryStabilizedHndl, reblurHitDistForTrackingHndl, reblurData1Hndl,
      reblurData2Hndl, reblurHitDistForTrackingTempHndl, reblurTmp2Hndl, reblurFastHistoryTempHndl, reblurTilesHndl, relaxIllumHndl,
      relaxIllumResponsiveHndl, relaxReflectionHitTHndl, relaxHistoryLengthHndl, relaxNormalRoughnessHndl, relaxMaterialIdHndl,
      relaxViewZHndl, relaxTilesHndl, relaxHistoryLengthTempHndl, relaxReprojectionConfidenceHndl, relaxIllumPingHndl,
      relaxIllumPongHndl, blurTempHndl;
    if (cfg.useRayReconstruction)
      registry.renameTexture("rtr_tex_unfiltered", "rtr_tex");
    else
    {
      ShaderGlobal::set_int(rtr_normal_roughness_bindless_sampler_slotVarId, bindless_sampler(linear));
      unfilteredHndl =
        registry.readTexture("rtr_tex_unfiltered").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      const Inputs inputs = reflection_inputs(params.checkerboardDepth, params.useSmartDepth);
      viewZHndl =
        registry.readTexture(reflection_view_z_name(inputs)).atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      normalRoughnessHndl =
        registry.readTexture(normal_roughness_name(inputs)).atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      if (params.method != ReflectionMethod::Blur)
        motionVectorsHndl = registry
                              .readTexture(inputs == Inputs::HalfRtr ? "denoiser_half_motion_vectors_rtr"
                                           : halfRes                 ? params.halfMotionVectorsName
                                                                     : params.motionVectorsName)
                              .atStage(dafg::Stage::CS)
                              .useAs(dafg::Usage::SHADER_RESOURCE)
                              .handle();
      denoisedHndl = registry.modifyTexture("rtr_tex").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      const auto resolution = registry.getResolution<2>(resolution_name.c_str(), halfRes ? 0.5f : 1.f);
      const auto tilesResolution = registry.getResolution<2>(resolution_name.c_str(), (halfRes ? 0.5f : 1.f) / 16).roundingUp();
      if (params.method == ReflectionMethod::Reblur)
      {
        reblurPrevViewZHndl = registry.create("rtr_reblur_prev_view_z")
                                .texture({TEXFMT_R32F | TEXCF_UNORDERED, resolution})
                                .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                .clear(ResourceClearValue{})
                                .atStage(dafg::Stage::CS)
                                .useAs(dafg::Usage::SHADER_RESOURCE)
                                .handle();
        reblurPrevViewZHistHndl =
          registry.readTextureHistory("rtr_reblur_prev_view_z").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        reblurPrevNormalRoughnessHndl = registry.create("rtr_reblur_prev_normal_roughness")
                                          .texture({TEXFMT_A2R10G10B10 | TEXCF_UNORDERED, resolution})
                                          .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                          .clear(ResourceClearValue{})
                                          .atStage(dafg::Stage::CS)
                                          .useAs(dafg::Usage::SHADER_RESOURCE)
                                          .handle();
        reblurPrevNormalRoughnessHistHndl = registry.readTextureHistory("rtr_reblur_prev_normal_roughness")
                                              .atStage(dafg::Stage::CS)
                                              .useAs(dafg::Usage::SHADER_RESOURCE)
                                              .handle();
        reblurPrevInternalDataHndl = registry.create("rtr_reblur_prev_internal_data")
                                       .texture({TEXFMT_R16UI | TEXCF_UNORDERED, resolution})
                                       .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                       .clear(ResourceClearValue{})
                                       .atStage(dafg::Stage::CS)
                                       .useAs(dafg::Usage::SHADER_RESOURCE)
                                       .handle();
        reblurPrevInternalDataHistHndl = registry.readTextureHistory("rtr_reblur_prev_internal_data")
                                           .atStage(dafg::Stage::CS)
                                           .useAs(dafg::Usage::SHADER_RESOURCE)
                                           .handle();
        reblurHistoryHndl = registry.create("rtr_reblur_history")
                              .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, resolution})
                              .withHistory(dafg::History::ClearZeroOnFirstFrame)
                              .clear(ResourceClearValue{})
                              .atStage(dafg::Stage::CS)
                              .useAs(dafg::Usage::SHADER_RESOURCE)
                              .handle();
        reblurHistoryHistHndl =
          registry.readTextureHistory("rtr_reblur_history").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        reblurFastHistoryHndl = registry.create("rtr_reblur_fast_history")
                                  .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                                  .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                  .clear(ResourceClearValue{})
                                  .atStage(dafg::Stage::CS)
                                  .useAs(dafg::Usage::SHADER_RESOURCE)
                                  .handle();
        reblurFastHistoryHistHndl =
          registry.readTextureHistory("rtr_reblur_fast_history").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        reblurHistoryStabilizedHndl = registry.create("rtr_reblur_history_stabilized")
                                        .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                                        .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                        .clear(ResourceClearValue{})
                                        .atStage(dafg::Stage::CS)
                                        .useAs(dafg::Usage::SHADER_RESOURCE)
                                        .handle();
        reblurHistoryStabilizedHistHndl = registry.readTextureHistory("rtr_reblur_history_stabilized")
                                            .atStage(dafg::Stage::CS)
                                            .useAs(dafg::Usage::SHADER_RESOURCE)
                                            .handle();
        reblurHitDistForTrackingHndl = registry.create("rtr_reblur_hit_dist_for_tracking")
                                         .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                                         .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                         .clear(ResourceClearValue{})
                                         .atStage(dafg::Stage::CS)
                                         .useAs(dafg::Usage::SHADER_RESOURCE)
                                         .handle();
        reblurHitDistForTrackingHistHndl = registry.readTextureHistory("rtr_reblur_hit_dist_for_tracking")
                                             .atStage(dafg::Stage::CS)
                                             .useAs(dafg::Usage::SHADER_RESOURCE)
                                             .handle();
        reblurData1Hndl = registry.create("rtr_reblur_data1")
                            .texture({TEXFMT_R8 | TEXCF_UNORDERED, resolution})
                            .atStage(dafg::Stage::CS)
                            .useAs(dafg::Usage::SHADER_RESOURCE)
                            .handle();
        reblurData2Hndl = registry.create("rtr_reblur_data2")
                            .texture({TEXFMT_R32UI | TEXCF_UNORDERED, resolution})
                            .atStage(dafg::Stage::CS)
                            .useAs(dafg::Usage::SHADER_RESOURCE)
                            .handle();
        reblurHitDistForTrackingTempHndl = registry.create("rtr_reblur_hit_dist_for_tracking_temp")
                                             .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                                             .atStage(dafg::Stage::CS)
                                             .useAs(dafg::Usage::SHADER_RESOURCE)
                                             .handle();
        reblurTmp2Hndl = registry.create("rtr_reblur_tmp2")
                           .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, resolution})
                           .atStage(dafg::Stage::CS)
                           .useAs(dafg::Usage::SHADER_RESOURCE)
                           .handle();
        reblurFastHistoryTempHndl = registry.create("rtr_reblur_fast_history_temp")
                                      .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                                      .atStage(dafg::Stage::CS)
                                      .useAs(dafg::Usage::SHADER_RESOURCE)
                                      .handle();
        reblurTilesHndl = registry.create("rtr_reblur_tiles")
                            .texture({TEXFMT_R8 | TEXCF_UNORDERED, tilesResolution})
                            .atStage(dafg::Stage::CS)
                            .useAs(dafg::Usage::SHADER_RESOURCE)
                            .handle();
      }
      else if (params.method == ReflectionMethod::Relax)
      {
        relaxIllumHndl = registry.create("rtr_relax_illum")
                           .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, resolution})
                           .withHistory(dafg::History::ClearZeroOnFirstFrame)
                           .clear(ResourceClearValue{})
                           .atStage(dafg::Stage::CS)
                           .useAs(dafg::Usage::SHADER_RESOURCE)
                           .handle();
        relaxIllumHistHndl =
          registry.readTextureHistory("rtr_relax_illum").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        relaxIllumResponsiveHndl = registry.create("rtr_relax_illum_responsive")
                                     .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, resolution})
                                     .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                     .clear(ResourceClearValue{})
                                     .atStage(dafg::Stage::CS)
                                     .useAs(dafg::Usage::SHADER_RESOURCE)
                                     .handle();
        relaxIllumResponsiveHistHndl = registry.readTextureHistory("rtr_relax_illum_responsive")
                                         .atStage(dafg::Stage::CS)
                                         .useAs(dafg::Usage::SHADER_RESOURCE)
                                         .handle();
        relaxReflectionHitTHndl = registry.create("rtr_relax_reflection_hit_t")
                                    .texture({TEXFMT_R16F | TEXCF_UNORDERED, resolution})
                                    .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                    .clear(ResourceClearValue{})
                                    .atStage(dafg::Stage::CS)
                                    .useAs(dafg::Usage::SHADER_RESOURCE)
                                    .handle();
        relaxReflectionHitTHistHndl = registry.readTextureHistory("rtr_relax_reflection_hit_t")
                                        .atStage(dafg::Stage::CS)
                                        .useAs(dafg::Usage::SHADER_RESOURCE)
                                        .handle();
        relaxHistoryLengthHndl = registry.create("rtr_relax_history_length")
                                   .texture({TEXFMT_R8 | TEXCF_UNORDERED, resolution})
                                   .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                   .clear(ResourceClearValue{})
                                   .atStage(dafg::Stage::CS)
                                   .useAs(dafg::Usage::SHADER_RESOURCE)
                                   .handle();
        relaxHistoryLengthHistHndl = registry.readTextureHistory("rtr_relax_history_length")
                                       .atStage(dafg::Stage::CS)
                                       .useAs(dafg::Usage::SHADER_RESOURCE)
                                       .handle();
        relaxNormalRoughnessHndl = registry.create("rtr_relax_normal_roughness")
                                     .texture({TEXFMT_A8R8G8B8 | TEXCF_UNORDERED, resolution})
                                     .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                     .clear(ResourceClearValue{})
                                     .atStage(dafg::Stage::CS)
                                     .useAs(dafg::Usage::SHADER_RESOURCE)
                                     .handle();
        relaxNormalRoughnessHistHndl = registry.readTextureHistory("rtr_relax_normal_roughness")
                                         .atStage(dafg::Stage::CS)
                                         .useAs(dafg::Usage::SHADER_RESOURCE)
                                         .handle();
        relaxMaterialIdHndl = registry.create("rtr_relax_material_id")
                                .texture({TEXFMT_R8 | TEXCF_UNORDERED, resolution})
                                .withHistory(dafg::History::ClearZeroOnFirstFrame)
                                .clear(ResourceClearValue{})
                                .atStage(dafg::Stage::CS)
                                .useAs(dafg::Usage::SHADER_RESOURCE)
                                .handle();
        relaxMaterialIdHistHndl =
          registry.readTextureHistory("rtr_relax_material_id").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        relaxViewZHndl = registry.create("rtr_relax_view_z")
                           .texture({TEXFMT_R32F | TEXCF_UNORDERED, resolution})
                           .withHistory(dafg::History::ClearZeroOnFirstFrame)
                           .clear(ResourceClearValue{})
                           .atStage(dafg::Stage::CS)
                           .useAs(dafg::Usage::SHADER_RESOURCE)
                           .handle();
        relaxViewZHistHndl =
          registry.readTextureHistory("rtr_relax_view_z").atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        relaxTilesHndl = registry.create("rtr_relax_tiles")
                           .texture({TEXFMT_R8 | TEXCF_UNORDERED, tilesResolution})
                           .atStage(dafg::Stage::CS)
                           .useAs(dafg::Usage::SHADER_RESOURCE)
                           .handle();
        relaxHistoryLengthTempHndl = registry.create("rtr_relax_history_length_temp")
                                       .texture({TEXFMT_R8 | TEXCF_UNORDERED, resolution})
                                       .atStage(dafg::Stage::CS)
                                       .useAs(dafg::Usage::SHADER_RESOURCE)
                                       .handle();
        relaxReprojectionConfidenceHndl = registry.create("rtr_relax_reprojection_confidence")
                                            .texture({TEXFMT_R8 | TEXCF_UNORDERED, resolution})
                                            .atStage(dafg::Stage::CS)
                                            .useAs(dafg::Usage::SHADER_RESOURCE)
                                            .handle();
        relaxIllumPingHndl = registry.create("rtr_relax_illum_ping")
                               .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, resolution})
                               .atStage(dafg::Stage::CS)
                               .useAs(dafg::Usage::SHADER_RESOURCE)
                               .handle();
        relaxIllumPongHndl = registry.create("rtr_relax_illum_pong")
                               .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, resolution})
                               .atStage(dafg::Stage::CS)
                               .useAs(dafg::Usage::SHADER_RESOURCE)
                               .handle();
      }
      else
        blurTempHndl = registry.create("rtr_blur_temp")
                         .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, resolution})
                         .atStage(dafg::Stage::CS)
                         .useAs(dafg::Usage::SHADER_RESOURCE)
                         .handle();
      if (params.validation)
        validationHndl = registry.create("rtr_validation")
                           .texture({TEXFMT_A8R8G8B8 | TEXCF_UNORDERED, resolution})
                           .atStage(dafg::Stage::CS)
                           .useAs(dafg::Usage::SHADER_RESOURCE)
                           .handle();
    }
    auto resources = eastl::make_tuple(unfilteredHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, denoisedHndl, validationHndl,
      reblurPrevViewZHndl, reblurPrevViewZHistHndl, reblurPrevNormalRoughnessHndl, reblurPrevNormalRoughnessHistHndl,
      reblurPrevInternalDataHndl, reblurPrevInternalDataHistHndl, reblurHistoryHndl, reblurHistoryHistHndl, reblurFastHistoryHndl,
      reblurFastHistoryHistHndl, reblurHistoryStabilizedHndl, reblurHistoryStabilizedHistHndl, reblurHitDistForTrackingHndl,
      reblurHitDistForTrackingHistHndl, reblurData1Hndl, reblurData2Hndl, reblurHitDistForTrackingTempHndl, reblurTmp2Hndl,
      reblurFastHistoryTempHndl, reblurTilesHndl, relaxIllumHndl, relaxIllumHistHndl, relaxIllumResponsiveHndl,
      relaxIllumResponsiveHistHndl, relaxReflectionHitTHndl, relaxReflectionHitTHistHndl, relaxHistoryLengthHndl,
      relaxHistoryLengthHistHndl, relaxNormalRoughnessHndl, relaxNormalRoughnessHistHndl, relaxMaterialIdHndl, relaxMaterialIdHistHndl,
      relaxViewZHndl, relaxViewZHistHndl, relaxTilesHndl, relaxHistoryLengthTempHndl, relaxReprojectionConfidenceHndl,
      relaxIllumPingHndl, relaxIllumPongHndl, blurTempHndl);
    return [paramsHndl, resources = eastl::make_unique<decltype(resources)>(resources),
             rendered = eastl::make_unique<RenderedFlag>(rtr_renderedVarId), method = params.method,
             useSmartDepth = params.useSmartDepth]() {
      const auto &[unfilteredHndl, viewZHndl, normalRoughnessHndl, motionVectorsHndl, denoisedHndl, validationHndl,
        reblurPrevViewZHndl, reblurPrevViewZHistHndl, reblurPrevNormalRoughnessHndl, reblurPrevNormalRoughnessHistHndl,
        reblurPrevInternalDataHndl, reblurPrevInternalDataHistHndl, reblurHistoryHndl, reblurHistoryHistHndl, reblurFastHistoryHndl,
        reblurFastHistoryHistHndl, reblurHistoryStabilizedHndl, reblurHistoryStabilizedHistHndl, reblurHitDistForTrackingHndl,
        reblurHitDistForTrackingHistHndl, reblurData1Hndl, reblurData2Hndl, reblurHitDistForTrackingTempHndl, reblurTmp2Hndl,
        reblurFastHistoryTempHndl, reblurTilesHndl, relaxIllumHndl, relaxIllumHistHndl, relaxIllumResponsiveHndl,
        relaxIllumResponsiveHistHndl, relaxReflectionHitTHndl, relaxReflectionHitTHistHndl, relaxHistoryLengthHndl,
        relaxHistoryLengthHistHndl, relaxNormalRoughnessHndl, relaxNormalRoughnessHistHndl, relaxMaterialIdHndl,
        relaxMaterialIdHistHndl, relaxViewZHndl, relaxViewZHistHndl, relaxTilesHndl, relaxHistoryLengthTempHndl,
        relaxReprojectionConfidenceHndl, relaxIllumPingHndl, relaxIllumPongHndl, blurTempHndl] = *resources;
      const ReflectionParams &params = paramsHndl.ref();
      ShaderGlobal::set_int(rtr_renderedVarId, params.render ? 1 : 0);
      if (!params.render || resolution_config.useRayReconstruction)
      {
        rendered->lastFrame = false;
        return;
      }
      ResetAfterGap reset(!rendered->lastFrame);
      rendered->lastFrame = true;
      ReflectionTextures tex;
      tex.unfiltered = ptr(unfilteredHndl);
      tex.viewZ = ptr(viewZHndl);
      tex.normalRoughness = ptr(normalRoughnessHndl);
      tex.motionVectors = ptr(motionVectorsHndl);
      tex.denoised = ptr(denoisedHndl);
      tex.validation = ptr(validationHndl);
      tex.reblur.prevViewZ = {ptr(reblurPrevViewZHndl), ptr(reblurPrevViewZHistHndl)};
      tex.reblur.prevNormalRoughness = {ptr(reblurPrevNormalRoughnessHndl), ptr(reblurPrevNormalRoughnessHistHndl)};
      tex.reblur.prevInternalData = {ptr(reblurPrevInternalDataHndl), ptr(reblurPrevInternalDataHistHndl)};
      tex.reblur.history = {ptr(reblurHistoryHndl), ptr(reblurHistoryHistHndl)};
      tex.reblur.fastHistory = {ptr(reblurFastHistoryHndl), ptr(reblurFastHistoryHistHndl)};
      tex.reblur.historyStabilized = {ptr(reblurHistoryStabilizedHndl), ptr(reblurHistoryStabilizedHistHndl)};
      tex.reblur.hitDistForTracking = {ptr(reblurHitDistForTrackingHndl), ptr(reblurHitDistForTrackingHistHndl)};
      tex.reblur.data1 = ptr(reblurData1Hndl);
      tex.reblur.data2 = ptr(reblurData2Hndl);
      tex.reblur.hitDistForTrackingTemp = ptr(reblurHitDistForTrackingTempHndl);
      tex.reblur.tmp2 = ptr(reblurTmp2Hndl);
      tex.reblur.fastHistoryTemp = ptr(reblurFastHistoryTempHndl);
      tex.reblur.tiles = ptr(reblurTilesHndl);
      tex.relax.illum = {ptr(relaxIllumHndl), ptr(relaxIllumHistHndl)};
      tex.relax.illumResponsive = {ptr(relaxIllumResponsiveHndl), ptr(relaxIllumResponsiveHistHndl)};
      tex.relax.reflectionHitT = {ptr(relaxReflectionHitTHndl), ptr(relaxReflectionHitTHistHndl)};
      tex.relax.historyLength = {ptr(relaxHistoryLengthHndl), ptr(relaxHistoryLengthHistHndl)};
      tex.relax.normalRoughness = {ptr(relaxNormalRoughnessHndl), ptr(relaxNormalRoughnessHistHndl)};
      tex.relax.materialId = {ptr(relaxMaterialIdHndl), ptr(relaxMaterialIdHistHndl)};
      tex.relax.viewZ = {ptr(relaxViewZHndl), ptr(relaxViewZHistHndl)};
      tex.relax.tiles = ptr(relaxTilesHndl);
      tex.relax.historyLengthTemp = ptr(relaxHistoryLengthTempHndl);
      tex.relax.reprojectionConfidence = ptr(relaxReprojectionConfidenceHndl);
      tex.relax.illumPing = ptr(relaxIllumPingHndl);
      tex.relax.illumPong = ptr(relaxIllumPongHndl);
      tex.blurTemp = ptr(blurTempHndl);
      denoise_reflection(params, tex, method, useSmartDepth);
    };
  });
}

} // namespace denoiser

#if _TARGET_PC_LINUX
#include <osApiWrappers/dag_cpuFeatures.h>
#endif

bool denoiser::is_available()
{
#if _TARGET_PC_LINUX && _TARGET_SIMD_SSE < 4
  // BVH denoiser needs SSE4, yet we can build denoiser lib into SSE2 binary to avoid multiple client binaries
  // so check features of CPU in this case to know that BVH is really available
  if (!cpu_feature_sse41_checked)
    return false;
#endif

  return true;
}
