// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "adaptation_manager.h"
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <ecs/core/entityManager.h>
#include <ecs/render/updateStageRender.h>
#include <daECS/core/coreEvents.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <render/daBfg/bfg.h>
#include <render/resourceSlot/registerAccess.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/renderSettings.h>
#include <render/world/frameGraphHelpers.h>
#include <render/renderEvent.h>
#include <util/dag_convar.h>

extern ConVarB adaptationIlluminance;
CONSOLE_BOOL_VAL("render", adaptationInstant, false);

ECS_REGISTER_BOXED_TYPE(AdaptationManager, nullptr);
ECS_AUTO_REGISTER_COMPONENT(AdaptationManager, "adaptation__manager", nullptr, 0);

static ShaderVariableInfo g_ExposureVarId("Exposure", true);
static ShaderVariableInfo ExposureInVarId("ExposureIn");
static ShaderVariableInfo ExposureOutVarId("ExposureOut");
static ShaderVariableInfo adaptation_cw_samples_countVarId("adaptation_cw_samples_count");
static ShaderVariableInfo EyeAdaptationParams_0VarId("EyeAdaptationParams_0");
static ShaderVariableInfo EyeAdaptationParams_1VarId("EyeAdaptationParams_1");
static ShaderVariableInfo EyeAdaptationParams_2VarId("EyeAdaptationParams_2");
static ShaderVariableInfo EyeAdaptationParams_3VarId("EyeAdaptationParams_3");
static ShaderVariableInfo adaptation_center_weighted_paramVarId("adaptation_center_weighted_param");

static ShaderVariableInfo use_albedo_luma_adaptationVarId("use_albedo_luma_adaptation", true);
static ShaderVariableInfo const_frame_exposureVarId("const_frame_exposure", true);

template <typename Callable>
static void get_set_exposure_ecs_query(Callable c);

template <typename Callable>
static void adaptation_node_init_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
static void get_adaptation_manager_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
static void get_adaptation_center_weight_override_ecs_query(Callable c);

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

  static constexpr int exposureTextureFlags = TEXCF_RTARGET | TEXCF_UNORDERED | TEXFMT_R32F;
  exposureTex = dag::create_tex(NULL, 1, 1, exposureTextureFlags, 1, "exposure_tex");

  registerExposureNodeHandle = dabfg::register_node("register_adapatation_resoureces", DABFG_PP_NODE_SRC,
    [exposure = ManagedTexView(exposureTex)](dabfg::Registry registry) {
      registry.multiplex(dabfg::multiplexing::Mode::None);
      registry.registerTexture2d("exposure_tex", [exposure](auto) -> ManagedTexView { return exposure; });
      registry.executionHas(dabfg::SideEffects::External);
    });
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

void AdaptationManager::clearExposureTex()
{
  // Run only once after sheduleClear
  if (!isClearNeeded)
    return;

  isClearNeeded = false;
  float ones[4] = {1, 1, 1, 1};
  d3d::clear_rwtexf(exposureTex.getTex2D(), ones, 0, 0);
}

bool AdaptationManager::getExposure(float &exp) const
{
  const float fixedExp = settings.fixedExposure;

  if (fixedExp > 0)
  {
    exp = fixedExp;
    return true;
  }

  get_set_exposure_ecs_query([&exp](float &adaptation__exposure) { exp = adaptation__exposure; });
  return bool(exposureReadback);
}

// Generate an HDR histogram
void AdaptationManager::genHistogramFromSource()
{
  ShaderGlobal::set_real(adaptation_cw_samples_countVarId, DispatchInfo::pixelsCount);
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
    ShaderGlobal::set_real(const_frame_exposureVarId, 1.0f);
    ShaderGlobal::set_buffer(g_ExposureVarId, g_NoExposureBuffer);
  }
  else
  {
    float exp = 1;
    getExposure(exp);
    ShaderGlobal::set_real(const_frame_exposureVarId, exp);
    ShaderGlobal::set_buffer(g_ExposureVarId, g_Exposure);
  }
}

bool AdaptationManager::writeExposure(float exposure)
{
  alignas(16) ExposureBuffer initExposure = {exposure, 1.0f / exposure, 1, 0.0f, kInitialMinLog, kInitialMaxLog,
    kInitialMaxLog - kInitialMinLog, 1.0f / (kInitialMaxLog - kInitialMinLog)};

  if (g_Exposure.getBuf()->updateData(0, data_size(initExposure), initExposure.data(), VBLOCK_WRITEONLY))
  {
    get_set_exposure_ecs_query([&exposure](float &adaptation__exposure) { adaptation__exposure = exposure; });
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
  ShaderGlobal::set_color4(EyeAdaptationParams_0VarId, settings.lowPart, settings.highPart, settings.minExposure,
    settings.maxExposure);
  ShaderGlobal::set_color4(EyeAdaptationParams_1VarId, settings.autoExposureScale, accumulatedTime, settings.adaptDownSpeed,
    settings.adaptUpSpeed);
  ShaderGlobal::set_color4(EyeAdaptationParams_2VarId, 1, 0, float(DispatchInfo::pixelsCount), settings.fadeMul);
  ShaderGlobal::set_color4(EyeAdaptationParams_3VarId, settings.brightnessPerceptionLinear, settings.brightnessPerceptionPower, 0.,
    adaptationInstant.get() ? 1.f : 0.f);
  d3d::set_rwtex(STAGE_CS, 1, exposureTex.getTex2D(), 0, 0);
  ShaderGlobal::set_buffer(ExposureOutVarId, g_Exposure);
  adaptExposureCS->dispatch(1, 1, 1);
  d3d::set_rwtex(STAGE_CS, 1, nullptr, 0, 0);
  d3d::resource_barrier({exposureTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

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
      get_set_exposure_ecs_query([data](float &adaptation__exposure) { adaptation__exposure = clamp(*(data), 1e-6f, 1e6f); });

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

static dabfg::NodeHandle makeUpdateReadbackExposureNode(
  AdaptationManager &adaptation__manager, bool adaptation, bool gpu_resident_adaptation, bool forward_rendering)
{
  return dabfg::register_node("update_readback_exposure", DABFG_PP_NODE_SRC,
    [&adaptation__manager, adaptation, gpu_resident_adaptation, forward_rendering](dabfg::Registry registry) {
      registry.orderMeAfter(forward_rendering ? "postfx_mobile" : "post_fx_node");
      registry.executionHas(dabfg::SideEffects::External);

      return [&adaptation__manager, adaptation, gpu_resident_adaptation]() {
        if (adaptation)
          adaptation__manager.updateReadbackExposure();

        if (!gpu_resident_adaptation)
        {
          float exp = 1;
          adaptation__manager.getExposure(exp);
          ShaderGlobal::set_real(const_frame_exposureVarId, exp);
        }
      };
    });
}

static dabfg::NodeHandle makeGenHistogramForwardNode(AdaptationManager &adaptation__manager, bool full_deferred)
{
  return dabfg::register_node("gen_histogram", DABFG_PP_NODE_SRC, [&adaptation__manager, full_deferred](dabfg::Registry registry) {
    registry.orderMeBefore("postfx_mobile");

    registry.readTexture("postfx_input").atStage(dabfg::Stage::CS).bindToShaderVar("frame_tex");

    registry.readTexture("prev_frame_tex").atStage(dabfg::Stage::COMPUTE).bindToShaderVar("no_effects_frame_tex").optional();
    registry.read("prev_frame_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("no_effects_frame_tex_samplerstate").optional();
    registry.modify("raw_exposure_histogram").buffer().atStage(dabfg::Stage::CS).bindToShaderVar("RawHistogram");

    return [&adaptation__manager, full_deferred]() {
      if (adaptation__manager.isFixedExposure())
        return;

      const bool useBaseColorAdaptation = !adaptationIlluminance.get() && full_deferred;
      ShaderGlobal::set_int(use_albedo_luma_adaptationVarId, useBaseColorAdaptation ? 1 : 0);
      adaptation__manager.genHistogramFromSource();
    };
  });
}

static resource_slot::NodeHandleWithSlotsAccess makeGenHistogramNode(AdaptationManager &adaptation__manager, bool full_deferred)
{
  return resource_slot::register_access("gen_histogram", DABFG_PP_NODE_SRC, {resource_slot::Read{"postfx_input_slot"}},
    [&adaptation__manager, full_deferred](resource_slot::State slotsState, dabfg::Registry registry) {
      registry.orderMeAfter("prepare_post_fx_node");

      read_gbuffer(registry, dabfg::Stage::PS, readgbuffer::ALBEDO);

      registry.readTexture("close_depth").atStage(dabfg::Stage::COMPUTE).bindToShaderVar("downsampled_close_depth_tex").optional();
      registry.read("close_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_close_depth_tex_samplerstate")
        .optional();

      registry.readTexture(slotsState.resourceToReadFrom("postfx_input_slot"))
        .atStage(dabfg::Stage::COMPUTE)
        .bindToShaderVar("frame_tex");
      registry.create("histogram_frame_sampler", dabfg::History::No)
        .blob(d3d::request_sampler({}))
        .bindToShaderVar("frame_tex_samplerstate");

      registry.readTexture("prev_frame_tex").atStage(dabfg::Stage::COMPUTE).bindToShaderVar("no_effects_frame_tex").optional();
      registry.read("prev_frame_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("no_effects_frame_tex_samplerstate").optional();
      registry.modify("raw_exposure_histogram").buffer().atStage(dabfg::Stage::CS).bindToShaderVar("RawHistogram");

      return [&adaptation__manager, full_deferred]() {
        if (adaptation__manager.isFixedExposure())
          return;

        const bool useBaseColorAdaptation = !adaptationIlluminance.get() && full_deferred;
        ShaderGlobal::set_int(use_albedo_luma_adaptationVarId, useBaseColorAdaptation ? 1 : 0);
        float centerWeight = adaptation__manager.getSettings().centerWeight;
        // NOTE: If multiple entities have this override component, only a random one will be selected.
        // if that is going to be a problem, you can add another ecs component with priority.
        get_adaptation_center_weight_override_ecs_query(
          [&centerWeight](float adaptation__centerWeightOverride) { centerWeight = adaptation__centerWeightOverride; });
        ShaderGlobal::set_real(adaptation_center_weighted_paramVarId, centerWeight);
        adaptation__manager.genHistogramFromSource();
      };
    });
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__gpuResidentAdaptation,
  render_settings__adaptation,
  render_settings__fullDeferred,
  render_settings__forwardRendering)
static void adaptation_settings_tracking_es(const ecs::Event &,
  bool render_settings__gpuResidentAdaptation,
  bool render_settings__adaptation,
  bool render_settings__fullDeferred,
  bool render_settings__forwardRendering)
{
  adaptation_node_init_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("adaptation_manager")),
    [&](AdaptationManager &adaptation__manager, dabfg::NodeHandle &adaptation__update_readback_exposure_node,
      dabfg::NodeHandle &adaptation__create_histogram_node, dabfg::NodeHandle &adaptation__gen_histogram_forward_node,
      resource_slot::NodeHandleWithSlotsAccess &adaptation__gen_histogram_node, dabfg::NodeHandle &adaptation__accumulate_histogram,
      dabfg::NodeHandle &adaptation__adapt_exposure_node, dabfg::NodeHandle &adaptation__set_exposure_node) {
      adaptation__update_readback_exposure_node = makeUpdateReadbackExposureNode(adaptation__manager, render_settings__adaptation,
        render_settings__gpuResidentAdaptation, render_settings__forwardRendering);

      adaptation__manager.sheduleClear();

      // Set exposure at beginnig of frame
      adaptation__set_exposure_node = dabfg::register_node("set_exposure", DABFG_PP_NODE_SRC,
        [&adaptation__manager, render_settings__gpuResidentAdaptation, render_settings__forwardRendering](dabfg::Registry registry) {
          registry.orderMeBefore(render_settings__forwardRendering ? "frame_data_setup_mobile" : "setup_world_rendering_node");
          registry.modify("exposure_tex").buffer().atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE);

          return [&adaptation__manager, render_settings__gpuResidentAdaptation]() {
            adaptation__manager.clearExposureTex();

            if (!render_settings__gpuResidentAdaptation)
            {
              float exp = 1;
              adaptation__manager.getExposure(exp);
              ShaderGlobal::set_real(const_frame_exposureVarId, exp);
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
        dabfg::register_node("clear_histogram", DABFG_PP_NODE_SRC, [&adaptation__manager](dabfg::Registry registry) {
          const auto histogram_hndl = registry.create("raw_exposure_histogram", dabfg::History::No)
                                        .structuredBufferUaSr<uint32_t>(256 * 32)
                                        .atStage(dabfg::Stage::CS)
                                        .useAs(dabfg::Usage::SHADER_RESOURCE)
                                        .handle();

          return [&adaptation__manager, histogram_hndl]() {
            if (adaptation__manager.isFixedExposure())
              return;

            uint32_t v[4] = {0, 0, 0, 0};
            d3d::clear_rwbufi(histogram_hndl.view().getBuf(), v);
          };
        });

      if (render_settings__forwardRendering)
      {
        adaptation__gen_histogram_forward_node = makeGenHistogramForwardNode(adaptation__manager, render_settings__fullDeferred);
      }
      else
      {
        adaptation__gen_histogram_node = makeGenHistogramNode(adaptation__manager, render_settings__fullDeferred);
      }

      adaptation__accumulate_histogram =
        dabfg::register_node("accumulate_histogram", DABFG_PP_NODE_SRC, [&adaptation__manager](dabfg::Registry registry) {
          registry.read("raw_exposure_histogram").buffer().atStage(dabfg::Stage::CS).bindToShaderVar("RawHistogram");

          registry.create("exposure_histogram", dabfg::History::No)
            .structuredBufferUaSr<uint32_t>(256)
            .atStage(dabfg::Stage::CS)
            .bindToShaderVar("Histogram");

          return [&adaptation__manager]() {
            if (adaptation__manager.isFixedExposure())
              return;

            adaptation__manager.accumulateHistogram();
          };
        });

      adaptation__adapt_exposure_node = dabfg::register_node("adapt_exposure", DABFG_PP_NODE_SRC,
        [&adaptation__manager, render_settings__forwardRendering](dabfg::Registry registry) {
          registry.orderMeBefore(render_settings__forwardRendering ? "postfx_mobile" : "post_fx_node");

          registry.read("exposure_histogram").buffer().atStage(dabfg::Stage::CS).bindToShaderVar("Histogram");

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
  if (config.getMemberOr(ECS_HASH("adaptation__on"), settings.fixedExposure < 0))
  {
    settings.fixedExposure = -1;
    READ_REAL("adaptation", autoExposureScale);
    READ_REAL("adaptation", lowPart);
    READ_REAL("adaptation", highPart);
    READ_REAL("adaptation", minExposure);
    READ_REAL("adaptation", maxExposure);
    READ_REAL("adaptation", adaptDownSpeed);
    READ_REAL("adaptation", adaptUpSpeed);
    READ_REAL("adaptation", brightnessPerceptionLinear);
    READ_REAL("adaptation", brightnessPerceptionPower);
    READ_REAL("adaptation", brightnessPerceptionPower);
    READ_REAL("adaptation", fadeMul);
    READ_REAL("adaptation", centerWeight);
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
static void get_default_settings_ecs_query(Callable c);

template <typename Callable>
static void get_level_settings_ecs_query(Callable c);

template <typename Callable>
static void get_override_settings_ecs_query(Callable c);

static void updateSettings()
{
  get_adaptation_manager_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("adaptation_manager")),
    [&](AdaptationManager &adaptation__manager) {
      AdaptationSettings settings;

      get_default_settings_ecs_query(
        [&](const ecs::Object &adaptation_default_settings) { overrideAdaptationSetting(settings, adaptation_default_settings); });

      get_level_settings_ecs_query(
        [&](const ecs::Object &adaptation_level_settings) { overrideAdaptationSetting(settings, adaptation_level_settings); });

      get_override_settings_ecs_query(
        [&](const ecs::Object &adaptation_override_settings) { overrideAdaptationSetting(settings, adaptation_override_settings); });

      adaptation__manager.setSettings(settings);
    });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, on_disappear)
ECS_TRACK(adaptation_level_settings)
ECS_REQUIRE(ecs::Object adaptation_level_settings)
static void adaptation_level_settings_es(const ecs::Event &) { updateSettings(); }

ECS_TAG(render)
ECS_ON_EVENT(on_appear, on_disappear)
ECS_TRACK(adaptation_default_settings)
ECS_REQUIRE(ecs::Object adaptation_default_settings)
static void adaptation_default_settings_es(const ecs::Event &) { updateSettings(); }

ECS_TAG(render)
ECS_ON_EVENT(on_appear, on_disappear)
ECS_TRACK(adaptation_override_settings)
ECS_REQUIRE(ecs::Object adaptation_override_settings)
static void adaptation_override_settings_es(const ecs::Event &) { updateSettings(); }

ECS_TAG(render)
ECS_ON_EVENT(AfterDeviceReset)
static void adaptation_after_device_reset_es(const ecs::Event &, AdaptationManager &adaptation__manager)
{
  adaptation__manager.uploadInitialExposure();
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void adaptation_update_time_es(
  const UpdateStageInfoBeforeRender &evt, AdaptationManager &adaptation__manager, bool adaptation__track_changes)
{
  if (adaptation__track_changes)
    updateSettings();

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
#endif
