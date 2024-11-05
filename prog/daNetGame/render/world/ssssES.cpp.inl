// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_driver.h>
#include <3d/dag_resizableTex.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_overrideStates.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <render/daBfg/ecs/frameGraphNode.h>

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <ecs/core/entityManager.h>
#include <ecs/render/postfx_renderer.h>
#include <render/resolution.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <render/world/frameGraphHelpers.h>
#include <shaders/dag_overrideStateId.h>
#include "ssss.h"

#define SSSS_GLOBAL_VARS_LIST                \
  VAR(ssss_transmittance_thickness_bias)     \
  VAR(ssss_transmittance_thickness_min)      \
  VAR(ssss_transmittance_thickness_scale)    \
  VAR(ssss_transmittance_translucency_scale) \
  VAR(ssss_transmittance_blur_scale)         \
  VAR(ssss_transmittance_sample_count)       \
  VAR(ssss_normal_offset)                    \
  VAR(ssss_reflectance_blur_width)           \
  VAR(ssss_reflectance_blur_quality)         \
  VAR(ssss_reflectance_follow_surface)       \
  VAR(ssss_reflectance_blur_pass)            \
  VAR(use_ssss)                              \
  VAR(ssss_quality)

#define VAR(a) static int a##VarId = -1;
SSSS_GLOBAL_VARS_LIST
#undef VAR


// Values should match use_ssss shader interval
enum
{
  SSSS_OFF = 0,
  SSSS_SUN_AND_DYNAMIC_LIGHTS = 1
};

// ssss_quality interval
enum class SsssQuality
{
  OFF,
  LOW, // Reflectance blur only
  HIGH // Reflectance blur and trasmittance
};

static SsssQuality ssss_string_to_enum(const ecs::string &str)
{
  if (str == "high")
    return SsssQuality::HIGH;
  else if (str == "low")
    return SsssQuality::LOW;
  else
  {
    if (str != "off")
      logerr("Unknown quality setting for ssss: %s", str.c_str());
    return SsssQuality::OFF;
  }
}

static eastl::string ssss_enum_to_string(SsssQuality quality)
{
  switch (quality)
  {
    case SsssQuality::HIGH: return "high";
    case SsssQuality::LOW: return "low";
    case SsssQuality::OFF: return "off";
  }
  return "off";
}

template <typename Callable>
inline void ssss_enabled_ecs_query(ecs::EntityId eid, Callable c);
static SsssQuality query_ssss_quality()
{
  SsssQuality quality = SsssQuality::OFF;
  ecs::EntityId ssssEid = g_entity_mgr->getSingletonEntity(ECS_HASH("render_settings"));
  ssss_enabled_ecs_query(ssssEid,
    [&quality](ecs::string &render_settings__ssssQuality) { quality = ssss_string_to_enum(render_settings__ssssQuality); });
  return quality;
}

bool isSsssReflectanceBlurEnabled() { return query_ssss_quality() >= SsssQuality::LOW; }

bool isSsssTransmittanceEnabled() { return query_ssss_quality() == SsssQuality::HIGH; }

bool isSsssReflectanceBlurEnabled(const eastl::string &setting) { return ssss_string_to_enum(setting) >= SsssQuality::LOW; }

bool isSsssTransmittanceEnabled(const eastl::string &setting) { return ssss_string_to_enum(setting) == SsssQuality::HIGH; }

static void ensure_shadervars_inited()
{
  if (ssss_transmittance_thickness_biasVarId < 0)
  {
#define VAR(a) a##VarId = ::get_shader_variable_id(#a, true);
    SSSS_GLOBAL_VARS_LIST
#undef VAR
  }
}

static dabfg::NodeHandle makeSsssReflectanceHorizontalNode();
static dabfg::NodeHandle makeSsssReflectanceVerticalNode();

template <typename Callable>
inline void ssss_init_ecs_query(ecs::EntityId eid, Callable c);

ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__ssssQuality, render_settings__forwardRendering, render_settings__combinedShadows)
static void ssss_settings_tracking_es(const ecs::Event &,
  const ecs::string render_settings__ssssQuality,
  bool render_settings__forwardRendering,
  bool render_settings__combinedShadows)
{
  ssss_init_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("ssss")),
    [&](dabfg::NodeHandle &ssss__horizontal_node, dabfg::NodeHandle &ssss__vertical_node) {
      ssss__horizontal_node = {};
      ssss__vertical_node = {};

      if (render_settings__forwardRendering || !render_settings__combinedShadows)
        return;

      if (isSsssReflectanceBlurEnabled(render_settings__ssssQuality))
      {
        ssss__horizontal_node = makeSsssReflectanceHorizontalNode();
        ssss__vertical_node = makeSsssReflectanceVerticalNode();
      }

      ShaderGlobal::set_int(use_ssssVarId,
        isSsssTransmittanceEnabled(render_settings__ssssQuality) ? SSSS_SUN_AND_DYNAMIC_LIGHTS : SSSS_OFF);
      ShaderGlobal::set_int(ssss_qualityVarId, static_cast<int>(ssss_string_to_enum(render_settings__ssssQuality)));
    });
}

static const float &get_vertical_fov_from_camera_params(const CameraParams &params) { return params.jitterPersp.hk; }

static void request_common_blur_data(dabfg::Registry registry)
{
  shaders::OverrideState zFuncEqualState;
  zFuncEqualState.set(shaders::OverrideState::Z_FUNC);
  zFuncEqualState.zFunc = CMPF_EQUAL;

  registry.requestState().setFrameBlock("global_frame").enableOverride(zFuncEqualState);

  read_gbuffer(registry);
  registry.readTexture("opaque_depth_with_water_before_clouds").atStage(dabfg::Stage::PS).bindToShaderVar("depth_gbuf");
  registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");

  registry.read("current_camera")
    .blob<CameraParams>()
    .bindToShaderVar<&get_vertical_fov_from_camera_params>("ssss_reflectance_vertical_fov");
}

static dabfg::NodeHandle makeSsssReflectanceHorizontalNode()
{
  return dabfg::register_node("ssss_reflectance_blur_horizontal_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    request_common_blur_data(registry);

    const auto target_fmt = get_frame_render_target_format();

    registry.create("ssss_intermediate_target", dabfg::History::No)
      .texture({target_fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view")});

    registry.requestRenderPass()
      .color({"ssss_intermediate_target"})
      .depthRw("ssss_depth_mask")
      .clear("ssss_intermediate_target", make_clear_value(0.f, 0.f, 0.f, 0.f));

    registry.read("opaque_with_envi").texture().atStage(dabfg::Stage::PS).bindToShaderVar("ssss_reflectance_blur_color_source_tex");
    {
      // TODO: Experiment with linear depth sampler, original paper seems to use that in SSSS_FOLLOW_SURFACE part
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      registry.create("ssss_reflectance_blur_sampler", dabfg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
        .bindToShaderVar("ssss_reflectance_blur_color_source_tex_samplerstate");
    }

    return [shader = PostFxRenderer("ssss_reflectance_blur_ps")] {
      ShaderGlobal::set_int(ssss_reflectance_blur_passVarId, 0);
      shader.render();
    };
  });
}

static dabfg::NodeHandle makeSsssReflectanceVerticalNode()
{
  return dabfg::register_node("ssss_reflectance_blur_vertical_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    request_common_blur_data(registry);

    registry.read("ssss_intermediate_target")
      .texture()
      .atStage(dabfg::Stage::PS)
      .bindToShaderVar("ssss_reflectance_blur_color_source_tex");
    {
      // TODO: Experiment with linear depth sampler, original paper seems to use that in SSSS_FOLLOW_SURFACE part
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      registry.create("ssss_intermediate_target_sampler", dabfg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
        .bindToShaderVar("ssss_reflectance_blur_color_source_tex_samplerstate");
    }

    registry.requestRenderPass().color({"opaque_processed"}).depthRw("ssss_depth_mask");

    return [shader = PostFxRenderer("ssss_reflectance_blur_ps")]() {
      ShaderGlobal::set_int(ssss_reflectance_blur_passVarId, 1);
      shader.render();
    };
  });
}

static void update_ssss_quality(SsssQuality quality)
{
  ssss_enabled_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("render_settings")),
    [&](ecs::string &render_settings__ssssQuality) { render_settings__ssssQuality = ssss_enum_to_string(quality); });
}

template <typename Callable>
inline void ssss_params_ecs_query(ecs::EntityId eid, Callable c);
static void ssssImguiWindow()
{
  auto setTooltip = [](const char *tooltip) {
    if (!ImGui::IsItemHovered() || tooltip == nullptr)
      return;
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(tooltip);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  };


  SsssQuality quality = query_ssss_quality();
  int intQuality = static_cast<int>(quality);
  ImGui::SliderInt("SSSS Quality", &intQuality, static_cast<int>(SsssQuality::OFF), static_cast<int>(SsssQuality::HIGH));
  setTooltip("0 = \"Off\"\n"
             "1 = \"Low\" = Reflectance blur only\n"
             "2 = \"High\" = Reflectance blur and transmittance");
  if (static_cast<int>(quality) != intQuality)
  {
    quality = static_cast<SsssQuality>(intQuality);
    update_ssss_quality(quality);
  }

  if (quality == SsssQuality::OFF)
    return;
  bool entityFound = false;
  ecs::EntityId ssssEid = g_entity_mgr->getSingletonEntity(ECS_HASH("ssss"));
  ssss_params_ecs_query(ssssEid, [&entityFound, &quality, &setTooltip](float &transmittance__thickness_bias,
                                   float &transmittance__thickness_min, float &transmittance__thickness_scale,
                                   float &transmittance__translucency_scale, float &transmittance__blur_scale,
                                   int &transmittance__sample_count, float &transmittance__normal_offset,
                                   float &reflectance__blur_width, int &reflectance__blur_quality,
                                   bool &reflectance__blur_follow_surface) {
    entityFound = true;

    ImGuiDagor::HelpMarker(
      "This Separable Sub-Surface Scattering implementation is responsible for making our skin materials more realistic.\n"
      "It consits of 2 main parts:\n"
      "  1. Transmittance\n"
      "    - This is responsible for simulating light transmitting through thin and thick slabs of geometry. Basically\n"
      "      the effect you want to see when you hold your fingers against a flashlight, or looking at a human character's\n"
      "      ears against the sun.\n"
      "    - This is achieved by estimating the distance the light is travelling within the geometry based on our dynamic\n"
      "      shadowmaps, calculating an exponential transmittance factor based on this thickness information, and applying\n"
      "      a color profile to simulate different diffusion of different frequencies of light.\n"
      "  2. Reflectance blur\n"
      "    - The separable blur part is responsible for blurring the lighting on the surface. Surfaces like human skin\n"
      "      should not have crisp lighting details due to normal maps, since light is bleeding through small bumps and\n"
      "      crevices in such materials. This is what this part is aiming to simulate.\n"
      "For more detailed and technical information, visit the resources linked at the top of ssss_transmittance_factor.sh file.",
      1000);

    if (quality >= SsssQuality::LOW && ImGui::CollapsingHeader("Reflectance blur", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::SliderFloat("Blur width", &reflectance__blur_width, 0, 0.03);
      setTooltip("Scaleing width of the blurring kernel in UV space. Wider kernels will blur "
                 "surface details more. It's scaled by "
                 "distance, so distant surfaces will be blurred less in screen-space.");

      ImGui::SliderInt("Blur quality", &reflectance__blur_quality, 0, 2);
      setTooltip("Quality of reflectance blur. Higher quality uses more samples, which is more "
                 "expensive, but also yields less artifacts "
                 "on surfaces at grazing angles.");

      ImGui::Checkbox("Blur follow surface", &reflectance__blur_follow_surface);
      setTooltip("Light diffusion should occur on the surface of the object, not in a screen "
                 "oriented plane. When enabled, this setting "
                 "will ensure that diffusion is more accurately calculated, at the expense of "
                 "more texture samples. This makes the "
                 "effect significantly more expensive, we should only enable it on capable "
                 "hardware (if at all).");
    }
    if (quality == SsssQuality::HIGH && ImGui::CollapsingHeader("Transmittance", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGuiDagor::HelpMarker(
        "Transmittance factor is calculated as follows:\n"
        "  translucency = translucency_from_gbuffer * translucency_scale\n"
        "  thickness_in_world_space = (raw_depth_of_pixel_from_sun - raw_depth_from_shadowmap + thickness_bias) "
        "* shadow_cascade_depth_range\n"
        "  thickness = max(thickness_min, thickness_in_world_space) * (1 - translucency) * thickness_scale * 100\n"
        "  transmittance_factor = exp(-thickness * thickness)",
        1000);

      ImGui::SliderFloat("Thickness bias", &transmittance__thickness_bias, -0.1, 0.1);
      setTooltip(
        "Bias value for calculating the difference in depth based on pixel world-space position and dynamic shadowmap depth.");

      ImGui::SliderFloat("Thickness min", &transmittance__thickness_min, 0, 0.1);
      setTooltip("Minimum thickness in meters. Used to avoid artifacts thickness values close to zero.");

      ImGui::SliderFloat("Thickness scale", &transmittance__thickness_scale, 0, 500);
      setTooltip("Scale value that is is used to multiply world-space thickness values before exponential transmittance calculation.");

      ImGui::SliderFloat("Translucency scale", &transmittance__translucency_scale, 0, 1.0);
      setTooltip(
        "Translucency values sampled from gbuffer will be scaled by this value to avoid artifacts caused by 100% translucency.");

      ImGui::SliderFloat("Blur scale", &transmittance__blur_scale, 0, 0.05);
      setTooltip("Light doesn't travel in a straight line when traveling inside these materials, it is getting scattered instead. "
                 "Transmittance blur helps in replicating this effect, and also helps with hiding low dynamic shadowmap resolutions. "
                 "The unit is meters.");

      ImGui::SliderInt("Sample count", &transmittance__sample_count, 0, 4);
      setTooltip("Number of Poisson-offseted samples taken from the shadowmap at each pixel. 2 and 4 samples are more expensive, but "
                 "generate less noise and reconstruct shadow borders better. Set it to 0 to disable transmittance.");

      ImGui::SliderFloat("Normal Offset", &transmittance__normal_offset, 0, 0.1);
      setTooltip("To work around some depth sampling artifacts, the worldPos taken from the "
                 "gbuffer needs to be moved inwards of the mesh in the direction of the normal. "
                 "This value sets the strength of that offset in meters.");
    }
  });
  if (!entityFound)
    logerr("ECS query haven't found a matching ssss entity, eid: %d", (ecs::entity_id_t)ssssEid);
}

ECS_ON_EVENT(on_appear)
ECS_TRACK(*)
ECS_TAG(render)
static void ssss_created_or_params_changed_es_event_handler(const ecs::Event &,
  float transmittance__thickness_bias,
  float transmittance__thickness_min,
  float transmittance__thickness_scale,
  float transmittance__translucency_scale,
  float transmittance__blur_scale,
  int transmittance__sample_count,
  float transmittance__normal_offset,
  float reflectance__blur_width,
  int reflectance__blur_quality,
  bool reflectance__blur_follow_surface)
{
  ensure_shadervars_inited();

  ShaderGlobal::set_real(ssss_transmittance_thickness_biasVarId, transmittance__thickness_bias);
  ShaderGlobal::set_real(ssss_transmittance_thickness_minVarId, transmittance__thickness_min);
  ShaderGlobal::set_real(ssss_transmittance_thickness_scaleVarId, transmittance__thickness_scale);
  ShaderGlobal::set_real(ssss_transmittance_translucency_scaleVarId, transmittance__translucency_scale);
  ShaderGlobal::set_real(ssss_transmittance_blur_scaleVarId, transmittance__blur_scale);
  ShaderGlobal::set_int(ssss_transmittance_sample_countVarId, transmittance__sample_count);
  ShaderGlobal::set_real(ssss_normal_offsetVarId, transmittance__normal_offset);
  ShaderGlobal::set_real(ssss_reflectance_blur_widthVarId, reflectance__blur_width);
  ShaderGlobal::set_int(ssss_reflectance_blur_qualityVarId, reflectance__blur_quality);
  ShaderGlobal::set_int(ssss_reflectance_follow_surfaceVarId, reflectance__blur_follow_surface);
}


REGISTER_IMGUI_WINDOW("Render", "Separable Sub-Surface Scattering", ssssImguiWindow);
