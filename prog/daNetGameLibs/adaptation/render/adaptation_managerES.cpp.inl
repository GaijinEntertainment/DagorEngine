// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "adaptation_manager.h"
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/updateStageRender.h>
#include <daECS/core/coreEvents.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_dynSceneRes.h>
#include <render/daFrameGraph/daFG.h>
#include <render/resourceSlot/registerAccess.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/renderSettings.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/aimRender.h>
#include <render/renderEvent.h>
#include <util/dag_convar.h>

extern ConVarB adaptationIlluminance;
CONSOLE_BOOL_VAL("render", adaptationInstant, false);

ECS_REGISTER_BOXED_TYPE(AdaptationManager, nullptr);
ECS_AUTO_REGISTER_COMPONENT(AdaptationManager, "adaptation__manager", nullptr, 0);

ECS_BROADCAST_EVENT_TYPE(AdaptationRebuildNodes)
ECS_REGISTER_EVENT(AdaptationRebuildNodes)

static ShaderVariableInfo g_ExposureVarId("Exposure", true);
static ShaderVariableInfo ExposureInVarId("ExposureIn");
static ShaderVariableInfo ExposureOutVarId("ExposureOut");
static ShaderVariableInfo adaptation_cw_samples_countVarId("adaptation_cw_samples_count", true);
static ShaderVariableInfo adaptation_use_luminance_trimmingVarId("adaptation_use_luminance_trimming");
static ShaderVariableInfo adaptation__includesHeroCockpitVarId("adaptation__includesHeroCockpit", true);
static ShaderVariableInfo adaptation__minSamplesVarId("adaptation__minSamples");
static ShaderVariableInfo EyeAdaptationParams_0VarId("EyeAdaptationParams_0");
static ShaderVariableInfo EyeAdaptationParams_1VarId("EyeAdaptationParams_1");
static ShaderVariableInfo EyeAdaptationParams_2VarId("EyeAdaptationParams_2");
static ShaderVariableInfo EyeAdaptationParams_3VarId("EyeAdaptationParams_3");

static ShaderVariableInfo adaptation_center_weighted_paramVarId("adaptation_center_weighted_param", true);
static ShaderVariableInfo adaptation_samples_countVarId("adaptation_samples_count", true);
static ShaderVariableInfo adaptation_uniform_samples_countVarId("adaptation_uniform_samples_count", true);
static ShaderVariableInfo adaptation_uniform_func_tanVarId("adaptation_uniform_func_tan", true);
static ShaderVariableInfo adaptation_nonlinear_func_fmaVarId("adaptation_nonlinear_func_fma", true);
static ShaderVariableInfo adaptation_nonlinear_func_powVarId("adaptation_nonlinear_func_pow", true);
static ShaderVariableInfo adaptation_sample_tm_xyVarId("adaptation_sample_tm_xy", true);
static ShaderVariableInfo adaptation_sample_tm_aVarId("adaptation_sample_tm_a", true);

static ShaderVariableInfo use_albedo_luma_adaptationVarId("use_albedo_luma_adaptation", true);
static ShaderVariableInfo const_frame_exposureVarId("const_frame_exposure", true);

template <typename Callable>
static void get_set_exposure_ecs_query(ecs::EntityManager &manager, Callable c);

template <typename Callable>
static void adaptation_node_init_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

template <typename Callable>
static void get_adaptation_manager_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

template <typename Callable>
static void get_adaptation_center_weight_override_ecs_query(ecs::EntityManager &manager, Callable c);

#define ADAPTATION_LOG_RANGE 15.0
const float kInitialMinLog = -12.0f;
const float kInitialMaxLog = kInitialMinLog + ADAPTATION_LOG_RANGE;

AdaptationManager::AdaptationManager()
{
  adaptExposureCS.reset(new_compute_shader("AdaptExposureCS"));
  if (!adaptExposureCS)
    return;

  g_Exposure = dag::buffers::create_ua_sr_structured(4, EXPOSURE_BUF_SIZE, "Exposure");
  g_NoExposureBuffer = dag::buffers::create_persistent_sr_structured(4, EXPOSURE_BUF_SIZE, "NoExposureBuffer");

  ShaderGlobal::set_buffer(g_ExposureVarId, g_Exposure);
  uploadInitialExposure();

  // generateHistogramCS = new_compute_shader("GenerateHistogramCS");
  generateHistogramCenterWeightedFromSourceCS.reset(new_compute_shader("GenerateHistogramCenterWeightedFromSourceCS"));
  accumulate_hist_cs.reset(new_compute_shader("accumulate_hist_cs"));

  static constexpr int exposureNormalizationFlags = TEXCF_RTARGET | TEXCF_UNORDERED | TEXFMT_R32F;
  exposureNormalizationFactor = dag::create_tex(NULL, 1, 1, exposureNormalizationFlags, 1, "exposure_normalization_factor");

  registerExposureNodeHandle = dafg::register_node("register_adaptation_resources", DAFG_PP_NODE_SRC,
    [normFactorView = ManagedTexView(exposureNormalizationFactor)](dafg::Registry registry) {
      registry.multiplex(dafg::multiplexing::Mode::None);
      registry.registerTexture("exposure_normalization_factor", [normFactorView](auto) -> ManagedTexView { return normFactorView; });
      registry.executionHas(dafg::SideEffects::External);
    });
}

void AdaptationManager::afterDeviceReset()
{
  uploadInitialExposure();
  if (exposureReadback)
    exposureReadback->reset();
}

void AdaptationManager::uploadInitialExposure()
{
  writeExposure(1);
  // fill no exposure buffer with lock, avoids any sync issues with writeExposure
  {
    alignas(16) ExposureBuffer initExposure = {
      1, 1, 1, 0.0f, kInitialMinLog, kInitialMaxLog, kInitialMaxLog - kInitialMinLog, 1.0f / (kInitialMaxLog - kInitialMinLog)};
    g_NoExposureBuffer->updateData(0, data_size(initExposure), initExposure.data(), VBLOCK_WRITEONLY);
  }
}

void AdaptationManager::clearNormalizationFactor()
{
  // Run only once after sheduleClear
  if (!isClearNeeded)
    return;

  isClearNeeded = false;
  float ones[4] = {1, 1, 1, 1};
  d3d::clear_rwtexf(exposureNormalizationFactor.getTex2D(), ones, 0, 0);
}

bool AdaptationManager::getExposure(float &exp) const
{
  const float fixedExp = settings.fixedExposure;

  if (fixedExp > 0)
  {
    exp = fixedExp;
    return true;
  }

  get_set_exposure_ecs_query(*g_entity_mgr, [&exp](float &adaptation__exposure) { exp = adaptation__exposure; });
  return bool(exposureReadback);
}

static inline eastl::array<Point2, 3> get_adaptation_sample_tm(const AdaptationSettings &settings, const CameraParams &prev_main_view)
{
  eastl::array<Point2, 3> adaptation_sample_tm = {Point2{0.5, 0}, Point2{0, 0.5}, Point2{0.5, 0.5}};
  if (!settings.focusOnAim)
    return adaptation_sample_tm;

  const AimRenderingData aimData = get_aim_rendering_data();
  if (!aimData.isAiming)
    return adaptation_sample_tm;

  if (!aimData.lensRenderEnabled)
  {
    if (aimData.entityWithScopeLensEid == ecs::INVALID_ENTITY_ID)
    { // With collimator or without scope
      adaptation_sample_tm[0] *= prev_main_view.noJitterPersp.wk / prev_main_view.noJitterPersp.hk * settings.focusAimScale.x;
      adaptation_sample_tm[1] *= settings.focusAimScale.y;
    }
    return adaptation_sample_tm;
  }

  const DynamicRenderableSceneInstance *scope = get_scope_lens(aimData);
  if (!scope)
    return adaptation_sample_tm;

  const float lensBoundingSphereRadius = aimData.lensBoundingSphereRadius;
  const TMatrix &lensPrevWtm = scope->getPrevNodeWtm(aimData.lensNodeId);
  const Point3 lensRight = -lensPrevWtm.col[0] * lensBoundingSphereRadius;
  const Point3 lensUp = lensPrevWtm.col[2] * lensBoundingSphereRadius;

  const Point3 centerWpos = lensPrevWtm.col[3];
  const Point3 rightWpos = centerWpos + lensRight;
  const Point3 upWpos = centerWpos + lensUp;

  const auto worldToUV = [&prev_main_view](const Point3 &wpos) {
    const Point4 posCs = Point4::xyz1(wpos) * prev_main_view.viewRotJitterProjTm;
    return safediv(Point2::xy(posCs), posCs.w);
  };

  const Point2 centerUV = worldToUV(centerWpos);
  adaptation_sample_tm[0] = (worldToUV(rightWpos) - centerUV) * 0.5;
  adaptation_sample_tm[1] = (worldToUV(upWpos) - centerUV) * 0.5;
  adaptation_sample_tm[2] = centerUV * 0.5 + Point2{0.5, 0.5};
  return adaptation_sample_tm;
}

static inline void set_adaptation_sample_tm_vars(const AdaptationSettings &settings, const CameraParams &prev_main_view)
{
  eastl::array<Point2, 3> adaptation_sample_tm = get_adaptation_sample_tm(settings, prev_main_view);
  ShaderGlobal::set_float4(adaptation_sample_tm_xyVarId, adaptation_sample_tm[0], adaptation_sample_tm[1].x,
    adaptation_sample_tm[1].y);
  ShaderGlobal::set_float4(adaptation_sample_tm_aVarId, adaptation_sample_tm[2]);
}

// Generate an HDR histogram
void AdaptationManager::genHistogramFromSource()
{
  { // legacy center weighted function params for backward compatibility
    float centerWeight = settings.centerWeight;
    // NOTE: If multiple entities have this override component, only a random one will be selected.
    // if that is going to be a problem, you can add another ecs component with priority.
    get_adaptation_center_weight_override_ecs_query(*g_entity_mgr,
      [&centerWeight](float adaptation__centerWeightOverride) { centerWeight = adaptation__centerWeightOverride; });
    ShaderGlobal::set_float(adaptation_center_weighted_paramVarId, centerWeight);
    ShaderGlobal::set_float(adaptation_cw_samples_countVarId, DispatchInfo::pixelsCount);
  }

  const float R = settings.fnNonLinearDistributionRadius;
  const float r = settings.fnUniformDistributionRadius;
  const float s = settings.fnUniformSamplesPercentage * 0.01f;
  const float k = s > 0.0f ? r / sqrt(s) : 0.0f;
  const float p = settings.fnNonLinearDistributionCurvinesPow;
  const Point2 fma = s > 0.0f ? Point2{R, -1.0f * (R - k * sqrt(s)) / pow(1.0f - s, p)} : Point2{R, 0.0f};

  ShaderGlobal::set_float(adaptation_samples_countVarId, DispatchInfo::pixelsCount);
  ShaderGlobal::set_float(adaptation_uniform_samples_countVarId, (float)DispatchInfo::pixelsCount * s);
  ShaderGlobal::set_float(adaptation_uniform_func_tanVarId, k);
  ShaderGlobal::set_float4(adaptation_nonlinear_func_fmaVarId, Point4{fma.x, fma.y, 0.0f, 0.0f});
  ShaderGlobal::set_float(adaptation_nonlinear_func_powVarId, p);

  ShaderGlobal::set_buffer(ExposureInVarId, g_Exposure);
  generateHistogramCenterWeightedFromSourceCS->dispatch(DispatchInfo::groupsCount, 1, 1);
}

void AdaptationManager::accumulateHistogram() { accumulate_hist_cs->dispatch(1, 1, 1); }

void AdaptationManager::updateExposure()
{
  if (isFixedExposure())
  {
    float exposure = settings.fixedExposure;
    if (exposure < 0)
      exposure = 1.0f;

    exposure = exposure * settings.fadeMul;
    if (lastFixedExposure != exposure)
    {
      if (writeExposure(exposure))
        lastFixedExposure = exposure;
    }
    return;
  }
  if (!exposureWritten)
    writeExposure(1);
  lastFixedExposure = -1;
}

void AdaptationManager::setExposure(bool value)
{
  if (!value)
  {
    ShaderGlobal::set_float(const_frame_exposureVarId, 1.0f);
    ShaderGlobal::set_buffer(g_ExposureVarId, g_NoExposureBuffer);
  }
  else
  {
    float exp = 1;
    getExposure(exp);
    ShaderGlobal::set_float(const_frame_exposureVarId, exp);
    ShaderGlobal::set_buffer(g_ExposureVarId, g_Exposure);
  }
}

bool AdaptationManager::writeExposure(float exposure)
{
  // this is not correct, we can't know for sure that prev frame Exposure was same (i.e. initExposure[3] = 1./exposure is not correct)
  // todo: correct way is continue call shader (different one, which will write buffer[3] = prevBuffer[1], and write everything else)
  alignas(16) ExposureBuffer initExposure = {exposure, 1.0f / exposure, 1, 1.0f / exposure, kInitialMinLog, kInitialMaxLog,
    kInitialMaxLog - kInitialMinLog, 1.0f / (kInitialMaxLog - kInitialMinLog), 1.f, 1.f};

  if (g_Exposure.getBuf()->updateData(0, data_size(initExposure), initExposure.data(), VBLOCK_WRITEONLY))
  {
    get_set_exposure_ecs_query(*g_entity_mgr, [&exposure](float &adaptation__exposure) { adaptation__exposure = exposure; });
    exposureWritten = true;
  }
  else
  {
    debug("Adaptation: can't lock buffer for exposure");
    exposureWritten = false;
  }
  return exposureWritten;
}

void AdaptationManager::adaptExposure()
{
  if (settings.useLuminanceTrimming && settings.luminanceLowRange >= settings.luminanceHighRange)
    LOGERR_ONCE("adaptation: luminanceLowRange=%@ should be lower than luminanceHighRange=%@", settings.luminanceLowRange,
      settings.luminanceHighRange);

  ShaderGlobal::set_float(adaptation_use_luminance_trimmingVarId, settings.useLuminanceTrimming ? 1.0f : 0.0f);
  ShaderGlobal::set_float4(EyeAdaptationParams_0VarId, settings.luminanceLowRange, settings.luminanceHighRange, settings.minExposure,
    settings.maxExposure);
  ShaderGlobal::set_float4(EyeAdaptationParams_1VarId, settings.autoExposureScale, accumulatedTime, settings.adaptDownSpeed,
    settings.adaptUpSpeed);
  ShaderGlobal::set_float4(EyeAdaptationParams_2VarId, 1, 0, float(DispatchInfo::pixelsCount), settings.fadeMul);
  ShaderGlobal::set_float4(EyeAdaptationParams_3VarId, settings.brightnessPerceptionLinear, settings.brightnessPerceptionPower, 0.,
    adaptationInstant.get() ? 1.f : 0.f);
  d3d::set_rwtex(STAGE_CS, 1, exposureNormalizationFactor.getTex2D(), 0, 0);
  ShaderGlobal::set_buffer(ExposureOutVarId, g_Exposure);
  adaptExposureCS->dispatch(1, 1, 1);
  d3d::set_rwtex(STAGE_CS, 1, nullptr, 0, 0);
  d3d::resource_barrier({exposureNormalizationFactor.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  accumulatedTime = 0;
}

void AdaptationManager::updateReadbackExposure()
{
  updateExposure();

  if (isFixedExposure())
    return;

  if (!exposureReadback)
  {
    exposureReadback.reset(new RingCPUBufferLock());
    exposureReadback->init(4, EXPOSURE_BUF_SIZE, 4, "exposureReadBack", SBCF_UA_STRUCTURED_READBACK, 0, false);
  }
  int stride;
  uint32_t latestReadbackFrameRead;
  if (float *data = (float *)exposureReadback->lock(stride, latestReadbackFrameRead, true))
  {
    // TODO we should set lastValidReadbackFrame as latestReadbackFrameCopied + 1 at writeExposure method to make it codition make
    // sence.
    if (lastValidReadbackFrame <= latestReadbackFrameRead)
      get_set_exposure_ecs_query(*g_entity_mgr,
        [data](float &adaptation__exposure) { adaptation__exposure = clamp(*(data), 1e-6f, 1e6f); });

    if (DAGOR_UNLIKELY(isDumpExposureBuffer))
    {
      for (uint32_t i = 0; i < EXPOSURE_BUF_SIZE; ++i)
        debug("Exposure[%d]: %f", i, data[i]);
      isDumpExposureBuffer = false;
    }

    memcpy(lastExposureBuffer.data(), data, sizeof(lastExposureBuffer));

    exposureReadback->unlock();
  }

  Sbuffer *newTarget = (Sbuffer *)exposureReadback->getNewTarget(latestReadbackFrameCopied);
  if (!newTarget)
    return;
  g_Exposure->copyTo(newTarget);
  exposureReadback->startCPUCopy();
}

static dafg::NodeHandle makeUpdateReadbackExposureNode(
  AdaptationManager &adaptation__manager, bool adaptation, bool gpu_resident_adaptation)
{
  return dafg::register_node("update_readback_exposure", DAFG_PP_NODE_SRC,
    [&adaptation__manager, adaptation, gpu_resident_adaptation](dafg::Registry registry) {
      registry.orderMeAfter("post_fx_node");
      registry.executionHas(dafg::SideEffects::External);

      return [&adaptation__manager, adaptation, gpu_resident_adaptation]() {
        if (adaptation)
          adaptation__manager.updateReadbackExposure();

        if (!gpu_resident_adaptation)
        {
          float exp = 1;
          adaptation__manager.getExposure(exp);
          ShaderGlobal::set_float(const_frame_exposureVarId, exp);
        }
      };
    });
}

static resource_slot::NodeHandleWithSlotsAccess makeGenHistogramNode(AdaptationManager &adaptation__manager, bool full_deferred)
{
  return resource_slot::register_access("gen_histogram", DAFG_PP_NODE_SRC, {resource_slot::Read{"postfx_input_slot"}},
    [&adaptation__manager, full_deferred](resource_slot::State slotsState, dafg::Registry registry) {
      registry.orderMeAfter("prepare_post_fx_node");

      read_gbuffer(registry, dafg::Stage::PS, readgbuffer::ALBEDO);

      registry.readTexture("close_depth").atStage(dafg::Stage::COMPUTE).bindToShaderVar("downsampled_close_depth_tex").optional();
      registry.read("close_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
        .optional();

      registry.readTexture(slotsState.resourceToReadFrom("postfx_input_slot"))
        .atStage(dafg::Stage::COMPUTE)
        .bindToShaderVar("frame_tex");
      if (!adaptation__manager.getSettings().includesHeroCockpit)
        registry.readTexture("gbuf_2").atStage(dafg::Stage::COMPUTE).bindToShaderVar("material_gbuf");
      registry.create("histogram_frame_sampler").blob(d3d::request_sampler({})).bindToShaderVar("frame_tex_samplerstate");

      registry.readTexture("prev_frame_tex").atStage(dafg::Stage::COMPUTE).bindToShaderVar("no_effects_frame_tex").optional();
      registry.read("prev_frame_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("no_effects_frame_tex_samplerstate").optional();
      registry.modify("raw_exposure_histogram").buffer().atStage(dafg::Stage::CS).bindToShaderVar("RawHistogram");
      auto prevMainCameraHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();

      return [&adaptation__manager, full_deferred, prevMainCameraHndl]() {
        if (adaptation__manager.isFixedExposure())
          return;

        const bool useBaseColorAdaptation = !adaptationIlluminance.get() && full_deferred;
        ShaderGlobal::set_int(use_albedo_luma_adaptationVarId, useBaseColorAdaptation ? 1 : 0);
        ShaderGlobal::set_int(adaptation__includesHeroCockpitVarId, adaptation__manager.getSettings().includesHeroCockpit);
        ShaderGlobal::set_float(adaptation__minSamplesVarId, adaptation__manager.getSettings().minSamples);
        set_adaptation_sample_tm_vars(adaptation__manager.getSettings(), prevMainCameraHndl.ref());

        adaptation__manager.genHistogramFromSource();
      };
    });
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady, AdaptationRebuildNodes)
ECS_TRACK(render_settings__gpuResidentAdaptation, render_settings__adaptation, render_settings__fullDeferred)
static void adaptation_settings_tracking_es(const ecs::Event &,
  ecs::EntityManager &manager,
  bool render_settings__gpuResidentAdaptation,
  bool render_settings__adaptation,
  bool render_settings__fullDeferred)
{
  adaptation_node_init_ecs_query(manager, manager.getSingletonEntity(ECS_HASH("adaptation_manager")),
    [&](AdaptationManager &adaptation__manager, dafg::NodeHandle &adaptation__update_readback_exposure_node,
      dafg::NodeHandle &adaptation__create_histogram_node, dafg::NodeHandle &adaptation__gen_histogram_forward_node,
      resource_slot::NodeHandleWithSlotsAccess &adaptation__gen_histogram_node, dafg::NodeHandle &adaptation__accumulate_histogram,
      dafg::NodeHandle &adaptation__adapt_exposure_node, dafg::NodeHandle &adaptation__set_exposure_node) {
      adaptation__update_readback_exposure_node =
        makeUpdateReadbackExposureNode(adaptation__manager, render_settings__adaptation, render_settings__gpuResidentAdaptation);

      adaptation__manager.sheduleClear();

      // Set exposure at beginnig of frame
      adaptation__set_exposure_node = dafg::register_node("set_exposure", DAFG_PP_NODE_SRC,
        [&adaptation__manager, render_settings__gpuResidentAdaptation](dafg::Registry registry) {
          registry.orderMeBefore("setup_world_rendering_node");
          registry.modify("exposure_normalization_factor").buffer().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE);

          return [&adaptation__manager, render_settings__gpuResidentAdaptation]() {
            adaptation__manager.clearNormalizationFactor();

            if (!render_settings__gpuResidentAdaptation)
            {
              float exp = 1;
              adaptation__manager.getExposure(exp);
              ShaderGlobal::set_float(const_frame_exposureVarId, exp);
            }

            ShaderGlobal::set_buffer(g_ExposureVarId, adaptation__manager.getExposureBufferId());
          };
        });

      adaptation__create_histogram_node = {};
      adaptation__gen_histogram_node = {};
      adaptation__gen_histogram_forward_node = {};
      adaptation__accumulate_histogram = {};
      adaptation__adapt_exposure_node = {};

      if (!render_settings__adaptation)
        return;

      adaptation__create_histogram_node =
        dafg::register_node("clear_histogram", DAFG_PP_NODE_SRC, [&adaptation__manager](dafg::Registry registry) {
          const auto histogram_hndl = registry.create("raw_exposure_histogram")
                                        .structuredBuffer<uint32_t>(256 * 32)
                                        .atStage(dafg::Stage::CS)
                                        .useAs(dafg::Usage::SHADER_RESOURCE)
                                        .handle();

          return [&adaptation__manager, histogram_hndl]() {
            if (adaptation__manager.isFixedExposure())
              return;

            d3d::zero_rwbufi(histogram_hndl.view().getBuf());
          };
        });

      adaptation__gen_histogram_node = makeGenHistogramNode(adaptation__manager, render_settings__fullDeferred);

      adaptation__accumulate_histogram =
        dafg::register_node("accumulate_histogram", DAFG_PP_NODE_SRC, [&adaptation__manager](dafg::Registry registry) {
          registry.read("raw_exposure_histogram").buffer().atStage(dafg::Stage::CS).bindToShaderVar("RawHistogram");

          registry.create("exposure_histogram").structuredBuffer<uint32_t>(256).atStage(dafg::Stage::CS).bindToShaderVar("Histogram");

          return [&adaptation__manager]() {
            if (adaptation__manager.isFixedExposure())
              return;

            adaptation__manager.accumulateHistogram();
          };
        });

      adaptation__adapt_exposure_node =
        dafg::register_node("adapt_exposure", DAFG_PP_NODE_SRC, [&adaptation__manager](dafg::Registry registry) {
          registry.orderMeBefore("post_fx_node");

          registry.read("exposure_histogram").buffer().atStage(dafg::Stage::CS).bindToShaderVar("Histogram");

          return [&adaptation__manager]() {
            if (adaptation__manager.isFixedExposure())
              return;

            // Note: with multiplexing, we will essentially write to the readback buffer multiple times.
            // This is slightly inaccurate, but probably fine, as on average, histograms will be the same.

            adaptation__manager.adaptExposure();
          };
        });
    });
};


static void overrideAdaptationSetting(AdaptationSettings &settings, const ecs::Object &config)
{
#define READ_REAL(group, name) settings.name = config[ECS_HASH(group "__" #name)].getOr<float>(settings.name)
#define READ_BOOL(group, name) settings.name = config[ECS_HASH(group "__" #name)].getOr<bool>(settings.name)

  if (config.getMemberOr(ECS_HASH("adaptation__on"), settings.fixedExposure < 0))
  {
    settings.fixedExposure = -1;
    READ_BOOL("adaptation", useLuminanceTrimming);
    READ_BOOL("adaptation", includesHeroCockpit);
    READ_BOOL("adaptation", focusOnAim);
    settings.focusAimScale = config[ECS_HASH("adaptation__focusAimScale")].getOr<Point2>(settings.focusAimScale);
    READ_REAL("adaptation", minSamples);
    READ_REAL("adaptation", autoExposureScale);
    READ_REAL("adaptation", luminanceLowRange);
    READ_REAL("adaptation", luminanceHighRange);
    READ_REAL("adaptation", minExposure);
    READ_REAL("adaptation", maxExposure);
    READ_REAL("adaptation", adaptDownSpeed);
    READ_REAL("adaptation", adaptUpSpeed);
    READ_REAL("adaptation", brightnessPerceptionLinear);
    READ_REAL("adaptation", brightnessPerceptionPower);
    READ_REAL("adaptation", fadeMul);
    READ_REAL("adaptation", centerWeight);
    READ_REAL("adaptation", fnUniformSamplesPercentage);
    READ_REAL("adaptation", fnUniformDistributionRadius);
    READ_REAL("adaptation", fnNonLinearDistributionRadius);
    READ_REAL("adaptation", fnNonLinearDistributionCurvinesPow);
  }
  else
  {
    READ_REAL("adaptation", fixedExposure);
    if (settings.fixedExposure < 0)
      settings.fixedExposure = 1.0;
  }
#undef READ_REAL
}

template <typename Callable>
static void get_default_settings_ecs_query(ecs::EntityManager &manager, Callable c);

template <typename Callable>
static void get_level_settings_ecs_query(ecs::EntityManager &manager, Callable c);

template <typename Callable>
static void get_override_settings_ecs_query(ecs::EntityManager &manager, Callable c);

bool AdaptationManager::setSettings(const AdaptationSettings &value)
{
  bool needRebuildNodes = settings.includesHeroCockpit != value.includesHeroCockpit;
  settings = value;
  return needRebuildNodes;
}

static void updateSettings(ecs::EntityManager &manager)
{
  get_adaptation_manager_ecs_query(manager, manager.getSingletonEntity(ECS_HASH("adaptation_manager")),
    [&](AdaptationManager &adaptation__manager) {
      AdaptationSettings settings;

      get_default_settings_ecs_query(manager,
        [&](const ecs::Object &adaptation_default_settings) { overrideAdaptationSetting(settings, adaptation_default_settings); });

      get_level_settings_ecs_query(manager,
        [&](const ecs::Object &adaptation_level_settings) { overrideAdaptationSetting(settings, adaptation_level_settings); });

      get_override_settings_ecs_query(manager,
        [&](const ecs::Object &adaptation_override_settings) { overrideAdaptationSetting(settings, adaptation_override_settings); });

      if (adaptation__manager.setSettings(settings)) // Need rebuild nodes
        manager.broadcastEventImmediate(AdaptationRebuildNodes());
    });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, on_disappear)
ECS_TRACK(adaptation_level_settings)
ECS_REQUIRE(ecs::Object adaptation_level_settings)
static void adaptation_level_settings_es(const ecs::Event &, ecs::EntityManager &manager) { updateSettings(manager); }

ECS_TAG(render)
ECS_ON_EVENT(on_appear, on_disappear)
ECS_TRACK(adaptation_default_settings)
ECS_REQUIRE(ecs::Object adaptation_default_settings)
static void adaptation_default_settings_es(const ecs::Event &, ecs::EntityManager &manager) { updateSettings(manager); }

ECS_TAG(render)
ECS_ON_EVENT(on_appear, on_disappear)
ECS_TRACK(adaptation_override_settings)
ECS_REQUIRE(ecs::Object adaptation_override_settings)
static void adaptation_override_settings_es(const ecs::Event &, ecs::EntityManager &manager) { updateSettings(manager); }

ECS_TAG(render)
ECS_ON_EVENT(AfterDeviceReset)
static void adaptation_after_device_reset_es(const ecs::Event &, AdaptationManager &adaptation__manager)
{
  adaptation__manager.afterDeviceReset();
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void adaptation_update_time_es(const UpdateStageInfoBeforeRender &evt,
  ecs::EntityManager &manager,
  AdaptationManager &adaptation__manager,
  bool adaptation__track_changes)
{
  if (adaptation__track_changes)
    updateSettings(manager);

  adaptation__manager.updateTime(evt.realDt);
}

ECS_TAG(render)
static void on_reset_exposure_evt_es(const RenderReinitCube &, AdaptationManager &adaptation__manager)
{
  adaptation__manager.writeExposure(1.0);
}

ECS_TAG(render)
static void on_set_no_exposure_evt_es(const RenderSetExposure &evnt, AdaptationManager &adaptation__manager)
{
  adaptation__manager.setExposure(evnt.value);
}

#if DAGOR_DBGLEVEL == 0
// Make sure the entity can be created even in release binaries with -dev flag.
class AdaptationDebug
{};
ECS_DECLARE_BOXED_TYPE(AdaptationDebug);
ECS_REGISTER_BOXED_TYPE(AdaptationDebug, nullptr);

class PostfxLevelsDebug
{};
ECS_DECLARE_BOXED_TYPE(PostfxLevelsDebug);
ECS_REGISTER_BOXED_TYPE(PostfxLevelsDebug, nullptr);
#endif
