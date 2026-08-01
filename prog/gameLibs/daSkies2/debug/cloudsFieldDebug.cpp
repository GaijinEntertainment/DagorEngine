// Copyright (C) Gaijin Games KFT.  All rights reserved.

size_t daSkies2_clouds_field_debug_pull = 0;
#if DAGOR_DBGLEVEL > 0
#include <math.h>
#include <render/daFrameGraph/daFG.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <util/dag_convar.h>

static dafg::NodeHandle cloudsFieldDebugNode;
static bool debugRenderActive;
static float debugRenderRange = 20000.f;
static int debugRenderAxis = -1;
static int debugRenderSlice = 0;
static int debugRenderSliceCnt = 1;
static float debugRenderSigmaMod = 0;
static bool debugRenderHeatmap = false;
static float debugRenderHeatmapScale = 1;
static bool debugRenderLowDensityDetection = false;
static float debugRenderLowDensityEpsilon = 0.001f;
extern ConVarT<bool, false> disable_cloud_render;
namespace var
{
static ShaderVariableInfo debug_clouds_field_range("debug_clouds_field_range", true);
static ShaderVariableInfo debug_clouds_field_axis("debug_clouds_field_axis", true);
static ShaderVariableInfo debug_clouds_field_slice("debug_clouds_field_slice", true);
static ShaderVariableInfo debug_clouds_field_slice_cnt("debug_clouds_field_slice_cnt", true);
static ShaderVariableInfo debug_clouds_field_sigma_mod("debug_clouds_field_sigma_mod", true);
static ShaderVariableInfo debug_clouds_field_heatmap("debug_clouds_field_heatmap", true);
static ShaderVariableInfo debug_clouds_field_heatscale("debug_clouds_field_heatscale", true);
static ShaderVariableInfo debug_clouds_field_low_density_detect("debug_clouds_field_low_density_detect", true);
static ShaderVariableInfo debug_clouds_field_low_density_epsilon("debug_clouds_field_low_density_epsilon", true);
} // namespace var

static dafg::NodeHandle createCloudFieldDebugNodes()
{
  return dafg::register_node("cloud_field_debug_render", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto debugNs = registry.root() / "debug";
    auto colorTarget = debugNs.modifyTexture("target_for_debug");
    registry.requestRenderPass().color({colorTarget});
    registry.readTexture("depth_for_postfx").atStage(dafg::Stage::PS).bindToShaderVar("depth_gbuf");

    return [debugRenderer = PostFxRenderer("debug_render_cloud_field")] { debugRenderer.render(); };
  });
}


static void cloudFieldDebugWindow()
{
  if (ImGui::Checkbox("Allow Render", &debugRenderActive))
    cloudsFieldDebugNode = debugRenderActive ? createCloudFieldDebugNodes() : dafg::NodeHandle{};

  disable_cloud_render.imguiWidget("Disable Cloud Rendering");

  if (ImGui::SliderFloat("Debug Transmittance Scale (logarithmic)", &debugRenderSigmaMod, -10, 3))
    ShaderGlobal::set_float(var::debug_clouds_field_sigma_mod, exp2(debugRenderSigmaMod));

  ImGui::Separator();

  if (ImGui::SliderFloat("Debug Range", &debugRenderRange, 10.f, 100000.f))
    ShaderGlobal::set_float(var::debug_clouds_field_range, debugRenderRange);

  if (ImGui::SliderInt("Debug Axis (-1: off, 0: x, 1: y, 2: z)", &debugRenderAxis, -1, 2))
    ShaderGlobal::set_int(var::debug_clouds_field_axis, debugRenderAxis);

  if (ImGui::SliderInt("Debug Slice Start", &debugRenderSlice, 0, 512))
    ShaderGlobal::set_int(var::debug_clouds_field_slice, debugRenderSlice);

  if (ImGui::SliderInt("Debug Slice Count", &debugRenderSliceCnt, 1, 100))
    ShaderGlobal::set_int(var::debug_clouds_field_slice_cnt, debugRenderSliceCnt);

  ImGui::Separator();

  if (ImGui::Checkbox("Use heatmap", &debugRenderHeatmap))
    ShaderGlobal::set_int(var::debug_clouds_field_heatmap, debugRenderHeatmap);

  if (ImGui::SliderFloat("Heat map scale", &debugRenderHeatmapScale, 0.01, 5))
    ShaderGlobal::set_float(var::debug_clouds_field_heatscale, debugRenderHeatmapScale);

  ImGui::Separator();

  if (ImGui::Checkbox("Low Density Detection", &debugRenderLowDensityDetection))
    ShaderGlobal::set_int(var::debug_clouds_field_low_density_detect, debugRenderLowDensityDetection);

  if (ImGui::SliderFloat("Low Density Threshold", &debugRenderLowDensityEpsilon, 0, 1))
    ShaderGlobal::set_float(var::debug_clouds_field_low_density_epsilon, debugRenderLowDensityEpsilon);
}

REGISTER_IMGUI_WINDOW("Render", "Clouds Field", cloudFieldDebugWindow);
#endif
