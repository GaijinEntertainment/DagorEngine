// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/denoiser.h>
#include <shaders/dag_computeShaders.h>
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

#if ENABLE_NRD_INTEGRATION_TEST
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"

#include "NRI/Include/NRI.h"
#include "NRI/Include/Extensions/NRIWrapperD3D12.h"
#include "NRI/Include/Extensions/NRIHelper.h"
#include "NRD/Include/NRD.h"
#include "NRD/Integration/NRDIntegration.hpp"

struct ID3D12GraphicsCommandList;
struct ID3D12CommandAllocator;
struct ID3D12Resource;

using ResCB = eastl::function<ID3D12Resource *(Texture *texture)>;
using NRDCB = eastl::function<void(ID3D12GraphicsCommandList *commandList, ID3D12CommandAllocator *commandAllocator, ResCB res)>;

static constexpr uint32_t ReflectionId = 3;

static struct NriInterface : public nri::CoreInterface, public nri::HelperInterface, public nri::WrapperD3D12Interface
{
} nriInterface;

static eastl::unique_ptr<NrdIntegration> nrdInterface;
static nri::Device *nriDevice;

#endif

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

static ComputeShaderElement *reblurSpecularPrepass = nullptr;
static ComputeShaderElement *reblurSpecularTemporalAccumulation = nullptr;
static ComputeShaderElement *reblurSpecularHistoryFix = nullptr;
static ComputeShaderElement *reblurSpecularBlur = nullptr;
static ComputeShaderElement *reblurSpecularPostBlur = nullptr;
static ComputeShaderElement *reblurSpecularCopy = nullptr;
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

static UniqueTex normal_roughness;
static UniqueTex view_z;
static UniqueTex half_normal_roughness;
static UniqueTex half_view_z;
static BaseTexture *half_motion_vectors = nullptr;

static UniqueTex ao_prev_view_z;
static UniqueTex ao_prev_normal_roughness;
static UniqueTex ao_prev_internal_data;
static UniqueTex ao_diff_fast_history;

static UniqueTex reblur_spec_history_confidence;
static UniqueTex reblur_spec_prev_view_z;
static UniqueTex reblur_spec_prev_normal_roughness;
static UniqueTex reblur_spec_prev_internal_data;
static UniqueTex reblur_spec_history;
static UniqueTex reblur_spec_fast_history;
static UniqueTex reblur_spec_hitdist_for_tracking_ping;
static UniqueTex reblur_spec_hitdist_for_tracking_pong;

static UniqueTex relax_spec_illum_prev;
static UniqueTex relax_spec_illum_responsive_prev;
static UniqueTex relax_spec_reflection_hit_t_curr;
static UniqueTex relax_spec_reflection_hit_t_prev;
static UniqueTex relax_spec_history_length_prev;
static UniqueTex relax_spec_normal_roughness_prev;
static UniqueTex relax_spec_material_id_prev;
static UniqueTex relax_spec_prev_view_z;

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
static int rtsm_is_translucentVarId = -1;
static int csm_bindless_slotVarId = -1;
static int csm_sampler_bindless_slotVarId = -1;
static int rtr_bindless_slotVarId = -1;

static int bindless_range = -1;

static const int rtao_bindless_index = 0;
static const int rtsm_bindless_index = 1;
static const int csm_bindless_index = 2;
static const int rtr_bindless_index = 3;

struct TransientKey
{
  int width;
  int height;
  unsigned int flags;

  bool operator==(const TransientKey &other) const { return width == other.width && height == other.height && flags == other.flags; }
};

struct TransientKeyHash
{
  uint64_t operator()(const TransientKey &key) const { return (uint64_t(key.width) << 16) | key.height | (uint64_t(key.flags) << 32); }
};

static eastl::unordered_map<TransientKey, eastl::vector<UniqueTex>, TransientKeyHash> transient_textures;

static int render_width = 0;
static int render_height = 0;

static int shadow_width = 0;
static int shadow_height = 0;

static int reflection_width = 0;
static int reflection_height = 0;

static int ao_width = 0;
static int ao_height = 0;

static int frame_counter = 0;

static TMatrix4 world_to_view;
static TMatrix4 prev_world_to_view;
static TMatrix4 view_to_clip;
static TMatrix4 prev_view_to_clip;
static TMatrix4 prev_world_to_clip;
static TMatrix4 view_to_world;
static TMatrix4 prev_view_to_world;

static TMatrix4 prev_world_to_view_hs;
static TMatrix4 prev_world_to_clip_hs;

static Point3 view_pos;
static Point3 prev_view_pos;

static Point3 view_dir;
static Point3 prev_view_dir;

static Point2 jitter;
static Point2 prev_jitter;

static Point3 motion_multiplier;

static BaseTexture *motion_vectors = nullptr;

static int max_accumulated_frame_num = 30;
static int max_fast_accumulated_frame_num = 6;

static float checkerboard_resolve_accum_speed = 1;
static float frame_rate_scale = 1;
static float time_delta_ms = 1;
static float smooth_time_delta_ms = 1;

static float jitter_delta = 0;

static bool reset_history = false;

struct SigmaSharedConstants
{
  TMatrix4 worldToView;
  TMatrix4 viewToClip;
  TMatrix4 worldToClipPrev;
  Point4 frustum;
  Point4 mvScale;
  Point2 resourceSizeInv;
  Point2 resourceSizeInvPrev;
  Point2 rectSize;
  Point2 rectSizeInv;
  Point2 rectSizePrev;
  Point2 resolutionScale;
  Point2 rectOffset;
  IPoint2 printfAt;
  IPoint2 rectOrigin;
  IPoint2 rectSizeMinusOne;
  IPoint2 tilesSizeMinusOne;
  float orthoMode;
  float unproject;
  float denoisingRange;
  float planeDistSensitivity;
  float blurRadiusScale;
  float continueAccumulation;
  float debug;
  float splitScreen;
  uint32_t frameIndex;
  uint32_t padding;
  Point4 rotator;
};

struct ReblurSharedConstants
{
  TMatrix4 viewToClip;
  TMatrix4 viewToWorld;
  TMatrix4 worldToViewPrev;
  TMatrix4 worldToClipPrev;
  TMatrix4 worldPrevToWorld;
  Point4 frustum;
  Point4 frustumPrev;
  Point4 cameraDelta;
  Point4 antilagParams;
  Point4 hitDistParams;
  Point4 viewVectorWorld;
  Point4 viewVectorWorldPrev;
  Point4 mvScale;
  Point2 resourceSize;
  Point2 resourceSizeInv;
  Point2 resourceSizeInvPrev;
  Point2 rectSize;
  Point2 rectSizeInv;
  Point2 rectSizePrev;
  Point2 resolutionScale;
  Point2 resolutionScalePrev;
  Point2 rectOffset;
  Point2 specProbabilityThresholdsForMvModification;
  Point2 jitter;
  IPoint2 printfAt;
  IPoint2 rectOrigin;
  IPoint2 rectSizeMinusOne;
  float disocclusionThreshold;
  float disocclusionThresholdAlternate;
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
  float minRectDimMulUnproject;
  float usePrepassNotOnlyForSpecularMotionEstimation;
  float splitScreen;
  float checkerboardResolveAccumSpeed;
  uint32_t hasHistoryConfidence;
  uint32_t hasDisocclusionThresholdMix;
  uint32_t diffCheckerboard;
  uint32_t specCheckerboard;
  uint32_t frameIndex;
  uint32_t diffMaterialMask;
  uint32_t specMaterialMask;
  uint32_t isRectChanged;
  uint32_t resetHistory;
  uint32_t padding1;
  uint32_t padding2;
  uint32_t padding3;
  Point4 rotator;
};

struct RelaxSharedConstants
{
  TMatrix4 worldToClipPrev;
  TMatrix4 worldToViewPrev;
  TMatrix4 worldPrevToWorld;
  Point4 frustumRight;
  Point4 frustumUp;
  Point4 frustumForward;
  Point4 prevFrustumRight;
  Point4 prevFrustumUp;
  Point4 prevFrustumForward;
  Point4 cameraDelta;
  Point4 mvScale;
  Point2 jitter;
  Point2 resolutionScale;
  Point2 rectOffset;
  Point2 resourceSizeInv;
  Point2 resourceSize;
  Point2 rectSizeInv;
  Point2 rectSizePrev;
  Point2 resourceSizeInvPrev;
  IPoint2 printfAt;
  IPoint2 rectOrigin;
  IPoint2 rectSize;
  float specMaxAccumulatedFrameNum;
  float specMaxFastAccumulatedFrameNum;
  float diffMaxAccumulatedFrameNum;
  float diffMaxFastAccumulatedFrameNum;
  float disocclusionDepthThreshold;
  float disocclusionDepthThresholdAlternate;
  float roughnessFraction;
  float specVarianceBoost;
  float splitScreen;
  float diffBlurRadius;
  float specBlurRadius;
  float depthThreshold;
  float diffLobeAngleFraction;
  float specLobeAngleFraction;
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
  float historyThreshold;
  uint32_t roughnessEdgeStoppingEnabled;
  uint32_t frameIndex;
  uint32_t diffCheckerboard;
  uint32_t specCheckerboard;
  uint32_t useConfidenceInputs;
  uint32_t useDisocclusionThresholdMix;
  uint32_t diffMaterialMask;
  uint32_t specMaterialMask;
  uint32_t resetHistory;
};

static int divide_up(int x, int y) { return (x + y - 1) / y; }

void initialize(int w, int h)
{
  if (render_width == w && render_height == h)
    return;

  render_width = shadow_width = w;
  render_height = shadow_height = h;

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
  if (!reblurSpecularCopy)
    reblurSpecularCopy = new_compute_shader("nrd_reblur_specular_copy");
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

  normal_roughness.close();
  normal_roughness =
    dag::create_tex(nullptr, render_width, render_height, TEXCF_UNORDERED | TEXFMT_A2R10G10B10, 1, "denoiser_normal_roughness");

  view_z.close();
  view_z = dag::create_tex(nullptr, render_width, render_height, TEXCF_UNORDERED | TEXFMT_R32F, 1, "denoiser_view_z");

  half_normal_roughness.close();
  half_normal_roughness =
    dag::create_tex(nullptr, render_width / 2, render_height / 2, TEXCF_UNORDERED | TEXFMT_A2R10G10B10, 1, // TODO: Ceil division.
      "denoiser_half_normal_roughness");

  half_view_z.close();
  half_view_z = dag::create_tex(nullptr, render_width / 2, render_height / 2, TEXCF_UNORDERED | TEXFMT_R32F, 1, // TODO: Ceil division.
    "denoiser_half_view_z");

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
  rtsm_is_translucentVarId = get_shader_variable_id("rtsm_is_translucent");
  csm_bindless_slotVarId = get_shader_variable_id("csm_bindless_slot", true);
  csm_sampler_bindless_slotVarId = get_shader_variable_id("csm_sampler_bindless_slot", true);
  rtr_bindless_slotVarId = get_shader_variable_id("rtr_bindless_slot");

  ShaderGlobal::set_int4(denoiser_resolutionVarId, render_width, render_height, 0, 0);

  bindless_range = d3d::allocate_bindless_resource_range(RES3D_TEX, 4);
}

#if ENABLE_NRD_INTEGRATION_TEST
static void teardown_nrd()
{
  if (!nrdInterface)
    return;

  nrdInterface->Destroy();
  nrdInterface.reset();
  nri::nriDestroyDevice(*nriDevice);
  nriDevice = nullptr;
}
#endif

static void close_ao_textures()
{
  ao_prev_view_z.close();
  ao_prev_normal_roughness.close();
  ao_prev_internal_data.close();
  ao_diff_fast_history.close();
}

static void close_reflection_textures()
{
  reblur_spec_history_confidence.close();
  reblur_spec_prev_view_z.close();
  reblur_spec_prev_normal_roughness.close();
  reblur_spec_prev_internal_data.close();
  reblur_spec_history.close();
  reblur_spec_fast_history.close();
  reblur_spec_hitdist_for_tracking_ping.close();
  reblur_spec_hitdist_for_tracking_pong.close();
  transient_textures.clear();

  relax_spec_illum_prev.close();
  relax_spec_illum_responsive_prev.close();
  relax_spec_reflection_hit_t_curr.close();
  relax_spec_reflection_hit_t_prev.close();
  relax_spec_history_length_prev.close();
  relax_spec_normal_roughness_prev.close();
  relax_spec_material_id_prev.close();
  relax_spec_prev_view_z.close();
}

static void clear_texture(Texture *tex)
{
  if (!tex)
    return;

  TextureInfo ti;
  tex->getinfo(ti);

  uint32_t zeroI[] = {0, 0, 0, 0};
  float zeroF[] = {0, 0, 0, 0};

  auto &desc = get_tex_format_desc(ti.cflg);
  if (desc.mainChannelsType == ChannelDType::SFLOAT || desc.mainChannelsType == ChannelDType::UFLOAT)
    d3d::clear_rwtexf(tex, zeroF, 0, 0);
  else
    d3d::clear_rwtexi(tex, zeroI, 0, 0);
};

static void clear_texture(UniqueTex &tex)
{
  if (tex)
    clear_texture(tex.getTex2D());
}

static void clear_history_textures()
{
  clear_texture(ao_prev_view_z);
  clear_texture(ao_prev_normal_roughness);
  clear_texture(ao_prev_internal_data);
  clear_texture(ao_diff_fast_history);
  clear_texture(reblur_spec_history_confidence);
  clear_texture(reblur_spec_prev_view_z);
  clear_texture(reblur_spec_prev_normal_roughness);
  clear_texture(reblur_spec_prev_internal_data);
  clear_texture(reblur_spec_history);
  clear_texture(reblur_spec_fast_history);
  clear_texture(reblur_spec_hitdist_for_tracking_ping);
  clear_texture(reblur_spec_hitdist_for_tracking_pong);
  clear_texture(relax_spec_illum_prev);
  clear_texture(relax_spec_illum_responsive_prev);
  clear_texture(relax_spec_reflection_hit_t_curr);
  clear_texture(relax_spec_reflection_hit_t_prev);
  clear_texture(relax_spec_history_length_prev);
  clear_texture(relax_spec_normal_roughness_prev);
  clear_texture(relax_spec_material_id_prev);
  clear_texture(relax_spec_prev_view_z);

  for (auto &kv : transient_textures)
    for (auto &tex : kv.second)
      clear_texture(tex);
}

static void do_make_shadow_maps(UniqueTex &shadow_value, UniqueTex &denoised_shadow, bool translucent)
{
  shadow_value = dag::create_tex(nullptr, shadow_width, shadow_height, TEXCF_UNORDERED | TEXFMT_G16R16F, 1, "rtsm_value");
  denoised_shadow = dag::create_tex(nullptr, shadow_width, shadow_height,
    TEXCF_UNORDERED | TEXCF_RTARGET | (translucent ? TEXFMT_A8R8G8B8 : TEXFMT_R8), 1, "rtsm_shadows_denoised");

  transient_textures.clear();
  clear_history_textures();
  clear_texture(denoised_shadow);
}

void make_shadow_maps(UniqueTex &shadow_value, UniqueTex &denoised_shadow)
{
  do_make_shadow_maps(shadow_value, denoised_shadow, false);
}

void make_shadow_maps(UniqueTex &shadow_value, UniqueTex &shadow_translucency, UniqueTex &denoised_shadow)
{
  do_make_shadow_maps(shadow_value, denoised_shadow, true);
  shadow_translucency =
    dag::create_tex(nullptr, shadow_width, shadow_height, TEXCF_UNORDERED | TEXFMT_A8R8G8B8, 1, "rtsm_translucency");

  clear_texture(shadow_translucency);
}

void make_ao_maps(UniqueTex &ao_value, UniqueTex &denoised_ao, bool half_res)
{
  int width = denoiser::ao_width = half_res ? denoiser::render_width / 2 : denoiser::render_width;     // TODO: Ceil division.
  int height = denoiser::ao_height = half_res ? denoiser::render_height / 2 : denoiser::render_height; // TODO: Ceil division.
  ao_value = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_L16, 1, "rtao_tex_unfiltered");
  denoised_ao = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_L16, 1, "rtao_tex");

  close_ao_textures();
  transient_textures.clear();

  ao_prev_view_z.close();
  ao_prev_view_z = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R32F, 1, "ao_prev_view_z");
  ao_prev_normal_roughness.close();
  ao_prev_normal_roughness = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_A8R8G8B8, 1, "ao_prev_normal_roughness");
  ao_prev_internal_data.close();
  ao_prev_internal_data = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R16UI, 1, "ao_prev_internal_data");
  ao_diff_fast_history.close();
  ao_diff_fast_history = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_L16, 1, "ao_diff_fast_history");

  clear_history_textures();
  clear_texture(denoised_ao);
}

void make_reflection_maps(UniqueTex &reflection_value, UniqueTex &denoised_reflection, ReflectionMethod method, bool half_res)
{
  int width = denoiser::reflection_width = half_res ? denoiser::render_width / 2 : denoiser::render_width;     // TODO: Ceil division.
  int height = denoiser::reflection_height = half_res ? denoiser::render_height / 2 : denoiser::render_height; // TODO: Ceil division.
  reflection_value = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "rtr_tex_unfiltered");
  denoised_reflection = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "rtr_tex");

  close_reflection_textures();
  transient_textures.clear();

  if (method == ReflectionMethod::Reblur)
  {
    reblur_spec_history_confidence.close();
    reblur_spec_history_confidence =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R8, 1, "reblur_spec_history_confidence");
    reblur_spec_prev_view_z.close();
    reblur_spec_prev_view_z = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R32F, 1, "reblur_spec_prev_view_z");
    reblur_spec_prev_normal_roughness.close();
    reblur_spec_prev_normal_roughness =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_A8R8G8B8, 1, "reblur_spec_prev_normal_roughness");
    reblur_spec_prev_internal_data.close();
    reblur_spec_prev_internal_data =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R16UI, 1, "reblur_spec_prev_internal_data");
    reblur_spec_history.close();
    reblur_spec_history = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "reblur_spec_history");
    reblur_spec_fast_history.close();
    reblur_spec_fast_history = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R16F, 1, "reblur_spec_fast_history");
    reblur_spec_hitdist_for_tracking_ping.close();
    reblur_spec_hitdist_for_tracking_ping =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R16F, 1, "reblur_spec_hitdist_for_tracking_ping");
    reblur_spec_hitdist_for_tracking_pong.close();
    reblur_spec_hitdist_for_tracking_pong =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R16F, 1, "reblur_spec_hitdist_for_tracking_pong");
  }
  else if (method == ReflectionMethod::Relax)
  {
    relax_spec_illum_prev =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "relax_spec_self_illum_prev");
    relax_spec_illum_responsive_prev =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "relax_spec_self_illum_responsive_prev");
    relax_spec_reflection_hit_t_curr =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R16F, 1, "relax_spec_reflection_hit_t_curr");
    relax_spec_reflection_hit_t_prev =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R16F, 1, "relax_spec_reflection_hit_t_prev");
    relax_spec_history_length_prev =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R8, 1, "relax_spec_history_length_prev");
    relax_spec_normal_roughness_prev =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_A8R8G8B8, 1, "relax_spec_normal_roughness_prev");
    relax_spec_material_id_prev =
      dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R8, 1, "relax_spec_material_id_prev");
    relax_spec_prev_view_z = dag::create_tex(nullptr, width, height, TEXCF_UNORDERED | TEXFMT_R32F, 1, "relax_spec_view_z_prev");
  }
  else
    G_ASSERTF(false, "Reflection denoiser not implemented!");

  clear_history_textures();
  clear_texture(denoised_reflection);

#if ENABLE_NRD_INTEGRATION_TEST
  //////////////////////////////////
  // NRD
  if (width > 0 && height > 0)
  {
    teardown_nrd();

    nri::DeviceCreationD3D12Desc deviceDesc = {};
    deviceDesc.enableNRIValidation = false;
    deviceDesc.d3d12Device = (ID3D12Device *)d3d::get_device();
    d3d::driver_command(Drv3dCommand::GET_RENDERING_COMMAND_QUEUE, (void *)&deviceDesc.d3d12GraphicsQueue);

    auto nriResult = nri::nriCreateDeviceFromD3D12Device(deviceDesc, nriDevice);
    assert(nriResult == nri::Result::SUCCESS);
    nriResult = nriGetInterface(*nriDevice, NRI_INTERFACE(nri::CoreInterface), (nri::CoreInterface *)&nriInterface);
    assert(nriResult == nri::Result::SUCCESS);
    nriResult = nriGetInterface(*nriDevice, NRI_INTERFACE(nri::HelperInterface), (nri::HelperInterface *)&nriInterface);
    assert(nriResult == nri::Result::SUCCESS);
    nriResult = nriGetInterface(*nriDevice, NRI_INTERFACE(nri::WrapperD3D12Interface), (nri::WrapperD3D12Interface *)&nriInterface);
    assert(nriResult == nri::Result::SUCCESS);

    const nrd::DenoiserDesc denoiserDescs[] = {
      {ReflectionId, method == ReflectionMethod::Reblur ? nrd::Denoiser::REBLUR_SPECULAR : nrd::Denoiser::RELAX_SPECULAR},
    };

    nrd::InstanceCreationDesc instanceCreationDesc = {};
    instanceCreationDesc.denoisers = denoiserDescs;
    instanceCreationDesc.denoisersNum = _countof(denoiserDescs);

    nrdInterface = eastl::make_unique<NrdIntegration>(2, true);
    auto nrdResult = nrdInterface->Initialize(width, height, instanceCreationDesc, *nriDevice, nriInterface, nriInterface);
    assert(nrdResult);
  }
#endif
}

template <typename T>
static void safe_delete(T *&ptr)
{
  if (ptr)
  {
    delete ptr;
    ptr = nullptr;
  }
}

void teardown()
{
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
  safe_delete(reblurSpecularPrepass);
  safe_delete(reblurSpecularTemporalAccumulation);
  safe_delete(reblurSpecularHistoryFix);
  safe_delete(reblurSpecularBlur);
  safe_delete(reblurSpecularPostBlur);
  safe_delete(reblurSpecularCopy);
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

  normal_roughness.close();
  view_z.close();
  half_view_z.close();
  half_normal_roughness.close();

  close_ao_textures();
  close_reflection_textures();

  transient_textures.clear();

  render_width = render_height = shadow_width = shadow_height = reflection_width = reflection_height = ao_width = ao_height =
    frame_counter = 0;

  if (bindless_range > 0)
  {
    d3d::free_bindless_resource_range(RES3D_TEX, bindless_range, 4);
    bindless_range = -1;
  }

#if ENABLE_NRD_INTEGRATION_TEST
  teardown_nrd();
#endif
}

static void make_textures(TransientKey key, int count)
{
  static int nameGen = 0;

  auto iter = transient_textures.insert(key);
  auto &bucket = iter.first->second;

  bucket.reserve(count);
  for (int i = bucket.size(); i < count; ++i)
  {
    String name(32, "denoiser_transient_%d", nameGen++);
    bucket.emplace_back(dag::create_tex(nullptr, key.width, key.height, key.flags, 1, name));
    clear_texture(bucket.back());
  }
}

void prepare(const FrameParams &params)
{
  frame_counter++;

  if (!normal_roughness)
    return;

  TIME_D3D_PROFILE(denoiser::prepare);

  reset_history = params.reset;

  if (reset_history)
    clear_history_textures();

  // This part is mandatory needed to preserve precision by making matrices camera relative
  Point3 translationDelta = params.prevViewPos - params.viewPos;

  prev_view_to_world = params.prevViewItm;
  view_to_world = params.viewItm;
  auto prevViewItm = params.prevViewItm;

  view_to_world.setrow(3, Point4(0, 0, 0, 1));

  prevViewItm.setrow(3, Point4(0, 0, 0, 1));
  prev_world_to_view_hs = inverse44(prevViewItm);

  prevViewItm.setrow(3, Point4::xyz1(translationDelta));

  world_to_view = inverse44(view_to_world);
  prev_world_to_view = inverse44(prevViewItm);
  view_to_clip = params.projTm;
  prev_view_to_clip = params.prevProjTm;

  prev_world_to_clip = prev_world_to_view * prev_view_to_clip;
  prev_world_to_clip_hs = prev_world_to_view_hs * prev_view_to_clip;

  view_pos = params.viewPos;
  prev_view_pos = params.prevViewPos;

  view_dir = -params.viewDir;
  prev_view_dir = -params.prevViewDir;

  jitter = params.jitter;
  prev_jitter = params.prevJitter;

  motion_multiplier = params.motionMultiplier;

  motion_vectors = params.motionVectors;

  half_motion_vectors = params.halfMotionVectors;

  Point2 scaledJitter = Point2(jitter.x * render_width, jitter.y * render_height);
  Point2 scaledPrevJitter = Point2(prev_jitter.x * render_width, prev_jitter.y * render_height);

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

  ShaderGlobal::set_int(denoiser_spec_confidence_half_resVarId, denoiser::reflection_width < denoiser::render_width ? 1 : 0);
  ShaderGlobal::set_int4(denoiser_resolutionVarId, render_width, render_height, 0, 0);
  ShaderGlobal::set_texture(denoiser_view_zVarId, view_z);
  ShaderGlobal::set_texture(denoiser_nrVarId, normal_roughness);
  ShaderGlobal::set_texture(denoiser_spec_history_confidenceVarId, reblur_spec_history_confidence);
  if ((denoiser::ao_width > 0 && denoiser::ao_width < denoiser::render_width) ||
      (denoiser::reflection_width > 0 && denoiser::reflection_width < denoiser::render_width))
  {
    ShaderGlobal::set_int(denoiser_half_resVarId, 1);
    ShaderGlobal::set_texture(denoiser_half_view_zVarId, half_view_z);
    ShaderGlobal::set_texture(denoiser_half_normalsVarId, params.halfNormals.getId());
    ShaderGlobal::set_texture(denoiser_half_nrVarId, half_normal_roughness.getTexId());
  }
  else
  {
    ShaderGlobal::set_int(denoiser_half_resVarId, 0);
    ShaderGlobal::set_texture(denoiser_half_view_zVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(denoiser_half_normalsVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(denoiser_half_nrVarId, BAD_TEXTUREID);
  }
  prepareCS->dispatchThreads(render_width, render_height, 1);
}

static float randuf() { return rand() / float(RAND_MAX); }
static float randsf() { return 2 * randuf() - 1; }
static Point4 rand4uf() { return Point4(randuf(), randuf(), randuf(), randuf()); }
static Point4 rand4sf() { return Point4(randsf(), randsf(), randsf(), randsf()); }

void denoise_shadow(const ShadowDenoiser &params)
{
  TIME_D3D_PROFILE(denoise_shadow);
  int width = denoiser::shadow_width, height = denoiser::shadow_height;

  int tilesW = divide_up(width, 16);
  int tilesH = divide_up(height, 16);

  unsigned int tmpFmt = params.shadowTranslucency ? TEXFMT_A8R8G8B8 : TEXFMT_R8;

  make_textures({tilesW, tilesH, TEXCF_UNORDERED | TEXFMT_A8R8G8B8}, 1);
  make_textures({tilesW, tilesH, TEXCF_UNORDERED | TEXFMT_R8G8}, 1);
  make_textures({width, height, TEXCF_UNORDERED | TEXFMT_G16R16F}, 2);
  make_textures({width, height, TEXCF_UNORDERED | tmpFmt}, 3);

  if (reset_history)
    clear_texture(params.denoisedShadowMap);

  auto tiles = transient_textures.find({tilesW, tilesH, TEXCF_UNORDERED | TEXFMT_A8R8G8B8})->second[0].getTex2D();
  auto smoothTiles = transient_textures.find({tilesW, tilesH, TEXCF_UNORDERED | TEXFMT_R8G8})->second[0].getTex2D();
  auto denoiseData1 = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_G16R16F})->second[0].getTex2D();
  auto denoiseData2 = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_G16R16F})->second[1].getTex2D();
  auto denoiseTemp1 = transient_textures.find({width, height, TEXCF_UNORDERED | tmpFmt})->second[0].getTex2D();
  auto denoiseTemp2 = transient_textures.find({width, height, TEXCF_UNORDERED | tmpFmt})->second[1].getTex2D();
  auto denoiseHistory = transient_textures.find({width, height, TEXCF_UNORDERED | tmpFmt})->second[2].getTex2D();

  // Rotators
  Point4 rndScale = Point4(1, 1, 1, 1) + rand4sf() * 0.25f;
  Point4 rndAngle = rand4uf() * DegToRad(360.0f);
  rndAngle.w = DegToRad(120.0f * float(frame_counter % 3));

  Frustum frustum(view_to_clip);
  float x0 = v_extract_z(frustum.camPlanes[Frustum::LEFT]) / v_extract_x(frustum.camPlanes[Frustum::LEFT]);
  float x1 = v_extract_z(frustum.camPlanes[Frustum::RIGHT]) / v_extract_x(frustum.camPlanes[Frustum::RIGHT]);
  float y0 = v_extract_z(frustum.camPlanes[Frustum::BOTTOM]) / v_extract_y(frustum.camPlanes[Frustum::BOTTOM]);
  float y1 = v_extract_z(frustum.camPlanes[Frustum::TOP]) / v_extract_y(frustum.camPlanes[Frustum::TOP]);
  float projecty = 2.0f / (y1 - y0);

  SigmaSharedConstants sigmaSharedConstants;
  sigmaSharedConstants.worldToView = world_to_view;
  sigmaSharedConstants.viewToClip = view_to_clip;
  sigmaSharedConstants.worldToClipPrev = prev_world_to_clip;
  sigmaSharedConstants.frustum = Point4(-x0, -y1, x0 - x1, y1 - y0);
  sigmaSharedConstants.mvScale = Point4(motion_multiplier.x, motion_multiplier.y, motion_multiplier.z, 0);
  sigmaSharedConstants.resourceSizeInv = Point2(1.0 / width, 1.0 / height);
  sigmaSharedConstants.resourceSizeInvPrev = Point2(1.0 / width, 1.0 / height);
  sigmaSharedConstants.rectSize = Point2(width, height);
  sigmaSharedConstants.rectSizeInv = Point2(1.0 / width, 1.0 / height);
  sigmaSharedConstants.rectSizePrev = Point2(width, height);
  sigmaSharedConstants.resolutionScale = Point2(1, 1);
  sigmaSharedConstants.rectOffset = Point2(0, 0);
  sigmaSharedConstants.printfAt = IPoint2(0, 0);
  sigmaSharedConstants.rectOrigin = IPoint2(0, 0);
  sigmaSharedConstants.rectSizeMinusOne = IPoint2(width - 1, height - 1);
  sigmaSharedConstants.tilesSizeMinusOne = IPoint2(tilesW - 1, tilesH - 1);
  sigmaSharedConstants.orthoMode = 0;
  sigmaSharedConstants.unproject = 1.0f / (0.5f * height * abs(projecty));
  sigmaSharedConstants.denoisingRange = 100000;
  sigmaSharedConstants.planeDistSensitivity = 0.005;
  sigmaSharedConstants.blurRadiusScale = 2;
  sigmaSharedConstants.continueAccumulation = reset_history ? 0 : 1;
  sigmaSharedConstants.debug = 0;
  sigmaSharedConstants.splitScreen = 0;
  sigmaSharedConstants.frameIndex = frame_counter;

  {
    TIME_D3D_PROFILE(sigma::classify_tiles);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(sigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, params.shadowValue, false);
    d3d::set_tex(STAGE_CS, 1, params.shadowTranslucency, false);
    d3d::set_rwtex(STAGE_CS, 0, tiles, 0, 0);

    sigmaClassifyTilesCS->dispatch(tilesW, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(sigma::smooth_tiles);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_rwtex(STAGE_CS, 0, smoothTiles, 0, 0);

    sigmaSmoothTilesCS->dispatchThreads(tilesW, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(sigma::blur);

    float ca = cos(rndAngle.y);
    float sa = sin(rndAngle.y);

    sigmaSharedConstants.rotator = Point4(ca, sa, -sa, ca) * rndScale.y;

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, normal_roughness.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 1, params.shadowValue, false);
    d3d::set_tex(STAGE_CS, 2, smoothTiles, false);
    d3d::set_tex(STAGE_CS, 3, params.denoisedShadowMap, false);
    d3d::set_tex(STAGE_CS, 4, params.shadowTranslucency, false);
    d3d::set_rwtex(STAGE_CS, 0, denoiseData1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, denoiseTemp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, denoiseHistory, 0, 0);

    sigmaBlurCS->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(sigma::post_blur);

    float ca = cos(rndAngle.z);
    float sa = sin(rndAngle.z);

    sigmaSharedConstants.rotator = Point4(ca, sa, -sa, ca) * rndScale.z;

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, normal_roughness.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 1, denoiseData1, false);
    d3d::set_tex(STAGE_CS, 2, smoothTiles, false);
    d3d::set_tex(STAGE_CS, 3, denoiseTemp1, false);
    d3d::set_rwtex(STAGE_CS, 0, denoiseData2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, denoiseTemp2, 0, 0);

    sigmaPostBlurCS->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(sigma::stabilize);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&sigmaSharedConstants, divide_up(sizeof(SigmaSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, motion_vectors, false);
    d3d::set_tex(STAGE_CS, 1, denoiseData2, false);
    d3d::set_tex(STAGE_CS, 2, denoiseTemp2, false);
    d3d::set_tex(STAGE_CS, 3, denoiseHistory, false);
    d3d::set_tex(STAGE_CS, 4, smoothTiles, false);
    d3d::set_rwtex(STAGE_CS, 0, params.denoisedShadowMap, 0, 0);

    sigmaStabilizeCS->dispatch(tilesW * 2, tilesH, 1);
  }

  d3d::update_bindless_resource(bindless_range + rtsm_bindless_index, params.denoisedShadowMap);
  d3d::resource_barrier({params.denoisedShadowMap, RB_RO_SRV | RB_STAGE_ALL_SHADERS, 0, 0});
  ShaderGlobal::set_int(rtsm_bindless_slotVarId, bindless_range + rtsm_bindless_index);
  ShaderGlobal::set_int(rtsm_is_translucentVarId, params.shadowTranslucency ? 1 : 0);

  if (params.csmTexture)
  {
    d3d::update_bindless_resource(bindless_range + csm_bindless_index, params.csmTexture);
    ShaderGlobal::set_int(csm_bindless_slotVarId, bindless_range + csm_bindless_index);
    auto samplerIx = d3d::register_bindless_sampler(params.csmSampler);
    ShaderGlobal::set_int(csm_sampler_bindless_slotVarId, samplerIx);
  }
}

static ReblurSharedConstants make_reblur_shared_constants(const Point4 &hit_dist_params, const Point4 &antilag_settings,
  bool use_history_confidence, int width, int height, bool checkerboard, bool high_speed_mode)
{
  Frustum frustum(view_to_clip);
  float x0 = v_extract_z(frustum.camPlanes[Frustum::LEFT]) / v_extract_x(frustum.camPlanes[Frustum::LEFT]);
  float x1 = v_extract_z(frustum.camPlanes[Frustum::RIGHT]) / v_extract_x(frustum.camPlanes[Frustum::RIGHT]);
  float y0 = v_extract_z(frustum.camPlanes[Frustum::BOTTOM]) / v_extract_y(frustum.camPlanes[Frustum::BOTTOM]);
  float y1 = v_extract_z(frustum.camPlanes[Frustum::TOP]) / v_extract_y(frustum.camPlanes[Frustum::TOP]);
  float projecty = 2.0f / (y1 - y0);

  Frustum prevFrustum(prev_view_to_clip);
  float prevX0 = v_extract_z(prevFrustum.camPlanes[Frustum::LEFT]) / v_extract_x(prevFrustum.camPlanes[Frustum::LEFT]);
  float prevX1 = v_extract_z(prevFrustum.camPlanes[Frustum::RIGHT]) / v_extract_x(prevFrustum.camPlanes[Frustum::RIGHT]);
  float prevY0 = v_extract_z(prevFrustum.camPlanes[Frustum::BOTTOM]) / v_extract_y(prevFrustum.camPlanes[Frustum::BOTTOM]);
  float prevY1 = v_extract_z(prevFrustum.camPlanes[Frustum::TOP]) / v_extract_y(prevFrustum.camPlanes[Frustum::TOP]);

  float disocclusionThresholdBonus = (1.0f + jitter_delta) / float(height);
  float disocclusionThreshold = 0.01 + disocclusionThresholdBonus;
  float disocclusionThresholdAlternate = 0.05 + disocclusionThresholdBonus;

  ReblurSharedConstants reblurSharedConstants;
  // TMatrix4
  reblurSharedConstants.viewToClip = view_to_clip;
  reblurSharedConstants.viewToWorld = view_to_world;
  reblurSharedConstants.worldToViewPrev = high_speed_mode ? prev_world_to_view_hs : prev_world_to_view;
  reblurSharedConstants.worldToClipPrev = high_speed_mode ? prev_world_to_clip_hs : prev_world_to_clip;
  reblurSharedConstants.worldPrevToWorld = TMatrix4::IDENT;

  // Point4
  reblurSharedConstants.frustum = Point4(-x0, -y1, x0 - x1, y1 - y0);
  reblurSharedConstants.frustumPrev = Point4(-prevX0, -prevY1, prevX0 - prevX1, prevY1 - prevY0);
  reblurSharedConstants.cameraDelta = high_speed_mode ? Point4::ZERO : Point4::xyz0(prev_view_pos - view_pos);
  reblurSharedConstants.antilagParams = antilag_settings;
  reblurSharedConstants.hitDistParams = hit_dist_params;
  reblurSharedConstants.viewVectorWorld = Point4::xyz0(view_dir);
  reblurSharedConstants.viewVectorWorldPrev = Point4::xyz0(prev_view_dir);
  reblurSharedConstants.mvScale = Point4(motion_multiplier.x, motion_multiplier.y, motion_multiplier.z, 0);

  // Point2
  reblurSharedConstants.resourceSize = Point2(width, height);
  reblurSharedConstants.resourceSizeInv = Point2(1.0 / width, 1.0 / height);
  reblurSharedConstants.resourceSizeInvPrev = Point2(1.0 / width, 1.0 / height);
  reblurSharedConstants.rectSize = Point2(width, height);
  reblurSharedConstants.rectSizeInv = Point2(1.0 / width, 1.0 / height);
  reblurSharedConstants.rectSizePrev = Point2(width, height);
  reblurSharedConstants.resolutionScale = Point2(1, 1);
  reblurSharedConstants.resolutionScalePrev = Point2(1, 1);
  reblurSharedConstants.rectOffset = Point2(0, 0);
  reblurSharedConstants.specProbabilityThresholdsForMvModification = Point2(2.0f, 3.0f);
  reblurSharedConstants.jitter = Point2(jitter.x * width, jitter.y * height);

  // IPoint2
  reblurSharedConstants.printfAt = IPoint2(0, 0);
  reblurSharedConstants.rectOrigin = IPoint2(0, 0);
  reblurSharedConstants.rectSizeMinusOne = IPoint2(width - 1, height - 1);

  // float
  reblurSharedConstants.disocclusionThreshold = disocclusionThreshold;
  reblurSharedConstants.disocclusionThresholdAlternate = disocclusionThresholdAlternate;
  reblurSharedConstants.stabilizationStrength = 1;
  reblurSharedConstants.debug = 0;
  reblurSharedConstants.orthoMode = 0;
  reblurSharedConstants.unproject = 1.0f / (0.5f * height * abs(projecty));
  reblurSharedConstants.denoisingRange = 100000;
  reblurSharedConstants.planeDistSensitivity = 0.005;
  reblurSharedConstants.framerateScale = frame_rate_scale;
  reblurSharedConstants.minBlurRadius = 1;
  reblurSharedConstants.maxBlurRadius = 15;
  reblurSharedConstants.diffPrepassBlurRadius = 30;
  reblurSharedConstants.specPrepassBlurRadius = 50;
  reblurSharedConstants.maxAccumulatedFrameNum = reset_history ? 0 : max_accumulated_frame_num;
  reblurSharedConstants.maxFastAccumulatedFrameNum = max_fast_accumulated_frame_num;
  reblurSharedConstants.antiFirefly = 0;
  reblurSharedConstants.lobeAngleFraction = 0.2;
  reblurSharedConstants.roughnessFraction = 0.15;
  reblurSharedConstants.responsiveAccumulationRoughnessThreshold = 0;
  reblurSharedConstants.historyFixFrameNum = 3;
  reblurSharedConstants.minRectDimMulUnproject = (float)min(width, height) * reblurSharedConstants.unproject;
  reblurSharedConstants.usePrepassNotOnlyForSpecularMotionEstimation = 1;
  reblurSharedConstants.splitScreen = 0;
  reblurSharedConstants.checkerboardResolveAccumSpeed = checkerboard_resolve_accum_speed;

  // uint32_t
  reblurSharedConstants.hasHistoryConfidence = use_history_confidence ? 1 : 0;
  reblurSharedConstants.hasDisocclusionThresholdMix = 0;
  reblurSharedConstants.diffCheckerboard = checkerboard ? 1 : 2;
  reblurSharedConstants.specCheckerboard = checkerboard ? 0 : 2;
  reblurSharedConstants.frameIndex = frame_counter;
  reblurSharedConstants.diffMaterialMask = 0;
  reblurSharedConstants.specMaterialMask = 0;
  reblurSharedConstants.isRectChanged = 0;
  reblurSharedConstants.resetHistory = reset_history ? 1 : 0;

  return reblurSharedConstants;
}

void denoise_ao(const AODenoiser &params)
{
  TIME_D3D_PROFILE(denoise_ao);
  int width = denoiser::ao_width, height = denoiser::ao_height;
  bool halfRes = width < denoiser::render_width;
  Texture *viewZ = halfRes ? half_view_z.getTex2D() : denoiser::view_z.getTex2D();
  Texture *motionVectors = halfRes ? half_motion_vectors : denoiser::motion_vectors;
  Texture *normalRoughness = halfRes ? half_normal_roughness.getTex2D() : denoiser::normal_roughness.getTex2D();

  int tilesW = divide_up(width, 16);
  int tilesH = divide_up(height, 16);

  ReblurSharedConstants reblurSharedConstants =
    make_reblur_shared_constants(params.hitDistParams, params.antilagSettings, false, width, height, true, false);

  static_assert(offsetof(ReblurSharedConstants, rotator) % 16 == 0,
    "`Point4 rotator` vector straddles 4-vector boundary of constant buffer.");

  // Rotators
  Point4 rndScale = Point4(1, 1, 1, 1) + rand4sf() * 0.25f;
  Point4 rndAngle = rand4uf() * DegToRad(360.0f);

  make_textures({tilesW, tilesH, TEXCF_UNORDERED | TEXFMT_R8}, 1);
  make_textures({width, height, TEXCF_UNORDERED | TEXFMT_L16}, 3);
  make_textures({width, height, TEXCF_UNORDERED | TEXFMT_R8G8}, 1);

  if (reset_history)
    clear_texture(params.denoisedAo);

  auto tiles = transient_textures.find({tilesW, tilesH, TEXCF_UNORDERED | TEXFMT_R8})->second[0].getTex2D();
  auto diffTemp1 = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_L16})->second[0].getTex2D();
  auto diffTemp2 = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_L16})->second[1].getTex2D();
  auto diffFastHistory = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_L16})->second[2].getTex2D();
  auto data1 = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_R8G8})->second[0].getTex2D();

  ShaderGlobal::set_int4(denoiser_resolutionVarId, width, height, 0, 0);
  {
    TIME_D3D_PROFILE(reblur::classify_tiles);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, viewZ, false);
    d3d::set_rwtex(STAGE_CS, 0, tiles, 0, 0);

    reblurClassifyTiles->dispatch(tilesW, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_accumulation);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);
    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 2, viewZ, false);
    d3d::set_tex(STAGE_CS, 3, motionVectors, false);
    d3d::set_tex(STAGE_CS, 4, ao_prev_view_z.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 5, ao_prev_normal_roughness.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 6, ao_prev_internal_data.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 7, viewZ, false);
    d3d::set_tex(STAGE_CS, 8, viewZ, false);
    d3d::set_tex(STAGE_CS, 9, params.aoValue, false);
    d3d::set_tex(STAGE_CS, 10, params.denoisedAo, false);
    d3d::set_tex(STAGE_CS, 11, ao_diff_fast_history.getTex2D(), false);
    d3d::set_rwtex(STAGE_CS, 0, diffTemp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, diffFastHistory, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, data1, 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfTemporalAccumulation->dispatch(tilesW * 2, tilesH, 1);
    else
      reblurDiffuseOcclusionTemporalAccumulation->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::history_fix);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 2, data1, false);
    d3d::set_tex(STAGE_CS, 3, viewZ, false);
    d3d::set_tex(STAGE_CS, 4, diffTemp2, false);
    d3d::set_tex(STAGE_CS, 5, diffFastHistory, false);
    d3d::set_rwtex(STAGE_CS, 0, diffTemp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, ao_diff_fast_history.getTex2D(), 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfHistoryFix->dispatch(tilesW * 2, tilesH, 1);
    else
      reblurDiffuseOcclusionHistoryFix->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::blur);

    float ca = cos(rndAngle.y);
    float sa = sin(rndAngle.y);
    reblurSharedConstants.rotator = Point4(ca, sa, -sa, ca) * rndScale.y;

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 2, data1, false);
    d3d::set_tex(STAGE_CS, 3, diffTemp1, false);
    d3d::set_tex(STAGE_CS, 4, viewZ, false);
    d3d::set_rwtex(STAGE_CS, 0, diffTemp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, ao_prev_view_z.getTex2D(), 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfBlur->dispatch(tilesW * 2, tilesH, 1);
    else
      reblurDiffuseOcclusionBlur->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::post_blur);

    float ca = cos(rndAngle.z);
    float sa = sin(rndAngle.z);
    reblurSharedConstants.rotator = Point4(ca, sa, -sa, ca) * rndScale.z;

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 2, data1, false);
    d3d::set_tex(STAGE_CS, 3, diffTemp2, false);
    d3d::set_tex(STAGE_CS, 4, ao_prev_view_z.getTex2D(), false);
    d3d::set_rwtex(STAGE_CS, 0, ao_prev_normal_roughness.getTex2D(), 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, params.denoisedAo, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, ao_prev_internal_data.getTex2D(), 0, 0);

    if (params.performanceMode)
      reblurDiffuseOcclusionPerfPostBlurNoTemporalStabilization->dispatch(tilesW * 2, tilesH, 1);
    else
      reblurDiffuseOcclusionPostBlurNoTemporalStabilization->dispatch(tilesW * 2, tilesH, 1);
  }

  d3d::update_bindless_resource(bindless_range + rtao_bindless_index, params.denoisedAo);
  ShaderGlobal::set_int(rtao_bindless_slotVarId, bindless_range + rtao_bindless_index);
}

static void denoise_reflection_reblur(const ReflectionDenoiser &params)
{
  TIME_D3D_PROFILE(denoise_reflection_reblur);
  int width = denoiser::reflection_width, height = denoiser::reflection_height;
  bool halfRes = width < denoiser::render_width;
  Texture *viewZ = halfRes ? half_view_z.getTex2D() : denoiser::view_z.getTex2D();
  Texture *motionVectors = halfRes ? half_motion_vectors : denoiser::motion_vectors;
  Texture *normalRoughness = halfRes ? half_normal_roughness.getTex2D() : denoiser::normal_roughness.getTex2D();

  int tilesW = divide_up(width, 16);
  int tilesH = divide_up(height, 16);

  ReblurSharedConstants reblurSharedConstants = make_reblur_shared_constants(params.hitDistParams, params.antilagSettings, true, width,
    height, params.checkerboard, params.highSpeedMode);

  // Rotators
  Point4 rndScale = Point4(1, 1, 1, 1) + rand4sf() * 0.25f;
  Point4 rndAngle = rand4uf() * DegToRad(360.0f);

  make_textures({tilesW, tilesH, TEXCF_UNORDERED | TEXFMT_R8}, 1);
  make_textures({width, height, TEXCF_UNORDERED | TEXFMT_R16F}, 2);
  make_textures({width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F}, 2);
  make_textures({width, height, TEXCF_UNORDERED | TEXFMT_R32UI}, 1);
  make_textures({width, height, TEXCF_UNORDERED | TEXFMT_R8G8}, 1);

  auto data1 = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_R8G8})->second[0].getTex2D();
  auto data2 = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_R32UI})->second[0].getTex2D();
  auto hitdistForTracking = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_R16F})->second[0].getTex2D();
  auto tmp1 = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F})->second[0].getTex2D();
  auto tmp2 = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F})->second[1].getTex2D();
  auto fastHistory = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_R16F})->second[1].getTex2D();
  auto tiles = transient_textures.find({tilesW, tilesH, TEXCF_UNORDERED | TEXFMT_R8})->second[0].getTex2D();

  auto ping =
    (frame_counter & 1) == 0 ? reblur_spec_hitdist_for_tracking_ping.getTex2D() : reblur_spec_hitdist_for_tracking_pong.getTex2D();
  auto pong =
    (frame_counter & 1) != 0 ? reblur_spec_hitdist_for_tracking_ping.getTex2D() : reblur_spec_hitdist_for_tracking_pong.getTex2D();

  ShaderGlobal::set_int4(denoiser_resolutionVarId, width, height, 0, 0);

  {
    TIME_D3D_PROFILE(reblur::classify_tiles);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, viewZ, false);
    d3d::set_rwtex(STAGE_CS, 0, tiles, 0, 0);

    reblurClassifyTiles->dispatch(tilesW, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::prepass);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 2, viewZ, false);
    d3d::set_tex(STAGE_CS, 3, params.reflectionValue, false);
    d3d::set_rwtex(STAGE_CS, 0, tmp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, hitdistForTracking, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfPrepass->dispatch(tilesW * 2, tilesH, 1);
    else
      reblurSpecularPrepass->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_accumulation);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 2, viewZ, false);
    d3d::set_tex(STAGE_CS, 3, motionVectors, false);
    d3d::set_tex(STAGE_CS, 4, reblur_spec_prev_view_z.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 5, reblur_spec_prev_normal_roughness.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 6, reblur_spec_prev_internal_data.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 7, viewZ, false);
    d3d::set_tex(STAGE_CS, 8, reblur_spec_history_confidence.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 9, tmp1, false);
    d3d::set_tex(STAGE_CS, 10, reblur_spec_history.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 11, reblur_spec_fast_history.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 12, ping, false);
    d3d::set_tex(STAGE_CS, 13, hitdistForTracking, false);
    d3d::set_rwtex(STAGE_CS, 0, tmp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, fastHistory, 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, pong, 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, data1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 4, data2, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfTemporalAccumulation->dispatch(tilesW * 2, tilesH, 1);
    else
      reblurSpecularTemporalAccumulation->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::history_fix);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 2, data1, false);
    d3d::set_tex(STAGE_CS, 3, viewZ, false);
    d3d::set_tex(STAGE_CS, 4, tmp2, false);
    d3d::set_tex(STAGE_CS, 5, fastHistory, false);
    d3d::set_rwtex(STAGE_CS, 0, tmp1, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, reblur_spec_fast_history.getTex2D(), 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfHistoryFix->dispatch(tilesW * 2, tilesH, 1);
    else
      reblurSpecularHistoryFix->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::blur);

    float ca = cos(rndAngle.y);
    float sa = sin(rndAngle.y);
    reblurSharedConstants.rotator = Point4(ca, sa, -sa, ca) * rndScale.y;

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 2, data1, false);
    d3d::set_tex(STAGE_CS, 3, tmp1, false);
    d3d::set_tex(STAGE_CS, 4, viewZ, false);
    d3d::set_rwtex(STAGE_CS, 0, tmp2, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, reblur_spec_prev_view_z.getTex2D(), 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfBlur->dispatch(tilesW * 2, tilesH, 1);
    else
      reblurSpecularBlur->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::post_blur);

    float ca = cos(rndAngle.z);
    float sa = sin(rndAngle.z);
    reblurSharedConstants.rotator = Point4(ca, sa, -sa, ca) * rndScale.z;

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 2, data1, false);
    d3d::set_tex(STAGE_CS, 3, tmp2, false);
    d3d::set_tex(STAGE_CS, 4, reblur_spec_prev_view_z.getTex2D(), false);
    d3d::set_rwtex(STAGE_CS, 0, reblur_spec_prev_normal_roughness.getTex2D(), 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, reblur_spec_history.getTex2D(), 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfPostBlur->dispatch(tilesW * 2, tilesH, 1);
    else
      reblurSpecularPostBlur->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::copy);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, params.denoisedReflection, false);
    d3d::set_rwtex(STAGE_CS, 0, tmp2, 0, 0);

    reblurSpecularCopy->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(reblur::temporal_stabilization);

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 2, nullptr, false);
    d3d::set_tex(STAGE_CS, 3, reblur_spec_prev_view_z.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 4, data1, false);
    d3d::set_tex(STAGE_CS, 5, data2, false);
    d3d::set_tex(STAGE_CS, 6, reblur_spec_history.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 7, tmp2, false);
    d3d::set_tex(STAGE_CS, 8, pong, false);
    d3d::set_tex(STAGE_CS, 9, motionVectors, false);
    d3d::set_tex(STAGE_CS, 10, reblur_spec_history_confidence.getTex2D(), false);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, reblur_spec_prev_internal_data.getTex2D(), 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, params.denoisedReflection, 0, 0);

    if (params.performanceMode)
      reblurSpecularPerfTemporalStabilization->dispatch(tilesW * 2, tilesH, 1);
    else
      reblurSpecularTemporalStabilization->dispatch(tilesW * 2, tilesH, 1);
  }

  if (params.validationTexture)
  {
    TIME_D3D_PROFILE(reblur::validation);

    reblurSharedConstants.padding1 = 0; // Has diffuse
    reblurSharedConstants.padding2 = 1; // Has specular

    d3d::set_cb0_data(STAGE_CS, (const float *)&reblurSharedConstants, divide_up(sizeof(reblurSharedConstants), 16));
    d3d::set_tex(STAGE_CS, 0, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 1, viewZ, false);
    d3d::set_tex(STAGE_CS, 2, motionVectors, false);
    d3d::set_tex(STAGE_CS, 3, data1, false);
    d3d::set_tex(STAGE_CS, 4, data2, false);
    d3d::set_tex(STAGE_CS, 5, params.reflectionValue, false);
    d3d::set_tex(STAGE_CS, 6, params.reflectionValue, false);
    d3d::set_rwtex(STAGE_CS, 0, params.validationTexture, 0, 0);

    reblurValidation->dispatch(tilesW * 2, tilesH, 1);
  }
}

#if ENABLE_NRD_INTEGRATION_TEST
static void denoise_reflection_nrd_lib(const ReflectionDenoiser &params)
{
  TIME_D3D_PROFILE(denoise_reflection_nrd_lib);

  auto cbReblur = new NRDCB([=](ID3D12GraphicsCommandList *commandList, ID3D12CommandAllocator *commandAllocator, ResCB res) {
    int width = denoiser::reflection_width;
    bool halfRes = width < denoiser::render_width;
    Texture *viewZ = halfRes ? half_view_z.getTex2D() : denoiser::view_z.getTex2D();
    Texture *motionVectors = halfRes ? half_motion_vectors : denoiser::motion_vectors;
    Texture *normalRoughness = halfRes ? half_normal_roughness.getTex2D() : denoiser::normal_roughness.getTex2D();

    nri::CommandBufferD3D12Desc commandBufferDesc = {};
    commandBufferDesc.d3d12CommandList = commandList;
    commandBufferDesc.d3d12CommandAllocator = commandAllocator;

    nri::CommandBuffer *nriCommandBuffer = nullptr;
    nriInterface.CreateCommandBufferD3D12(*nriDevice, commandBufferDesc, nriCommandBuffer);

    nrdInterface->NewFrame();

    static nrd::CommonSettings commonSettings;
    memcpy_s(commonSettings.viewToClipMatrixPrev, sizeof(commonSettings.viewToClipMatrixPrev), &prev_view_to_clip, sizeof(TMatrix4));
    memcpy_s(commonSettings.worldToViewMatrixPrev, sizeof(commonSettings.worldToViewMatrixPrev),
      params.highSpeedMode ? &prev_world_to_view_hs : &prev_world_to_view, sizeof(TMatrix4));
    memcpy_s(commonSettings.worldToViewMatrix, sizeof(commonSettings.worldToViewMatrix), &world_to_view, sizeof(TMatrix4));
    memcpy_s(commonSettings.viewToClipMatrix, sizeof(commonSettings.viewToClipMatrix), &view_to_clip, sizeof(TMatrix4));
    commonSettings.cameraJitterPrev[0] = commonSettings.cameraJitter[0];
    commonSettings.cameraJitterPrev[1] = commonSettings.cameraJitter[1];
    commonSettings.denoisingRange = 100000;
    commonSettings.frameIndex = frame_counter;
    commonSettings.cameraJitter[0] = jitter.x * reflection_width;
    commonSettings.cameraJitter[1] = jitter.y * reflection_height;
    commonSettings.motionVectorScale[0] = 1;
    commonSettings.motionVectorScale[1] = 1;
    commonSettings.motionVectorScale[2] = 1;
    commonSettings.isMotionVectorInWorldSpace = false;
    commonSettings.enableValidation = !!params.validationTexture;
    commonSettings.resourceSize[0] = reflection_width;
    commonSettings.resourceSize[1] = reflection_height;
    commonSettings.resourceSizePrev[0] = reflection_width;
    commonSettings.resourceSizePrev[1] = reflection_height;
    commonSettings.rectSize[0] = reflection_width;
    commonSettings.rectSize[1] = reflection_height;
    commonSettings.rectSizePrev[0] = reflection_width;
    commonSettings.rectSizePrev[1] = reflection_height;

    nrdInterface->SetCommonSettings(commonSettings);

    nrd::ReblurSettings reblurSettings;
    reblurSettings.hitDistanceParameters.A = params.hitDistParams.x;
    reblurSettings.hitDistanceParameters.B = params.hitDistParams.y;
    reblurSettings.hitDistanceParameters.C = params.hitDistParams.z;
    reblurSettings.hitDistanceParameters.D = params.hitDistParams.w;
    reblurSettings.antilagSettings.luminanceSigmaScale = params.antilagSettings.x;
    reblurSettings.antilagSettings.hitDistanceSigmaScale = params.antilagSettings.y;
    reblurSettings.antilagSettings.luminanceAntilagPower = params.antilagSettings.z;
    reblurSettings.antilagSettings.hitDistanceAntilagPower = params.antilagSettings.w;
    reblurSettings.maxAccumulatedFrameNum = max_accumulated_frame_num;
    reblurSettings.checkerboardMode = nrd::CheckerboardMode::WHITE;

    nrdInterface->SetDenoiserSettings(ReflectionId, &reblurSettings);

    nri::TextureBarrierDesc reflectionDesc = {};
    nri::TextureBarrierDesc mvDesc = {};
    nri::TextureBarrierDesc nrDesc = {};
    nri::TextureBarrierDesc vzDesc = {};
    nri::TextureBarrierDesc outRefDesc = {};
    nri::TextureBarrierDesc outValDesc = {};

    nri::TextureD3D12Desc textureDesc = {};

    textureDesc.d3d12Resource = res(params.reflectionValue);
    nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)reflectionDesc.texture);
    reflectionDesc.after.access = nri::AccessBits::SHADER_RESOURCE;
    reflectionDesc.after.layout = nri::Layout::SHADER_RESOURCE;

    textureDesc.d3d12Resource = res(motionVectors);
    nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)mvDesc.texture);
    mvDesc.after.access = nri::AccessBits::SHADER_RESOURCE;
    mvDesc.after.layout = nri::Layout::SHADER_RESOURCE;

    textureDesc.d3d12Resource = res(normalRoughness);
    nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)nrDesc.texture);
    nrDesc.after.access = nri::AccessBits::SHADER_RESOURCE;
    nrDesc.after.layout = nri::Layout::SHADER_RESOURCE;

    textureDesc.d3d12Resource = res(viewZ);
    nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)vzDesc.texture);
    vzDesc.after.access = nri::AccessBits::SHADER_RESOURCE;
    vzDesc.after.layout = nri::Layout::SHADER_RESOURCE;

    textureDesc.d3d12Resource = res(params.denoisedReflection);
    nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)outRefDesc.texture);
    outRefDesc.after.access = nri::AccessBits::SHADER_RESOURCE_STORAGE;
    outRefDesc.after.layout = nri::Layout::SHADER_RESOURCE_STORAGE;

    if (params.validationTexture)
    {
      textureDesc.d3d12Resource = res(params.validationTexture);
      nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)outValDesc.texture);
      outValDesc.after.access = nri::AccessBits::SHADER_RESOURCE_STORAGE;
      outValDesc.after.layout = nri::Layout::SHADER_RESOURCE_STORAGE;
    }

    NrdUserPool userPool = {};
    {
      NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST, {&reflectionDesc, nri::Format::RGBA16_SFLOAT});
      NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_MV, {&mvDesc, nri::Format::RGBA16_SFLOAT});
      NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_NORMAL_ROUGHNESS, {&nrDesc, nri::Format::R10_G10_B10_A2_UNORM});
      NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_VIEWZ, {&vzDesc, nri::Format::R32_SFLOAT});
      NrdIntegration_SetResource(userPool, nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST, {&outRefDesc, nri::Format::RGBA16_SFLOAT});
      if (params.validationTexture)
        NrdIntegration_SetResource(userPool, nrd::ResourceType::OUT_VALIDATION, {&outValDesc, nri::Format::BGRA8_UNORM});
    };

    const nrd::Identifier denoisers[] = {ReflectionId};

    nrdInterface->Denoise(denoisers, _countof(denoisers), *nriCommandBuffer, userPool);

    nriInterface.DestroyTexture((nri::Texture &)*reflectionDesc.texture);
    nriInterface.DestroyTexture((nri::Texture &)*mvDesc.texture);
    nriInterface.DestroyTexture((nri::Texture &)*nrDesc.texture);
    nriInterface.DestroyTexture((nri::Texture &)*vzDesc.texture);
    nriInterface.DestroyTexture((nri::Texture &)*outRefDesc.texture);
    if (params.validationTexture)
      nriInterface.DestroyTexture((nri::Texture &)*outValDesc.texture);
    nriInterface.DestroyCommandBuffer(*nriCommandBuffer);
  });

  auto cbRelax = new NRDCB([=](ID3D12GraphicsCommandList *commandList, ID3D12CommandAllocator *commandAllocator, ResCB res) {
    int width = denoiser::reflection_width;
    bool halfRes = width < denoiser::render_width;
    Texture *viewZ = halfRes ? half_view_z.getTex2D() : denoiser::view_z.getTex2D();
    Texture *motionVectors = halfRes ? half_motion_vectors : denoiser::motion_vectors;
    Texture *normalRoughness = halfRes ? half_normal_roughness.getTex2D() : denoiser::normal_roughness.getTex2D();

    nri::CommandBufferD3D12Desc commandBufferDesc = {};
    commandBufferDesc.d3d12CommandList = commandList;
    commandBufferDesc.d3d12CommandAllocator = commandAllocator;

    nri::CommandBuffer *nriCommandBuffer = nullptr;
    nriInterface.CreateCommandBufferD3D12(*nriDevice, commandBufferDesc, nriCommandBuffer);

    nrdInterface->NewFrame();

    static nrd::CommonSettings commonSettings;
    memcpy_s(commonSettings.viewToClipMatrixPrev, sizeof(commonSettings.viewToClipMatrixPrev), &prev_view_to_clip, sizeof(TMatrix4));
    memcpy_s(commonSettings.worldToViewMatrixPrev, sizeof(commonSettings.worldToViewMatrixPrev),
      params.highSpeedMode ? &prev_world_to_view_hs : &prev_world_to_view, sizeof(TMatrix4));
    memcpy_s(commonSettings.worldToViewMatrix, sizeof(commonSettings.worldToViewMatrix), &world_to_view, sizeof(TMatrix4));
    memcpy_s(commonSettings.viewToClipMatrix, sizeof(commonSettings.viewToClipMatrix), &view_to_clip, sizeof(TMatrix4));
    commonSettings.cameraJitterPrev[0] = commonSettings.cameraJitter[0];
    commonSettings.cameraJitterPrev[1] = commonSettings.cameraJitter[1];
    commonSettings.denoisingRange = 100000;
    commonSettings.frameIndex = frame_counter;
    commonSettings.cameraJitter[0] = jitter.x * reflection_width;
    commonSettings.cameraJitter[1] = jitter.y * reflection_height;
    commonSettings.motionVectorScale[0] = 1;
    commonSettings.motionVectorScale[1] = 1;
    commonSettings.motionVectorScale[2] = 1;
    commonSettings.isMotionVectorInWorldSpace = false;
    commonSettings.enableValidation = !!params.validationTexture;
    commonSettings.resourceSize[0] = reflection_width;
    commonSettings.resourceSize[1] = reflection_height;
    commonSettings.resourceSizePrev[0] = reflection_width;
    commonSettings.resourceSizePrev[1] = reflection_height;
    commonSettings.rectSize[0] = reflection_width;
    commonSettings.rectSize[1] = reflection_height;
    commonSettings.rectSizePrev[0] = reflection_width;
    commonSettings.rectSizePrev[1] = reflection_height;

    nrdInterface->SetCommonSettings(commonSettings);

    nrd::RelaxSettings relaxSettings;
    relaxSettings.checkerboardMode = params.checkerboard ? nrd::CheckerboardMode::WHITE : nrd::CheckerboardMode::OFF;

    nrdInterface->SetDenoiserSettings(ReflectionId, &relaxSettings);

    nri::TextureBarrierDesc reflectionDesc = {};
    nri::TextureBarrierDesc mvDesc = {};
    nri::TextureBarrierDesc nrDesc = {};
    nri::TextureBarrierDesc vzDesc = {};
    nri::TextureBarrierDesc outRefDesc = {};
    nri::TextureBarrierDesc outValDesc = {};

    nri::TextureD3D12Desc textureDesc = {};

    textureDesc.d3d12Resource = res(params.reflectionValue);
    nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)reflectionDesc.texture);
    reflectionDesc.after.access = nri::AccessBits::SHADER_RESOURCE;
    reflectionDesc.after.layout = nri::Layout::SHADER_RESOURCE;

    textureDesc.d3d12Resource = res(motionVectors);
    nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)mvDesc.texture);
    mvDesc.after.access = nri::AccessBits::SHADER_RESOURCE;
    mvDesc.after.layout = nri::Layout::SHADER_RESOURCE;

    textureDesc.d3d12Resource = res(normalRoughness);
    nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)nrDesc.texture);
    nrDesc.after.access = nri::AccessBits::SHADER_RESOURCE;
    nrDesc.after.layout = nri::Layout::SHADER_RESOURCE;

    textureDesc.d3d12Resource = res(viewZ);
    nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)vzDesc.texture);
    vzDesc.after.access = nri::AccessBits::SHADER_RESOURCE;
    vzDesc.after.layout = nri::Layout::SHADER_RESOURCE;

    textureDesc.d3d12Resource = res(params.denoisedReflection);
    nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)outRefDesc.texture);
    outRefDesc.after.access = nri::AccessBits::SHADER_RESOURCE_STORAGE;
    outRefDesc.after.layout = nri::Layout::SHADER_RESOURCE_STORAGE;

    if (params.validationTexture)
    {
      textureDesc.d3d12Resource = res(params.validationTexture);
      nriInterface.CreateTextureD3D12(*nriDevice, textureDesc, (nri::Texture *&)outValDesc.texture);
      outValDesc.after.access = nri::AccessBits::SHADER_RESOURCE_STORAGE;
      outValDesc.after.layout = nri::Layout::SHADER_RESOURCE_STORAGE;
    }

    NrdUserPool userPool = {};
    {
      NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST, {&reflectionDesc, nri::Format::RGBA16_SFLOAT});
      NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_MV, {&mvDesc, nri::Format::RGBA16_SFLOAT});
      NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_NORMAL_ROUGHNESS, {&nrDesc, nri::Format::R10_G10_B10_A2_UNORM});
      NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_VIEWZ, {&vzDesc, nri::Format::R32_SFLOAT});
      NrdIntegration_SetResource(userPool, nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST, {&outRefDesc, nri::Format::RGBA16_SFLOAT});
      if (params.validationTexture)
        NrdIntegration_SetResource(userPool, nrd::ResourceType::OUT_VALIDATION, {&outValDesc, nri::Format::BGRA8_UNORM});
    };

    const nrd::Identifier denoisers[] = {ReflectionId};

    nrdInterface->Denoise(denoisers, _countof(denoisers), *nriCommandBuffer, userPool);

    nriInterface.DestroyTexture((nri::Texture &)*reflectionDesc.texture);
    nriInterface.DestroyTexture((nri::Texture &)*mvDesc.texture);
    nriInterface.DestroyTexture((nri::Texture &)*nrDesc.texture);
    nriInterface.DestroyTexture((nri::Texture &)*vzDesc.texture);
    nriInterface.DestroyTexture((nri::Texture &)*outRefDesc.texture);
    if (params.validationTexture)
      nriInterface.DestroyTexture((nri::Texture &)*outValDesc.texture);
    nriInterface.DestroyCommandBuffer(*nriCommandBuffer);
  });

  G_UNUSED(cbReblur);
  G_UNUSED(cbRelax);

  d3d::driver_command(Drv3dCommand::NRD_TEST, cbRelax);
}
#endif

// From the NRD
inline Point3 relax_get_frustum_forward(const TMatrix4 &view_to_world_arg, const Point4 &frustum)
{
  Point4 frustumForwardView = Point4(frustum.x + frustum.z * 0.5f, frustum.y + frustum.w * 0.5f, 1.0f, 0.0f);
  Point3 frustumForwardWorld = Point3::xyz(frustumForwardView * view_to_world_arg);
  return frustumForwardWorld;
}

static RelaxSharedConstants make_relax_shared_constants(int width, int height, bool checkerboard)
{
  Frustum frustum(view_to_clip);
  float x0 = v_extract_z(frustum.camPlanes[Frustum::LEFT]) / v_extract_x(frustum.camPlanes[Frustum::LEFT]);
  float x1 = v_extract_z(frustum.camPlanes[Frustum::RIGHT]) / v_extract_x(frustum.camPlanes[Frustum::RIGHT]);
  float y0 = v_extract_z(frustum.camPlanes[Frustum::BOTTOM]) / v_extract_y(frustum.camPlanes[Frustum::BOTTOM]);
  float y1 = v_extract_z(frustum.camPlanes[Frustum::TOP]) / v_extract_y(frustum.camPlanes[Frustum::TOP]);
  float projecty = 2.0f / (y1 - y0);

  Frustum prevFrustum(prev_view_to_clip);
  float prevX0 = v_extract_z(prevFrustum.camPlanes[Frustum::LEFT]) / v_extract_x(prevFrustum.camPlanes[Frustum::LEFT]);
  float prevX1 = v_extract_z(prevFrustum.camPlanes[Frustum::RIGHT]) / v_extract_x(prevFrustum.camPlanes[Frustum::RIGHT]);
  float prevY0 = v_extract_z(prevFrustum.camPlanes[Frustum::BOTTOM]) / v_extract_y(prevFrustum.camPlanes[Frustum::BOTTOM]);
  float prevY1 = v_extract_z(prevFrustum.camPlanes[Frustum::TOP]) / v_extract_y(prevFrustum.camPlanes[Frustum::TOP]);

  float tanHalfFov = 1.0f / view_to_clip._11;
  float aspect = view_to_clip._11 / view_to_clip._22;
  Point3 frustumRight = Point3::xyz(world_to_view.getcol(0)) * tanHalfFov;
  Point3 frustumUp = Point3::xyz(world_to_view.getcol(1)) * tanHalfFov * aspect;
  Point3 frustumForward = relax_get_frustum_forward(view_to_world, Point4(-x0, -y1, x0 - x1, y1 - y0));

  float prevTanHalfFov = 1.0f / prev_view_to_clip._11;
  float prevAspect = prev_view_to_clip._11 / prev_view_to_clip._22;
  Point3 prevFrustumRight = Point3::xyz(prev_world_to_view.getcol(0)) * prevTanHalfFov;
  Point3 prevFrustumUp = Point3::xyz(prev_world_to_view.getcol(1)) * prevTanHalfFov * prevAspect;
  Point3 prevFrustumForward =
    relax_get_frustum_forward(prev_view_to_world, Point4(-prevX0, -prevY1, prevX0 - prevX1, prevY1 - prevY0));

  RelaxSharedConstants relaxSharedConstants;

  // TMatrix4
  relaxSharedConstants.worldToClipPrev = prev_world_to_view * prev_view_to_clip;
  relaxSharedConstants.worldToViewPrev = prev_world_to_view;
  relaxSharedConstants.worldPrevToWorld = TMatrix::IDENT;

  // Point4
  relaxSharedConstants.frustumRight = Point4::xyz0(frustumRight);
  relaxSharedConstants.frustumUp = Point4::xyz0(frustumUp);
  relaxSharedConstants.frustumForward = Point4::xyz0(frustumForward);
  relaxSharedConstants.prevFrustumRight = Point4::xyz0(prevFrustumRight);
  relaxSharedConstants.prevFrustumUp = Point4::xyz0(prevFrustumUp);
  relaxSharedConstants.prevFrustumForward = Point4::xyz0(prevFrustumForward);
  relaxSharedConstants.cameraDelta = Point4::xyz0(prev_view_pos - view_pos);
  relaxSharedConstants.mvScale = Point4(motion_multiplier.x, motion_multiplier.y, motion_multiplier.z, 0);

  // Point2
  relaxSharedConstants.jitter = Point2(jitter.x * width, jitter.y * height);
  relaxSharedConstants.resolutionScale = Point2(1, 1);
  relaxSharedConstants.rectOffset = Point2(0, 0);
  relaxSharedConstants.resourceSizeInv = Point2(1.0 / width, 1.0 / height);
  relaxSharedConstants.resourceSize = Point2(width, height);
  relaxSharedConstants.rectSizeInv = Point2(1.0 / width, 1.0 / height);
  relaxSharedConstants.rectSizePrev = Point2(width, height);
  relaxSharedConstants.resourceSizeInvPrev = Point2(1.0 / width, 1.0 / height);

  // IPoint2
  relaxSharedConstants.printfAt = IPoint2(0, 0);
  relaxSharedConstants.rectOrigin = IPoint2(0, 0);
  relaxSharedConstants.rectSize = IPoint2(width, height);

  // float
  relaxSharedConstants.specMaxAccumulatedFrameNum = reset_history ? 0 : max_accumulated_frame_num;
  relaxSharedConstants.specMaxFastAccumulatedFrameNum = max_fast_accumulated_frame_num;
  relaxSharedConstants.diffMaxAccumulatedFrameNum = 5;
  relaxSharedConstants.diffMaxFastAccumulatedFrameNum = 1;
  relaxSharedConstants.disocclusionDepthThreshold = 0.0109259;
  relaxSharedConstants.disocclusionDepthThresholdAlternate = 0.0509259;
  relaxSharedConstants.roughnessFraction = 0.15;
  relaxSharedConstants.specVarianceBoost = 0;
  relaxSharedConstants.splitScreen = 0;
  relaxSharedConstants.diffBlurRadius = 30;
  relaxSharedConstants.specBlurRadius = 50;
  relaxSharedConstants.depthThreshold = 0.003;
  relaxSharedConstants.diffLobeAngleFraction = 0.5;
  relaxSharedConstants.specLobeAngleFraction = 0.5;
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
  relaxSharedConstants.diffMaxLuminanceRelativeDifference = INFINITY;
  relaxSharedConstants.specMaxLuminanceRelativeDifference = INFINITY;
  relaxSharedConstants.luminanceEdgeStoppingRelaxation = 1;
  relaxSharedConstants.confidenceDrivenRelaxationMultiplier = 0;
  relaxSharedConstants.confidenceDrivenLuminanceEdgeStoppingRelaxation = 0;
  relaxSharedConstants.confidenceDrivenNormalEdgeStoppingRelaxation = 0;
  relaxSharedConstants.debug = 0;
  relaxSharedConstants.OrthoMode = 0;
  relaxSharedConstants.unproject = 1.0f / (0.5f * height * abs(projecty));
  relaxSharedConstants.framerateScale = frame_rate_scale;
  relaxSharedConstants.checkerboardResolveAccumSpeed = checkerboard_resolve_accum_speed;
  relaxSharedConstants.jitterDelta = jitter_delta;
  relaxSharedConstants.historyFixFrameNum = 4;
  relaxSharedConstants.historyThreshold = 3;

  // uint32_t
  relaxSharedConstants.roughnessEdgeStoppingEnabled = 1;
  relaxSharedConstants.frameIndex = frame_counter;
  relaxSharedConstants.diffCheckerboard = checkerboard ? 1 : 2;
  relaxSharedConstants.specCheckerboard = checkerboard ? 0 : 2;
  relaxSharedConstants.useConfidenceInputs = 0;
  relaxSharedConstants.useDisocclusionThresholdMix = 0;
  relaxSharedConstants.diffMaterialMask = 0;
  relaxSharedConstants.specMaterialMask = 0;
  relaxSharedConstants.resetHistory = reset_history ? 1 : 0;

  return relaxSharedConstants;
}

static void denoise_reflection_relax(const ReflectionDenoiser &params)
{
  TIME_D3D_PROFILE(denoise_reflection_relax);
  int width = denoiser::reflection_width, height = denoiser::reflection_height;
  bool halfRes = width < denoiser::render_width;
  Texture *viewZ = halfRes ? half_view_z.getTex2D() : denoiser::view_z.getTex2D();
  Texture *motionVectors = halfRes ? half_motion_vectors : denoiser::motion_vectors;
  Texture *normalRoughness = halfRes ? half_normal_roughness.getTex2D() : denoiser::normal_roughness.getTex2D();

  int tilesW = divide_up(width, 16);
  int tilesH = divide_up(height, 16);

  RelaxSharedConstants relaxSharedConstants = make_relax_shared_constants(width, height, params.checkerboard);

  // Rotators
  Point4 rndScale = Point4(1, 1, 1, 1) + rand4sf() * 0.25f;
  Point4 rndAngle = rand4uf() * DegToRad(360.0f);

  make_textures({tilesW, tilesH, TEXCF_UNORDERED | TEXFMT_R8}, 1);
  make_textures({width, height, TEXCF_UNORDERED | TEXFMT_R8}, 2);
  make_textures({width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F}, 2);

  auto tiles = transient_textures.find({tilesW, tilesH, TEXCF_UNORDERED | TEXFMT_R8})->second[0].getTex2D();
  auto historyLength = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_R8})->second[0].getTex2D();
  auto specReprojectionConfidence = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_R8})->second[1].getTex2D();
  auto specIllumPing = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F})->second[0].getTex2D();
  auto specIllumPong = transient_textures.find({width, height, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F})->second[1].getTex2D();

  ShaderGlobal::set_int4(denoiser_resolutionVarId, width, height, 0, 0);

  {
    TIME_D3D_PROFILE(relax::classify_tiles);

    struct PassData
    {
      RelaxSharedConstants relaxSharedConstants;
      uint32_t padding;
    } passData;

    passData.relaxSharedConstants = relaxSharedConstants;

    static_assert(sizeof(passData) % (4 * sizeof(float)) == 0,
      "RelaxSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");

    d3d::set_cb0_data(STAGE_CS, (const float *)&passData, divide_up(sizeof(passData), 16));
    d3d::set_tex(STAGE_CS, 0, viewZ, false);
    d3d::set_rwtex(STAGE_CS, 0, tiles, 0, 0);

    relaxClassifyTiles->dispatch(tilesW, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(relax::prepass);

    struct PassData
    {
      RelaxSharedConstants relaxSharedConstants;
      uint32_t padding;
      Point4 rotator;
    } passData;

    float ca = cos(rndAngle.x);
    float sa = sin(rndAngle.x);

    passData.relaxSharedConstants = relaxSharedConstants;
    passData.rotator = Point4(ca, sa, -sa, ca) * rndScale.x;

    static_assert(sizeof(passData) % (4 * sizeof(float)) == 0,
      "RelaxSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");

    static_assert(offsetof(PassData, rotator) % 16 == 0, "`Point4 rotator` vector straddles 4-vector boundary of constant buffer.");

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&passData, divide_up(sizeof(passData), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, params.reflectionValue, false);
    d3d::set_tex(STAGE_CS, 2, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 3, viewZ, false);
    d3d::set_rwtex(STAGE_CS, 0, params.denoisedReflection, 0, 0);

    relaxSpecularPrepass->dispatch(tilesW, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(relax::temporal_accumulation);

    struct PassData
    {
      RelaxSharedConstants relaxSharedConstants;
      uint32_t padding;
    } passData;

    passData.relaxSharedConstants = relaxSharedConstants;

    static_assert(sizeof(passData) % (4 * sizeof(float)) == 0,
      "RelaxSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&passData, divide_up(sizeof(passData), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, params.denoisedReflection, false);
    d3d::set_tex(STAGE_CS, 2, motionVectors, false);
    d3d::set_tex(STAGE_CS, 3, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 4, viewZ, false);
    d3d::set_tex(STAGE_CS, 5, relax_spec_illum_responsive_prev.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 6, relax_spec_illum_prev.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 7, relax_spec_normal_roughness_prev.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 8, relax_spec_prev_view_z.getTex2D(), false);
    if ((frame_counter & 1) == 0)
      d3d::set_tex(STAGE_CS, 9, relax_spec_reflection_hit_t_prev.getTex2D(), false);
    else
      d3d::set_tex(STAGE_CS, 9, relax_spec_reflection_hit_t_curr.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 10, relax_spec_history_length_prev.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 11, relax_spec_material_id_prev.getTex2D(), false);
    d3d::set_tex(STAGE_CS, 12, viewZ, false); // hasConfidenceInputs
    d3d::set_tex(STAGE_CS, 13, viewZ, false); // hasDisocclusionThresholdMix
    d3d::set_rwtex(STAGE_CS, 0, specIllumPing, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, specIllumPong, 0, 0);
    if ((frame_counter & 1) == 0)
      d3d::set_rwtex(STAGE_CS, 2, relax_spec_reflection_hit_t_curr.getTex2D(), 0, 0);
    else
      d3d::set_rwtex(STAGE_CS, 2, relax_spec_reflection_hit_t_prev.getTex2D(), 0, 0);
    d3d::set_rwtex(STAGE_CS, 3, historyLength, 0, 0);
    d3d::set_rwtex(STAGE_CS, 4, specReprojectionConfidence, 0, 0);

    relaxSpecularTemporalAccumulation->dispatch(tilesW * 2, tilesH, 1);
  }

  {
    TIME_D3D_PROFILE(relax::history_fix);

    struct PassData
    {
      RelaxSharedConstants relaxSharedConstants;
      uint32_t padding;
    } passData;

    passData.relaxSharedConstants = relaxSharedConstants;

    static_assert(sizeof(passData) % (4 * sizeof(float)) == 0,
      "RelaxSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&passData, divide_up(sizeof(passData), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, specIllumPing, false);
    d3d::set_tex(STAGE_CS, 2, historyLength, false);
    d3d::set_tex(STAGE_CS, 3, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 4, viewZ, false);
    d3d::set_rwtex(STAGE_CS, 0, specIllumPong, 0, 0);

    relaxSpecularHistoryFix->dispatch(tilesW * 2, tilesH * 2, 1);
  }

  {
    TIME_D3D_PROFILE(relax::history_clamping);

    struct PassData
    {
      RelaxSharedConstants relaxSharedConstants;
      uint32_t padding;
    } passData;

    passData.relaxSharedConstants = relaxSharedConstants;

    static_assert(sizeof(passData) % (4 * sizeof(float)) == 0,
      "RelaxSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&passData, divide_up(sizeof(passData), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    d3d::set_tex(STAGE_CS, 1, params.denoisedReflection, false);
    d3d::set_tex(STAGE_CS, 2, specIllumPing, false);
    d3d::set_tex(STAGE_CS, 3, specIllumPong, false);
    d3d::set_tex(STAGE_CS, 4, historyLength, false);
    d3d::set_rwtex(STAGE_CS, 0, relax_spec_illum_prev.getTex2D(), 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, relax_spec_illum_responsive_prev.getTex2D(), 0, 0);
    d3d::set_rwtex(STAGE_CS, 2, relax_spec_history_length_prev.getTex2D(), 0, 0);

    relaxSpecularHistoryClamping->dispatch(tilesW * 2, tilesH * 2, 1);
  }

  if (params.antiFirefly)
  {
    {
      TIME_D3D_PROFILE(relax::copy);

      struct PassData
      {
        RelaxSharedConstants relaxSharedConstants;
        uint32_t padding;
      } passData;

      passData.relaxSharedConstants = relaxSharedConstants;

      static_assert(sizeof(passData) % (4 * sizeof(float)) == 0,
        "RelaxSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");

      d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
      d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

      d3d::set_cb0_data(STAGE_CS, (const float *)&passData, divide_up(sizeof(passData), 16));
      d3d::set_tex(STAGE_CS, 0, relax_spec_illum_prev.getTex2D(), false);
      d3d::set_rwtex(STAGE_CS, 0, params.denoisedReflection, 0, 0);

      relaxSpecularCopy->dispatch(tilesW * 2, tilesH * 2, 1);
    }

    {
      TIME_D3D_PROFILE(relax::anti_firefly);

      struct PassData
      {
        RelaxSharedConstants relaxSharedConstants;
        uint32_t padding;
      } passData;

      passData.relaxSharedConstants = relaxSharedConstants;

      static_assert(sizeof(passData) % (4 * sizeof(float)) == 0,
        "RelaxSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");

      d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
      d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

      d3d::set_cb0_data(STAGE_CS, (const float *)&passData, divide_up(sizeof(passData), 16));
      d3d::set_tex(STAGE_CS, 0, tiles, false);
      d3d::set_tex(STAGE_CS, 1, params.denoisedReflection, false);
      d3d::set_tex(STAGE_CS, 2, normalRoughness, false);
      d3d::set_tex(STAGE_CS, 3, viewZ, false);
      d3d::set_rwtex(STAGE_CS, 0, relax_spec_illum_prev.getTex2D(), 0, 0);

      relaxSpecularAntiFirefly->dispatch(tilesW * 2, tilesH * 2, 1);
    }
  }

  struct ATorusData
  {
    RelaxSharedConstants relaxSharedConstants;
    uint32_t stepSize;
    uint32_t isLastPass;
    uint32_t padding[3];
  } atorusData;

  atorusData.relaxSharedConstants = relaxSharedConstants;
  atorusData.stepSize = 1;
  atorusData.isLastPass = 0;

  static_assert(sizeof(atorusData) % (4 * sizeof(float)) == 0,
    "RelaxSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");

  for (int iter = 0; iter < 5; ++iter)
  {
    TIME_D3D_PROFILE(relax::a - torus);

    atorusData.stepSize = 1 << iter;
    atorusData.isLastPass = (iter == 4) ? 1 : 0;

    bool isSmem = iter == 0;
    bool isEven = iter % 2 == 0;
    bool isLast = iter == 4;

    d3d::set_sampler(STAGE_CS, 0, samplerNearestClamp);
    d3d::set_sampler(STAGE_CS, 1, samplerLinearClamp);

    d3d::set_cb0_data(STAGE_CS, (const float *)&atorusData, divide_up(sizeof(atorusData), 16));
    d3d::set_tex(STAGE_CS, 0, tiles, false);
    if (isSmem)
      d3d::set_tex(STAGE_CS, 1, relax_spec_illum_prev.getTex2D(), false);
    else if (isEven)
      d3d::set_tex(STAGE_CS, 1, specIllumPong, false);
    else
      d3d::set_tex(STAGE_CS, 1, specIllumPing, false);
    d3d::set_tex(STAGE_CS, 2, historyLength, false);
    d3d::set_tex(STAGE_CS, 3, specReprojectionConfidence, false);
    d3d::set_tex(STAGE_CS, 4, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 5, viewZ, false);
    d3d::set_tex(STAGE_CS, 6, viewZ, false); // hasConfidenceInputs

    if (isLast)
      d3d::set_rwtex(STAGE_CS, 0, params.denoisedReflection, 0, 0);
    else if (isEven)
      d3d::set_rwtex(STAGE_CS, 0, specIllumPing, 0, 0);
    else
      d3d::set_rwtex(STAGE_CS, 0, specIllumPong, 0, 0);

    if (isSmem)
    {
      d3d::set_rwtex(STAGE_CS, 1, relax_spec_normal_roughness_prev.getTex2D(), 0, 0);
      d3d::set_rwtex(STAGE_CS, 2, relax_spec_material_id_prev.getTex2D(), 0, 0);
      d3d::set_rwtex(STAGE_CS, 3, relax_spec_prev_view_z.getTex2D(), 0, 0);
    }

    if (isSmem)
      relaxSpecularATorusSmem->dispatch(tilesW * 2, tilesH * 2, 1);
    else
      relaxSpecularATorus->dispatch(tilesW, tilesH, 1);
  }

  if (params.validationTexture)
  {
    TIME_D3D_PROFILE(relax::validation);

    struct PassData
    {
      RelaxSharedConstants relaxSharedConstants;
      uint32_t padding;
    } passData;

    passData.relaxSharedConstants = relaxSharedConstants;

    static_assert(sizeof(passData) % (4 * sizeof(float)) == 0,
      "RelaxSharedConstants size must be multiple of sizeof(float4) for d3d::set_cb0_data");

    d3d::set_cb0_data(STAGE_CS, (const float *)&passData, divide_up(sizeof(passData), 16));
    d3d::set_tex(STAGE_CS, 0, normalRoughness, false);
    d3d::set_tex(STAGE_CS, 1, viewZ, false);
    d3d::set_tex(STAGE_CS, 2, motionVectors, false);
    d3d::set_tex(STAGE_CS, 3, historyLength, false);
    d3d::set_rwtex(STAGE_CS, 0, params.validationTexture, 0, 0);

    relaxValidation->dispatch(tilesW * 2, tilesH, 1);
  }
}

void denoise_reflection(const ReflectionDenoiser &params)
{
  if (reset_history)
    clear_texture(params.denoisedReflection);

#if ENABLE_NRD_INTEGRATION_TEST
  if (params.useNRDLib)
    denoise_reflection_nrd_lib(params);
  else
#endif
    if (reblur_spec_history_confidence)
    denoise_reflection_reblur(params);
  else if (relax_spec_illum_prev)
    denoise_reflection_relax(params);
  else
    G_ASSERTF(false, "Reflection denoiser not implemented!");

  d3d::update_bindless_resource(bindless_range + rtr_bindless_index, params.denoisedReflection);
  ShaderGlobal::set_int(rtr_bindless_slotVarId, bindless_range + rtr_bindless_index);
}

TEXTUREID get_nr_texId(int w) { return w < denoiser::render_width ? half_normal_roughness.getTexId() : normal_roughness.getTexId(); }

TEXTUREID get_viewz_texId(int w) { return w < denoiser::render_width ? half_view_z.getTexId() : view_z.getTexId(); }

int get_frame_number() { return frame_counter; }

void get_memory_stats(int &common_size_mb, int &ao_size_mb, int &reflection_size_mb, int &transient_size_mb)
{
  auto ts = [](const UniqueTex &t) { return t ? t->ressize() : 0; };

  common_size_mb = ts(normal_roughness) + ts(view_z) + ts(half_normal_roughness) + ts(half_view_z);

  ao_size_mb = ts(ao_prev_view_z) + ts(ao_prev_normal_roughness) + ts(ao_prev_internal_data) + ts(ao_diff_fast_history);

  reflection_size_mb = ts(reblur_spec_history_confidence) + ts(reblur_spec_prev_view_z) + ts(reblur_spec_prev_normal_roughness) +
                       ts(reblur_spec_prev_internal_data) + ts(reblur_spec_history) + ts(reblur_spec_fast_history) +
                       ts(reblur_spec_hitdist_for_tracking_ping) + ts(reblur_spec_hitdist_for_tracking_pong) +
                       ts(relax_spec_illum_prev) + ts(relax_spec_illum_responsive_prev) + ts(relax_spec_reflection_hit_t_curr) +
                       ts(relax_spec_reflection_hit_t_prev) + ts(relax_spec_history_length_prev) +
                       ts(relax_spec_normal_roughness_prev) + ts(relax_spec_material_id_prev) + ts(relax_spec_prev_view_z);

  transient_size_mb = 0;
  for (auto &bucket : transient_textures)
    for (auto &tex : bucket.second)
      transient_size_mb += tex->ressize();

  common_size_mb /= 1024 * 1024;
  ao_size_mb /= 1024 * 1024;
  reflection_size_mb /= 1024 * 1024;
  transient_size_mb /= 1024 * 1024;
}

} // namespace denoiser