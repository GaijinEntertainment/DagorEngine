// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "screenSpaceReflectionDenoiser.h"

#include <render/world/frameGraphHelpers.h>

#include <ecs/core/entityManager.h>
#include <render/renderEvent.h>
#include <shaders/dag_postFxRenderer.h>

static ShaderVariableInfo ssr_hit_dist_paramsVarId("ssr_hit_dist_params", true);
static ShaderVariableInfo denoiser_frame_indexVarId("denoiser_frame_index", true);
static ShaderVariableInfo rtr_validation_textureVarId("rtr_validation_texture", true);
eastl::unique_ptr<SSRDenoiser> ssrDenoiser;

SSRDenoiser *init_ssr_denoiser(int w, int h, bool isHalfRes)
{
  ssrDenoiser = eastl::make_unique<SSRDenoiser>(w, h, isHalfRes);
  return ssrDenoiser.get();
}

void teardown_ssr_denoiser() { ssrDenoiser.reset(); }
SSRDenoiser *get_ssr_denoiser() { return ssrDenoiser.get(); }

ECS_NO_ORDER
ECS_TAG(dev, render)
ECS_ON_EVENT(RenderEventDebugGUI)
static inline void draw_validaton_es_event_handler(const ecs::Event &)
{
  if (ssrDenoiser)
    ssrDenoiser->drawValidation();
}

void SSRDenoiser::setShaderVars() const
{
  ShaderGlobal::set_int(denoiser_frame_indexVarId, denoiser::get_frame_number());
  ShaderGlobal::set_color4(ssr_hit_dist_paramsVarId, getHitDistParams());
}

void SSRDenoiser::drawValidation()
{
  if (!show_validation)
    return;

  if (!validation_renderer)
    validation_renderer = eastl::make_unique<PostFxRenderer>("rtr_validation_renderer");

  ShaderGlobal::set_texture(rtr_validation_textureVarId, validation_texture.getTexId());
  validation_renderer->render();
}

SSRDenoiser::SSRDenoiser(uint32_t w, uint32_t h, bool halfRes) : halfRes(halfRes)
{
  width = w;
  height = h;

  denoisedUnpackCS.reset(new_compute_shader("ssr_unpack_denoised"));
  denoiser::initialize(w, h);
  validation_texture = dag::create_tex(nullptr, w, h, TEXCF_UNORDERED, 1, "rtr_validation_texture");
  UniqueTex reflection_value; // Ignore this param, we will use own texture
  denoiser::make_reflection_maps(reflection_value, denoised_reflection, output_type, halfRes);
}

dabfg::NodeHandle SSRDenoiser::makeDenoiserPrepareNode() const
{
  return dabfg::register_node("ssr_denoiser_prepare", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
    registry.executionHas(dabfg::SideEffects::External);
    registry.createBlob<OrderingToken>("denoiser_ready_token", dabfg::History::No);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto prevCameraHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();
    registry.readTexture("close_depth").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("downsampled_close_depth_tex");
    auto downsampledNormalsHndl =
      registry.readTexture("downsampled_normals").atStage(dabfg::Stage::POST_RASTER).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    auto motionVectorsHndl = registry.readTexture(halfRes ? "downsampled_motion_vectors_tex" : "motion_vecs")
                               .atStage(dabfg::Stage::PS_OR_CS)
                               .useAs(dabfg::Usage::SHADER_RESOURCE)
                               .handle();

    read_gbuffer(registry, dabfg::Stage::PS_OR_CS);
    read_gbuffer_depth(registry, dabfg::Stage::PS_OR_CS);

    return [cameraHndl, prevCameraHndl, motionVectorsHndl, downsampledNormalsHndl]() {
      denoiser::FrameParams params;
      params.viewPos = cameraHndl.ref().viewItm.getcol(3);
      params.prevViewPos = prevCameraHndl.ref().viewItm.getcol(3);
      params.viewDir = cameraHndl.ref().viewItm.getcol(2);
      params.prevViewDir = prevCameraHndl.ref().viewItm.getcol(2);
      params.viewItm = cameraHndl.ref().viewItm;
      params.prevViewItm = prevCameraHndl.ref().viewItm;
      params.projTm = cameraHndl.ref().noJitterProjTm;
      params.prevProjTm = prevCameraHndl.ref().noJitterProjTm;
      params.jitter = Point2(cameraHndl.ref().jitterPersp.ox, cameraHndl.ref().jitterPersp.oy);
      params.prevJitter = Point2(prevCameraHndl.ref().jitterPersp.ox, prevCameraHndl.ref().jitterPersp.oy);
      params.motionVectors = motionVectorsHndl.view().getTex2D();
      params.halfMotionVectors = motionVectorsHndl.view().getTex2D();
      params.halfNormals = TextureIDPair{downsampledNormalsHndl.view().getTex2D(), downsampledNormalsHndl.view().getTexId()};
      params.motionMultiplier = Point3(1, 1, 1);
      denoiser::prepare(params);
    };
  });
}

dabfg::NodeHandle SSRDenoiser::makeDenoiseNode() const
{
  return dabfg::register_node("ssr_denoising", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
    registry.readBlob<OrderingToken>("denoiser_ready_token");
    const auto ssrHandle =
      registry.readTexture("ssr_target_before_denoise").atStage(dabfg::Stage::COMPUTE).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    registry.registerTexture2d("denoised_ssr",
      [this](const dabfg::multiplexing::Index) -> ManagedTexView { return denoised_reflection; });

    read_gbuffer_motion(registry, dabfg::Stage::PS_OR_CS);
    registry.readTexture(halfRes ? "downsampled_motion_vectors_tex" : "motion_vecs")
      .atStage(dabfg::Stage::PS_OR_CS)
      .useAs(dabfg::Usage::SHADER_RESOURCE);

    return [this, ssrHandle]() {
      denoiser::ReflectionDenoiser params;
      params.hitDistParams = getHitDistParams();
      params.denoisedReflection = denoised_reflection.getTex2D();
      params.reflectionValue = ssrHandle.view().getTex2D();
      params.performanceMode = performance_mode;
      params.validationTexture = show_validation ? validation_texture.getTex2D() : nullptr;
      denoiser::denoise_reflection(params);
    };
  });
}

dabfg::NodeHandle SSRDenoiser::makeDenoiseUnpackNode() const
{
  return dabfg::register_node("ssr_denoise_unpack", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
    registry.readTexture("denoised_ssr").atStage(dabfg::Stage::COMPUTE).bindToShaderVar("denoised_ssr");

    registry
      .createTexture2d("ssr_target", dabfg::History::ClearZeroOnFirstFrame,
        {TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, registry.getResolution<2>("main_view", halfRes ? 0.5f : 1.0f)})
      .atStage(dabfg::Stage::COMPUTE)
      .bindToShaderVar("ssr_target");

    return [this]() { denoisedUnpackCS->dispatchThreads(width, height, 1); };
  });
}

#if DAGOR_DBGLEVEL > 0

#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>

const char *to_string(denoiser::ReflectionMethod mode)
{
  switch (mode)
  {
    case denoiser::ReflectionMethod::Reblur: return "Reblur";
    case denoiser::ReflectionMethod::Relax: return "Relax";
    default: return "Unknown";
  }
}

void SSRDenoiser::drawImguiWindow()
{
  ImGui::SliderFloat("Water ray length", &water_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Gloss ray length", &gloss_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Rough ray length", &rough_ray_length, 0.01f, 5000);
  ImGui::SliderFloat("Distance factor", &distance_factor, 0, 1);
  ImGui::SliderFloat("Scatter factor", &scatter_factor, 0, 2);
  ImGui::SliderFloat("Roughness factor", &roughness_factor, 0, 10);

  auto oldType = output_type;

  using RDM = denoiser::ReflectionMethod;
  ImGuiDagor::EnumCombo("Denoiser", RDM::Reblur, RDM::Relax, output_type, &to_string);

  if (output_type == RDM::Reblur)
    ImGui::Checkbox("Performance mode", &performance_mode);

  ImGui::Checkbox("Show validation layer", &show_validation);

  UniqueTex reflection_value; // Ignore this param, we will use own texture

  if (oldType != output_type)
    denoiser::make_reflection_maps(reflection_value, denoised_reflection, output_type, performance_mode);
}

static void imguiWindow()
{
  if (!ssrDenoiser)
  {
    ImGui::Text("Enable denoiser by render.ssr_denoiser command");
    return;
  }

  ssrDenoiser->drawImguiWindow();
}

REGISTER_IMGUI_WINDOW("Render", "SSRDenoiser", imguiWindow);
#endif