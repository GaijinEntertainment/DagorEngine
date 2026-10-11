// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>

#include <3d/dag_nvFeatures.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_mathUtils.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <startup/dag_globalSettings.h>

#include <render/cameraParams.h>
#include <render/daFrameGraph/daFG.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/neuralRendering.h>
#include <render/world/frameGraphHelpers.h>

// The DLSS-NR nodes live on the "neural_rendering" entity, this one does not: frame_after_aa needs
// its renamer in every game, including those that never create that entity.
static dafg::NodeHandle neural_rendering_passthrough_node;

static nv::DlssNROptions neural_rendering_options;
static uint32_t neural_rendering_options_version = 1;
static uint32_t neural_rendering_pushed_options_version = 0;
static bool neural_rendering_configured = false;

static ShaderVariableInfo neural_rendering_control_mask_defaultVarId("neural_rendering_control_mask_default", true);
static ShaderVariableInfo neural_rendering_control_mask_skinVarId("neural_rendering_control_mask_skin", true);
static ShaderVariableInfo neural_rendering_control_mask_foliageVarId("neural_rendering_control_mask_foliage", true);

bool is_neural_rendering_active() { return neural_rendering_configured; }

static bool is_dlss_nr_supported()
{
  nv::Streamline *streamline = nullptr;
  d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
  return streamline && streamline->isDlssNRSupported() == nv::SupportState::Supported;
}

static dafg::NodeHandle make_neural_rendering_tonemap_node()
{
  return dafg::register_node("neural_rendering_tonemap", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);

    registry.read("frame_after_aa").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar("neural_rendering_source_frame");

    const auto displayResolution = registry.getResolution<2>("display");
    registry.create("neural_rendering_sdr_frame")
      .texture({TEXFMT_A2R10G10B10 | TEXCF_UNORDERED, displayResolution})
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("neural_rendering_sdr_target");

    registry.dispatchThreads("neural_rendering_tonemap").x<&IPoint2::x>(displayResolution).y<&IPoint2::y>(displayResolution).z(1);
  });
}

static dafg::NodeHandle make_neural_rendering_control_mask_node()
{
  return dafg::register_node("neural_rendering_control_mask", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);

    read_gbuffer(registry, dafg::Stage::COMPUTE, readgbuffer::NORMAL | readgbuffer::MATERIAL);
    read_gbuffer_depth(registry, dafg::Stage::COMPUTE);

    const auto renderResolution = registry.getResolution<2>("main_view");
    registry.create("neural_rendering_control_mask")
      .texture({TEXFMT_R8G8B8A8 | TEXCF_UNORDERED, renderResolution})
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("neural_rendering_control_mask_target");

    registry.dispatchThreads("neural_rendering_control_mask").x<&IPoint2::x>(renderResolution).y<&IPoint2::y>(renderResolution).z(1);
  });
}

static dafg::NodeHandle make_neural_rendering_dlss_nr_node()
{
  return dafg::register_node("neural_rendering_dlss_nr", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);

    auto sdrFrameHndl = registry.read("neural_rendering_sdr_frame")
                          .texture()
                          .atStage(dafg::Stage::PS_OR_CS)
                          .useAs(dafg::Usage::SHADER_RESOURCE)
                          .handle();
    auto outputFrameHndl = registry.create("neural_rendering_sdr_output")
                             .texture({TEXFMT_A2R10G10B10 | TEXCF_UNORDERED, registry.getResolution<2>("display")})
                             .atStage(dafg::Stage::PS_OR_CS)
                             .useAs(dafg::Usage::SHADER_RESOURCE)
                             .handle();
    auto depthHndl =
      registry.readTexture("depth_after_transparency").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto motionVectorsHndl = registry.readTexture("motion_vecs_after_transparency")
                               .atStage(dafg::Stage::PS_OR_CS)
                               .useAs(dafg::Usage::SHADER_RESOURCE)
                               .handle();
    // Missing while neural_rendering__autoMask is on, DLSS-NR then masks on its own.
    auto controlMaskHndl = registry.readTexture("neural_rendering_control_mask")
                             .atStage(dafg::Stage::PS_OR_CS)
                             .useAs(dafg::Usage::SHADER_RESOURCE)
                             .optional()
                             .handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto cameraHistoryHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();

    const auto renderResolution = registry.getResolution<2>("main_view");

    return [sdrFrameHndl, outputFrameHndl, depthHndl, motionVectorsHndl, controlMaskHndl, cameraHndl, cameraHistoryHndl,
             renderResolution]() {
      if (neural_rendering_pushed_options_version != neural_rendering_options_version)
      {
        int viewIndex = 0;
        d3d::driver_command(Drv3dCommand::SET_DLSS_NR_OPTIONS, &neural_rendering_options, &viewIndex);
        neural_rendering_pushed_options_version = neural_rendering_options_version;
      }

      const CameraParams &camera = cameraHndl.ref();
      const CameraParams &cameraHistory = cameraHistoryHndl.ref();
      const IPoint2 inputResolution = renderResolution.get();

      nv::DlssNRParams<BaseTexture> params = {};
      params.inColor = sdrFrameHndl.get();
      params.outColor = outputFrameHndl.get();
      params.inDepth = depthHndl.get();
      params.inMotionVectors = motionVectorsHndl.get();
      params.inControlMask = controlMaskHndl.get();
      params.inJitterOffsetX = camera.jitterOffset.x;
      params.inJitterOffsetY = camera.jitterOffset.y;
      // Motion vectors are already in {-1,1} here, same as the DLSS path.
      params.inMVScaleX = 1;
      params.inMVScaleY = 1;
      params.inWidth = inputResolution.x;
      params.inHeight = inputResolution.y;
      params.frameId = dagor_get_global_frame_id();
      params.inReset = is_teleporting(camera, cameraHistory);
      params.camera.projection = camera.noJitterProjTm;
      params.camera.projectionInverse = inverse44(camera.noJitterProjTm);
      params.camera.reprojection = inverse44(cameraHistory.noJitterGlobtm) * camera.noJitterGlobtm;
      params.camera.reprojectionInverse = inverse44(camera.noJitterGlobtm) * cameraHistory.noJitterGlobtm;
      params.camera.worldToView = camera.viewTm;
      params.camera.viewToWorld = camera.viewItm;
      params.camera.position = camera.viewItm.getcol(3);
      params.camera.up = camera.viewItm.getcol(1);
      params.camera.right = camera.viewItm.getcol(0);
      params.camera.forward = camera.viewItm.getcol(2);
      params.camera.nearZ = camera.noJitterPersp.zn;
      params.camera.farZ = camera.noJitterPersp.zf;
      params.camera.fov = 2 * atan(1.f / camera.noJitterPersp.wk);
      params.camera.aspect = camera.noJitterPersp.hk / camera.noJitterPersp.wk;

      int viewIndex = 0;
      d3d::driver_command(Drv3dCommand::EXECUTE_DLSS_NR, &params, &viewIndex);
    };
  });
}

static dafg::NodeHandle make_neural_rendering_inverse_tonemap_node()
{
  return dafg::register_node("neural_rendering_inverse_tonemap", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);

    registry.read("neural_rendering_sdr_output")
      .texture()
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("neural_rendering_output_frame");
    // The pre-NR HDR frame is the highlight recovery reference.
    registry.read("frame_after_aa").texture().atStage(dafg::Stage::COMPUTE).bindToShaderVar("neural_rendering_source_frame");

    const auto displayResolution = registry.getResolution<2>("display");
    registry.create("frame_after_neural_rendering")
      .texture({TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, displayResolution})
      .withHistory(dafg::History::ClearZeroOnFirstFrame)
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("neural_rendering_target_frame");

    registry.dispatchThreads("neural_rendering_inverse_tonemap")
      .x<&IPoint2::x>(displayResolution)
      .y<&IPoint2::y>(displayResolution)
      .z(1);
  });
}

static dafg::NodeHandle make_neural_rendering_passthrough_node()
{
  return dafg::register_node("neural_rendering_passthrough", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(
      dafg::multiplexing::Mode::SubSampling | dafg::multiplexing::Mode::SuperSampling | dafg::multiplexing::Mode::Viewport);
    // Only the tsr node reads this history; daFG drops it in the modes that do not.
    registry.rename("frame_after_aa", "frame_after_neural_rendering").texture().withHistory(dafg::History::ClearZeroOnFirstFrame);
  });
}

template <typename Callable>
static void recreate_neural_rendering_nodes_ecs_query(ecs::EntityManager &manager, Callable c);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady, ChangeRenderFeaturesEarly)
ECS_TRACK(render_settings__neuralRendering, render_settings__antialiasing_mode)
static void neural_rendering_nodes_es(const ecs::Event &,
  ecs::EntityManager &manager,
  bool render_settings__neuralRendering,
  const ecs::string &render_settings__antialiasing_mode)
{
  neural_rendering_options.enabled = render_settings__neuralRendering;
  neural_rendering_options_version++;

  // The Streamline NR feature is created alongside the DLSS ones, so neural rendering follows DLSS.
  // That also matches the SDK guidance to feed it a temporally stable, already upscaled image.
  const bool enabled = render_settings__neuralRendering && render_settings__antialiasing_mode == "dlss" && is_dlss_nr_supported();

  recreate_neural_rendering_nodes_ecs_query(manager,
    [&](dafg::NodeHandle &neural_rendering__tonemap_node, dafg::NodeHandle &neural_rendering__dlss_nr_node,
      dafg::NodeHandle &neural_rendering__inverse_tonemap_node, dafg::NodeHandle &neural_rendering__control_mask_node,
      bool neural_rendering__autoMask) {
      neural_rendering__tonemap_node = {};
      neural_rendering__dlss_nr_node = {};
      neural_rendering__inverse_tonemap_node = {};
      neural_rendering__control_mask_node = {};
      neural_rendering_configured = false;

      if (!enabled)
        return;

      // A fresh feature starts with the SDK defaults, so make the new node push before it evaluates.
      neural_rendering_pushed_options_version = 0;

      neural_rendering__tonemap_node = make_neural_rendering_tonemap_node();
      neural_rendering__dlss_nr_node = make_neural_rendering_dlss_nr_node();
      neural_rendering__inverse_tonemap_node = make_neural_rendering_inverse_tonemap_node();
      neural_rendering__control_mask_node =
        neural_rendering__autoMask ? dafg::NodeHandle{} : make_neural_rendering_control_mask_node();
      neural_rendering_configured = true;
    });

  neural_rendering_passthrough_node = neural_rendering_configured ? dafg::NodeHandle{} : make_neural_rendering_passthrough_node();
}

// Fallback for contexts where OnRenderSettingsReady never fires (e.g. daEditor with DNG renderer).
// Without this, frame_after_neural_rendering is never produced and the frame graph breaks.
ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void neural_rendering_ensure_passthrough_es(const ecs::Event &)
{
  if (!neural_rendering_configured)
    neural_rendering_passthrough_node = make_neural_rendering_passthrough_node();
}

// The mask node reads the gbuffer, which is not there in every mode, so it may only exist next to the NR nodes.
ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(neural_rendering__autoMask)
static void neural_rendering_control_mask_node_es(
  const ecs::Event &, bool neural_rendering__autoMask, dafg::NodeHandle &neural_rendering__control_mask_node)
{
  neural_rendering__control_mask_node =
    neural_rendering_configured && !neural_rendering__autoMask ? make_neural_rendering_control_mask_node() : dafg::NodeHandle{};
}


// The index into this table is the DLSS-NR preset value: 0 is the default model, 1..7 pin a fixed one.
static const char *const neural_rendering_preset_names[] = {"default", "a", "b", "c", "d", "e", "f", "g"};

static uint32_t neural_rendering_preset_from_string(const ecs::string &name)
{
  for (uint32_t preset = 0; preset < countof(neural_rendering_preset_names); ++preset)
    if (name == neural_rendering_preset_names[preset])
      return preset;

  logerr("neural rendering: unknown preset '%s', falling back to the default model", name.c_str());
  return 0;
}

// Tuning comes from the "neural_rendering" singleton so artists can retune it live and per level.
// Without that entity the SDK defaults stand, which is the neutral look.
ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(neural_rendering__style,
  neural_rendering__preset,
  neural_rendering__intensity,
  neural_rendering__localToneStrength,
  neural_rendering__localStructureStrength,
  neural_rendering__autoMask,
  neural_rendering__skinStructureStrength,
  neural_rendering__controlMaskDefault,
  neural_rendering__controlMaskSkin,
  neural_rendering__controlMaskFoliage)
static void neural_rendering_params_es(const ecs::Event &,
  int neural_rendering__style,
  const ecs::string &neural_rendering__preset,
  float neural_rendering__intensity,
  float neural_rendering__localToneStrength,
  float neural_rendering__localStructureStrength,
  bool neural_rendering__autoMask,
  float neural_rendering__skinStructureStrength,
  const Point4 &neural_rendering__controlMaskDefault,
  const Point4 &neural_rendering__controlMaskSkin,
  const Point4 &neural_rendering__controlMaskFoliage)
{
  ShaderGlobal::set_float4(neural_rendering_control_mask_defaultVarId, neural_rendering__controlMaskDefault);
  ShaderGlobal::set_float4(neural_rendering_control_mask_skinVarId, neural_rendering__controlMaskSkin);
  ShaderGlobal::set_float4(neural_rendering_control_mask_foliageVarId, neural_rendering__controlMaskFoliage);

  neural_rendering_options.style = neural_rendering__style;
  neural_rendering_options.preset = neural_rendering_preset_from_string(neural_rendering__preset);
  neural_rendering_options.intensity = neural_rendering__intensity;
  neural_rendering_options.localToneStrength = neural_rendering__localToneStrength;
  neural_rendering_options.localStructureStrength = neural_rendering__localStructureStrength;
  neural_rendering_options.useAutoMask = neural_rendering__autoMask;
  neural_rendering_options.skinStructureStrength = neural_rendering__skinStructureStrength;
  neural_rendering_options_version++;
}
