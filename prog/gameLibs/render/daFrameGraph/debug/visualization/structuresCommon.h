// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include <frontend/internalRegistry.h>
#include <backend/intermediateRepresentation.h>
#include <runtime/typeDb.h>

#include <imgui.h>
#include <imgui-node-editor/imgui_bezier_math.h>


[[maybe_unused]] constexpr auto IMGUI_WINDOW_GROUP_FG2 = "FrameGraph WIP";
[[maybe_unused]] constexpr auto IMGUI_USG_WIN_NAME = "User Graph Visualizer";
[[maybe_unused]] constexpr auto IMGUI_IRG_WIN_NAME = "IR Graph Visualizer";
[[maybe_unused]] constexpr auto IMGUI_RES_WIN_NAME = "Resourse Visualizer";
[[maybe_unused]] constexpr auto IMGUI_UNTRACKED_WIN_NAME = "Untracked Resources";
[[maybe_unused]] constexpr auto IMGUI_TEX_WIN_NAME = "Texture Visualizer";
[[maybe_unused]] constexpr auto IMGUI_GPU_CAPTURE_WIN_NAME = "GPU Capture";


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
const char *buffer_bind_name(const int flags);

void format_pretty(ImDrawList *draw_list, const int format);
const char *multi_line_name(const char *name);


void request_user_node_focus(const NodeNameId id);
bool check_user_node_focus_request();
NodeNameId get_user_node_focus_request();

void request_user_resource_focus(const ResNameId id);
bool check_user_resource_focus_request();
ResNameId get_user_resource_focus_request();


void print_blob_structured(const ResourceSubtypeTag tag, void *data);


struct CanvasCamera
{
  const ImVec2 initCenter = ImVec2(0.f, 0.f);
  const float initScale = 1.f;
  const ImVec2 scaleBounds = ImVec2(0.01f, 2.f);
  const float scaleSens = 1.2f;

  ImVec2 center = initCenter;
  float scale = initScale;

  ImVec2 targetCenter = initCenter;
  const float targetingDistance = 5.f;
  const float targetingSpeed = 15.f;

  float targetScale = initScale;
  const float scalingRelativeThreshold = 0.01f;
  const float scalingSpeed = 15.f;

public:
  void targetCamera(const ImVec2 point) { targetCenter = point; }
  void moveCamera(const ImVec2 delta) { targetCenter += delta; }

  bool zoomIn()
  {
    if (targetScale != scaleBounds.y)
    {
      targetScale = min(targetScale * scaleSens, scaleBounds.y);
      return true;
    }
    return false;
  }
  bool zoomOut()
  {
    if (targetScale != scaleBounds.x)
    {
      targetScale = max(targetScale / scaleSens, scaleBounds.x);
      return true;
    }
    return false;
  }
  void scalePointRelative(const ImVec2 canvas_point)
  {
    const ImVec2 relativeOld = canvas_point - center;
    const ImVec2 relativeNew = relativeOld * scale / targetScale;
    targetCamera(center + relativeOld - relativeNew);
  }

  void update(const float dt = 0.f, const bool instantly = true)
  {
    if (center != targetCenter)
    {
      const ImVec2 delta = targetCenter - center;
      const ImVec2 step = delta * targetingSpeed * dt;
      if (instantly || ImLength(delta) < targetingDistance || ImLength(step) > ImLength(delta))
        center = targetCenter;
      else
        center += step;
    }

    if (scale != targetScale)
    {
      const float delta = targetScale - scale;
      const float step = delta * scalingSpeed * dt;
      if (instantly || abs(delta) / scale < scalingRelativeThreshold || abs(step) > abs(delta))
        scale = targetScale;
      else
        scale += step;
    }
  };

  void reset()
  {
    center = targetCenter = initCenter;
    scale = targetScale = initScale;
  }
};

// searches for subsequence 'search' in 'primary_string' and 'secondary_string' and returns combined score;
// if 'search' is subsequence, saves matches as <position,size> of substrings;
// if not, corresponding vector will be cleared and its score will be 0;
// primary string score is twice more valuable than secondary;
float fuzzy_search(const eastl::string_view search, const eastl::string_view primary_string, const eastl::string_view secondary_string,
  dag::Vector<eastl::pair<uint16_t, uint16_t>> &primary_matches, dag::Vector<eastl::pair<uint16_t, uint16_t>> &secondary_matches);

// prints 'text' starting at position 'pos';
// pairs from 'highlight_regions' are <position,size> of substrings, that will be printed with 'highlight_color';
// other characters will be printed with 'normal_color';
// regions in 'highlight_regions' must go in ascending order and be inside text, without overlapping;
// returns position after last character in 'text'
ImVec2 draw_highlighted_text(const ImVec2 pos, const eastl::string_view text, const ImU32 normal_color, const ImU32 highlight_color,
  const dag::Vector<eastl::pair<uint16_t, uint16_t>> &highlight_regions);

} // namespace dafg::visualization
