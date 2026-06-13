// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include <frontend/internalRegistry.h>
#include <backend/intermediateRepresentation.h>

#include <imgui.h>
#include <imgui-node-editor/imgui_bezier_math.h>


[[maybe_unused]] constexpr auto IMGUI_WINDOW_GROUP_FG2 = "FrameGraph WIP";
[[maybe_unused]] constexpr auto IMGUI_USG_WIN_NAME = "User Graph Visualizer";
[[maybe_unused]] constexpr auto IMGUI_IRG_WIN_NAME = "IR Graph Visualizer";
[[maybe_unused]] constexpr auto IMGUI_RES_WIN_NAME = "Resourse Visualizer";
[[maybe_unused]] constexpr auto IMGUI_TEX_WIN_NAME = "Texture Visualizer";


namespace dafg::visualization
{

ImU32 color_by_hash(size_t hash);
ImU32 apply_alpha_coeff(ImU32 color, float k = 1.f);
bool rect_is_visible(const ImRect &rect, const ImRect &view_rect);
bool spline_is_visible(const ImCubicBezierPoints &spline, const ImRect &view_rect);
void add_triangle_filled_multicolor(ImDrawList *draw_list, const ImVec2 &p0, const ImVec2 &p1, const ImVec2 &p2, ImU32 c0, ImU32 c1,
  ImU32 c2);


const char *side_effect_name(const SideEffects effect);
const char *bind_type_name(const BindingType type);
const char *vrs_combiner_name(const VariableRateShadingCombiner comb);
const char *res_type_name(const ResourceType type);
const char *res_clear_flag_name(const ResourceClearFlags flags);
const char *compare_func_name(const uint8_t func);
const char *stencil_op_name(const uint8_t op);
const char *blend_op_name(const uint8_t op);
const char *blend_factor_name(const uint8_t factor);
const char *texture_type_name(const D3DResourceType type);
const char *clear_stage_name(const intermediate::ClearStage stage);
const char *activation_action_name(const ResourceActivationAction action);


const char *override_state_descr(const shaders::OverrideState &state);
const char *state_reqs_descr(const NodeStateRequirements &reqs);
const char *multiplexing_descr(const multiplexing::Mode mode);
const char *clear_value_descr(const ResourceClearValue &value);
const char *auto_res_data_descr(const AutoResTypeData &auto_res, const float mul);

const char *texture_flags_descr(const int flags);
const char *buffer_flags_descr(const int flags);

void format_pretty(ImDrawList *draw_list, const int format);


void request_user_node_focus(const NodeNameId id);
bool check_user_node_focus_request();
NodeNameId get_user_node_focus_request();

void request_user_resource_focus(const ResNameId id);
bool check_user_resource_focus_request();
ResNameId get_user_resource_focus_request();


struct CanvasCamera
{
  const ImVec2 initCanvasOffset = ImVec2(0.f, 0.f);
  const float initCanvasScale = 1.f;
  const ImVec2 scaleBounds = ImVec2(0.01f, 2.f);
  const float scaleSens = 1.2f;

  ImVec2 canvasOffset = ImVec2(0.f, 0.f);
  float canvasScale = 1.f;

public:
  float getZoom() const { return canvasScale; }

  void zoomIn() { canvasScale = min(canvasScale * scaleSens, scaleBounds.y); }
  void zoomOut() { canvasScale = max(canvasScale / scaleSens, scaleBounds.x); }

  void reset()
  {
    canvasOffset = initCanvasOffset;
    canvasScale = initCanvasScale;
  }
};

} // namespace dafg::visualization