// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/foam/foamFx.h"

#include <perfMon/dag_statDrv.h>
#include <util/dag_console.h>

#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <math/dag_Point3.h>
#include <render/viewVecs.h>
#include <sstream>
#include <dag/dag_vectorMap.h>
#include <drv/3d/dag_renderTarget.h>

enum class FoamTexture
{
  MASK_TARGET,
  MASK_DEPTH,
  TILE,
  GRADIENT,
  OVERFOAM,
  UNDERFOAM,
  UNDERFOAM_DOWNSAMPLED,
  RESULT_COLOR,
  RESULT_NORMAL,
};

static const char *foamTextureNames[]{
  "mask_target",
  "mask_depth",
  "tile",
  "gradient",
  "overfoam",
  "underfoam",
  "underfoam downsampled",
  "color result",
  "normal result",
};

static const char *getFoamTextureName(FoamTexture tex) { return foamTextureNames[(int)tex]; }

static dag::VectorMap<FoamTexture, BaseTexture *> debugTextures;

static const char *prepare_debug_step[]{
  "pattern",
  "masked pattern",
  "threshold",
  "thresholded pattern",
  "all",
};

static const char *debugTextures2[] = {
  "projected_on_water_effects_tex",
  "wfx_hmap",
  "wfx_normals",
};

#define GLOBAL_VARS_LIST                \
  VAR(foam_mask)                        \
  VAR(foam_mask_samplerstate)           \
  VAR(foam_mask_depth)                  \
  VAR(foam_mask_depth_samplerstate)     \
  VAR(foam_overfoam_tex)                \
  VAR(foam_overfoam_tex_samplerstate)   \
  VAR(foam_underfoam_tex)               \
  VAR(foam_underfoam_tex_samplerstate)  \
  VAR(tex)                              \
  VAR(tex_samplerstate)                 \
  VAR(texsz)                            \
  VAR(foam_tile_uv_scale)               \
  VAR(foam_distortion_scale)            \
  VAR(foam_normal_scale)                \
  VAR(foam_pattern_gamma)               \
  VAR(foam_mask_gamma)                  \
  VAR(foam_gradient_gamma)              \
  VAR(foam_underfoam_threshold)         \
  VAR(foam_overfoam_threshold)          \
  VAR(foam_underfoam_weight)            \
  VAR(foam_overfoam_weight)             \
  VAR(foam_underfoam_color)             \
  VAR(foam_overfoam_color)              \
  VAR(foam_reflectivity)                \
  VAR(foam_generator_tile_samplerstate) \
  VAR(foam_generator_gradient_samplerstate)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

FoamFx::FoamFx(int _width, int _height) : width(_width), height(_height)
{
  maskTarget = dag::create_tex(nullptr, width, height, TEXFMT_R8 | TEXCF_RTARGET, 1, "foam_mask_target");
  debugTextures[FoamTexture::MASK_TARGET] = maskTarget.getBaseTex();
  maskDepth = dag::create_tex(nullptr, width, height, TEXFMT_DEPTH24 | TEXCF_RTARGET, 1, "foam_mask_depth");
  debugTextures[FoamTexture::MASK_DEPTH] = maskDepth.getBaseTex();

  fullTargetPool = RTargetPool::get(width, height, TEXFMT_R8 | TEXCF_RTARGET, 1);
  downTargetPool = RTargetPool::get(width / 8, height / 8, TEXFMT_R8 | TEXCF_RTARGET, 1);
  tempTargetPool = RTargetPool::get(width / 8, height, TEXFMT_R8 | TEXCF_RTARGET, 1);

  foamPrepare.init("foam_prepare");
  foamCombine.init("foam_combine");
  foamHeight.init("foam_height");
  foamBlurX.init("foam_blur_x");
  foamBlurY.init("foam_blur_y");

  debugTextures[FoamTexture::TILE] = foamGeneratorTileTex.getBaseTex();
  debugTextures[FoamTexture::GRADIENT] = foamGeneratorGradientTex.getBaseTex();

#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR

  d3d::SamplerHandle sampler = d3d::request_sampler({});
  ShaderGlobal::set_sampler(foam_mask_samplerstateVarId, sampler);
  ShaderGlobal::set_sampler(foam_mask_depth_samplerstateVarId, sampler);
  ShaderGlobal::set_sampler(foam_generator_tile_samplerstateVarId, sampler);
  ShaderGlobal::set_sampler(foam_overfoam_tex_samplerstateVarId, sampler);
  ShaderGlobal::set_sampler(foam_underfoam_tex_samplerstateVarId, sampler);
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  sampler = d3d::request_sampler(smpInfo);
  ShaderGlobal::set_sampler(foam_generator_gradient_samplerstateVarId, sampler);
}

FoamFx::~FoamFx() {}

FoamFxParams FoamFx::prepareParams(float tile_uv_scale, float distortion_scale, float normal_scale, float pattern_gamma,
  float mask_gamma, float gradient_gamma, float underfoam_threshold, float overfoam_threshold, float underfoam_weight,
  float overfoam_weight, const Point3 &underfoam_color, const Point3 &overfoam_color, float reflectivity, const String &tile_tex,
  const String &gradient_tex)
{
  Point3 scale(tile_uv_scale, distortion_scale, normal_scale);
  Point3 gamma(pattern_gamma, mask_gamma, gradient_gamma);
  Point2 threshold(underfoam_threshold, overfoam_threshold);
  Point2 weight(underfoam_weight, overfoam_weight);
  FoamFxParams params;
  params.scale = scale;
  params.gamma = gamma;
  params.threshold = threshold;
  params.weight = weight;
  params.underfoamColor = underfoam_color;
  params.overfoamColor = overfoam_color;
  params.reflectivity = reflectivity;
  params.tileTex = tile_tex;
  params.gradientTex = gradient_tex;
  return params;
}

void FoamFx::setParams(const FoamFxParams &params)
{
  ShaderGlobal::set_real(foam_tile_uv_scaleVarId, params.scale.x);
  ShaderGlobal::set_real(foam_distortion_scaleVarId, params.scale.y);
  ShaderGlobal::set_real(foam_normal_scaleVarId, params.scale.z);
  ShaderGlobal::set_real(foam_pattern_gammaVarId, params.gamma.x);
  ShaderGlobal::set_real(foam_mask_gammaVarId, params.gamma.y);
  ShaderGlobal::set_real(foam_gradient_gammaVarId, params.gamma.z);
  ShaderGlobal::set_real(foam_underfoam_thresholdVarId, params.threshold.x);
  ShaderGlobal::set_real(foam_overfoam_thresholdVarId, params.threshold.y);
  ShaderGlobal::set_real(foam_underfoam_weightVarId, params.weight.x);
  ShaderGlobal::set_real(foam_overfoam_weightVarId, params.weight.y);
  ShaderGlobal::set_color4(foam_underfoam_colorVarId, params.underfoamColor, 1.0f);
  ShaderGlobal::set_color4(foam_overfoam_colorVarId, params.overfoamColor, 1.0f);
  ShaderGlobal::set_real(foam_reflectivityVarId, params.reflectivity);

  foamGeneratorTileTex.close();
  foamGeneratorTileTex = SharedTexHolder(dag::get_tex_gameres(params.tileTex), "foam_generator_tile");
  G_ASSERT(foamGeneratorTileTex);
  foamGeneratorGradientTex.close();
  foamGeneratorGradientTex = SharedTexHolder(dag::get_tex_gameres(params.gradientTex), "foam_generator_gradient");
  G_ASSERT(foamGeneratorGradientTex);

  debugTextures[FoamTexture::TILE] = foamGeneratorTileTex.getBaseTex();
  debugTextures[FoamTexture::GRADIENT] = foamGeneratorGradientTex.getBaseTex();
}

void FoamFx::prepare(const TMatrix4 &view_tm, const TMatrix4 &proj_tm)
{
  TIME_D3D_PROFILE(FoamFx_Prepare);

  set_inv_globtm_to_shader(view_tm, proj_tm, false);

  ShaderGlobal::set_texture(foam_maskVarId, maskTarget);
  ShaderGlobal::set_texture(foam_mask_depthVarId, maskDepth);

  SCOPE_RENDER_TARGET;
  d3d::set_render_target();

  overfoam = fullTargetPool->acquire();
  RTarget::Ptr underfoam = fullTargetPool->acquire();
  d3d::set_render_target(0, overfoam->getBaseTex(), 0);
  d3d::set_render_target(1, underfoam->getBaseTex(), 0);
  foamPrepare.render();

  debugTextures[FoamTexture::OVERFOAM] = overfoam->getBaseTex();
  debugTextures[FoamTexture::UNDERFOAM] = underfoam->getBaseTex();

  underfoam_downsampled = downTargetPool->acquire();
  RTarget::Ptr temp = tempTargetPool->acquire();

  ShaderGlobal::set_color4(texszVarId, Color4(0.5f, -0.5f, 1.0f / width, 1.0f / height));

  d3d::set_render_target(temp->getBaseTex(), 0);
  ShaderGlobal::set_texture(texVarId, underfoam->getTexId());
  ShaderGlobal::set_sampler(tex_samplerstateVarId, d3d::request_sampler({}));
  foamBlurX.render();
  d3d::set_render_target(underfoam_downsampled->getBaseTex(), 0);
  ShaderGlobal::set_texture(texVarId, temp->getTexId());
  foamBlurY.render();
  ShaderGlobal::set_sampler(tex_samplerstateVarId, d3d::SamplerHandle::Invalid);

  debugTextures[FoamTexture::UNDERFOAM_DOWNSAMPLED] = underfoam_downsampled->getBaseTex();
}

void FoamFx::renderHeight()
{
  TIME_D3D_PROFILE(FoamFx_RenderHeight);

  ShaderGlobal::set_texture(foam_overfoam_texVarId, *overfoam);

  foamHeight.render();
}

void FoamFx::renderFoam()
{
  TIME_D3D_PROFILE(FoamFx_RenderFoam);

  ShaderGlobal::set_texture(foam_overfoam_texVarId, *overfoam);
  ShaderGlobal::set_texture(foam_underfoam_texVarId, *underfoam_downsampled);
  foamCombine.render();
}

void FoamFx::beginMaskRender()
{
  d3d::set_render_target(maskTarget.getBaseTex(), 0);
  d3d::set_depth(maskDepth.getBaseTex(), DepthAccess::RW);
  d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0, 0, 0);
}

void foam_debug()
{
  if (ImGui::Button("Reload Shaders"))
  {
    console::command("render.reload_shaders");
  }
  if (ImGui::Button("Phys to boat"))
  {
    console::command("phys.teleport 38.2391 0.0305787 351.952");
  }
  if (ImGui::Button("Switch FoamFx"))
  {
    console::command("render.waterfoam.useFoamFx");
  }
  if (ImGui::Button("Slow down"))
  {
    console::command("app.timeSpeed 0.00001");
  }
  ImGui::SameLine();
  if (ImGui::Button("Speed up"))
  {
    console::command("app.timeSpeed 1");
  }
  for (const auto &tex : debugTextures)
  {
    if (tex.second)
    {
      String label(0, "Show '%s'", getFoamTextureName(tex.first), tex.second->getTexName());
      if (ImGui::Button(label.c_str()))
      {
        String command(0, "render.show_tex %s", tex.second->getTexName());
        console::command(command.c_str());
      }
      ImGui::SameLine();
      ImGui::Text("%s", tex.second->getTexName());
    }
  }
  for (const auto &tex : debugTextures2)
  {
    String label(0, "Show '%s'", tex);
    if (ImGui::Button(label.c_str()))
    {
      String command(0, "render.show_tex %s", tex);
      console::command(command.c_str());
    }
  }

  static int prepareDebugStepVarId = get_shader_variable_id("foam_prepare_debug_step");
  static int prepareStep = ShaderGlobal::get_int(prepareDebugStepVarId);
  if (ImGui::BeginCombo("Prepare step", prepare_debug_step[prepareStep]))
  {
    int currentStep = 0;
    for (const auto &stepName : prepare_debug_step)
    {
      const bool selected = currentStep == prepareStep;
      if (ImGui::Selectable(stepName, selected))
      {
        prepareStep = currentStep;
        ShaderGlobal::set_int(prepareDebugStepVarId, prepareStep);
      }
      currentStep++;
    }
    ImGui::EndCombo();
  }
}

REGISTER_IMGUI_WINDOW_EX("Render", "FoamFx debug", nullptr, 100, ImGuiWindowFlags_MenuBar, foam_debug);
