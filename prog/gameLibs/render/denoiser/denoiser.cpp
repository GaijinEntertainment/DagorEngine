// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define ML_NAMESPACE
#include "ml.h"
#include "ml.hlsli"

#include <render/denoiser.h>
#include <shaders/dag_computeShaders.h>
#include <image/dag_texPixel.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/algorithm.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_info.h>
#include "denoiser_names.h"

#include "shaders/common_consts.hlsli"

#include "sobol_256_4d.h"
#include "scrambling_ranking_128x128_2d_1spp.h"

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
static ComputeShaderElement *sigmaCopyCS = nullptr;
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

static d3d::SamplerHandle samplerNearestClamp = d3d::INVALID_SAMPLER_HANDLE;
static d3d::SamplerHandle samplerNearestMirror = d3d::INVALID_SAMPLER_HANDLE;
static d3d::SamplerHandle samplerLinearClamp = d3d::INVALID_SAMPLER_HANDLE;
static d3d::SamplerHandle samplerLinearMirror = d3d::INVALID_SAMPLER_HANDLE;

static UniqueTexHolder scramblingRankingTexture;
static UniqueTexHolder sobolTexture;

static UniqueBuf reblurSharedCb;

static int denoiser_resolutionVarId = -1;
static int denoiser_view_zVarId = -1;
static int denoiser_nrVarId = -1;
static int denoiser_spec_history_confidenceVarId = -1;
static int denoiser_half_view_zVarId = -1;
static int denoiser_half_nrVarId = -1;
static int denoiser_half_normalsVarId = -1;
static int denoiser_half_resVarId = -1;
static int denoiser_spec_confidence_half_resVarId = -1;

static int rtao_bindless_slotVarId = -1;
static int rtsm_bindless_slotVarId = -1;
static int ptgi_bindless_slotVarId = -1;
static int ptgi_depth_bindless_slotVarId = -1;
static int rtsm_is_translucentVarId = -1;
static int csm_bindless_slotVarId = -1;
static int csm_sampler_bindless_slotVarId = -1;
static int vsm_bindless_slotVarId = -1;
static int vsm_sampler_bindless_slotVarId = -1;
static int rtr_bindless_slotVarId = -1;
static int rtr_bindless_sampler_slotVarId = -1;
static int ptgi_bindless_sampler_slotVarId = -1;
static int ptgi_depth_bindless_sampler_slotVarId = -1;
static int rtao_bindless_sampler_slotVarId = -1;

static int blue_noise_frame_indexVarId = -1;

static int bindless_range = -1;

enum
{
  rtao_bindless_index = 0,
  ptgi_bindless_index,
  ptgi_depth_bindless_index,
  rtsm_bindless_index,
  csm_bindless_index,
  vsm_bindless_index,
  rtr_bindless_index,

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
  int2 printfAt;
  int2 rectOrigin;
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
  float4 hitDistParams;
  float4 viewVectorWorld;
  float4 viewVectorWorldPrev;
  float4 mvScale;
  float2 antilagParams;
  float2 resourceSize;
  float2 resourceSizeInv;
  float2 resourceSizeInvPrev;
  float2 rectSize;
  float2 rectSizeInv;
  float2 rectSizePrev;
  float2 resolutionScale;
  float2 resolutionScalePrev;
  float2 rectOffset;
  float2 specProbabilityThresholdsForMvModification;
  float2 jitter;
  int2 printfAt;
  int2 rectOrigin;
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
  float responsiveAccumulationRoughnessThreshold;
  float historyFixFrameNum;
  float historyFixBasePixelStride;
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
  uint32_t hasHistoryConfidence;
  uint32_t hasDisocclusionThresholdMix;
  uint32_t diffCheckerboard;
  uint32_t specCheckerboard;
  uint32_t frameIndex;
  uint32_t isRectChanged;
  uint32_t resetHistory;
  // Validation layer
  uint32_t hasDiffuse;
  uint32_t hasSpecular;

  uint32_t pad1;
  uint32_t pad2;
  uint32_t pad3;
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
  int2 printfAt;
  int2 rectOrigin;
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
  float colorBoxSigmaScale;
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
  float OrthoMode;
  float unproject;
  float framerateScale;
  float checkerboardResolveAccumSpeed;
  float jitterDelta;
  float historyFixFrameNum;
  float historyFixBasePixelStride;
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

void init_blue_noise()
{

  if (!scramblingRankingTexture)
  {
    static_assert(sizeof(scrambling_ranking_128x128_2d_1spp) == 128 * 128 * 4);
    TexImage32 *image = TexImage32::create(128, 128, tmpmem);
    memcpy(image->getPixels(), scrambling_ranking_128x128_2d_1spp, sizeof(scrambling_ranking_128x128_2d_1spp));
    scramblingRankingTexture = dag::create_tex(image, 128, 128, TEXFMT_R8G8B8A8, 1, "scrambling_ranking_texture");
    scramblingRankingTexture.setVar();
    memfree(image, tmpmem);
  }

  if (!sobolTexture)
  {
    static_assert(sizeof(sobol_256_4d) == 256 * 1 * 4);
    TexImage32 *image = TexImage32::create(256, 1, tmpmem);
    memcpy(image->getPixels(), sobol_256_4d, sizeof(sobol_256_4d));
    sobolTexture = dag::create_tex(image, 256, 1, TEXFMT_R8G8B8A8, 1, "sobol_texture");
    sobolTexture.setVar();
    memfree(image, tmpmem);
  }
}

void close_blue_noise()
{
  sobolTexture.close();
  scramblingRankingTexture.close();
}


void initialize(int w, int h, bool use_ray_reconstruction)
{
  G_ASSERT(w && h);
  G_ASSERT((resolution_config.width == 0 && resolution_config.height == 0) ||
           (resolution_config.width == w && resolution_config.height == h &&
             use_ray_reconstruction == resolution_config.useRayReconstruction));

  if (
    resolution_config.width == w && resolution_config.height == h && use_ray_reconstruction == resolution_config.useRayReconstruction)
    return;

  resolution_config.init(w, h, use_ray_reconstruction);

  if (!prepareCS)
    prepareCS = new_compute_shader("denoiser_prepare");

  if (!sigmaClassifyTilesCS)
    sigmaClassifyTilesCS = new_compute_shader("nrd_sigma_shadow_classify_tiles");
  if (!sigmaSmoothTilesCS)
    sigmaSmoothTilesCS = new_compute_shader("nrd_sigma_shadow_smooth_tiles");
  if (!sigmaCopyCS)
    sigmaCopyCS = new_compute_shader("nrd_sigma_shadow_copy");
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
  denoiser_view_zVarId = get_shader_variable_id("denoiser_view_z");
  denoiser_nrVarId = get_shader_variable_id("denoiser_nr");
  denoiser_half_view_zVarId = get_shader_variable_id("denoiser_half_view_z");
  denoiser_half_nrVarId = get_shader_variable_id("denoiser_half_nr");
  denoiser_half_normalsVarId = get_shader_variable_id("denoiser_half_normals");
  denoiser_spec_history_confidenceVarId = get_shader_variable_id("denoiser_spec_history_confidence");
  denoiser_half_resVarId = get_shader_variable_id("denoiser_half_res");
  denoiser_spec_confidence_half_resVarId = get_shader_variable_id("denoiser_spec_confidence_half_res");

  rtao_bindless_slotVarId = get_shader_variable_id("rtao_bindless_slot");
  rtsm_bindless_slotVarId = get_shader_variable_id("rtsm_bindless_slot");
  ptgi_bindless_slotVarId = get_shader_variable_id("ptgi_bindless_slot");
  ptgi_depth_bindless_slotVarId = get_shader_variable_id("ptgi_depth_bindless_slot");
  rtsm_is_translucentVarId = get_shader_variable_id("rtsm_is_translucent");
  csm_bindless_slotVarId = get_shader_variable_id("csm_bindless_slot", true);
  csm_sampler_bindless_slotVarId = get_shader_variable_id("csm_sampler_bindless_slot", true);
  vsm_bindless_slotVarId = get_shader_variable_id("vsm_bindless_slot", true);
  vsm_sampler_bindless_slotVarId = get_shader_variable_id("vsm_sampler_bindless_slot", true);
  rtr_bindless_slotVarId = get_shader_variable_id("rtr_bindless_slot");
  rtr_bindless_sampler_slotVarId = get_shader_variable_id("rtr_bindless_sampler_slot");
  ptgi_bindless_sampler_slotVarId = get_shader_variable_id("ptgi_bindless_sampler_slot");
  ptgi_depth_bindless_sampler_slotVarId = get_shader_variable_id("ptgi_depth_bindless_sampler_slot");
  rtao_bindless_sampler_slotVarId = get_shader_variable_id("rtao_bindless_sampler_slot");

  blue_noise_frame_indexVarId = get_shader_variable_id("blue_noise_frame_index");

  ShaderGlobal::set_int4(denoiser_resolutionVarId, resolution_config.width, resolution_config.height, 0, 0);

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

inline void clear_texture(const TextureIDPair &tex)
{
  if (tex.getTex2D())
    clear_texture(tex.getTex2D());
}

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

void teardown()
{
  safe_delete(prepareCS);
  safe_delete(sigmaClassifyTilesCS);
  safe_delete(sigmaSmoothTilesCS);
  safe_delete(sigmaCopyCS);
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

  close_blue_noise();

  reblurSharedCb.close();

  resolution_config.closeAll();
  frame_counter = 0;

  if (bindless_range > 0)
  {
    d3d::free_bindless_resource_range(D3DResourceType::TEX, bindless_range, bindless_texture_count);
    bindless_range = -1;
  }
}

void add_persistent_texture(TexInfoMap &textures, const char *name, int w, int h, int flg)
{
  auto &ti = textures[name];
  ti.w = w;
  ti.h = h;
  ti.mipLevels = 1;
  ti.type = D3DResourceType::TEX;
  ti.cflg = TEXCF_UNORDERED | flg;
}

void add_transient_texture(TexInfoMap &textures, const char *name, int w, int h, int flg)
{
  auto &ti = textures[name];
  ti.w = w;
  ti.h = h;
  ti.mipLevels = 1;
  ti.type = D3DResourceType::TEX;
  ti.cflg = TEXCF_UNORDERED | flg;
}

void get_required_persistent_texture_descriptors(TexInfoMap &persistent_textures, bool need_half_res)
{
  if (resolution_config.useRayReconstruction)
    return;

  add_persistent_texture(persistent_textures, TextureNames::denoiser_normal_roughness, resolution_config.width,
    resolution_config.height, TEXFMT_A2R10G10B10);
  add_persistent_texture(persistent_textures, TextureNames::denoiser_view_z, resolution_config.width, resolution_config.height,
    TEXFMT_R32F);
  if (need_half_res)
  {
    add_persistent_texture(persistent_textures, TextureNames::denoiser_half_normal_roughness, resolution_config.width / 2,
      resolution_config.height / 2, TEXFMT_A2R10G10B10);
    add_persistent_texture(persistent_textures, TextureNames::denoiser_half_view_z, resolution_config.width / 2,
      resolution_config.height / 2, TEXFMT_R32F);
  }
}

void get_required_persistent_texture_descriptors_for_ao(TexInfoMap &persistent_textures)
{
  if (resolution_config.width == 0 || resolution_config.height == 0)
    return;

  int width = resolution_config.rtao.width;
  int height = resolution_config.rtao.height;

  if (resolution_config.useRayReconstruction)
  {
    add_persistent_texture(persistent_textures, AODenoiser::TextureNames::rtao_tex_unfiltered, width, height, TEXFMT_L16);

    ShaderGlobal::set_int(rtao_bindless_sampler_slotVarId, d3d::register_bindless_sampler(samplerNearestClamp));
  }
  else
  {
    ShaderGlobal::set_int(rtao_bindless_sampler_slotVarId,
      d3d::register_bindless_sampler(resolution_config.rtao.isHalfRes ? samplerLinearClamp : samplerNearestClamp));

    add_persistent_texture(persistent_textures, AODenoiser::TextureNames::rtao_tex_unfiltered, width, height, TEXFMT_L16);
    add_persistent_texture(persistent_textures, AODenoiser::TextureNames::denoised_ao, width, height, TEXFMT_L16);
    add_persistent_texture(persistent_textures, AODenoiser::TextureNames::ao_prev_view_z, width, height, TEXFMT_R32F);
    add_persistent_texture(persistent_textures, AODenoiser::TextureNames::ao_prev_normal_roughness, width, height, TEXFMT_A2R10G10B10);
    add_persistent_texture(persistent_textures, AODenoiser::TextureNames::ao_prev_internal_data, width, height, TEXFMT_R16UI);
    add_persistent_texture(persistent_textures, AODenoiser::TextureNames::ao_diff_fast_history, width, height, TEXFMT_L16);
  }
}

void get_required_transient_texture_descriptors_for_ao(TexInfoMap &transient_textures)
{
  if (resolution_config.width == 0 || resolution_config.height == 0)
    return;

  if (resolution_config.useRayReconstruction)
    return;

  int width = resolution_config.rtao.width;
  int height = resolution_config.rtao.height;
  int tilesW = divide_up(width, 16);
  int tilesH = divide_up(height, 16);

  add_transient_texture(transient_textures, AODenoiser::TextureNames::ao_tiles, tilesW, tilesH, TEXFMT_R8);
  add_transient_texture(transient_textures, AODenoiser::TextureNames::ao_diffTemp2, width, height, TEXFMT_L16);
  add_transient_texture(transient_textures, AODenoiser::TextureNames::ao_diffFastHistory, width, height, TEXFMT_L16);
  add_transient_texture(transient_textures, AODenoiser::TextureNames::ao_data1, width, height, TEXFMT_R8);
}

inline int calc_tiles_width(int w, bool checkerboard)
{
  int checkerboardWidth = divide_up(w, 2);
  return divide_up(checkerboard ? checkerboardWidth : w, 8);
}

inline int calc_tiles_height(int h) { return divide_up(h, 8); }

void get_required_persistent_texture_descriptors_for_rtr(TexInfoMap &persistent_textures, ReflectionMethod method)
{
  if (resolution_config.width == 0 || resolution_config.height == 0)
    return;

  int width = resolution_config.rtr.width;
  int height = resolution_config.rtr.height;

  if (resolution_config.useRayReconstruction)
  {

    // R is distance, BGA is color
    add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::rtr_tex_unfiltered, width, height,
      TEXFMT_A16B16G16R16F);

    ShaderGlobal::set_int(rtr_bindless_sampler_slotVarId, d3d::register_bindless_sampler(samplerNearestClamp));
  }
  else
  {
    ShaderGlobal::set_int(rtr_bindless_sampler_slotVarId,
      d3d::register_bindless_sampler(resolution_config.rtr.isHalfRes ? samplerLinearClamp : samplerNearestClamp));

    // division.
    int tilesWidth = calc_tiles_width(width, resolution_config.rtr.isHalfRes);
    int tilesHeight = calc_tiles_height(height);

    add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::rtr_tex_unfiltered, width, height,
      TEXFMT_A16B16G16R16F);
    add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::rtr_tex, width, height, TEXFMT_A16B16G16R16F);
    add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::rtr_sample_tiles, tilesWidth, tilesHeight,
      TEXCF_UNORDERED | TEXFMT_R8UI);

    if (method == ReflectionMethod::Reblur)
    {
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::reblur_spec_prev_view_z, width, height,
        TEXFMT_R32F);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::reblur_spec_prev_normal_roughness, width, height,
        TEXFMT_A2R10G10B10);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::reblur_spec_prev_internal_data, width, height,
        TEXFMT_R16UI);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::reblur_spec_history, width, height,
        TEXFMT_A16B16G16R16F);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::reblur_spec_fast_history, width, height,
        TEXFMT_R16F);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::reblur_spec_history_stabilized_ping, width, height,
        TEXFMT_R16F);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::reblur_spec_history_stabilized_pong, width, height,
        TEXFMT_R16F);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::reblur_spec_hitdist_for_tracking_ping, width,
        height, TEXFMT_R16F);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::reblur_spec_hitdist_for_tracking_pong, width,
        height, TEXFMT_R16F);
    }
    else if (method == ReflectionMethod::Relax)
    {
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::relax_spec_illum_prev, width, height,
        TEXFMT_A16B16G16R16F);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::relax_spec_illum_responsive_prev, width, height,
        TEXFMT_A16B16G16R16F);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::relax_spec_reflection_hit_t_curr, width, height,
        TEXFMT_R16F);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::relax_spec_reflection_hit_t_prev, width, height,
        TEXFMT_R16F);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::relax_spec_history_length_prev, width, height,
        TEXFMT_R8);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::relax_spec_normal_roughness_prev, width, height,
        TEXFMT_A8R8G8B8);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::relax_spec_material_id_prev, width, height,
        TEXFMT_R8);
      add_persistent_texture(persistent_textures, ReflectionDenoiser::TextureNames::relax_spec_view_z_prev, width, height,
        TEXFMT_R32F);
    }
    else
      G_ASSERTF(false, "Reflection denoiser not implemented!");
  }
}

void get_required_transient_texture_descriptors_for_rtr(TexInfoMap &transient_textures, ReflectionMethod method)
{
  if (resolution_config.width == 0 || resolution_config.height == 0)
    return;

  if (resolution_config.useRayReconstruction)
    return;

  int width = resolution_config.rtr.width;
  int height = resolution_config.rtr.height;
  int tilesW = divide_up(width, 16);
  int tilesH = divide_up(height, 16);

  if (method == ReflectionMethod::Reblur)
  {
    add_transient_texture(transient_textures, ReflectionDenoiser::TextureNames::rtr_data1, width, height, TEXFMT_R8);
    add_transient_texture(transient_textures, ReflectionDenoiser::TextureNames::rtr_data2, width, height, TEXFMT_R32UI);
    add_transient_texture(transient_textures, ReflectionDenoiser::TextureNames::rtr_hitdistForTracking, width, height, TEXFMT_R16F);
    add_transient_texture(transient_textures, ReflectionDenoiser::TextureNames::rtr_tmp2, width, height, TEXFMT_A16B16G16R16F);
    add_transient_texture(transient_textures, ReflectionDenoiser::TextureNames::rtr_fastHistory, width, height, TEXFMT_R16F);
    add_transient_texture(transient_textures, ReflectionDenoiser::TextureNames::rtr_reblur_tiles, tilesW, tilesH, TEXFMT_R8);
  }
  else if (method == ReflectionMethod::Relax)
  {
    add_transient_texture(transient_textures, ReflectionDenoiser::TextureNames::rtr_relax_tiles, tilesW, tilesH, TEXFMT_R8);
    add_transient_texture(transient_textures, ReflectionDenoiser::TextureNames::rtr_historyLength, width, height, TEXFMT_R8);
    add_transient_texture(transient_textures, ReflectionDenoiser::TextureNames::rtr_specReprojectionConfidence, width, height,
      TEXFMT_R8);
    add_transient_texture(transient_textures, ReflectionDenoiser::TextureNames::rtr_specIllumPing, width, height,
      TEXFMT_A16B16G16R16F);
    add_transient_texture(transient_textures, ReflectionDenoiser::TextureNames::rtr_specIllumPong, width, height,
      TEXFMT_A16B16G16R16F);
  }
  else
    G_ASSERTF(false, "Reflection denoiser not implemented!");
}

void get_required_persistent_texture_descriptors_for_rtsm(TexInfoMap &persistent_textures, bool translucent)
{
  if (resolution_config.width == 0 || resolution_config.height == 0)
    return;

  int width = resolution_config.rtsm.width;
  int height = resolution_config.rtsm.height;

  if (resolution_config.useRayReconstruction)
  {
    add_persistent_texture(persistent_textures, ShadowDenoiser::TextureNames::rtsm_value, width, height, TEXCF_RTARGET | TEXFMT_R8);
  }
  else
  {
    add_persistent_texture(persistent_textures, ShadowDenoiser::TextureNames::rtsm_historyLengthPersistent, width, height,
      TEXFMT_R32UI);
    add_persistent_texture(persistent_textures, ShadowDenoiser::TextureNames::rtsm_value, width, height, TEXFMT_R16F);
    add_persistent_texture(persistent_textures, ShadowDenoiser::TextureNames::rtsm_shadows_denoised, width, height,
      TEXCF_RTARGET | (translucent ? TEXFMT_A8R8G8B8 : TEXFMT_R8));
    if (translucent)
      add_persistent_texture(persistent_textures, ShadowDenoiser::TextureNames::rtsm_translucency, width, height, TEXFMT_A8R8G8B8);
  }
}

void get_required_transient_texture_descriptors_for_rtsm(TexInfoMap &transient_textures, bool translucent)
{
  if (resolution_config.width == 0 || resolution_config.height == 0)
    return;

  if (resolution_config.useRayReconstruction)
    return;

  int width = resolution_config.rtsm.width;
  int height = resolution_config.rtsm.height;
  int tilesW = divide_up(width, 16);
  int tilesH = divide_up(height, 16);

  unsigned int tmpFmt = translucent ? TEXFMT_A8R8G8B8 : TEXFMT_R8;

  add_transient_texture(transient_textures, ShadowDenoiser::TextureNames::rtsm_tiles, tilesW, tilesH, TEXFMT_A8R8G8B8);
  add_transient_texture(transient_textures, ShadowDenoiser::TextureNames::rtsm_smoothTiles, tilesW, tilesH, TEXFMT_R8G8);
  add_transient_texture(transient_textures, ShadowDenoiser::TextureNames::rtsm_denoiseData1, width, height, TEXFMT_R16F);
  add_transient_texture(transient_textures, ShadowDenoiser::TextureNames::rtsm_denoiseData2, width, height, TEXFMT_R16F);
  add_transient_texture(transient_textures, ShadowDenoiser::TextureNames::rtsm_denoiseTemp1, width, height, tmpFmt);
  add_transient_texture(transient_textures, ShadowDenoiser::TextureNames::rtsm_denoiseTemp2, width, height, tmpFmt);
  add_transient_texture(transient_textures, ShadowDenoiser::TextureNames::rtsm_denoiseHistory, width, height, tmpFmt);
  add_transient_texture(transient_textures, ShadowDenoiser::TextureNames::rtsm_denoiseHistoryLengthTransient, width, height,
    TEXFMT_R32UI);
}

void get_required_persistent_texture_descriptors_for_gi(TexInfoMap &persistent_textures)
{
  if (resolution_config.width == 0 || resolution_config.height == 0)
    return;

  int width = resolution_config.ptgi.width;
  int height = resolution_config.ptgi.height;

  if (resolution_config.useRayReconstruction)
  {

    add_persistent_texture(persistent_textures, GIDenoiser::TextureNames::ptgi_tex_unfiltered, width, height, TEXFMT_A16B16G16R16F);

    ShaderGlobal::set_int(ptgi_bindless_sampler_slotVarId, d3d::register_bindless_sampler(samplerNearestClamp));
  }
  else
  {
    ShaderGlobal::set_int(ptgi_bindless_sampler_slotVarId, d3d::register_bindless_sampler(samplerLinearClamp));
    ShaderGlobal::set_int(ptgi_depth_bindless_sampler_slotVarId, d3d::register_bindless_sampler(samplerLinearClamp));

    add_persistent_texture(persistent_textures, GIDenoiser::TextureNames::ptgi_tex_unfiltered, width, height, TEXFMT_A16B16G16R16F);
    add_persistent_texture(persistent_textures, GIDenoiser::TextureNames::denoised_gi, width, height, TEXFMT_A16B16G16R16F);

    add_persistent_texture(persistent_textures, GIDenoiser::TextureNames::gi_prev_view_z, width, height, TEXFMT_R32F);
    add_persistent_texture(persistent_textures, GIDenoiser::TextureNames::gi_prev_normal_roughness, width, height, TEXFMT_A2R10G10B10);
    add_persistent_texture(persistent_textures, GIDenoiser::TextureNames::gi_prev_internal_data, width, height, TEXFMT_R16UI);
    add_persistent_texture(persistent_textures, GIDenoiser::TextureNames::gi_diff_history, width, height, TEXFMT_A16B16G16R16F);
    add_persistent_texture(persistent_textures, GIDenoiser::TextureNames::gi_diff_fast_history, width, height, TEXFMT_R16F);
    add_persistent_texture(persistent_textures, GIDenoiser::TextureNames::gi_diff_history_stabilized_ping, width, height, TEXFMT_R16F);
    add_persistent_texture(persistent_textures, GIDenoiser::TextureNames::gi_diff_history_stabilized_pong, width, height, TEXFMT_R16F);
  }
}

void get_required_transient_texture_descriptors_for_gi(TexInfoMap &transient_textures)
{
  if (resolution_config.width == 0 || resolution_config.height == 0)
    return;

  if (resolution_config.useRayReconstruction)
    return;

  int width = resolution_config.ptgi.width;
  int height = resolution_config.ptgi.height;
  int tilesW = divide_up(width, 16);
  int tilesH = divide_up(height, 16);

  add_transient_texture(transient_textures, GIDenoiser::TextureNames::gi_data1, width, height, TEXFMT_R8);
  add_transient_texture(transient_textures, GIDenoiser::TextureNames::gi_data2, width, height, TEXFMT_R8UI);
  add_transient_texture(transient_textures, GIDenoiser::TextureNames::gi_diffTemp2, width, height, TEXFMT_A16B16G16R16F);
  add_transient_texture(transient_textures, GIDenoiser::TextureNames::gi_diffFastHistory, width, height, TEXFMT_R16F);
  add_transient_texture(transient_textures, GIDenoiser::TextureNames::gi_tiles, tilesW, tilesH, TEXFMT_R8);
}

inline float4x4 convertx(const TMatrix4 &m)
{
  return float4x4(m._11, m._12, m._13, m._14, m._21, m._22, m._23, m._24, m._31, m._32, m._33, m._34, m._41, m._42, m._43, m._44);
}
inline float4x4 convert(const TMatrix4 &m)
{
  return float4x4(m._11, m._21, m._31, m._41, m._12, m._22, m._32, m._42, m._13, m._23, m._33, m._43, m._14, m._24, m._34, m._44);
}

void prepare(const FrameParams &params)
{
  frame_counter++;

  ShaderGlobal::set_int(blue_noise_frame_indexVarId, frame_counter);

  if (resolution_config.useRayReconstruction)
    return;

  auto normal_roughness_iter = params.textures.find(TextureNames::denoiser_normal_roughness);
  G_ASSERT_RETURN(normal_roughness_iter != params.textures.end(), );

  auto view_z_iter = params.textures.find(TextureNames::denoiser_view_z);
  G_ASSERT_RETURN(view_z_iter != params.textures.end(), );

  auto normal_roughness = normal_roughness_iter->second.getTex2D();
  auto normal_roughness_id = normal_roughness_iter->second.getId();
  auto view_z = view_z_iter->second.getTex2D();
  auto view_z_id = view_z_iter->second.getId();

  G_ASSERT_RETURN(normal_roughness && view_z, );

  TIME_D3D_PROFILE(denoiser::prepare);

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

  Point2 scaledJitter = Point2(jitter.x * resolution_config.width, jitter.y * resolution_config.height);
  Point2 scaledPrevJitter = Point2(prev_jitter.x * resolution_config.width, prev_jitter.y * resolution_config.height);

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

  auto reblur_history_confidence_iter = params.textures.find(TextureNames::reblur_history_confidence);
  auto reblur_history_confidence_id =
    reblur_history_confidence_iter == params.textures.end() ? BAD_TEXTUREID : reblur_history_confidence_iter->second.getId();

  ShaderGlobal::set_int(denoiser_spec_confidence_half_resVarId, resolution_config.rtr.isHalfRes ? 1 : 0);
  ShaderGlobal::set_int4(denoiser_resolutionVarId, resolution_config.width, resolution_config.height, 0, 0);
  ShaderGlobal::set_texture(denoiser_view_zVarId, view_z_id);
  ShaderGlobal::set_texture(denoiser_nrVarId, normal_roughness_id);
  ShaderGlobal::set_texture(denoiser_spec_history_confidenceVarId, reblur_history_confidence_id);
  if ((resolution_config.rtao.width > 0 && resolution_config.rtao.isHalfRes) ||
      (resolution_config.rtr.width > 0 && resolution_config.rtr.isHalfRes) ||
      (resolution_config.ptgi.width > 0 && resolution_config.ptgi.isHalfRes))
  {
    auto half_normal_roughness_iter = params.textures.find(TextureNames::denoiser_half_normal_roughness);
    auto half_view_z_iter = params.textures.find(TextureNames::denoiser_half_view_z);
    auto half_normals_iter = params.textures.find(TextureNames::half_normals);
    G_ASSERT_RETURN(half_normal_roughness_iter != params.textures.end() && half_view_z_iter != params.textures.end() &&
                      half_normals_iter != params.textures.end(), );

    auto half_normal_roughness_id = half_normal_roughness_iter->second.getId();
    auto half_view_z_id = half_view_z_iter->second.getId();
    auto half_normals_id = half_normals_iter->second.getId();

    ShaderGlobal::set_int(denoiser_half_resVarId, 1);
    ShaderGlobal::set_texture(denoiser_half_view_zVarId, half_view_z_id);
    ShaderGlobal::set_texture(denoiser_half_normalsVarId, half_normals_id);
    ShaderGlobal::set_texture(denoiser_half_nrVarId, half_normal_roughness_id);
  }
  else
  {
    ShaderGlobal::set_int(denoiser_half_resVarId, 0);
    ShaderGlobal::set_texture(denoiser_half_view_zVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(denoiser_half_normalsVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(denoiser_half_nrVarId, BAD_TEXTUREID);
  }
  prepareCS->dispatchThreads(resolution_config.width, resolution_config.height, 1);
}

static float randuf() { return rand() / float(RAND_MAX); }
static float randsf() { return 2 * randuf() - 1; }
static Point4 rand4uf() { return Point4(randuf(), randuf(), randuf(), randuf()); }
static Point4 rand4sf() { return Point4(randsf(), randsf(), randsf(), randsf()); }

template <typename T>
static eastl::tuple<Texture *, Texture *, Texture *> get_denoiser_textures(const T &params, bool half_res)
{
  auto normal_roughness_iter =
    params.textures.find(half_res ? TextureNames::denoiser_half_normal_roughness : TextureNames::denoiser_normal_roughness);
  auto view_z_iter = params.textures.find(half_res ? TextureNames::denoiser_half_view_z : TextureNames::denoiser_view_z);
  auto motion_vectors_iter = params.textures.find(half_res ? TextureNames::half_motion_vectors : TextureNames::motion_vectors);
  if (normal_roughness_iter == params.textures.end() || view_z_iter == params.textures.end() ||
      motion_vectors_iter == params.textures.end())
    return eastl::make_tuple(nullptr, nullptr, nullptr);
  auto normal_roughness = normal_roughness_iter->second.getTex2D();
  auto view_z = view_z_iter->second.getTex2D();
  auto motion_vectors = motion_vectors_iter->second.getTex2D();
  return eastl::make_tuple(normal_roughness, view_z, motion_vectors);
}

#define GET_DENOISER_TEXTURES(params, half_res) auto [normalRoughness, viewZ, motionVectors] = get_denoiser_textures(params, half_res)

#define ACQUIRE_TEXTURE(name) ACQUIRE_DENOISER_TEXTURE(params, name)

void denoise_shadow(const ShadowDenoiser &params)
{
  if (params.csmTexture)
  {
    d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + csm_bindless_index, params.csmTexture);
    ShaderGlobal::set_int(csm_bindless_slotVarId, bindless_range + csm_bindless_index);
    auto samplerIx = d3d::register_bindless_sampler(params.csmSampler);
    ShaderGlobal::set_int(csm_sampler_bindless_slotVarId, samplerIx);
  }

  if (params.vsmTexture)
  {
    d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + vsm_bindless_index, params.vsmTexture);
    ShaderGlobal::set_int(vsm_bindless_slotVarId, bindless_range + vsm_bindless_index);
    auto samplerIx = d3d::register_bindless_sampler(params.vsmSampler);
    ShaderGlobal::set_int(vsm_sampler_bindless_slotVarId, samplerIx);
  }

  if (resolution_config.useRayReconstruction)
  {
    ACQUIRE_TEXTURE(rtsm_value);
    d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + rtsm_bindless_index, rtsm_value);
    ShaderGlobal::set_int(rtsm_bindless_slotVarId, bindless_range + rtsm_bindless_index);
    ShaderGlobal::set_int(rtsm_is_translucentVarId, 0);

    d3d::resource_barrier(ResourceBarrierDesc(rtsm_value, RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));

    return;
  }

  TIME_D3D_PROFILE(denoise_shadow);
  int width = resolution_config.rtsm.width, height = resolution_config.rtsm.height;

  int tilesW = divide_up(width, 16);
  int tilesH = divide_up(height, 16);

#define NAME(name) ACQUIRE_TEXTURE(name)
  ENUM_RTSM_DENOISED_PERSISTENT_NAMES
  ENUM_RTSM_DENOISED_TRANSIENT_NAMES
#undef NAME

  auto titer = params.textures.find(ShadowDenoiser::TextureNames::rtsm_translucency);
  auto rtsm_translucency = titer != params.textures.end() ? titer->second.getTex2D() : nullptr;

  GET_DENOISER_TEXTURES(params, false);

  d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + rtsm_bindless_index, rtsm_shadows_denoised);
  ShaderGlobal::set_int(rtsm_bindless_slotVarId, bindless_range + rtsm_bindless_index);
  ShaderGlobal::set_int(rtsm_is_translucentVarId, rtsm_translucency ? 1 : 0);

  if (reset_history)
    clear_texture(rtsm_shadows_denoised);

  float3 lightDirectionView =
    Rotate(world_to_view, float3(-params.lightDirection.x, -params.lightDirection.y, -params.lightDirection.z));

  float3 cameraDelta = prev_view_pos - view_pos;

  uint32_t frameNum = min(params.maxStabilizedFrameNum, ShadowDenoiser::MAX_HISTORY_FRAME_NUM);
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
  sigmaSharedConstants.resourceSizeInvPrev = float2(1.0 / width, 1.0 / height);
  sigmaSharedConstants.rectSize = float2(width, height);
  sigmaSharedConstants.rectSizeInv = float2(1.0 / width, 1.0 / height);
  sigmaSharedConstants.rectSizePrev = float2(width, height);
  sigmaSharedConstants.resolutionScale = float2(1, 1);
  sigmaSharedConstants.rectOffset = float2(0, 0);
  sigmaSharedConstants.printfAt = int2(0, 0);
  sigmaSharedConstants.rectOrigin = int2(0, 0);
  sigmaSharedConstants.rectSizeMinusOne = int2(width - 1, height - 1);
  sigmaSharedConstants.tilesSizeMinusOne = int2(tilesW - 1, tilesH - 1);
  sigmaSharedConstants.orthoMode = 0;
  sigmaSharedConstants.unproject = unproject;
  sigmaSharedConstants.denoisingRange = 100000;
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
    d3d::set_tex(STAGE_CS, 0, viewZ);
    d3d::set_tex(STAGE_CS, 1, rtsm_value);
    if (rtsm_translucency)
      d3d::set_tex(STAGE_CS, 2, rtsm_translucency);
    d3d::set_rwtex(STAGE_CS, 0, rtsm_tiles, 0, 0);

    sigmaClassifyTilesCS->dispatch(tilesW, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(sigma::smooth_tiles);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, rtsm_tiles);
    d3d::set_rwtex(STAGE_CS, 0, rtsm_smoothTiles, 0, 0);

    sigmaSmoothTilesCS->dispatchThreads(tilesW, tilesH, 1);
  }

  if (stabilizationEnabled)
  {
    TIME_D3D_PROFILE(sigma::copy);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, rtsm_smoothTiles);
    d3d::set_tex(STAGE_CS, 1, rtsm_shadows_denoised);
    d3d::set_tex(STAGE_CS, 2, rtsm_historyLengthPersistent);
    d3d::set_rwtex(STAGE_CS, 0, rtsm_denoiseHistory, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, rtsm_denoiseHistoryLengthTransient, 0, 0);

    sigmaCopyCS->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(sigma::blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, viewZ);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, rtsm_value);
    d3d::set_tex(STAGE_CS, 3, rtsm_smoothTiles);
    if (rtsm_translucency)
      d3d::set_tex(STAGE_CS, 4, rtsm_translucency);
    d3d::set_rwtex(STAGE_CS, 0, rtsm_denoiseData1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, rtsm_denoiseTemp1, 0, 0);

    sigmaBlurCS->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(sigma::post_blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, viewZ);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, rtsm_denoiseData1);
    d3d::set_tex(STAGE_CS, 3, rtsm_smoothTiles);
    d3d::set_tex(STAGE_CS, 4, rtsm_denoiseTemp1);
    d3d::set_rwtex(STAGE_CS, 0, rtsm_denoiseData2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, stabilizationEnabled ? rtsm_denoiseTemp2 : rtsm_shadows_denoised, 0, 0);

    sigmaPostBlurCS->dispatch(tilesW * 2, tilesH, 1);
  }

  if (stabilizationEnabled)
  {
    TIME_D3D_PROFILE(sigma::temporal_stabilization);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, viewZ);
    d3d::set_tex(STAGE_CS, 1, motionVectors);
    d3d::set_tex(STAGE_CS, 2, rtsm_denoiseData2);
    d3d::set_tex(STAGE_CS, 3, rtsm_denoiseTemp2);
    d3d::set_tex(STAGE_CS, 4, rtsm_denoiseHistory);
    d3d::set_tex(STAGE_CS, 5, rtsm_denoiseHistoryLengthTransient);
    d3d::set_tex(STAGE_CS, 6, rtsm_smoothTiles);
    d3d::set_rwtex(STAGE_CS, 0, rtsm_shadows_denoised, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, rtsm_historyLengthPersistent, 0, 0);

    sigmaStabilizeCS->dispatch(tilesW * 2, tilesH, 1);
  }

  d3d::resource_barrier({rtsm_shadows_denoised, RB_RO_SRV | RB_STAGE_ALL_SHADERS, 0, 0});
}

static ReblurSharedConstants make_reblur_shared_constants(const Point4 &hit_dist_params, const Point2 &antilag_settings,
  bool use_history_confidence, int width, int height, int max_stabilized_frame_num, bool checkerboard, bool occlusion,
  bool antiFirefly)
{
  float disocclusionThresholdBonus = (1.0f + jitter_delta) / float(height);
  float disocclusionThreshold = 0.01 + disocclusionThresholdBonus;
  float disocclusionThresholdAlternate = 0.05 + disocclusionThresholdBonus;

  float3 cameraDelta = prev_view_pos - view_pos;

  uint32_t frameNum =
    min(max_stabilized_frame_num, occlusion ? AODenoiser::MAX_HISTORY_FRAME_NUM : ReflectionDenoiser::REBLUR_MAX_HISTORY_FRAME_NUM);
  float stabilizationStrength = frameNum / (1.0f + frameNum);

  float unproject = 1.0f / (0.5f * height * abs(projectY));

  float lobeAngleFraction = occlusion ? 0.5f : 0.15f;

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
  reblurSharedConstants.hitDistParams = float4(hit_dist_params.x, hit_dist_params.y, hit_dist_params.z, hit_dist_params.w);
  reblurSharedConstants.viewVectorWorld = float4(view_dir, 0);
  reblurSharedConstants.viewVectorWorldPrev = float4(prev_view_dir, 0);
  reblurSharedConstants.mvScale = float4(motion_multiplier, 0);

  // float2
  reblurSharedConstants.antilagParams = float2(antilag_settings.x, antilag_settings.y);
  reblurSharedConstants.resourceSize = float2(width, height);
  reblurSharedConstants.resourceSizeInv = float2(1.0 / width, 1.0 / height);
  reblurSharedConstants.resourceSizeInvPrev = float2(1.0 / width, 1.0 / height);
  reblurSharedConstants.rectSize = float2(width, height);
  reblurSharedConstants.rectSizeInv = float2(1.0 / width, 1.0 / height);
  reblurSharedConstants.rectSizePrev = float2(width, height);
  reblurSharedConstants.resolutionScale = float2(1, 1);
  reblurSharedConstants.resolutionScalePrev = float2(1, 1);
  reblurSharedConstants.rectOffset = float2(0, 0);
  reblurSharedConstants.specProbabilityThresholdsForMvModification = float2(2.0f, 3.0f);
  reblurSharedConstants.jitter = float2(jitter.x * width, jitter.y * height);

  // int2
  reblurSharedConstants.printfAt = int2(0, 0);
  reblurSharedConstants.rectOrigin = int2(0, 0);
  reblurSharedConstants.rectSizeMinusOne = int2(width - 1, height - 1);

  // float
  reblurSharedConstants.disocclusionThreshold = disocclusionThreshold;
  reblurSharedConstants.disocclusionThresholdAlternate = disocclusionThresholdAlternate;
  reblurSharedConstants.cameraAttachedReflectionMaterialID = CAMERA_ATTACHED_REFLECTION_MATERIAL_ID;
  reblurSharedConstants.strandMaterialID = 999;
  reblurSharedConstants.strandThickness = 80e-6f;
  reblurSharedConstants.stabilizationStrength = reset_history ? 0 : stabilizationStrength;
  reblurSharedConstants.debug = 0;
  reblurSharedConstants.orthoMode = 0;
  reblurSharedConstants.unproject = unproject;
  reblurSharedConstants.denoisingRange = 100000;
  reblurSharedConstants.planeDistSensitivity = 0.02;
  reblurSharedConstants.framerateScale = frame_rate_scale;
  reblurSharedConstants.minBlurRadius = occlusion ? 5 : 1;
  reblurSharedConstants.maxBlurRadius = 30;
  reblurSharedConstants.diffPrepassBlurRadius = 30;
  reblurSharedConstants.specPrepassBlurRadius = 50;
  reblurSharedConstants.maxAccumulatedFrameNum = reset_history ? 0 : max_accumulated_frame_num;
  reblurSharedConstants.maxFastAccumulatedFrameNum = reset_history ? 0 : max_fast_accumulated_frame_num;
  reblurSharedConstants.antiFirefly = antiFirefly ? 1 : 0;
  reblurSharedConstants.lobeAngleFraction = lobeAngleFraction * lobeAngleFraction;
  reblurSharedConstants.roughnessFraction = 0.15;
  reblurSharedConstants.responsiveAccumulationRoughnessThreshold = 0;
  reblurSharedConstants.historyFixFrameNum = occlusion ? 2 : 3;
  reblurSharedConstants.historyFixBasePixelStride = 14;
  reblurSharedConstants.minRectDimMulUnproject = (float)min(width, height) * unproject;
  reblurSharedConstants.usePrepassNotOnlyForSpecularMotionEstimation = 1;
  reblurSharedConstants.splitScreen = 0;
  reblurSharedConstants.splitScreenPrev = 0;
  reblurSharedConstants.checkerboardResolveAccumSpeed = checkerboard_resolve_accum_speed;
  reblurSharedConstants.viewZScale = 1;
  reblurSharedConstants.fireflySuppressorMinRelativeScale = 2;
  reblurSharedConstants.minHitDistanceWeight = 0.1f;
  reblurSharedConstants.diffMinMaterial = 0;
  reblurSharedConstants.specMinMaterial = 0;

  // uint32_t
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

  return reblurSharedConstants;
}

void denoise_ao(const AODenoiser &params)
{
  if (resolution_config.useRayReconstruction)
  {
    ACQUIRE_TEXTURE(rtao_tex_unfiltered);
    d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + rtao_bindless_index, rtao_tex_unfiltered);
    ShaderGlobal::set_int(rtao_bindless_slotVarId, bindless_range + rtao_bindless_index);
    d3d::resource_barrier(ResourceBarrierDesc(rtao_tex_unfiltered, RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));
    return;
  }

  TIME_D3D_PROFILE(denoise_ao);

#define NAME(name) ACQUIRE_TEXTURE(name)
  ENUM_AO_DENOISED_NAMES
#undef NAME

  TextureInfo ti, tilesTi;
  rtao_tex_unfiltered->getinfo(ti);
  ao_tiles->getinfo(tilesTi);

  int width = ti.w, height = ti.h, tilesWidth = tilesTi.w, tilesHeight = tilesTi.h;
  bool halfRes = params.halfResolution;

  GET_DENOISER_TEXTURES(params, halfRes);
  auto &ao_diffTemp1 = denoised_ao;

  d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + rtao_bindless_index, denoised_ao);
  ShaderGlobal::set_int(rtao_bindless_slotVarId, bindless_range + rtao_bindless_index);

  if (!reblurSharedCb)
  {
    reblurSharedCb =
      dag::buffers::create_one_frame_cb(dag::buffers::cb_struct_reg_count<ReblurSharedConstants>(), "ReblurSharedConstantsCB");
  }

  ReblurSharedConstants reblurSharedConstants = make_reblur_shared_constants(params.hitDistParams, params.antilagSettings, false,
    width, height, params.maxStabilizedFrameNum, params.checkerboard, true, false);
  reblurSharedCb->updateData(0, sizeof(ReblurSharedConstants), &reblurSharedConstants, VBLOCK_WRITEONLY | VBLOCK_DISCARD);

  if (reset_history)
    clear_texture(denoised_ao);

  ShaderGlobal::set_int4(denoiser_resolutionVarId, width, height, 0, 0);

  {
    TIME_D3D_PROFILE(reblur::classify_tiles);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, viewZ);
    d3d::set_rwtex(STAGE_CS, 0, ao_tiles, 0, 0);

    reblurClassifyTiles->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_accumulation);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);
    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, ao_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, viewZ);
    d3d::set_tex(STAGE_CS, 3, motionVectors);
    d3d::set_tex(STAGE_CS, 4, ao_prev_view_z);
    d3d::set_tex(STAGE_CS, 5, ao_prev_normal_roughness);
    d3d::set_tex(STAGE_CS, 6, ao_prev_internal_data);
    d3d::set_tex(STAGE_CS, 7, viewZ); // DUMMY
    d3d::set_tex(STAGE_CS, 8, viewZ); // DUMMY
    d3d::set_tex(STAGE_CS, 9, rtao_tex_unfiltered);
    d3d::set_tex(STAGE_CS, 10, denoised_ao);
    d3d::set_tex(STAGE_CS, 11, ao_diff_fast_history);
    d3d::set_rwtex(STAGE_CS, 0, ao_diffTemp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, ao_diffFastHistory, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, ao_data1, 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseOcclusionTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::history_fix);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, ao_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, ao_data1);
    d3d::set_tex(STAGE_CS, 3, viewZ);
    d3d::set_tex(STAGE_CS, 4, ao_diffTemp2);
    d3d::set_tex(STAGE_CS, 5, ao_diffFastHistory);
    d3d::set_rwtex(STAGE_CS, 0, ao_diffTemp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, ao_diff_fast_history, 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseOcclusionHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, ao_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, ao_data1);
    d3d::set_tex(STAGE_CS, 3, ao_diffTemp1);
    d3d::set_tex(STAGE_CS, 4, viewZ);
    d3d::set_rwtex(STAGE_CS, 0, ao_diffTemp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, ao_prev_view_z, 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseOcclusionBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::post_blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, ao_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, ao_data1);
    d3d::set_tex(STAGE_CS, 3, ao_diffTemp2);
    d3d::set_tex(STAGE_CS, 4, ao_prev_view_z);
    d3d::set_rwtex(STAGE_CS, 0, ao_prev_normal_roughness, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, denoised_ao, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, ao_prev_internal_data, 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfPostBlurNoTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseOcclusionPostBlurNoTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  d3d::set_const_buffer(STAGE_CS, 0, nullptr);

  d3d::resource_barrier(ResourceBarrierDesc(denoised_ao, RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));
}

void denoise_gi(const GIDenoiser &params)
{
  TIME_D3D_PROFILE(denoise_gi);

  if (resolution_config.useRayReconstruction)
  {
    ACQUIRE_TEXTURE(ptgi_tex_unfiltered);
    d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + ptgi_bindless_index, ptgi_tex_unfiltered);
    ShaderGlobal::set_int(ptgi_bindless_slotVarId, bindless_range + ptgi_bindless_index);
    d3d::resource_barrier(ResourceBarrierDesc(ptgi_tex_unfiltered, RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));
    return;
  }

#define NAME(name) ACQUIRE_TEXTURE(name)
  ENUM_GI_DENOISED_NAMES
#undef NAME

  TextureInfo ti, tilesTi;
  ptgi_tex_unfiltered->getinfo(ti);
  gi_tiles->getinfo(tilesTi);

  int width = ti.w, height = ti.h, tilesWidth = tilesTi.w, tilesHeight = tilesTi.h;
  bool halfRes = params.halfResolution;

  GET_DENOISER_TEXTURES(params, halfRes);
  auto &gi_diffTemp1 = denoised_gi;

  d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + ptgi_bindless_index, denoised_gi);
  d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + ptgi_depth_bindless_index, viewZ);
  ShaderGlobal::set_int(ptgi_bindless_slotVarId, bindless_range + ptgi_bindless_index);
  ShaderGlobal::set_int(ptgi_depth_bindless_slotVarId, bindless_range + ptgi_depth_bindless_index);

  if (!reblurSharedCb)
  {
    reblurSharedCb =
      dag::buffers::create_one_frame_cb(dag::buffers::cb_struct_reg_count<ReblurSharedConstants>(), "ReblurSharedConstantsCB");
  }

  ReblurSharedConstants reblurSharedConstants = make_reblur_shared_constants(params.hitDistParams, params.antilagSettings, false,
    width, height, params.maxStabilizedFrameNum, params.checkerboard, true, false);
  reblurSharedCb->updateData(0, sizeof(ReblurSharedConstants), &reblurSharedConstants, VBLOCK_WRITEONLY | VBLOCK_DISCARD);

  if (reset_history)
    clear_texture(denoised_gi);

  ShaderGlobal::set_int4(denoiser_resolutionVarId, width, height, 0, 0);

  auto stab_ping = (frame_counter & 1) == 0 ? gi_diff_history_stabilized_ping : gi_diff_history_stabilized_pong;
  auto stab_pong = (frame_counter & 1) != 0 ? gi_diff_history_stabilized_ping : gi_diff_history_stabilized_pong;

  {
    TIME_D3D_PROFILE(reblur::classify_tiles);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, viewZ);
    d3d::set_rwtex(STAGE_CS, 0, gi_tiles, 0, 0);

    reblurClassifyTiles->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::prepass);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, gi_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, viewZ);
    d3d::set_tex(STAGE_CS, 3, ptgi_tex_unfiltered);
    d3d::set_rwtex(STAGE_CS, 0, gi_diffTemp1, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfPrepass->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffusePrepass->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_accumulation);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);
    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, gi_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, viewZ);
    d3d::set_tex(STAGE_CS, 3, motionVectors);
    d3d::set_tex(STAGE_CS, 4, gi_prev_view_z);
    d3d::set_tex(STAGE_CS, 5, gi_prev_normal_roughness);
    d3d::set_tex(STAGE_CS, 6, gi_prev_internal_data);
    d3d::set_tex(STAGE_CS, 7, viewZ); // DUMMY
    d3d::set_tex(STAGE_CS, 8, viewZ); // DUMMY
    d3d::set_tex(STAGE_CS, 9, gi_diffTemp1);
    d3d::set_tex(STAGE_CS, 10, gi_diff_history);
    d3d::set_tex(STAGE_CS, 11, gi_diff_fast_history);
    d3d::set_rwtex(STAGE_CS, 0, gi_diffTemp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, gi_diffFastHistory, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, gi_data1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, gi_data2, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::history_fix);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, gi_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, gi_data1);
    d3d::set_tex(STAGE_CS, 3, viewZ);
    d3d::set_tex(STAGE_CS, 4, gi_diffTemp2);
    d3d::set_tex(STAGE_CS, 5, gi_diffFastHistory);
    d3d::set_rwtex(STAGE_CS, 0, gi_diffTemp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, gi_diff_fast_history, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, gi_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, gi_data1);
    d3d::set_tex(STAGE_CS, 3, gi_diffTemp1);
    d3d::set_tex(STAGE_CS, 4, viewZ);
    d3d::set_rwtex(STAGE_CS, 0, gi_diffTemp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, gi_prev_view_z, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::post_blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, gi_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, gi_data1);
    d3d::set_tex(STAGE_CS, 3, gi_diffTemp2);
    d3d::set_tex(STAGE_CS, 4, gi_prev_view_z);
    d3d::set_rwtex(STAGE_CS, 0, gi_prev_normal_roughness, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, gi_diff_history, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, gi_prev_internal_data, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfPostBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffusePostBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_stabilization);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, gi_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, viewZ);
    d3d::set_tex(STAGE_CS, 3, gi_data1);
    d3d::set_tex(STAGE_CS, 4, gi_data2);
    d3d::set_tex(STAGE_CS, 5, gi_diff_history);
    d3d::set_tex(STAGE_CS, 6, stab_ping);
    d3d::set_tex(STAGE_CS, 7, motionVectors);
    d3d::set_rwtex(STAGE_CS, 0, gi_prev_internal_data, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, denoised_gi, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, stab_pong, 0, 0);

    if (params.performanceMode)
      reblurDiffusePerfTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurDiffuseTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  d3d::set_const_buffer(STAGE_CS, 0, nullptr);

  d3d::resource_barrier(ResourceBarrierDesc(denoised_gi, RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));
}

static void denoise_reflection_reblur(const ReflectionDenoiser &params)
{
  TIME_D3D_PROFILE(denoise_reflection_reblur);

#define NAME(name) ACQUIRE_TEXTURE(name)
  ENUM_RTR_DENOISED_SHARED_PERSISTENT_NAMES;
  ENUM_RTR_DENOISED_SHARED_TRANSIENT_NAMES;
  ENUM_RTR_DENOISED_REBLUR_PERSISTENT_NAMES;
  ENUM_RTR_DENOISED_REBLUR_TRANSIENT_NAMES;
#undef NAME

  TextureInfo ti, tilesTi;
  rtr_tex->getinfo(ti);
  rtr_reblur_tiles->getinfo(tilesTi);

  int width = ti.w, height = ti.h, tilesWidth = tilesTi.w, tilesHeight = tilesTi.h;
  bool halfRes = params.halfResolution;

  GET_DENOISER_TEXTURES(params, halfRes);
  auto &rtr_tmp1 = rtr_tex;

  if (!reblurSharedCb)
  {
    reblurSharedCb =
      dag::buffers::create_one_frame_cb(dag::buffers::cb_struct_reg_count<ReblurSharedConstants>(), "ReblurSharedConstantsCB");
  }

  ReblurSharedConstants reblurSharedConstants = make_reblur_shared_constants(params.hitDistParams, params.antilagSettings, false,
    width, height, params.maxStabilizedFrameNum, params.checkerboard, false, params.antiFirefly);
  reblurSharedCb->updateData(0, sizeof(ReblurSharedConstants), &reblurSharedConstants, VBLOCK_WRITEONLY | VBLOCK_DISCARD);

  auto hitd_ping = (frame_counter & 1) == 0 ? reblur_spec_hitdist_for_tracking_ping : reblur_spec_hitdist_for_tracking_pong;
  auto hitd_pong = (frame_counter & 1) != 0 ? reblur_spec_hitdist_for_tracking_ping : reblur_spec_hitdist_for_tracking_pong;

  auto stab_ping = (frame_counter & 1) == 0 ? reblur_spec_history_stabilized_ping : reblur_spec_history_stabilized_pong;
  auto stab_pong = (frame_counter & 1) != 0 ? reblur_spec_history_stabilized_ping : reblur_spec_history_stabilized_pong;

  ShaderGlobal::set_int4(denoiser_resolutionVarId, width, height, 0, 0);

  {
    TIME_D3D_PROFILE(reblur::classify_tiles);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, viewZ);
    d3d::set_rwtex(STAGE_CS, 0, rtr_reblur_tiles, 0, 0);

    reblurClassifyTiles->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::prepass);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, rtr_reblur_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, viewZ);
    d3d::set_tex(STAGE_CS, 3, rtr_tex_unfiltered);
    d3d::set_rwtex(STAGE_CS, 0, rtr_tmp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, rtr_hitdistForTracking, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfPrepass->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurSpecularPrepass->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_accumulation);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, rtr_reblur_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, viewZ);
    d3d::set_tex(STAGE_CS, 3, motionVectors);
    d3d::set_tex(STAGE_CS, 4, reblur_spec_prev_view_z);
    d3d::set_tex(STAGE_CS, 5, reblur_spec_prev_normal_roughness);
    d3d::set_tex(STAGE_CS, 6, reblur_spec_prev_internal_data);
    d3d::set_tex(STAGE_CS, 7, viewZ); // dummy
    d3d::set_tex(STAGE_CS, 8, viewZ); // dummy
    d3d::set_tex(STAGE_CS, 9, rtr_tmp1);
    d3d::set_tex(STAGE_CS, 10, reblur_spec_history);
    d3d::set_tex(STAGE_CS, 11, reblur_spec_fast_history);
    d3d::set_tex(STAGE_CS, 12, hitd_ping);
    d3d::set_tex(STAGE_CS, 13, rtr_hitdistForTracking);
    d3d::set_rwtex(STAGE_CS, 0, rtr_tmp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, rtr_fastHistory, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, hitd_pong, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, rtr_data1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 4, rtr_data2, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurSpecularTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::history_fix);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, rtr_reblur_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, rtr_data1);
    d3d::set_tex(STAGE_CS, 3, viewZ);
    d3d::set_tex(STAGE_CS, 4, rtr_tmp2);
    d3d::set_tex(STAGE_CS, 5, rtr_fastHistory);
    d3d::set_rwtex(STAGE_CS, 0, rtr_tmp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, reblur_spec_fast_history, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurSpecularHistoryFix->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, rtr_reblur_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, rtr_data1);
    d3d::set_tex(STAGE_CS, 3, rtr_tmp1);
    d3d::set_tex(STAGE_CS, 4, viewZ);
    d3d::set_rwtex(STAGE_CS, 0, rtr_tmp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, reblur_spec_prev_view_z, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurSpecularBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::post_blur);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, rtr_reblur_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, rtr_data1);
    d3d::set_tex(STAGE_CS, 3, rtr_tmp2);
    d3d::set_tex(STAGE_CS, 4, reblur_spec_prev_view_z);
    d3d::set_rwtex(STAGE_CS, 0, reblur_spec_prev_normal_roughness, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, reblur_spec_history, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfPostBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurSpecularPostBlur->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_stabilization);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, rtr_reblur_tiles);
    d3d::set_tex(STAGE_CS, 1, normalRoughness);
    d3d::set_tex(STAGE_CS, 2, viewZ); // dummy
    d3d::set_tex(STAGE_CS, 3, reblur_spec_prev_view_z);
    d3d::set_tex(STAGE_CS, 4, rtr_data1);
    d3d::set_tex(STAGE_CS, 5, rtr_data2);
    d3d::set_tex(STAGE_CS, 6, reblur_spec_history);
    d3d::set_tex(STAGE_CS, 7, stab_ping);
    d3d::set_tex(STAGE_CS, 8, hitd_pong);
    d3d::set_tex(STAGE_CS, 9, motionVectors);
    d3d::set_rwtex(STAGE_CS, 0, reblur_spec_prev_internal_data, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, rtr_tex, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, stab_pong, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
    else
      reblurSpecularTemporalStabilization->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  if (auto iter = params.textures.find(ReflectionDenoiser::TextureNames::rtr_validation); iter != params.textures.end())
  {
    TIME_D3D_PROFILE(reblur::validation);

    d3d::set_const_buffer(STAGE_CS, 0, reblurSharedCb.getBuf());
    d3d::set_tex(STAGE_CS, 0, normalRoughness);
    d3d::set_tex(STAGE_CS, 1, viewZ);
    d3d::set_tex(STAGE_CS, 2, motionVectors);
    d3d::set_tex(STAGE_CS, 3, rtr_data1);
    d3d::set_tex(STAGE_CS, 4, rtr_data2);
    d3d::set_tex(STAGE_CS, 5, rtr_tex_unfiltered);
    d3d::set_tex(STAGE_CS, 6, rtr_tex_unfiltered);
    d3d::set_rwtex(STAGE_CS, 0, iter->second.getTex2D(), 0, 0);

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

static RelaxSharedConstants make_relax_shared_constants(int width, int height, bool checkerboard, int max_stabilized_frame_num,
  int max_fast_stabilized_frame_num)
{
  float unproject = 1.0f / (0.5f * height * abs(projectY));

  float tanHalfFov = 1.0f / view_to_clip.a00;
  float aspect = view_to_clip.a00 / view_to_clip.a11;
  float3 frustumRight = float3(world_to_view.GetRow0().xyz) * tanHalfFov;
  float3 frustumUp = float3(world_to_view.GetRow1().xyz) * tanHalfFov * aspect;
  float3 frustumForward = RELAX_GetFrustumForward(view_to_world, frustum);

  float prevTanHalfFov = 1.0f / prev_view_to_clip.a00;
  float prevAspect = prev_view_to_clip.a00 / prev_view_to_clip.a11;
  float3 prevFrustumRight = float3(prev_world_to_view.GetRow0().xyz) * prevTanHalfFov;
  float3 prevFrustumUp = float3(prev_world_to_view.GetRow1().xyz) * prevTanHalfFov * prevAspect;
  float3 prevFrustumForward = RELAX_GetFrustumForward(prev_view_to_world, frustum_prev);

  float3 cameraDelta = prev_view_pos - view_pos;

  float specMaxAccumulatedFrameNum =
    reset_history ? 0 : min(max_stabilized_frame_num, ReflectionDenoiser::RELAX_MAX_HISTORY_FRAME_NUM);
  float specMaxFastAccumulatedFrameNum =
    reset_history ? 0 : min(max_fast_stabilized_frame_num, ReflectionDenoiser::RELAX_MAX_HISTORY_FRAME_NUM);
  float diffMaxAccumulatedFrameNum = reset_history ? 0 : min(30, ReflectionDenoiser::RELAX_MAX_HISTORY_FRAME_NUM);
  float diffMaxFastAccumulatedFrameNum = reset_history ? 0 : min(6, ReflectionDenoiser::RELAX_MAX_HISTORY_FRAME_NUM);

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
  relaxSharedConstants.rectSizePrev = float2(width, height);
  relaxSharedConstants.resourceSizeInvPrev = float2(1.0 / width, 1.0 / height);

  // int2
  relaxSharedConstants.printfAt = int2(0, 0);
  relaxSharedConstants.rectOrigin = int2(0, 0);
  relaxSharedConstants.rectSize = int2(width, height);

  // float
  relaxSharedConstants.specMaxAccumulatedFrameNum = specMaxAccumulatedFrameNum;
  relaxSharedConstants.specMaxFastAccumulatedFrameNum = specMaxFastAccumulatedFrameNum;
  relaxSharedConstants.diffMaxAccumulatedFrameNum = diffMaxAccumulatedFrameNum;
  relaxSharedConstants.diffMaxFastAccumulatedFrameNum = diffMaxFastAccumulatedFrameNum;
  relaxSharedConstants.disocclusionThreshold = disocclusionThreshold;
  relaxSharedConstants.disocclusionThresholdAlternate = disocclusionThresholdAlternate;
  relaxSharedConstants.cameraAttachedReflectionMaterialID = CAMERA_ATTACHED_REFLECTION_MATERIAL_ID;
  relaxSharedConstants.strandMaterialID = 999;
  relaxSharedConstants.strandThickness = 80e-6f;
  relaxSharedConstants.roughnessFraction = 0.15;
  relaxSharedConstants.specVarianceBoost = 0;
  relaxSharedConstants.splitScreen = 0;
  relaxSharedConstants.diffBlurRadius = 30;
  relaxSharedConstants.specBlurRadius = 50;
  relaxSharedConstants.depthThreshold = 0.003;
  relaxSharedConstants.lobeAngleFraction = 0.5;
  relaxSharedConstants.specLobeAngleSlack = DegToRad(0.15);
  relaxSharedConstants.historyFixEdgeStoppingNormalPower = 8;
  relaxSharedConstants.roughnessEdgeStoppingRelaxation = 1;
  relaxSharedConstants.normalEdgeStoppingRelaxation = 0.3;
  relaxSharedConstants.colorBoxSigmaScale = 2;
  relaxSharedConstants.historyAccelerationAmount = 0.3;
  relaxSharedConstants.historyResetTemporalSigmaScale = 0.5;
  relaxSharedConstants.historyResetSpatialSigmaScale = 4.5;
  relaxSharedConstants.historyResetAmount = 0.5;
  relaxSharedConstants.denoisingRange = 100000;
  relaxSharedConstants.specPhiLuminance = 1;
  relaxSharedConstants.diffPhiLuminance = 2;
  relaxSharedConstants.diffMaxLuminanceRelativeDifference = maxDiffuseLuminanceRelativeDifference;
  relaxSharedConstants.specMaxLuminanceRelativeDifference = maxSpecularLuminanceRelativeDifference;
  relaxSharedConstants.luminanceEdgeStoppingRelaxation = 1;
  relaxSharedConstants.confidenceDrivenRelaxationMultiplier = 0;
  relaxSharedConstants.confidenceDrivenLuminanceEdgeStoppingRelaxation = 0;
  relaxSharedConstants.confidenceDrivenNormalEdgeStoppingRelaxation = 0;
  relaxSharedConstants.debug = 0;
  relaxSharedConstants.OrthoMode = 0;
  relaxSharedConstants.unproject = unproject;
  relaxSharedConstants.framerateScale = frame_rate_scale_relax;
  relaxSharedConstants.checkerboardResolveAccumSpeed = checkerboard_resolve_accum_speed;
  relaxSharedConstants.jitterDelta = jitter_delta;
  relaxSharedConstants.historyFixFrameNum = 4;
  relaxSharedConstants.historyFixBasePixelStride = 14;
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

static void denoise_reflection_relax(const ReflectionDenoiser &params)
{
  TIME_D3D_PROFILE(denoise_reflection_relax);

#define NAME(name) ACQUIRE_TEXTURE(name)
  ENUM_RTR_DENOISED_SHARED_PERSISTENT_NAMES;
  ENUM_RTR_DENOISED_SHARED_TRANSIENT_NAMES;
  ENUM_RTR_DENOISED_RELAX_PERSISTENT_NAMES;
  ENUM_RTR_DENOISED_RELAX_TRANSIENT_NAMES;
#undef NAME

  TextureInfo ti, tilesTi;
  rtr_tex->getinfo(ti);
  rtr_relax_tiles->getinfo(tilesTi);

  int width = ti.w, height = ti.h, tilesWidth = tilesTi.w, tilesHeight = tilesTi.h;
  bool halfRes = params.halfResolution;

  GET_DENOISER_TEXTURES(params, halfRes);

  RelaxSharedConstants relaxSharedConstants =
    make_relax_shared_constants(width, height, params.checkerboard, params.maxStabilizedFrameNum, params.maxFastStabilizedFrameNum);

  ShaderGlobal::set_int4(denoiser_resolutionVarId, width, height, 0, 0);

  {
    TIME_D3D_PROFILE(relax::classify_tiles);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, viewZ);
    d3d::set_rwtex(STAGE_CS, 0, rtr_relax_tiles, 0, 0);

    relaxClassifyTiles->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(relax::prepass);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, rtr_relax_tiles);
    d3d::set_tex(STAGE_CS, 1, rtr_tex_unfiltered);
    d3d::set_tex(STAGE_CS, 2, normalRoughness);
    d3d::set_tex(STAGE_CS, 3, viewZ);
    d3d::set_rwtex(STAGE_CS, 0, rtr_tex, 0, 0);

    relaxSpecularPrepass->dispatch(tilesWidth, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(relax::temporal_accumulation);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, rtr_relax_tiles);
    d3d::set_tex(STAGE_CS, 1, rtr_tex);
    d3d::set_tex(STAGE_CS, 2, motionVectors);
    d3d::set_tex(STAGE_CS, 3, normalRoughness);
    d3d::set_tex(STAGE_CS, 4, viewZ);
    d3d::set_tex(STAGE_CS, 5, relax_spec_illum_responsive_prev);
    d3d::set_tex(STAGE_CS, 6, relax_spec_illum_prev);
    d3d::set_tex(STAGE_CS, 7, relax_spec_normal_roughness_prev);
    d3d::set_tex(STAGE_CS, 8, relax_spec_view_z_prev);
    if ((frame_counter & 1) == 0)
      d3d::set_tex(STAGE_CS, 9, relax_spec_reflection_hit_t_prev);
    else
      d3d::set_tex(STAGE_CS, 9, relax_spec_reflection_hit_t_curr);
    d3d::set_tex(STAGE_CS, 10, relax_spec_history_length_prev);
    d3d::set_tex(STAGE_CS, 11, relax_spec_material_id_prev);
    d3d::set_tex(STAGE_CS, 12, viewZ); // DUMMY
    d3d::set_tex(STAGE_CS, 13, viewZ); // DUMMY
    d3d::set_rwtex(STAGE_CS, 0, rtr_specIllumPing, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, rtr_specIllumPong, 0, 0);
    if ((frame_counter & 1) == 0)
      d3d::set_rwtex(STAGE_CS, 2, relax_spec_reflection_hit_t_curr, 0, 0);
    else
      d3d::set_rwtex(STAGE_CS, 2, relax_spec_reflection_hit_t_prev, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, rtr_historyLength, 0, 0);
    d3d::set_rwtex(STAGE_CS, 4, rtr_specReprojectionConfidence, 0, 0);

    relaxSpecularTemporalAccumulation->dispatch(tilesWidth * 2, tilesHeight, 1);
  }

  {
    TIME_D3D_PROFILE(relax::history_fix);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, rtr_relax_tiles);
    d3d::set_tex(STAGE_CS, 1, rtr_specIllumPing);
    d3d::set_tex(STAGE_CS, 2, rtr_historyLength);
    d3d::set_tex(STAGE_CS, 3, normalRoughness);
    d3d::set_tex(STAGE_CS, 4, viewZ);
    d3d::set_rwtex(STAGE_CS, 0, rtr_specIllumPong, 0, 0);

    relaxSpecularHistoryFix->dispatch(tilesWidth * 2, tilesHeight * 2, 1);
  }

  {
    TIME_D3D_PROFILE(relax::history_clamping);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, rtr_relax_tiles);
    d3d::set_tex(STAGE_CS, 1, viewZ);
    d3d::set_tex(STAGE_CS, 2, rtr_tex);
    d3d::set_tex(STAGE_CS, 3, rtr_specIllumPing);
    d3d::set_tex(STAGE_CS, 4, rtr_specIllumPong);
    d3d::set_tex(STAGE_CS, 5, rtr_historyLength);
    d3d::set_rwtex(STAGE_CS, 0, relax_spec_illum_prev, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, relax_spec_illum_responsive_prev, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, relax_spec_history_length_prev, 0, 0);

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
    d3d::set_tex(STAGE_CS, 0, rtr_relax_tiles);
    if (isSmem)
      d3d::set_tex(STAGE_CS, 1, relax_spec_illum_prev);
    else if (isEven)
      d3d::set_tex(STAGE_CS, 1, rtr_specIllumPong);
    else
      d3d::set_tex(STAGE_CS, 1, rtr_specIllumPing);
    d3d::set_tex(STAGE_CS, 2, rtr_historyLength);
    d3d::set_tex(STAGE_CS, 3, rtr_specReprojectionConfidence);
    d3d::set_tex(STAGE_CS, 4, normalRoughness);
    d3d::set_tex(STAGE_CS, 5, viewZ);
    d3d::set_tex(STAGE_CS, 6, viewZ); // hasConfidenceInputs

    if (isLast)
      d3d::set_rwtex(STAGE_CS, 0, rtr_tex, 0, 0);
    else if (isEven)
      d3d::set_rwtex(STAGE_CS, 0, rtr_specIllumPing, 0, 0);
    else
      d3d::set_rwtex(STAGE_CS, 0, rtr_specIllumPong, 0, 0);

    if (isSmem)
    {
      d3d::set_rwtex(STAGE_CS, 1, relax_spec_normal_roughness_prev, 0, 0);
      d3d::set_rwtex(STAGE_CS, 2, relax_spec_material_id_prev, 0, 0);
      d3d::set_rwtex(STAGE_CS, 3, relax_spec_view_z_prev, 0, 0);
    }

    if (isSmem)
      relaxSpecularATorusSmem->dispatch(tilesWidth * 2, tilesHeight * 2, 1);
    else
      relaxSpecularATorus->dispatch(tilesWidth, tilesHeight, 1);
  }

  if (auto iter = params.textures.find(ReflectionDenoiser::TextureNames::rtr_validation); iter != params.textures.end())
  {
    TIME_D3D_PROFILE(relax::validation);

    d3d::set_cb0_data(STAGE_CS, (const float *)&relaxSharedConstants, divide_up(sizeof(relaxSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, normalRoughness);
    d3d::set_tex(STAGE_CS, 1, viewZ);
    d3d::set_tex(STAGE_CS, 2, motionVectors);
    d3d::set_tex(STAGE_CS, 3, rtr_historyLength);
    d3d::set_rwtex(STAGE_CS, 0, iter->second.getTex2D(), 0, 0);

    relaxValidation->dispatch(tilesWidth * 2, tilesHeight, 1);
  }
}

void denoise_reflection(const ReflectionDenoiser &params)
{
  if (resolution_config.useRayReconstruction)
  {
    ACQUIRE_TEXTURE(rtr_tex_unfiltered);
    d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + rtr_bindless_index, rtr_tex_unfiltered);
    ShaderGlobal::set_int(rtr_bindless_slotVarId, bindless_range + rtr_bindless_index);
    d3d::resource_barrier(ResourceBarrierDesc(rtr_tex_unfiltered, RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));
    return;
  }

  ACQUIRE_TEXTURE(rtr_tex);

  d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + rtr_bindless_index, rtr_tex);
  ShaderGlobal::set_int(rtr_bindless_slotVarId, bindless_range + rtr_bindless_index);

  if (reset_history)
    clear_texture(rtr_tex);

  switch (params.method)
  {
    case ReflectionMethod::Reblur: denoise_reflection_reblur(params); break;
    case ReflectionMethod::Relax: denoise_reflection_relax(params); break;
    default: G_ASSERTF(false, "Reflection denoiser not implemented!");
  }

  d3d::resource_barrier(ResourceBarrierDesc(rtr_tex, RB_STAGE_ALL_SHADERS | RB_RO_SRV, 0, 0));
}

int get_frame_number() { return frame_counter; }

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
