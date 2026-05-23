// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "userGraphVisualizer.h"

#include <debug/textureVisualization.h>

#include <runtime/runtime.h>
#include <frontend/nodeTracker.h>
#include <frontend/nameResolver.h>
#include <backend/simdBitVector.h>
#include <backend/simdBitMatrix.h>

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <imgui-node-editor/imgui_canvas.h>
#include <imgui-node-editor/imgui_bezier_math.h>

#include <generic/dag_reverseView.h>
#include <generic/dag_bitset.h>
#include <util/dag_convar.h>
#include <util/dag_console.h>
#include <render/debugTexOverlay.h>
#include <perfMon/dag_statDrv.h>


// Originally inspired by mr ocornut
// https://gist.github.com/ocornut/7e9b3ec566a333d725d4


constexpr float NODE_MIN_X_SIZE = 300.f;
constexpr float NODE_MIN_Y_SIZE = 350.f;
constexpr float NODE_Y_ANCHORS_START = 100.f;
constexpr float NODE_ANCHORS_MARGIN = 5.f;
constexpr float NODE_FOCUS_BORDER_WIDTH = 5.f;
constexpr ImVec2 NODE_BORDER_PADDING = ImVec2{10.f, 15.f};

constexpr float OBJ_X_BASE_MARGIN = 3.0f * NODE_MIN_X_SIZE;
constexpr float OBJ_Y_BASE_MARGIN = 1.0f * NODE_MIN_Y_SIZE;

constexpr float NAMESPACE_BORDER_WIDTH = 2.f;
constexpr ImVec2 NAMESPACE_BORDER_PADDING = ImVec2{50.f, 50.f};

constexpr ImVec2 NAMETAG_PADDING = ImVec2{5.f, 5.f};

constexpr float BASE_DEP_WIDTH = 10.f;
constexpr float RES_FOCUS_WIDTH = 2.0f * BASE_DEP_WIDTH;
constexpr float DEP_INSPECT_WIDTH = 1.8f * BASE_DEP_WIDTH;
constexpr float CYCLED_DEP_WIDTH = 1.6f * BASE_DEP_WIDTH;
constexpr float DISABLED_DEP_WIDTH = 1.4f * BASE_DEP_WIDTH;

constexpr float HISTORY_WAVE_LEN = NODE_MIN_X_SIZE / 3.f;
constexpr float HISTORY_WAVE_HEIGHT = BASE_DEP_WIDTH * 3.f;


constexpr ImU32 NODE_BASE_COLOR = IM_COL32(128, 128, 128, 255); // rgb(128, 128, 128)

constexpr ImU32 DEP_COLOR_PREV = IM_COL32(0, 255, 255, 255);     // rgb(0, 255, 255)
constexpr ImU32 DEP_COLOR_FOLLOW = IM_COL32(128, 128, 255, 255); // rgb(128, 128, 255)
constexpr ImU32 DEP_COLOR_MOD = IM_COL32(255, 255, 0, 255);      // rgb(255, 255, 0)
constexpr ImU32 DEP_COLOR_READ = IM_COL32(0, 128, 0, 255);       // rgb(0, 128, 0)
constexpr ImU32 DEP_COLOR_CONS = IM_COL32(255, 0, 255, 255);     // rgb(255, 0, 255)
constexpr ImU32 DEP_COLOR_HIST = IM_COL32(128, 255, 0, 255);     // rgb(128, 255, 0)

constexpr ImU32 FOCUS_COLOR = IM_COL32(255, 255, 128, 255);     // rgb(255, 255, 128)
constexpr ImU32 RES_INSPECT_COLOR = IM_COL32(255, 128, 0, 255); // rgb(255, 128, 0)
constexpr ImU32 CYCLED_DEP_COLOR = IM_COL32(255, 0, 0, 255);    // rgb(255, 0, 0)
constexpr ImU32 DISABLED_DEP_COLOR = CYCLED_DEP_COLOR;


constexpr ImU32 OPT_POPUP_POS_COLOR = IM_COL32(64, 196, 64, 255);  // rgb(64, 196, 64)
constexpr ImU32 OPT_POPUP_WAR_COLOR = IM_COL32(196, 196, 64, 255); // rgb(196, 196, 64)
constexpr ImU32 OPT_POPUP_NEG_COLOR = IM_COL32(196, 64, 64, 255);  // rgb(196, 64, 64)

constexpr ImU32 SIDE_EFF_NON_COLOR = IM_COL32(64, 196, 64, 255);  // rgb(64, 196, 64)
constexpr ImU32 SIDE_EFF_INT_COLOR = IM_COL32(196, 64, 196, 255); // rgb(196, 64, 196)
constexpr ImU32 SIDE_EFF_EXT_COLOR = IM_COL32(196, 64, 0, 255);   // rgb(196, 64, 0)

constexpr auto NODE_EXEX_REQ_POPUP = "node_exec_reqs";
constexpr auto NODE_RENDER_REQ_POPUP = "node_render_reqs";
constexpr auto NODE_BINDINGS_POPUP = "node_bindings";
constexpr auto NODE_IDX_VTX_POPUP = "node_iv_bufs";
constexpr auto EDGE_RESOURCES_POPUP = "edge_resources";


template <class T, class... Ts>
static void hash_pack(size_t &hash, T first, const Ts &...other)
{
  hash ^= eastl::hash<T>{}(first) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  (hash_pack(hash, other), ...);
}


namespace dafg
{

extern ConVarT<bool, false> recompile_graph;

void show_dependencies_window() { imgui_window_set_visible(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME, true); }

namespace visualization::usergraph
{

static const eastl::array<eastl::pair<NodeColorationType, const char *>, static_cast<size_t>(NodeColorationType::COUNT)>
  nodeColorationTypeArray{eastl::pair{NodeColorationType::None, (const char *)"None"},
    eastl::pair{NodeColorationType::Framebuffer, (const char *)"Color by framebuffers"},
    eastl::pair{NodeColorationType::Pass, (const char *)"Color by passes"}};

static const char *node_coloration_name_by_type(NodeColorationType type)
{
  return nodeColorationTypeArray[static_cast<size_t>(type)].second;
}

static const eastl::array<eastl::pair<ResourceFocusType, const char *>, static_cast<size_t>(ResourceFocusType::COUNT)>
  resourceFocusTypeArray{eastl::pair{ResourceFocusType::All, (const char *)"Show all"},
    eastl::pair{ResourceFocusType::Resource, (const char *)"Focus on resource"},
    eastl::pair{ResourceFocusType::ResourceAndRenames, (const char *)"Focus on resource and renames"}};

static const char *res_focus_name_by_type(ResourceFocusType type) { return resourceFocusTypeArray[static_cast<size_t>(type)].second; }

struct DependencyTypeInfo
{
  eastl::string name;
  uint32_t imguiColor = 0;
  bool visible = true;
};

static eastl::array<DependencyTypeInfo, static_cast<size_t>(DependencyType::COUNT)> dependencyInfoByTypeArray{
  DependencyTypeInfo{"Precedes", DEP_COLOR_PREV},
  DependencyTypeInfo{"Follows", DEP_COLOR_FOLLOW},
  DependencyTypeInfo{"Modifies", DEP_COLOR_MOD},
  DependencyTypeInfo{"Reads", DEP_COLOR_READ},
  DependencyTypeInfo{"Consumes", DEP_COLOR_CONS},
  DependencyTypeInfo{"Reads History", DEP_COLOR_HIST},
};

static DependencyTypeInfo &dependency_info_by_type(const DependencyType type)
{
  return dependencyInfoByTypeArray[static_cast<size_t>(type)];
}


static inline const char *side_effect_name_by_type(const SideEffects type)
{
  switch (type)
  {
    case SideEffects::None: return "None";
    case SideEffects::Internal: return "Internal";
    case SideEffects::External: return "External";
    default: return "";
  }
}

static inline const char *bind_type_name_by_type(const BindingType type)
{
  switch (type)
  {
    case BindingType::ShaderVar: return "SV";
    case BindingType::ViewMatrix: return "VM";
    case BindingType::ProjMatrix: return "PM";
    case BindingType::Invalid: return "IN";
    default: return "";
  }
}

static inline const char *vrs_combiner_name_by_type(const VariableRateShadingCombiner comb)
{
  switch (comb)
  {
    case VariableRateShadingCombiner::VRS_PASSTHROUGH: return "PASSTHROUGH";
    case VariableRateShadingCombiner::VRS_OVERRIDE: return "OVERRIDE";
    case VariableRateShadingCombiner::VRS_MIN: return "MIN";
    case VariableRateShadingCombiner::VRS_MAX: return "MAX";
    case VariableRateShadingCombiner::VRS_SUM: return "SUM";
    default: return "";
  }
}

static inline const char *compare_func_name_by_type(const uint8_t func)
{
  switch (func)
  {
    case CMPF::CMPF_NEVER: return "NEVER";
    case CMPF::CMPF_LESS: return "<";
    case CMPF::CMPF_EQUAL: return "=";
    case CMPF::CMPF_LESSEQUAL: return "<=";
    case CMPF::CMPF_GREATER: return ">";
    case CMPF::CMPF_NOTEQUAL: return "!=";
    case CMPF::CMPF_GREATEREQUAL: return ">=";
    case CMPF::CMPF_ALWAYS: return "ALWAYS";
    default: return "";
  }
}

static inline const char *stencil_op_name_by_type(const uint8_t op)
{
  switch (op)
  {
    case STNCLOP_KEEP: return "KEEP";
    case STNCLOP_ZERO: return "ZERO";
    case STNCLOP_REPLACE: return "REPLACE";
    case STNCLOP_INCRSAT: return "INCRSAT";
    case STNCLOP_DECRSAT: return "DECRSAT";
    case STNCLOP_INVERT: return "INVERT";
    case STNCLOP_INCR: return "INCREASE";
    case STNCLOP_DECR: return "DECREASE";
    default: return "";
  }
}

static inline const char *blend_op_name_by_type(const uint8_t op)
{
  switch (op)
  {
    case BLENDOP::BLENDOP_ADD: return "ADD";
    case BLENDOP::BLENDOP_SUBTRACT: return "SUB";
    case BLENDOP::BLENDOP_REVSUBTRACT: return "REV SUB";
    case BLENDOP::BLENDOP_MIN: return "MIN";
    case BLENDOP::BLENDOP_MAX: return "MAX";
    default: return "";
  }
}

static inline const char *blend_factor_name_by_type(const uint8_t factor)
{
  switch (factor)
  {
    case BLEND_FACTOR::BLEND_ZERO: return "ZERO";
    case BLEND_FACTOR::BLEND_ONE: return "ONE";
    case BLEND_FACTOR::BLEND_SRCCOLOR: return "SRC COLOR";
    case BLEND_FACTOR::BLEND_INVSRCCOLOR: return "INV SRC COLOR";
    case BLEND_FACTOR::BLEND_SRCALPHA: return "SRC ALPHA";
    case BLEND_FACTOR::BLEND_INVSRCALPHA: return "INV SRC ALPHA";
    case BLEND_FACTOR::BLEND_DESTALPHA: return "DST ALPHA";
    case BLEND_FACTOR::BLEND_INVDESTALPHA: return "INV DST ALPHA";
    case BLEND_FACTOR::BLEND_DESTCOLOR: return "DST COLOR";
    case BLEND_FACTOR::BLEND_INVDESTCOLOR: return "INV DST COLOR";
    case BLEND_FACTOR::BLEND_SRCALPHASAT: return "SRC ALPHA SAT";
    case BLEND_FACTOR::BLEND_BOTHINVSRCALPHA: return "BOTH INV SRC ALPHA";
    case BLEND_FACTOR::BLEND_BLENDFACTOR: return "BLEND FACTOR";
    case BLEND_FACTOR::BLEND_INVBLENDFACTOR: return "INV BLEND FACTOR";

    case EXT_BLEND_FACTOR::EXT_BLEND_SRC1COLOR: return "SRC 1 COLOR";
    case EXT_BLEND_FACTOR::EXT_BLEND_INVSRC1COLOR: return "INV SRC 1 COLOR";
    case EXT_BLEND_FACTOR::EXT_BLEND_SRC1ALPHA: return "SRC 1 ALPHA";
    case EXT_BLEND_FACTOR::EXT_BLEND_INVSRC1ALPHA: return "INV SRC 1 ALPHA";

    default: return "";
  }
}

static const char *state_reqs_descr(const NodeStateRequirements &reqs)
{
  static String res;
  res.clear();

  res.aprintf(0, "Support wireframe: %s\n", reqs.supportsWireframe ? "Y" : "N");

  if (!(reqs.vrsState.rateX == 1 && reqs.vrsState.rateY == 1 &&
        reqs.vrsState.vertexCombiner == VariableRateShadingCombiner::VRS_PASSTHROUGH &&
        reqs.vrsState.pixelCombiner == VariableRateShadingCombiner::VRS_PASSTHROUGH))
  {
    res.append("\n");
    res.aprintf(0, "Vrs rates X/Y: %d %d\n", reqs.vrsState.rateX, reqs.vrsState.rateY);
    res.aprintf(0, "Vrs vtx combiner: %s\n", vrs_combiner_name_by_type(reqs.vrsState.vertexCombiner));
    res.aprintf(0, "Vrs pix combiner: %s\n", vrs_combiner_name_by_type(reqs.vrsState.pixelCombiner));
  }

  if (reqs.pipelineStateOverride)
  {
    res.append("\n");

    const auto &override = *reqs.pipelineStateOverride;

    res.aprintf(0, "Color write mask: 0x%X\n", override.colorWr);

    res.append("Z overrides:\n");
    if (override.isOn(shaders::OverrideState::Z_TEST_DISABLE))
      res.append("  test disabled\n");
    if (override.isOn(shaders::OverrideState::Z_WRITE_DISABLE))
      res.append("  write disabled\n");
    if (override.isOn(shaders::OverrideState::Z_WRITE_ENABLE))
      res.append("  write enabled\n");
    if (override.isOn(shaders::OverrideState::Z_BOUNDS_ENABLED))
      res.append("  bounds enabled\n");
    if (override.isOn(shaders::OverrideState::Z_CLAMP_ENABLED))
      res.append("  clamp enabled\n");
    if (override.isOn(shaders::OverrideState::Z_FUNC))
      res.aprintf(0, "  function: %s\n", compare_func_name_by_type(override.zFunc));
    if (override.isOn(shaders::OverrideState::Z_BIAS))
      res.aprintf(0, "  bias: %f\n  slope: %f\n", override.zBias, override.slopeZBias);

    if (override.isOn(shaders::OverrideState::STENCIL))
      res.aprintf(0,
        "Stencil satate override:\n"
        "  func: %s\n"
        "  fail: 0x%X\n"
        "  zFail: 0x%X\n"
        "  pass: 0x%X\n"
        "  readMask: 0x%X\n"
        "  writeMask: 0x%X\n",
        compare_func_name_by_type(override.stencil.func), stencil_op_name_by_type(override.stencil.fail),
        stencil_op_name_by_type(override.stencil.zFail), stencil_op_name_by_type(override.stencil.pass), override.stencil.readMask,
        override.stencil.writeMask);

    res.append("Blend overrides:\n");
    if (override.isOn(shaders::OverrideState::BLEND_OP))
      res.aprintf(0, "  operation: %s\n", blend_op_name_by_type(override.blendOp));
    if (override.isOn(shaders::OverrideState::BLEND_OP_A))
      res.aprintf(0, "  operation A: %s\n", blend_op_name_by_type(override.blendOpA));
    if (override.isOn(shaders::OverrideState::BLEND_SRC_DEST))
      res.aprintf(0, "  factors SRC/DST:\n    %d\n    %d\n", blend_factor_name_by_type(override.sblend),
        blend_factor_name_by_type(override.dblend));
    if (override.isOn(shaders::OverrideState::BLEND_SRC_DEST_A))
      res.aprintf(0, "  factors A SRC/DST:\n    %d\n    %d\n", blend_factor_name_by_type(override.sblenda),
        blend_factor_name_by_type(override.dblenda));

    res.append("Culling overrides:\n");
    if (override.isOn(shaders::OverrideState::CULL_NONE))
      res.append("  disabled\n");
    if (override.isOn(shaders::OverrideState::FLIP_CULL))
      res.append("  flip cull\n");

    if (override.isOn(shaders::OverrideState::FORCED_SAMPLE_COUNT))
      res.aprintf(0, "Forced sample count: %d\n", override.forcedSampleCount);
    if (override.isOn(shaders::OverrideState::CONSERVATIVE))
      res.append("Conservative rasterization\n");
    if (override.isOn(shaders::OverrideState::SCISSOR_ENABLED))
      res.append("Scissor test\n");
    if (override.isOn(shaders::OverrideState::ALPHA_TO_COVERAGE))
      res.append("Alpha-to-coverage\n");
  }

  return res.c_str();
}

static const char *multiplexing_descr(const multiplexing::Mode mode)
{
  static String res;
  res.clear();

  if (mode == multiplexing::Mode::None)
  {
    res.append("Exec once\n");
  }
  else
  {
    if (eastl::to_underlying(mode & multiplexing::Mode::SuperSampling))
      res.append("Super Sampling\n");
    if (eastl::to_underlying(mode & multiplexing::Mode::SubSampling))
      res.append("Sub Sampling\n");
    if (eastl::to_underlying(mode & multiplexing::Mode::Viewport))
      res.append("Viewport\n");
    if (eastl::to_underlying(mode & multiplexing::Mode::CameraInCamera))
      res.append("Camera In Camera\n");
  }

  return res.c_str();
}


static inline const char *res_type_name_by_type(const ResourceType type)
{
  switch (type)
  {
    case ResourceType::Invalid: return "Invalid";
    case ResourceType::Texture: return "Tex";
    case ResourceType::Buffer: return "Buf";
    case ResourceType::Blob: return "Blob";
    default: return "";
  }
}

static inline const char *res_clear_flag_name_by_type(const ResourceClearFlags type)
{
  switch (type)
  {
    case ResourceClearFlags::RESOURCE_CLEAR_DEPTH: return "Depth";
    case ResourceClearFlags::RESOURCE_CLEAR_STENCIL: return "Stencil";
    case ResourceClearFlags::RESOURCE_CLEAR_ALL_CONTENT: return "All";
    default: return "";
  }
}


static ImU32 color_by_hash(size_t hash)
{
  constexpr size_t HUE_COUNT = 256;

  // Hash some more to get different colors for close numbers
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = (hash >> 16) ^ hash;
  float hue = static_cast<float>((6607 * hash) % HUE_COUNT) / static_cast<float>(HUE_COUNT - 1);

  ImVec4 color(0, 0, 0, 1);
  ImGui::ColorConvertHSVtoRGB(hue, 1.f, 1.f, color.x, color.y, color.z);
  return ImGui::ColorConvertFloat4ToU32(color);
};

static ImU32 apply_alpha_coeff(ImU32 color, float k = 1.f)
{
  return ImGui::ColorConvertFloat4ToU32(ImGui::ColorConvertU32ToFloat4(color) * ImVec4{1.f, 1.f, 1.f, k});
}


static inline bool rect_is_visible(const ImRect &rect, const ImRect &view_rect) { return view_rect.Overlaps(rect); }

static inline bool spline_is_visible(const ImCubicBezierPoints &spline, const ImRect &view_rect)
{
  return !(
    spline.P0.x < view_rect.Min.x && spline.P1.x < view_rect.Min.x && spline.P2.x < view_rect.Min.x && spline.P3.x < view_rect.Min.x ||
    spline.P0.x > view_rect.Max.x && spline.P1.x > view_rect.Max.x && spline.P2.x > view_rect.Max.x && spline.P3.x > view_rect.Max.x ||
    spline.P0.y < view_rect.Min.y && spline.P1.y < view_rect.Min.y && spline.P2.y < view_rect.Min.y && spline.P3.y < view_rect.Min.y ||
    spline.P0.y > view_rect.Max.y && spline.P1.y > view_rect.Max.y && spline.P2.y > view_rect.Max.y && spline.P3.y > view_rect.Max.y);
}


Visualizer::Visualizer(InternalRegistry &int_registry, const DependencyData &dep_data, const intermediate::Graph &ir_graph,
  const PassColoring &coloring) :
  registry(int_registry), depData(dep_data), intermediateGraph(ir_graph), intermediatePassColoring(coloring)
{
  REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME, [&]() { this->draw(); });
}

void Visualizer::draw()
{
  if (ImGui::IsWindowCollapsed())
    return;

  if (updateNeeded)
    updateVisualization();

  hoverState.reset();
  hoverState.window = ImGui::IsWindowHovered();


  drawUI();

  drawCanvas();


  processPopup();

  processInput();

  processTextureDebug();
}

void Visualizer::drawUI()
{
  {
    if (ImGuiDagor::ComboWithFilter("User Node Search", nsNodeNames, focusedNodeIndex, nodeSearchInput, false, true,
          "Search for node...") &&
        focusedNodeIndex != UNKNOWN_INDEX)
    {
      resetFocusedResource();
      setFocusedNode(regNodesRepresent[nsNodeNameIds[focusedNodeIndex]]);
      centerOnFocusedNode();
    }
    hoverState.searchBox |= ImGui::IsItemHovered();

    if (focusedNode.valid())
    {
      ImGui::SameLine();
      if (ImGui::Button("Center on node"))
        centerOnFocusedNode();

      ImGui::SameLine();
      if (ImGui::Button("Reset node focus"))
        resetFocusedNode();
    }
  }

  {
    if (treeWithFilter("FG Managed Resource Search", nsResNames, focusedResourceIndex, resourceSearchInput, true,
          "Search for resource...") &&
        focusedResourceIndex != UNKNOWN_INDEX)
    {
      resetFocusedNode();
      setFocusedResource(regResRepresent[nsResNameIds[focusedResourceIndex]]);
      centerOnFocusedRes();
    }
    hoverState.searchBox |= ImGui::IsItemHovered();

    if (focusedResource.valid())
    {
      ImGui::SameLine();
      if (ImGui::Button("Center on resource"))
        centerOnFocusedRes();

      ImGui::SameLine();
      if (ImGui::Button("Reset resource focus"))
        resetFocusedResource();

      ImGui::SameLine();
      ImGuiDagor::EnumCombo("##resourceRenamesCombo", ResourceFocusType::All,
        focusedResource.hasRenames ? ResourceFocusType::ResourceAndRenames : ResourceFocusType::Resource, focusedResource.type,
        &res_focus_name_by_type, ImGuiComboFlags_WidthFitPreview);
    }
  }

  {
    ImGui::Text("Edge types: ");

    for (auto &info : dependencyInfoByTypeArray)
    {
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(info.imguiColor));
      ImGui::Checkbox(info.name.c_str(), &info.visible);
      ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    if (ImGui::BeginCombo("##coloration", node_coloration_name_by_type(coloration)))
    {
      for (const auto [type, name] : nodeColorationTypeArray)
      {
        const bool selected = coloration == type;
        if (ImGui::Selectable(name, selected))
          coloration = type;
        if (selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::Text("Hierarchical view:");
    ImGui::SameLine();
    ImGui::Checkbox("##hierarchicalView", &hierarchicalView);
  }

  {
    if (ImGui::Button("Recompile"))
      dafg::recompile_graph.set(true);

    ImGui::SameLine();
    if (deselect_button("Deselect"))
      inspectedDependency.reset();

    fg_texture_visualization_imgui_line(registry);

    ImGui::SameLine();
    overlay_checkbox("Overlay mode");
  }

  {
    ImGui::Text("Use mouse wheel button to pan.");
  }
}

void Visualizer::drawCanvas()
{
  if (!canvas.Begin("##scrolling_region", ImVec2(0, 0)))
    return;

  hoverState.canvas = canvas.ViewRect().Contains(ImGui::GetMousePos());
  checkHovering();

  // here, we will perform draw, based on precalculated positions
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  drawList->ChannelsSplit(CanvasChannels::COUNT);
  {
    const auto &layoutToUse = hierarchicalView ? condensedLayout : generalLayout;

    drawNodes(drawList, layoutToUse);

    drawEdges(drawList, layoutToUse);

    if (hierarchicalView)
      drawNodeBoxes(drawList);

    // TODO: generate edges for disabled dependencies and draw them like others
    {
      for (const auto depId : disabledDependencies)
      {
        const auto &dep = registryDependencies[depId];
        const auto resNameId = nameIdByResId(dep.resource);
        auto [fromOffset, fromSize] = layoutToUse.nodes[dep.from];
        auto [toOffset, toSize] = layoutToUse.nodes[dep.to];

        if (fromOffset.x > toOffset.x)
        {
          eastl::swap(fromOffset, toOffset);
          eastl::swap(fromSize, toSize);
        }
        const ImVec2 leftAnchor = fromOffset + ImVec2{0.f, fromSize.y / 2.f};
        const ImVec2 rightAnchor = toOffset + ImVec2{toSize.x, toSize.y / 2.f};
        const float topBound = min(rightAnchor.y, leftAnchor.y) - abs(rightAnchor.x - leftAnchor.x) / 3;
        const float leftBound = leftAnchor.x - (leftAnchor.y - topBound);
        const float rightBound = rightAnchor.x + (rightAnchor.y - topBound);
        const ImCubicBezierPoints backEdgeCurve = {leftAnchor, ImVec2{leftBound, topBound}, ImVec2{rightBound, topBound}, rightAnchor};

        drawList->ChannelsSetCurrent(CanvasChannels::TEXTS);
        drawList->AddBezierCubic(backEdgeCurve.P0, backEdgeCurve.P1, backEdgeCurve.P2, backEdgeCurve.P3, DISABLED_DEP_COLOR,
          DISABLED_DEP_WIDTH);

        if (ImProjectOnCubicBezier(ImGui::GetIO().MousePos, backEdgeCurve).Distance < DISABLED_DEP_WIDTH)
          hoverState.tooltip.aprintf(0, "disabled: %s\n", getName(resNameId));
      }
    }

    // TODO: generate edges for history dependencies and draw them like others
    {
      const auto &[_, depColor, depVisible] = dependency_info_by_type(DependencyType::IMPLICIT_RES_HIST);
      if (depVisible)
      {
        drawList->ChannelsSetCurrent(CanvasChannels::EDGES);

        const float vertOffset = NODE_ANCHORS_MARGIN + 1.5f * BASE_DEP_WIDTH;
        const float wavesThird = HISTORY_WAVE_LEN / 3.f;

        for (auto [nodeId, node] : userNodes.enumerate())
        {
          const auto [offset, size] = layoutToUse.nodes[nodeId];
          ImVec2 leftAnchor = offset + ImVec2{0.f, size.y - vertOffset};
          ImVec2 rightAnchor = offset + ImVec2{size.x, size.y - vertOffset};

          for (const auto depId : node.inHistDeps)
          {
            const auto resId = registryDependencies[depId].resource;

            if (isResourceVisible(resId))
            {
              const ImCubicBezierPoints spline = {leftAnchor, leftAnchor - ImVec2{1.f * wavesThird, HISTORY_WAVE_HEIGHT},
                leftAnchor - ImVec2{2.f * wavesThird, -HISTORY_WAVE_HEIGHT}, leftAnchor - ImVec2{3.f * wavesThird, 0.f}};

              drawList->AddBezierCubic(spline.P0, spline.P1, spline.P2, spline.P3, depColor, BASE_DEP_WIDTH);

              if (ImProjectOnCubicBezier(ImGui::GetIO().MousePos, spline).Distance < BASE_DEP_WIDTH)
                hoverState.tooltip.aprintf(0, "history of: %s\n", getName(nameIdByResId(resId)));
            }

            leftAnchor.y -= 3.f * BASE_DEP_WIDTH;
          }

          for (const auto depId : node.outHistDeps)
          {
            const auto resId = registryDependencies[depId].resource;

            if (isResourceVisible(resId))
            {
              const ImCubicBezierPoints spline = {rightAnchor, rightAnchor + ImVec2{1.f * wavesThird, HISTORY_WAVE_HEIGHT},
                rightAnchor + ImVec2{2.f * wavesThird, -HISTORY_WAVE_HEIGHT}, rightAnchor + ImVec2{3.f * wavesThird, 0.f}};

              drawList->AddBezierCubic(spline.P0, spline.P1, spline.P2, spline.P3, depColor, BASE_DEP_WIDTH);

              if (ImProjectOnCubicBezier(ImGui::GetIO().MousePos, spline).Distance < BASE_DEP_WIDTH)
                hoverState.tooltip.aprintf(0, "to next frame: %s\n", getName(nameIdByResId(resId)));
            }

            rightAnchor.y -= 3.f * BASE_DEP_WIDTH;
          }
        }
      }
    }
  }
  drawList->ChannelsMerge();

  canvas.End();
}

void Visualizer::drawNodes(ImDrawList *draw_list, const CanvasLayout &layout)
{
  const ImRect canvasView = canvas.ViewRect();
  const bool mouseRclick = ImGui::IsMouseClicked(ImGuiMouseButton_Right);

  for (auto [nodeId, nodeRect] : layout.nodes.enumerate())
  {
    const ImVec2 leftTopPos = nodeRect.offset;
    const ImVec2 rightBottomPos = nodeRect.offset + nodeRect.size;
    if (!rect_is_visible(ImRect{leftTopPos, rightBottomPos}, canvasView))
      continue;

    const auto &node = userNodes[nodeId];
    const auto nodeNameId = nameIdByNodeId(nodeId);
    const auto nodeName = getName(nodeNameId);
    auto &nodeData = registry.nodes[node.regId];

    const ImVec2 nameTagSize = ImMax(ImGui::CalcTextSize(nodeName.data()), ImVec2{nodeRect.size.x, 0.f}) + 2.f * NAMETAG_PADDING;
    const ImVec2 nameTagLeftTopPos = leftTopPos - ImVec2{NAMETAG_PADDING.x, nameTagSize.y / 2};
    const ImVec2 nameTagRightBottomPos = nameTagLeftTopPos + nameTagSize;

    // draw node rectangle and borders
    {
      draw_list->ChannelsSetCurrent(CanvasChannels::NODES);

      ImU32 nodeFillColor = NODE_BASE_COLOR;
      switch (coloration)
      {
        case NodeColorationType::None: break;
        case NodeColorationType::Framebuffer: nodeFillColor = node.fbImColor; break;
        case NodeColorationType::Pass: nodeFillColor = node.passImColor; break;
        default: break;
      }
      if (nodeData.enabled)
        nodeFillColor = apply_alpha_coeff(nodeFillColor, 0.9f);
      else
        nodeFillColor = apply_alpha_coeff(nodeFillColor, 0.5f);

      if (nodeId == focusedNode.id)
        draw_list->AddRect(leftTopPos - ImVec2{NODE_FOCUS_BORDER_WIDTH, NODE_FOCUS_BORDER_WIDTH},
          rightBottomPos + ImVec2{NODE_FOCUS_BORDER_WIDTH, NODE_FOCUS_BORDER_WIDTH}, FOCUS_COLOR, 4.0f, 0, NODE_FOCUS_BORDER_WIDTH);

      if (node.cycled)
        draw_list->AddRect(leftTopPos - ImVec2{DISABLED_DEP_WIDTH, DISABLED_DEP_WIDTH},
          rightBottomPos + ImVec2{DISABLED_DEP_WIDTH, DISABLED_DEP_WIDTH}, CYCLED_DEP_COLOR, 4.0f, 0, DISABLED_DEP_WIDTH);

      draw_list->AddRectFilled(leftTopPos, rightBottomPos, nodeFillColor, 4.0f);
      draw_list->AddRectFilled(nameTagLeftTopPos, nameTagRightBottomPos, NODE_BASE_COLOR, 4.0f);
    }

    // draw texts and buttons
    {
      draw_list->ChannelsSetCurrent(CanvasChannels::TEXTS);

      if (hoverState.node == nodeId)
        switch (coloration)
        {
          case NodeColorationType::None: break;
          case NodeColorationType::Framebuffer: break;
          case NodeColorationType::Pass:
          {
            if (node.passColor != UNKNOWN_PASS_COLOR)
              hoverState.tooltip.printf(0, "Pass color: %d\n", eastl::to_underlying(node.passColor));
            else
              hoverState.tooltip.printf(0, "Culled node\n");
            break;
          }
          default: break;
        }

      {
        ImGui::SetCursorScreenPos(nameTagLeftTopPos + NAMETAG_PADDING);
        ImGui::TextUnformatted(nodeName.data());
        if (ImGui::IsItemHovered())
          hoverState.tooltip.printf(0, "nameId: %d\ngeneration: %d\npriority: %d", eastl::to_underlying(nodeNameId),
            nodeData.generation, nodeData.priority);
      }

      ImGui::SetCursorScreenPos(leftTopPos + NODE_BORDER_PADDING);
      ImGui::BeginGroup();
      {
        static const auto conditionalPopup = [&, nodeId = nodeId](const char *label, const bool condition,
                                               const char *popup_name = nullptr, const char *hover_msg = "R-click for details") {
          ImGui::TextUnformatted(label);
          ImGui::SameLine();
          ImGui::PushStyleColor(ImGuiCol_Text, condition ? OPT_POPUP_POS_COLOR : OPT_POPUP_NEG_COLOR);
          ImGui::TextUnformatted(condition ? "[...]" : " --- ");
          ImGui::PopStyleColor();

          if (condition && ImGui::IsItemHovered())
          {
            hoverState.tooltip.printf(0, hover_msg);
            if (popup_name && mouseRclick)
            {
              popupNode = nodeId;
              draw_list->ChannelsSetCurrent(CanvasChannels::SUSPEND);
              canvas.Suspend();
              ImGui::OpenPopup(popup_name);
              canvas.Resume();
              draw_list->ChannelsSetCurrent(CanvasChannels::TEXTS);
            }
          }
        };


        ImGui::Checkbox(String(0, "Enable node##%s", nodeName), &nodeData.enabled);

        ImGui::TextUnformatted("");
        conditionalPopup("State reqs:", bool(nodeData.stateRequirements), nullptr,
          nodeData.stateRequirements ? state_reqs_descr(*nodeData.stateRequirements) : nullptr);
        conditionalPopup("Exec reqs:", !eastl::holds_alternative<eastl::monostate>(nodeData.executeRequirements), NODE_EXEX_REQ_POPUP);
        conditionalPopup("Render reqs:", bool(nodeData.renderingRequirements), NODE_RENDER_REQ_POPUP);

        ImGui::TextUnformatted("");
        conditionalPopup("Binds:", !nodeData.bindings.empty(), NODE_BINDINGS_POPUP);
        {
          bool hasIdxVtxBuf = bool(nodeData.indexSource);
          for (const auto &vtxBuf : nodeData.vertexSources)
            hasIdxVtxBuf |= bool(vtxBuf);
          conditionalPopup("Idx/Vtx:", hasIdxVtxBuf, NODE_IDX_VTX_POPUP);
        }

        ImGui::TextUnformatted("Multiplex:");
        ImGui::SameLine();
        if (nodeData.multiplexingMode)
        {
          if (*nodeData.multiplexingMode == multiplexing::Mode::None)
          {
            ImGui::PushStyleColor(ImGuiCol_Text, OPT_POPUP_NEG_COLOR);
            ImGui::TextUnformatted("Ignore");
            if (ImGui::IsItemHovered())
              hoverState.tooltip.printf(0, "Executes once");
          }
          else
          {
            ImGui::PushStyleColor(ImGuiCol_Text, OPT_POPUP_POS_COLOR);
            ImGui::TextUnformatted("YES");
            if (ImGui::IsItemHovered())
              hoverState.tooltip.printf(0, multiplexing_descr(*nodeData.multiplexingMode));
          }
        }
        else
        {
          ImGui::PushStyleColor(ImGuiCol_Text, OPT_POPUP_WAR_COLOR);
          ImGui::TextUnformatted("Global");
          if (ImGui::IsItemHovered())
            hoverState.tooltip.printf(0, multiplexing_descr(registry.defaultMultiplexingMode));
        }
        ImGui::PopStyleColor();

        ImGui::TextUnformatted("Source file:");
        ImGui::SameLine();
        if (!nodeData.nodeSource.empty())
        {
          ImGui::PushStyleColor(ImGuiCol_Text, OPT_POPUP_POS_COLOR);
          ImGui::TextUnformatted("[...]");
          if (ImGui::IsItemHovered())
          {
            hoverState.tooltip.printf(0, "%s\n", nodeData.nodeSource.c_str());
#if _TARGET_PC_WIN // there are some problems with calling system on not win platform
            hoverState.tooltip.aprintf(0, "  L-click to open in VScode\n");
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
              // Open source file via VSCode: https://code.visualstudio.com/docs/editor/command-line
              system(String(0, "code -g \"%s\"", nodeData.nodeSource).c_str());
#endif
            hoverState.tooltip.aprintf(0, "  R-click to copy\n");
            if (mouseRclick)
              ImGui::SetClipboardText(nodeData.nodeSource.c_str());
          }
        }
        else
        {
          ImGui::PushStyleColor(ImGuiCol_Text, OPT_POPUP_NEG_COLOR);
          ImGui::TextUnformatted("no source");
        }
        ImGui::PopStyleColor();


        ImGui::TextUnformatted("");

        ImGui::TextUnformatted("side effect:");
        ImGui::SameLine();
        switch (nodeData.sideEffect)
        {
          case SideEffects::None: ImGui::PushStyleColor(ImGuiCol_Text, SIDE_EFF_NON_COLOR); break;
          case SideEffects::Internal: ImGui::PushStyleColor(ImGuiCol_Text, SIDE_EFF_INT_COLOR); break;
          case SideEffects::External: ImGui::PushStyleColor(ImGuiCol_Text, SIDE_EFF_EXT_COLOR); break;
        }
        ImGui::TextUnformatted(side_effect_name_by_type(nodeData.sideEffect));
        ImGui::PopStyleColor();

        ImGui::TextUnformatted("async pipeline:");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, nodeData.allowAsyncPipelines ? OPT_POPUP_POS_COLOR : OPT_POPUP_NEG_COLOR);
        ImGui::TextUnformatted(nodeData.allowAsyncPipelines ? "Y" : "N");
        ImGui::PopStyleColor();

        ImGui::Text("shader block layers: %d %d %d", nodeData.shaderBlockLayers.objectLayer, nodeData.shaderBlockLayers.frameLayer,
          nodeData.shaderBlockLayers.sceneLayer);
      }
      ImGui::EndGroup();
    }
  }
}

void Visualizer::drawEdges(ImDrawList *draw_list, const CanvasLayout &layout)
{
  draw_list->ChannelsSetCurrent(CanvasChannels::EDGES);

  const ImRect canvasView = canvas.ViewRect();
  for (const auto &[edgeDeps, edgeSplines] : layout.edges)
  {
    if (eastl::find_if(edgeSplines.begin(), edgeSplines.end(),
          [&canvasView](const auto &spline) { return spline_is_visible(spline, canvasView); }) == edgeSplines.end())
      continue;

    const auto &frontDep = registryDependencies[edgeDeps.front()];
    const auto &[_, depColor, depVisible] = dependency_info_by_type(frontDep.type);
    const auto resId = frontDep.resource;
    if (!(depVisible && isResourceVisible(resId)))
      continue;

    auto drawSpline = [&](const ImCubicBezierPoints &spline, const ImU32 color, const float line_thickness) {
      draw_list->AddBezierCubic(spline.P0, spline.P1, spline.P2, spline.P3, color, line_thickness);
    };

    if (resId == ResourceId::Invalid)
    {
      const bool cycled = registryDependencies[edgeDeps.front()].cycled;
      const float alpha = focusedResource.valid() ? 0.2f : 1.0f;
      for (const auto &spline : edgeSplines)
      {
        if (cycled)
          drawSpline(spline, CYCLED_DEP_COLOR, CYCLED_DEP_WIDTH);
        drawSpline(spline, apply_alpha_coeff(depColor, alpha), BASE_DEP_WIDTH);
      }
      continue;
    }

    const bool focusingThisRes = focusedResource.id == resId;
    bool edgeCycled = false;
    bool edgeInspected = false;
    for (const auto depId : edgeDeps)
    {
      edgeCycled |= registryDependencies[depId].cycled;
      edgeInspected |= inspectedDependency.id == depId;
    }

    for (const auto &spline : edgeSplines)
    {
      if (focusingThisRes)
        drawSpline(spline, FOCUS_COLOR, RES_FOCUS_WIDTH);

      if (edgeInspected)
        drawSpline(spline, RES_INSPECT_COLOR, DEP_INSPECT_WIDTH);

      if (edgeCycled)
        drawSpline(spline, CYCLED_DEP_COLOR, CYCLED_DEP_WIDTH);

      drawSpline(spline, depColor, BASE_DEP_WIDTH);
    }
  }

  if (hoverState.resource != ResourceId::Invalid)
  {
    const auto hoveredResNameId = nameIdByResId(hoverState.resource);
    const auto origResNameId = depData.renamingRepresentatives[hoveredResNameId];
    const auto resData = registry.resources[origResNameId];

    hoverState.tooltip.printf(0, "(%s) %s\n",
      res_type_name_by_type(resData.createdResData ? resData.createdResData->type : ResourceType::Invalid), getName(hoveredResNameId));
  }
}

void Visualizer::drawNodeBoxes(ImDrawList *draw_list)
{
  for (auto [boxId, box] : nodeBoxes.enumerate())
    if (box.nameSpace != registry.knownNames.root())
    {
      draw_list->ChannelsSetCurrent(CanvasChannels::NAMESPACES);

      const ImU32 nameSpaceBorderColor = color_by_hash(size_t(box.nameSpace));
      const ImU32 nameSpaceFillColor = apply_alpha_coeff(nameSpaceBorderColor, 0.1f);

      const ImVec2 leftTopPos = box.rect.offset;
      const ImVec2 rightBottomPos = leftTopPos + box.rect.size;

      draw_list->AddRectFilled(leftTopPos, rightBottomPos, nameSpaceFillColor, 4.0f);
      draw_list->AddRect(leftTopPos, rightBottomPos, nameSpaceBorderColor, 4.0f, 0, NAMESPACE_BORDER_WIDTH);


      draw_list->ChannelsSetCurrent(CanvasChannels::TEXTS);

      const char *nameSpaceName = registry.knownNames.getName(box.nameSpace);
      const ImVec2 nameBoxSize = ImGui::CalcTextSize(nameSpaceName) + 2.f * NAMETAG_PADDING;

      const ImVec2 nameBoxLeftTopPos = leftTopPos - ImVec2{0.f, nameBoxSize.y / 2};
      const ImVec2 nameBoxRightBottomPos = nameBoxLeftTopPos + nameBoxSize;
      const ImVec2 nameBoxTextStart = nameBoxLeftTopPos + NAMETAG_PADDING;

      draw_list->AddRectFilled(nameBoxLeftTopPos, nameBoxRightBottomPos, nameSpaceBorderColor, 4.0f);
      ImGui::SetCursorPos(nameBoxTextStart);
      ImGui::TextUnformatted(nameSpaceName);
    }
}


void Visualizer::checkHovering()
{
  if (!hoverState.window)
    return;

  const auto &layout = hierarchicalView ? condensedLayout : generalLayout;
  const ImRect canvasView = canvas.ViewRect();

  for (auto [nodeId, nodeRect] : layout.nodes.enumerate())
  {
    const ImVec2 leftTopPos = nodeRect.offset;
    const ImVec2 rightBottomPos = nodeRect.offset + nodeRect.size;
    if (
      rect_is_visible(ImRect{leftTopPos, rightBottomPos}, canvasView) && ImGui::IsMouseHoveringRect(leftTopPos, rightBottomPos, true))
      hoverState.node = nodeId;
  }

  const ImVec2 mousePos = ImGui::GetMousePos();
  for (const auto &[edgeDeps, edgeSplines] : layout.edges)
  {
    if (eastl::find_if(edgeSplines.begin(), edgeSplines.end(),
          [&canvasView](const auto &spline) { return spline_is_visible(spline, canvasView); }) == edgeSplines.end())
      continue;

    const auto &frontDep = registryDependencies[edgeDeps.front()];
    const bool depVisible = dependency_info_by_type(frontDep.type).visible;
    const auto resId = frontDep.resource;
    if (depVisible && resId != ResourceId::Invalid && isResourceVisible(resId))
      for (const auto &spline : edgeSplines)
        if (
          mousePos.x >= spline.P0.x && mousePos.x <= spline.P3.x && ImProjectOnCubicBezier(mousePos, spline).Distance < BASE_DEP_WIDTH)
        {
          hoverState.deps = {edgeDeps.begin(), edgeDeps.end()};
          hoverState.resource = resId;
        }
  }
}

void Visualizer::processInput()
{
  if (hoverState.window && hoverState.canvas)
  {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
    {
      canvasCamera.canvasOffset += ImGui::GetIO().MouseDelta;
      canvas.SetView(canvasCamera.canvasOffset, canvasCamera.getZoom());
    }
    if (abs(ImGui::GetIO().MouseWheel) > 0.1 && !hoverState.searchBox)
    {
      ImGui::GetIO().MouseWheel > 0 ? canvasCamera.zoomIn() : canvasCamera.zoomOut();

      ImVec2 oldPos = canvas.ToLocal(ImGui::GetIO().MousePos);
      canvas.SetView(canvasCamera.canvasOffset, canvasCamera.getZoom());
      ImVec2 newPos = canvas.ToLocal(ImGui::GetIO().MousePos);

      canvasCamera.canvasOffset += canvas.FromLocalV(newPos - oldPos);
      canvas.SetView(canvasCamera.canvasOffset, canvasCamera.getZoom());
    }
  }

  if (hoverState.node != NodeId::Invalid && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
  {
    resetFocusedResource();
    setFocusedNode(hoverState.node);
    focusedNodeIndex =
      eastl::find(nsNodeNameIds.begin(), nsNodeNameIds.end(), nameIdByNodeId(hoverState.node)) - nsNodeNameIds.begin();
    nodeSearchInput = getName(nameIdByNodeId(hoverState.node));
  }

  if (hoverState.resource != ResourceId::Invalid)
  {
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
      resetFocusedNode();
      setFocusedResource(hoverState.resource);
      focusedResourceIndex =
        eastl::find(nsResNameIds.begin(), nsResNameIds.end(), nameIdByResId(hoverState.resource)) - nsResNameIds.begin();
      resourceSearchInput = getName(nameIdByResId(hoverState.resource));
    }
    else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
      popupDeps = hoverState.deps;
      ImGui::OpenPopup(EDGE_RESOURCES_POPUP);
    }
  }
}

void Visualizer::processPopup()
{
  if (ImGui::BeginPopup(NODE_EXEX_REQ_POPUP))
  {
    static String label;

    eastl::visit(
      [&](const auto &reqs) {
        using T = eastl::decay_t<decltype(reqs)>;
        if constexpr (std::is_same_v<T, DispatchRequirements>)
        {
          const DispatchRequirements &dispReqs = reqs;
          ImGui::Text("Shader: %s", registry.knownShaders.getName(dispReqs.shaderId).data());
        }
        else if constexpr (std::is_same_v<T, DrawRequirements>)
        {
          const DrawRequirements &drawReqs = reqs;
          ImGui::Text("Shader: %s", registry.knownShaders.getName(drawReqs.shaderId).data());
        }
      },
      registry.nodes[userNodes[popupNode].regId].executeRequirements);

    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup(NODE_RENDER_REQ_POPUP))
  {
    static String label;

    const auto &pass = *registry.nodes[userNodes[popupNode].regId].renderingRequirements;

    ImGui::TextUnformatted("(mip,layer) name");

    ImGui::TextUnformatted("Color attachments:");
    if (pass.colorAttachments.empty())
      ImGui::TextUnformatted("   NONE");
    for (const auto &colorAtt : pass.colorAttachments)
    {
      const bool resValid = colorAtt.nameId != ResNameId::Invalid;

      label.printf(0, "   (%d,%d) %s", colorAtt.mipLevel, colorAtt.layer,
        resValid ? getName(colorAtt.nameId).data() : "Invalid name id");
      if (ImGui::Selectable(label) && resValid)
      {
        resetFocusedNode();
        setFocusedResource(regResRepresent[colorAtt.nameId]);
      }
    }

    ImGui::TextUnformatted("Depth attachment:");
    {
      const auto &depthAtt = pass.depthAttachment;
      const bool resValid = depthAtt.nameId != ResNameId::Invalid;

      label.printf(0, "%s (%d,%d) %s", pass.depthReadOnly ? "RO" : "RW", depthAtt.mipLevel, depthAtt.layer,
        resValid ? getName(depthAtt.nameId).data() : "Invalid name id");
      if (ImGui::Selectable(label) && resValid)
      {
        resetFocusedNode();
        setFocusedResource(regResRepresent[depthAtt.nameId]);
      }
    }

    if (pass.vrsRateAttachment.nameId != ResNameId::Invalid)
    {
      ImGui::TextUnformatted("VRS attachment:");

      const auto &vrsAtt = pass.vrsRateAttachment;
      label.printf(0, "   (%d,%d) %s", vrsAtt.mipLevel, vrsAtt.layer, getName(vrsAtt.nameId).data());
      if (ImGui::Selectable(label))
      {
        resetFocusedNode();
        setFocusedResource(regResRepresent[vrsAtt.nameId]);
      }
    }

    if (!pass.resolves.empty())
    {
      ImGui::TextUnformatted("MSAA resolves:");
      for (const auto [from, to] : pass.resolves)
        ImGui::Text("   %s -> %s", from != ResNameId::Invalid ? getName(from).data() : "Invalid name id",
          to != ResNameId::Invalid ? getName(to).data() : "Invalid name id");
    }

    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup(NODE_BINDINGS_POPUP))
  {
    static String label;

    const auto &nodeData = registry.nodes[userNodes[popupNode].regId];

    ImGui::TextUnformatted("idx: type (hist?)(reset?)(opt?) name");
    if (ImGui::IsItemHovered())
      hoverState.tooltip.printf(0, "SV - ShaderVar\nVM - ViewMatrix\nPM - ProjMatrix\nIN - Invalid\n");

    for (const auto &[index, binding] : nodeData.bindings)
    {
      const auto resNameId = binding.resource;
      const bool resValid = resNameId != ResNameId::Invalid;

      label.printf(0, "%d: %s %s%s%s %s", index, bind_type_name_by_type(binding.type), binding.history ? "+" : "-",
        binding.reset ? "+" : "-", binding.optional ? "+" : "-", resValid ? getName(resNameId).data() : "Invalid name id");
      if (ImGui::Selectable(label) && resValid)
      {
        resetFocusedNode();
        setFocusedResource(regResRepresent[resNameId]);
      }
    }

    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup(NODE_IDX_VTX_POPUP))
  {
    static String label;

    const auto &nodeData = registry.nodes[userNodes[popupNode].regId];

    ImGui::TextUnformatted("Index source:");
    if (nodeData.indexSource)
    {
      const auto resNameId = nodeData.indexSource->buffer;
      const bool resValid = resNameId != ResNameId::Invalid;

      label.printf(0, "  %s", resValid ? getName(resNameId).data() : "Invalid name id");
      if (ImGui::Selectable(label) && resValid)
      {
        resetFocusedNode();
        setFocusedResource(regResRepresent[resNameId]);
      }
    }
    else
    {
      ImGui::TextUnformatted("  NONE");
    }

    ImGui::TextUnformatted("Vertex sources:");
    if (ImGui::IsItemHovered())
      hoverState.tooltip.printf(0, "i: (stride) name\n");
    for (uint32_t i = 0; i < nodeData.vertexSources.size(); ++i)
      if (nodeData.vertexSources[i])
      {
        const auto resNameId = nodeData.vertexSources[i]->buffer;
        const bool resValid = resNameId != ResNameId::Invalid;

        label.printf(0, "  %d: (%d) %s", i, nodeData.vertexSources[i]->stride,
          resValid ? getName(resNameId).data() : "Invalid name id");
        if (ImGui::Selectable(label) && resValid)
        {
          resetFocusedNode();
          setFocusedResource(regResRepresent[resNameId]);
        }
      }
      else
      {
        ImGui::Text("  %d: NONE", i);
      }

    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup(EDGE_RESOURCES_POPUP))
  {
    static String label;

    const auto &frontDep = registryDependencies[popupDeps.front()];
    const auto resNameId = nameIdByResId(frontDep.resource);
    const auto &resData = registry.resources[depData.renamingRepresentatives[resNameId]];

    ImGui::TextUnformatted(getName(resNameId).data());
    if (resData.createdResData)
    {
      const auto &createdData = *resData.createdResData;
      ImGui::Text("type: %s ; clear flags: %s ; history: %c", res_type_name_by_type(createdData.type),
        res_clear_flag_name_by_type(createdData.clearFlags), resData.history != History::No ? 'Y' : 'N');
    }
    ImGui::Text("%s:", dependency_info_by_type(frontDep.type).name.data());
    for (const auto depId : popupDeps)
    {
      const auto &dep = registryDependencies[depId];
      const bool inspectingThisDep = inspectedDependency.id == depId;
      const auto fromNodeName = getName(nameIdByNodeId(dep.from)).data();
      const auto toNodeName = getName(nameIdByNodeId(dep.to)).data();

      label.printf(0, "%s##%u", fromNodeName, static_cast<uint32_t>(depId));
      if (ImGui::Button(label))
      {
        resetFocusedResource();
        setFocusedNode(dep.from);
      }
      ImGui::SameLine();
      ImGui::TextUnformatted(" -> ");
      ImGui::SameLine();
      label.printf(0, "%s##%u", toNodeName, static_cast<uint32_t>(depId));
      if (ImGui::Button(label))
      {
        resetFocusedResource();
        setFocusedNode(dep.to);
      }
      ImGui::SameLine();
      if (inspectingThisDep)
      {
        ImGui::TextUnformatted("INPECTING");
      }
      else
      {
        label.printf(0, "Inspect##%u", static_cast<uint32_t>(depId));
        if (ImGui::Button(label))
        {
          inspectedDependency.set(depId);
          resetFocusedNode();
          resetFocusedResource();
        }
      }
    }

    ImGui::EndPopup();
  }

  if (!hoverState.tooltip.empty() && !hoverState.searchBox)
  {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(hoverState.tooltip);
    ImGui::EndTooltip();
  }
}

void Visualizer::processTextureDebug()
{
  if (eastl::exchange(inspectedDependency.wasChanged, false))
  {
    eastl::optional<Selection> resSelection = eastl::nullopt;

    if (inspectedDependency.id != DependencyId::Invalid)
    {
      const auto &dep = registryDependencies[inspectedDependency.id];
      const auto fromNodeNameId = userNodes[dep.from].regId;
      const auto toNodeNameId = userNodes[dep.to].regId;
      const auto resNameId = userResources[dep.resource].regId;
      resSelection = Selection{PreciseTimePoint{fromNodeNameId, toNodeNameId}, resNameId};
    }

    update_fg_debug_tex(resSelection, registry, depData);
  }
}

void Visualizer::hideResourcesInSubTree(NameSpaceNameId name_space_id)
{
  nameSpaces[name_space_id].visibleResourcesInSubtree = 0;
  for (const auto &subNameSpace : nameSpaces[name_space_id].subNameSpaces)
    hideResourcesInSubTree(subNameSpace);
}

void Visualizer::hideResourcesInNameSpace(NameSpaceNameId namespace_id)
{
  const uint16_t old = nameSpaces[namespace_id].visibleResourcesInSubtree;
  hideResourcesInSubTree(namespace_id);
  while (namespace_id != registry.knownNames.root())
  {
    namespace_id = registry.knownNames.getParent(namespace_id);
    nameSpaces[namespace_id].visibleResourcesInSubtree -= old;
  }
}

void Visualizer::showResourcesInSubTree(NameSpaceNameId namespace_id)
{
  nameSpaces[namespace_id].visibleResourcesInSubtree = nameSpaces[namespace_id].totalResourcesInSubtree;
  for (const auto &subNameSpace : nameSpaces[namespace_id].subNameSpaces)
    showResourcesInSubTree(subNameSpace);
}

void Visualizer::showResourcesInNameSpace(NameSpaceNameId namespace_id)
{
  const uint16_t diff = nameSpaces[namespace_id].totalResourcesInSubtree - nameSpaces[namespace_id].visibleResourcesInSubtree;
  showResourcesInSubTree(namespace_id);
  while (namespace_id != registry.knownNames.root())
  {
    namespace_id = registry.knownNames.getParent(namespace_id);
    nameSpaces[namespace_id].visibleResourcesInSubtree += diff;
  }
}


void Visualizer::setFocusedNode(NodeId node_id)
{
  focusedNode = {
    node_id,
    true,
  };
}

void Visualizer::centerOnFocusedNode()
{
  if (!focusedNode.valid())
    return;

  const auto nodeRect = (hierarchicalView ? condensedLayout : generalLayout).nodes[focusedNode.id];
  const ImVec2 centerView = nodeRect.offset + nodeRect.size / 2.f;

  canvas.CenterView(centerView);
  canvasCamera.canvasOffset = canvas.ViewOrigin();
}

void Visualizer::setFocusedResource(ResourceId res_id, ResourceFocusType focus_type)
{
  focusedResource = {
    res_id,
    true,
    false,
    focus_type,
  };

  if (focusedResource.valid())
  {
    const auto focusedResNameId = nameIdByResId(focusedResource.id);
    for (const auto [from, to] : depData.renamingChains.enumerate())
      if ((from != focusedResNameId && to == focusedResNameId) || (from == focusedResNameId && to != focusedResNameId))
      {
        focusedResource.hasRenames = true;
        return;
      }
  }
}

void Visualizer::centerOnFocusedRes()
{
  if (!focusedResource.valid())
    return;

  constexpr ImVec2 MAX_IMVEC2 = ImVec2{eastl::numeric_limits<float>::max(), eastl::numeric_limits<float>::max()};

  ImVec2 leftPos = MAX_IMVEC2;
  ImVec2 rightPos = MAX_IMVEC2;
  for (auto [edgeId, edge] : (hierarchicalView ? condensedLayout : generalLayout).edges.enumerate())
    if (registryDependencies[edge.carriedDeps.front()].resource == focusedResource.id)
      for (const auto &spline : edge.splines)
        if (spline.P0.x < leftPos.x)
        {
          leftPos = spline.P0;
          rightPos = spline.P3;
        }

  if (leftPos == MAX_IMVEC2 || rightPos == MAX_IMVEC2)
    return;

  canvas.CenterView((leftPos + rightPos) / 2.f);
  canvasCamera.canvasOffset = canvas.ViewOrigin();
}


void Visualizer::updateVisualization()
{
  TIME_PROFILE(update_usergraph_visualization);

  updateNeeded = true;

  if (!imgui_window_is_visible(nullptr, IMGUI_USG_WIN_NAME) || imgui_window_is_collapsed(nullptr, IMGUI_USG_WIN_NAME))
    return;


  updateNodesRess();
  updateIRInfo();
  updateNameSpaces();
  updateDependencies();

  calculateNodesColors();
  checkCycles();
  condenseGraph();

  performLayout();
  performCondensedLayout();

  update_external_texture_visualization(registry, depData);

  updateNeeded = false;
}


void Visualizer::updateNodesRess()
{
  TIME_PROFILE(update_nodes_resources);

  const uint32_t totalNodeCount = registry.nodes.size();
  const uint32_t totalResourceCount = registry.resources.size();

  userNodes.clear();
  userResources.clear();
  regNodesRepresent.clear();
  regResRepresent.clear();

  userNodes.reserve(totalNodeCount);
  userResources.reserve(totalResourceCount);
  regNodesRepresent.resize(totalNodeCount, NodeId::Invalid);
  regResRepresent.resize(totalResourceCount, ResourceId::Invalid);

  for (const auto nodeNameId : registry.nodes.keys())
    if (!getName(nodeNameId).starts_with("/.debug"))
      regNodesRepresent[nodeNameId] = userNodes.appendNew(Node{nodeNameId}).first;

  for (const auto resNameId : registry.resources.keys())
    if (!getName(resNameId).starts_with("/.debug"))
      regResRepresent[resNameId] = userResources.appendNew(Resource{resNameId}).first;
}

void Visualizer::updateIRInfo()
{
  TIME_PROFILE(update_ir_info);

  uint32_t currExecTime = 0;
  for (auto [irIndex, irNode] : intermediateGraph.nodes.enumerate())
    if (irNode.multiplexingIndex == 0 && irNode.frontendNode && nodeIsPresented(*irNode.frontendNode))
    {
      auto &node = userNodes[regNodesRepresent[*irNode.frontendNode]];
      node.passColor = intermediatePassColoring[irIndex];
      node.executionTime = currExecTime;
      ++currExecTime;
    }
}

void Visualizer::updateNameSpaces()
{
  TIME_PROFILE(update_name_spaces);

  nameSpaces.clear();
  nsNodeNameIds.clear();
  nsResNameIds.clear();

  nameSpaces.resize(registry.knownNames.nameCount<NameSpaceNameId>(), {});
  nsNodeNameIds.reserve(userNodes.size());
  nsResNameIds.reserve(userResources.size());


  // collect sub namespaces
  for (const auto nameSpaceId : nameSpaces.keys())
    if (nameSpaceId != registry.knownNames.root())
      nameSpaces[getParent(nameSpaceId)].subNameSpaces.push_back(nameSpaceId);

  // collect nodes and resources ids
  for (const auto &node : userNodes)
    nameSpaces[getParent(node.regId)].nodes.push_back(node.regId);
  for (const auto &resource : userResources)
    nameSpaces[getParent(resource.regId)].resources.push_back(resource.regId);

  // calculate contents
  {
    const auto nameComparator = [&](const auto first, const auto second) { return getShortName(first) < getShortName(second); };

    dag::FixedMoveOnlyFunction<24, void(const NameSpaceNameId)> calculateContents = [&](const NameSpaceNameId name_space_id) {
      NameSpace &nameSpace = nameSpaces[name_space_id];
      nameSpace.totalNodesInSubtree = nameSpace.nodes.size();
      nameSpace.totalResourcesInSubtree = nameSpace.resources.size();

      stlsort::sort(nameSpace.subNameSpaces.begin(), nameSpace.subNameSpaces.end(), nameComparator);
      stlsort::sort(nameSpace.nodes.begin(), nameSpace.nodes.end(), nameComparator);
      stlsort::sort(nameSpace.resources.begin(), nameSpace.resources.end(), nameComparator);

      for (const auto &subNameSpaceId : nameSpace.subNameSpaces)
      {
        calculateContents(subNameSpaceId);

        const NameSpace &subNameSpace = nameSpaces[subNameSpaceId];
        nameSpace.totalNodesInSubtree += subNameSpace.totalNodesInSubtree;
        nameSpace.totalResourcesInSubtree += subNameSpace.totalResourcesInSubtree;
      }

      nameSpace.visibleResourcesInSubtree = nameSpace.totalResourcesInSubtree;

      nsNodeNameIds.insert(nsNodeNameIds.end(), nameSpace.nodes.begin(), nameSpace.nodes.end());
      nsResNameIds.insert(nsResNameIds.end(), nameSpace.resources.begin(), nameSpace.resources.end());
    };

    calculateContents(registry.knownNames.root());
  }

  // get names
  nsNodeNames.clear();
  nsResNames.clear();
  nsNodeNames = gatherNames<NodeNameId>(nsNodeNameIds);
  nsResNames = gatherNames<ResNameId>(nsResNameIds);
}

void Visualizer::updateDependencies()
{
  TIME_PROFILE(update_dependencies);

  registryDependencies.clear();

  auto addDependency = [&](const NodeId from, const NodeId to, const DependencyType dep_type,
                         const ResourceId res = ResourceId::Invalid) {
    const auto depId = registryDependencies.appendNew(Dependency{dep_type, from, to, res}).first;
    userNodes[from].outDeps.push_back(depId);
    userNodes[to].inDeps.push_back(depId);
  };

  auto addHistoryRead = [&](const NodeId from, const NodeId to, const ResourceId res) {
    const auto depId = registryDependencies.appendNew(Dependency{DependencyType::IMPLICIT_RES_HIST, from, to, res}).first;
    userNodes[from].outHistDeps.push_back(depId);
    userNodes[to].inHistDeps.push_back(depId);
  };


  // filling explicit dependencies
  for (auto [nodeId, node] : userNodes.enumerate())
  {
    const auto &nodeData = registry.nodes[node.regId];

    for (const auto prevNameId : nodeData.precedingNodeIds)
      if (nodeIsPresented(prevNameId))
        addDependency(regNodesRepresent[prevNameId], nodeId, DependencyType::EXPLICIT_PREVIOUS);

    for (const auto nextNameId : nodeData.followingNodeIds)
      if (nodeIsPresented(nextNameId))
        addDependency(nodeId, regNodesRepresent[nextNameId], DependencyType::EXPLICIT_FOLLOW);
  }

  // filling resource dependencies
  for (auto [resourceId, resource] : userResources.enumerate())
  {
    const auto &lifetime = depData.resourceLifetimes[resource.regId];

    /*
    consumer is dependent on every reader
    if there are no readers, consumer is dependent on last modifier
    if there are no modifiers and no readers, consumer is dependent on introducer

    every reader is dependent on last modifier
    if there are no modifiers, every reader is dependent on introducer

    first modifier is dependent on introducer

    // clang-format off
    so, if we have modifiers, we get such dependencies:

              M           R
            / |         / | \
           /  M        /  R  \
          /   |       / / | \ \
         /    ~      / /  ~  \ \
        I           M===~   ~===C
              ~    / \ \  ~  / /
              |   /   \ \ | / /
              M  /     \  R  /
              | /       \ | /
              M           R

    or, if we dont have modifiers:

              R
            / | \
           /  R  \
          / / | \ \
         / /  ~  \ \
        I===~   ~===C
         \ \  ~  / /
          \ \ | / /
           \  R  /
            \ | /
              R

    // clang-format on
    */

    NodeId introducerId = lifetime.introducedBy != NodeNameId::Invalid ? regNodesRepresent[lifetime.introducedBy] : NodeId::Invalid;
    NodeId consumerId = lifetime.consumedBy != NodeNameId::Invalid ? regNodesRepresent[lifetime.consumedBy] : NodeId::Invalid;
    NodeId readSourceId = introducerId;

    dag::Vector<NodeId, framemem_allocator> modifiers;
    for (const auto modifierNameId : lifetime.modificationChain)
      if (nodeIsPresented(modifierNameId))
        modifiers.push_back(regNodesRepresent[modifierNameId]);

    dag::Vector<NodeId, framemem_allocator> readers;
    for (const auto readerNameId : lifetime.readers)
      if (nodeIsPresented(readerNameId))
        readers.push_back(regNodesRepresent[readerNameId]);

    const bool resHasIntroducer = introducerId != NodeId::Invalid;
    const bool resHasModifiers = !modifiers.empty();
    const bool resHasReaders = !readers.empty();
    const bool resHasConsumer = consumerId != NodeId::Invalid;

    if (resHasModifiers)
    {
      // we sort modifiers as we can
      // nodes, which were used in intermediate graph, will go first in execution order
      // all other nodes will be considered culled away and will go in random order after exucutable nodes
      dag::Vector<NodeId, framemem_allocator> modifiersOrder(modifiers.begin(), modifiers.end());
      eastl::sort(modifiersOrder.begin(), modifiersOrder.end(),
        [&](const NodeId first, const NodeId second) { return userNodes[first].executionTime < userNodes[second].executionTime; });

      if (resHasIntroducer)
        addDependency(introducerId, modifiersOrder.front(), DependencyType::IMPLICIT_RES_MODIFY, resourceId);

      for (uint32_t modifierIdx = 1; modifierIdx < modifiersOrder.size(); ++modifierIdx)
        addDependency(modifiersOrder[modifierIdx - 1], modifiersOrder[modifierIdx], DependencyType::IMPLICIT_RES_MODIFY, resourceId);

      readSourceId = modifiersOrder.back();
    }

    const bool readSourceValid = readSourceId != NodeId::Invalid;

    for (const auto readerNameId : lifetime.readers)
      if (nodeIsPresented(readerNameId))
      {
        if (readSourceValid)
          addDependency(readSourceId, regNodesRepresent[readerNameId], DependencyType::IMPLICIT_RES_READ, resourceId);
        if (resHasConsumer)
          addDependency(regNodesRepresent[readerNameId], consumerId, DependencyType::IMPLICIT_RES_CONSUME, resourceId);
      }
    if (!resHasReaders && readSourceValid && resHasConsumer)
      addDependency(readSourceId, consumerId, DependencyType::IMPLICIT_RES_CONSUME, resourceId);

    if (!resHasConsumer && readSourceValid)
      for (const auto readerNameId : lifetime.historyReaders)
        if (nodeIsPresented(readerNameId))
          addHistoryRead(readSourceId, regNodesRepresent[readerNameId], resourceId);
  }
}


void Visualizer::calculateNodesColors()
{
  for (auto &node : userNodes)
  {
    node.fbImColor = NODE_BASE_COLOR;
    node.passImColor = NODE_BASE_COLOR;

    auto &nodeData = registry.nodes[node.regId];
    if (nodeData.renderingRequirements)
    {
      const auto &pass = *nodeData.renderingRequirements;

      size_t hash = 0;
      hash_pack(hash, pass.depthReadOnly);
      for (auto colorAtt : pass.colorAttachments)
        hash_pack(hash, colorAtt.nameId, colorAtt.mipLevel, colorAtt.layer);
      hash_pack(hash, pass.depthAttachment.nameId, pass.depthAttachment.mipLevel, pass.depthAttachment.layer);

      node.fbImColor = color_by_hash(hash);
    }

    if (node.passColor != UNKNOWN_PASS_COLOR)
      node.passImColor = color_by_hash(static_cast<size_t>(node.passColor));
  }
}

static dag::Vector<NodeIdx> get_reversed_topsort(const AdjacencyLists &graph)
{
  dag::Vector<NodeIdx> res;
  res.reserve(graph.size());

  // we will use DFS to sort indicies in topoligical order
  dag::Vector<bool, framemem_allocator> indexVisited;
  indexVisited.resize(graph.size(), false);

  dag::FixedMoveOnlyFunction<32, void(const NodeIdx)> dfsNode = [&](const NodeIdx node_index) {
    indexVisited[node_index] = true;

    for (const auto to : graph[node_index])
      if (!indexVisited[to])
        dfsNode(to);

    res.push_back(node_index);
  };

  // iterate on every index and we will get reversed topsort
  for (uint32_t i = 0; i < graph.size(); ++i)
    if (!indexVisited[i])
      dfsNode(i);

  return res;
}

void Visualizer::checkCycles()
{
  TIME_PROFILE(check_cycled_dependencies);

  // here we will perform Kosaraju's algorithm to find all strongly connected components (SCC)
  const uint32_t nodesCount = userNodes.size();
  constexpr uint32_t NONE = static_cast<uint32_t>(-1);
  IdIndexedMapping<NodeId, uint32_t, framemem_allocator> nodesSCC(nodesCount, NONE);

  uint32_t currentSCC = 0;
  {
    AdjacencyLists graphAdjLists(nodesCount);
    AdjacencyLists reversedGraphAdjLists(nodesCount);
    for (const auto &dep : registryDependencies)
      if (dep.type != DependencyType::IMPLICIT_RES_HIST)
      {
        graphAdjLists[NodeIdx(dep.from)].push_back(NodeIdx(dep.to));
        reversedGraphAdjLists[NodeIdx(dep.to)].push_back(NodeIdx(dep.from));
      }
    dag::Vector<NodeIdx> componentOrder = get_reversed_topsort(reversedGraphAdjLists);

    dag::Vector<uint32_t, framemem_allocator> visitStack;
    dag::Vector<uint32_t, framemem_allocator> dfsStack;
    dag::Vector<uint32_t, framemem_allocator> leftOut;

    visitStack.reserve(nodesCount);
    dfsStack.reserve(nodesCount);
    leftOut.reserve(nodesCount);

    for (const auto nodeIndex : dag::ReverseView{componentOrder})
      if (nodesSCC[NodeId(nodeIndex)] == NONE)
      {
        visitStack.push_back(nodeIndex);
        while (!visitStack.empty())
        {
          dfsStack.push_back(visitStack.back());
          leftOut.push_back(0);
          visitStack.pop_back();

          nodesSCC[NodeId(dfsStack.back())] = currentSCC;

          for (const auto toNodeIndex : graphAdjLists[dfsStack.back()])
            if (nodesSCC[NodeId(toNodeIndex)] == NONE)
            {
              visitStack.push_back(toNodeIndex);
              ++leftOut.back();
            }

          while (!leftOut.empty() && leftOut.back() == 0)
          {
            dfsStack.pop_back();
            leftOut.pop_back();
            if (!leftOut.empty())
              --leftOut.back();
          }
        }
        ++currentSCC;
      }
  }
  const uint32_t sccCount = currentSCC;


  // if each node is not a separate SCC - then some of them form a cycle
  if (sccCount != nodesCount)
  {
    dag::Vector<uint32_t, framemem_allocator> sccSizes(sccCount, 0);
    for (const auto nodeId : userNodes.keys())
      ++sccSizes[nodesSCC[nodeId]];

    dag::Vector<uint32_t, framemem_allocator> sccWithLoopsIndices;
    for (uint32_t sccIndex = 0; sccIndex < sccCount; ++sccIndex)
      if (sccSizes[sccIndex] > 1)
        sccWithLoopsIndices.push_back(sccIndex);

    dag::Vector<dag::Vector<NodeId, framemem_allocator>, framemem_allocator> sccMembers;
    sccMembers.resize(sccCount, {});
    for (const uint32_t sccIndex : dag::ReverseView{sccWithLoopsIndices})
      sccMembers[sccIndex].reserve(sccSizes[sccIndex]);
    for (const auto nodeId : userNodes.keys())
      if (sccSizes[nodesSCC[nodeId]] > 1)
        sccMembers[nodesSCC[nodeId]].push_back(nodeId);

    dag::Vector<dag::Vector<DependencyId, framemem_allocator>, framemem_allocator> sccDependencies;
    sccDependencies.resize(sccCount, {});
    for (const uint32_t sccIndex : dag::ReverseView{sccWithLoopsIndices})
      for (auto [depId, dep] : registryDependencies.enumerate())
        if (nodesSCC[dep.from] == sccIndex && nodesSCC[dep.to] == sccIndex && dep.type != DependencyType::IMPLICIT_RES_HIST)
          sccDependencies[sccIndex].push_back(depId);


    for (const auto sccIndex : sccWithLoopsIndices)
    {
      const auto &sccNodes = sccMembers[sccIndex];
      const auto &sccDeps = sccDependencies[sccIndex];

      IdIndexedMapping<NodeId, uint32_t, framemem_allocator> localIdx(nodesCount, NONE);
      for (uint32_t i = 0; i < sccNodes.size(); ++i)
        localIdx[sccNodes[i]] = i;

      // Alert! Vibecoded!
      // abandon all hope, ye who enter here
      //
      // Find minimum feedback arc set (FAS) by finding a node ordering that minimizes
      // backward edges. FAS = backward edges in the optimal ordering.
      //
      // For small SCCs (n <= 20): exact bitmask DP, O(2^n * n).
      // For larger SCCs: Eades greedy heuristic, O(n^2), roughly 2x optimal.
      {
        const uint32_t n = sccNodes.size();

        dag::Vector<uint32_t, framemem_allocator> ordering(n);

        constexpr uint32_t EXACT_THRESHOLD = 20;
        if (n <= EXACT_THRESHOLD)
        {
          // adjMask[u] has bit v set if there is an edge u->v
          dag::Vector<uint32_t, framemem_allocator> adjMask(n, 0);
          for (const auto depId : sccDeps)
          {
            const auto &dep = registryDependencies[depId];
            adjMask[localIdx[dep.from]] |= (1u << localIdx[dep.to]);
          }

          const uint32_t fullMask = (1u << n) - 1;
          dag::Vector<uint32_t, framemem_allocator> dp(fullMask + 1, UINT32_MAX);
          dag::Vector<uint8_t, framemem_allocator> lastPlaced(fullMask + 1, 0xFF);
          dp[0] = 0;
          for (uint32_t mask = 0; mask < fullMask; ++mask)
          {
            if (dp[mask] == UINT32_MAX)
              continue;
            for (uint32_t v = 0; v < n; ++v)
            {
              if (mask & (1u << v))
                continue;
              // edges from v to already-placed nodes become backward edges
              uint32_t cost = Bitset<32>(adjMask[v] & mask).count();
              uint32_t newMask = mask | (1u << v);
              if (dp[mask] + cost < dp[newMask])
              {
                dp[newMask] = dp[mask] + cost;
                lastPlaced[newMask] = static_cast<uint8_t>(v);
              }
            }
          }
          uint32_t mask = fullMask;
          for (int pos = static_cast<int>(n) - 1; pos >= 0; --pos)
          {
            ordering[pos] = lastPlaced[mask];
            mask ^= (1u << lastPlaced[mask]);
          }
        }
        else
        {
          // adjMatrix[u*n + v] = true if edge u->v exists
          dag::Vector<bool, framemem_allocator> adjMatrix(n * n, false);
          for (const auto depId : sccDeps)
          {
            const auto &dep = registryDependencies[depId];
            adjMatrix[localIdx[dep.from] * n + localIdx[dep.to]] = true;
          }

          dag::Vector<int, framemem_allocator> outDeg(n, 0), inDeg(n, 0);
          for (uint32_t u = 0; u < n; ++u)
            for (uint32_t v = 0; v < n; ++v)
              if (adjMatrix[u * n + v])
              {
                ++outDeg[u];
                ++inDeg[v];
              }

          dag::Vector<bool, framemem_allocator> removed(n, false);
          dag::Vector<uint32_t, framemem_allocator> seqLeft, seqRight;
          seqLeft.reserve(n);
          seqRight.reserve(n);

          auto removeNode = [&](uint32_t v) {
            removed[v] = true;
            for (uint32_t u = 0; u < n; ++u)
            {
              if (removed[u])
                continue;
              if (adjMatrix[v * n + u])
                --inDeg[u];
              if (adjMatrix[u * n + v])
                --outDeg[u];
            }
          };

          int remaining = static_cast<int>(n);
          while (remaining > 0)
          {
            bool progress = true;
            while (progress)
            {
              progress = false;
              for (uint32_t v = 0; v < n; ++v)
              {
                if (removed[v])
                  continue;
                if (inDeg[v] == 0)
                {
                  seqLeft.push_back(v);
                  removeNode(v);
                  --remaining;
                  progress = true;
                }
                else if (outDeg[v] == 0)
                {
                  seqRight.push_back(v);
                  removeNode(v);
                  --remaining;
                  progress = true;
                }
              }
            }
            if (remaining > 0)
            {
              uint32_t best = 0;
              int bestDelta = INT_MIN;
              for (uint32_t v = 0; v < n; ++v)
                if (!removed[v] && outDeg[v] - inDeg[v] > bestDelta)
                {
                  bestDelta = outDeg[v] - inDeg[v];
                  best = v;
                }
              seqLeft.push_back(best);
              removeNode(best);
              --remaining;
            }
          }
          eastl::copy(seqLeft.begin(), seqLeft.end(), ordering.begin());
          eastl::reverse_copy(seqRight.begin(), seqRight.end(), ordering.begin() + seqLeft.size());
        }

        // pos[local_idx] = position in ordering
        dag::Vector<uint32_t, framemem_allocator> pos(n);
        for (uint32_t i = 0; i < n; ++i)
          pos[ordering[i]] = i;

        // Disable all deps that go backward in the ordering
        for (const auto depId : sccDeps)
        {
          auto &dep = registryDependencies[depId];
          if (pos[localIdx[dep.from]] > pos[localIdx[dep.to]])
            dep.disabled = true;
        }
      }
      // Vibecode ended

      for (const auto nodeId : sccNodes)
        userNodes[nodeId].cycled = true;
      for (const auto depId : sccDeps)
        registryDependencies[depId].cycled = true;
    }
  }


  // and finally, we recreate lists of dependencies per node,
  // but we filter out disabled dependencies to keep graph acyclic
  for (auto &node : userNodes)
  {
    node.inDeps.clear();
    node.outDeps.clear();
  }
  disabledDependencies.clear();

  for (auto [depId, dep] : registryDependencies.enumerate())
    if (!dep.disabled)
    {
      if (dep.type != DependencyType::IMPLICIT_RES_HIST)
      {
        userNodes[dep.from].outDeps.push_back(depId);
        userNodes[dep.to].inDeps.push_back(depId);
      }
    }
    else
    {
      disabledDependencies.push_back(depId);
    }
}

void Visualizer::condenseGraph()
{
  TIME_PROFILE(condense_graph_nodes);

  nodeBoxes.clear();

  IdIndexedMapping<NameSpaceNameId, dag::Vector<NodeBoxId, framemem_allocator>, framemem_allocator> nameSpacesNodeBoxes;
  nameSpacesNodeBoxes.resize(registry.knownNames.nameCount<NameSpaceNameId>(), {});
  for (auto [nameSpaceId, nameSpaceBoxes] : dag::ReverseView{nameSpacesNodeBoxes.enumerate()})
    nameSpaceBoxes.reserve(nameSpaces[nameSpaceId].nodes.size());

  {
    dag::FixedMoveOnlyFunction<24, void(const NameSpaceNameId)> initNSBoxes = [&](const NameSpaceNameId namespace_id) {
      if (nameSpaces[namespace_id].nodes.size() != 0)
        nameSpacesNodeBoxes[namespace_id].push_back(nodeBoxes.appendNew(NodeBox{namespace_id}).first);

      for (const auto subNameSpaceId : nameSpaces[namespace_id].subNameSpaces)
        initNSBoxes(subNameSpaceId);
    };
    initNSBoxes(registry.knownNames.root());
  }


  // algorithm is similar to pass coloration in PassColorer::performSubpassColoring()
  // but we dont check conflicts, caused by barriers, because we dont have them in user graph
  {
    const uint32_t userNodesCount = userNodes.size();

    nodesNodeBox.clear();
    nodesNodeBox.resize(userNodesCount, NodeBoxId::Invalid);

    SimdBitMatrix<NodeBoxId, framemem_allocator> boxReachability(userNodesCount, false);
    dag::Vector<NodeIdx, framemem_allocator> topOrder;
    {
      AdjacencyLists graphAdjLists(userNodesCount);
      for (auto [nodeId, node] : userNodes.enumerate())
      {
        const auto nodeIdx = NodeIdx(nodeId);
        graphAdjLists[nodeIdx].reserve(node.outDeps.size());
        for (const auto depId : node.outDeps)
          graphAdjLists[nodeIdx].push_back(NodeIdx(registryDependencies[depId].to));
      }
      topOrder = get_reversed_topsort(graphAdjLists);
    }
    for (const auto nodeIndex : dag::ReverseView{topOrder})
    {
      const auto nodeId = NodeId(nodeIndex);
      const auto &node = userNodes[nodeId];
      const auto nameSpaceId = getParent(node.regId);
      const auto &nodePredecessors = node.inDeps;

      SimdBitVector<NodeBoxId, framemem_allocator> reachableFromCurrent(userNodesCount, false);
      {
        dag::VectorSet<NodeBoxId, eastl::less<NodeBoxId>, framemem_allocator> predecessorBoxes;
        predecessorBoxes.reserve(nodePredecessors.size());
        for (const auto depId : nodePredecessors)
          predecessorBoxes.insert(nodesNodeBox[registryDependencies[depId].from]);
        for (const auto boxId : predecessorBoxes)
          reachableFromCurrent |= boxReachability.row(boxId);
      }

      const auto candidateBoxId =
        !nameSpacesNodeBoxes[nameSpaceId].empty() ? nameSpacesNodeBoxes[nameSpaceId].back() : NodeBoxId::Invalid;
      if (candidateBoxId != NodeBoxId::Invalid && !reachableFromCurrent.test(candidateBoxId))
      {
        nodesNodeBox[nodeId] = candidateBoxId;
      }
      else
      {
        const auto newBoxId = nodeBoxes.appendNew(NodeBox{nameSpaceId}).first;
        nameSpacesNodeBoxes[nameSpaceId].push_back(newBoxId);
        nodesNodeBox[nodeId] = newBoxId;
      }

      for (const auto depId : nodePredecessors)
        if (nodesNodeBox[registryDependencies[depId].from] != nodesNodeBox[nodeId])
          reachableFromCurrent.set(nodesNodeBox[registryDependencies[depId].from], true);
      boxReachability.row(nodesNodeBox[nodeId]) |= reachableFromCurrent;
    }

    for (const auto nodeId : userNodes.keys())
      nodeBoxes[nodesNodeBox[nodeId]].nodes.push_back(nodeId);
  }


  // now, when we separated all nodes by boxes, we can
  // add dependencies, that connect boxes with each other
  for (auto [depId, dep] : registryDependencies.enumerate())
  {
    const auto fromNodeBoxId = nodesNodeBox[dep.from];
    const auto toNodeBoxId = nodesNodeBox[dep.to];
    if (fromNodeBoxId != toNodeBoxId && dep.type != DependencyType::IMPLICIT_RES_HIST)
    {
      nodeBoxes[fromNodeBoxId].outDeps.push_back(depId);
      nodeBoxes[toNodeBoxId].inDeps.push_back(depId);
    }
  }
}

void Visualizer::performLayout()
{
  TIME_PROFILE(general_layout);

  generalLayout = {};

  const uint32_t userNodesCount = userNodes.size();
  if (userNodesCount == 0)
    return;


  generalLayout.nodes.resize(userNodesCount);

  dag::Vector<NodeId> nodeIds = {userNodes.keys().begin(), userNodes.keys().end()};

  dag::Vector<dag::Vector<DependencyId>> nodeOutDeps;
  for (const auto nodeId : nodeIds)
    nodeOutDeps.push_back({userNodes[nodeId].outDeps.begin(), userNodes[nodeId].outDeps.end()});

  GraphLayout nodeLayout = {};
  {
    TIME_PROFILE(ranking_nodes);
    AdjacencyLists graphAdjLists(userNodesCount);
    for (uint32_t nodeIndex = 0; nodeIndex < userNodesCount; ++nodeIndex)
    {
      graphAdjLists[nodeIndex].reserve(nodeOutDeps[nodeIndex].size());
      for (const auto depId : nodeOutDeps[nodeIndex])
        graphAdjLists[nodeIndex].push_back(NodeIdx(registryDependencies[depId].to));
    }
    nodeLayout = layouter->layout(graphAdjLists, get_reversed_topsort(graphAdjLists), true);
  }
  const uint32_t objectsCount = nodeLayout.adjacencyList.size();


  RawLayout rawGeneralLayout = {.userNodeIds = eastl::move(nodeIds),
    .outDeps = eastl::move(nodeOutDeps),
    .layouterResult = eastl::move(nodeLayout),
    .rawObjects = dag::Vector<RawObject>(objectsCount, RawObject{}),
    .objectsOffsets = dag::Vector<ImVec2>(objectsCount, ImVec2{0.f, 0.f}),
    .objectsSizes = dag::Vector<ImVec2>(objectsCount, ImVec2{0.f, 0.f}),
    .objectsColumnSizes = dag::Vector<float>(objectsCount, 0.f)};
  generateRawEdges(rawGeneralLayout);
  generateAnchors(rawGeneralLayout);

  calculateObjectsSizes(rawGeneralLayout);
  calculateObjectsPositions(rawGeneralLayout);
  calculateAnchorsPositions(rawGeneralLayout);

  calculateNodeRectangles(generalLayout, rawGeneralLayout);
  calculateEdgeSplines(generalLayout, rawGeneralLayout);
}

void Visualizer::performCondensedLayout()
{
  TIME_PROFILE(condenced_layout);

  condensedLayout = {};

  const uint32_t userNodesCount = userNodes.size();
  if (userNodesCount == 0)
    return;

  const uint32_t nodeBoxesCount = nodeBoxes.size();
  if (nodeBoxesCount == 0)
    return;


  condensedLayout.nodes.resize(userNodesCount);

  IdIndexedMapping<NodeBoxId, RawLayout, framemem_allocator> boxesRawLayouts(nodeBoxesCount);
  for (auto [nodeBoxId, nodeBox] : nodeBoxes.enumerate())
  {
    const uint32_t boxNodesCount = nodeBox.nodes.size();
    if (boxNodesCount == 0)
      continue;


    dag::Vector<NodeId> nodeIds = {nodeBox.nodes.begin(), nodeBox.nodes.end()};

    dag::Vector<dag::Vector<DependencyId>> nodeOutDeps;
    for (const auto nodeId : nodeIds)
      nodeOutDeps.push_back({userNodes[nodeId].outDeps.begin(), userNodes[nodeId].outDeps.end()});
    nodeOutDeps.push_back({nodeBox.inDeps.begin(), nodeBox.inDeps.end()});
    nodeOutDeps.push_back({});

    GraphLayout nodeLayout = {};
    {
      TIME_PROFILE(ranking_nodes_in_box);

      const uint32_t sourceIndex = boxNodesCount + 0;
      const uint32_t sinkIndex = boxNodesCount + 1;

      AdjacencyLists boxAdjLists(boxNodesCount + 2);
      for (uint32_t nodeIndex = 0; nodeIndex < boxNodesCount; ++nodeIndex)
      {
        boxAdjLists[nodeIndex].reserve(nodeOutDeps[nodeIndex].size());
        for (const auto depId : nodeOutDeps[nodeIndex])
        {
          const auto &dep = registryDependencies[depId];
          if (nodesNodeBox[dep.from] == nodesNodeBox[dep.to])
          {
            for (uint32_t targetNodeIndex = 0; targetNodeIndex < boxNodesCount; ++targetNodeIndex)
              if (nodeBox.nodes[targetNodeIndex] == dep.to)
              {
                boxAdjLists[nodeIndex].push_back(targetNodeIndex);
                break;
              }
          }
          else
          {
            boxAdjLists[nodeIndex].push_back(sinkIndex);
          }
        }
      }
      {
        boxAdjLists[sourceIndex].reserve(nodeOutDeps[sourceIndex].size());
        for (const auto depId : nodeOutDeps[sourceIndex])
          for (uint32_t targetNodeIndex = 0; targetNodeIndex < boxNodesCount; ++targetNodeIndex)
            if (nodeBox.nodes[targetNodeIndex] == registryDependencies[depId].to)
            {
              boxAdjLists[sourceIndex].push_back(targetNodeIndex);
              break;
            }
      }
      nodeLayout = layouter->layout(boxAdjLists, get_reversed_topsort(boxAdjLists), true);
    }
    const uint32_t objectsCount = nodeLayout.adjacencyList.size();


    auto &boxRawLayout = boxesRawLayouts[nodeBoxId];
    boxRawLayout = {.userNodeIds = eastl::move(nodeIds),
      .outDeps = eastl::move(nodeOutDeps),
      .layouterResult = eastl::move(nodeLayout),
      .rawObjects = dag::Vector<RawObject>(objectsCount, RawObject{}),
      .objectsOffsets = dag::Vector<ImVec2>(objectsCount, ImVec2{0.f, 0.f}),
      .objectsSizes = dag::Vector<ImVec2>(objectsCount, ImVec2{0.f, 0.f}),
      .objectsColumnSizes = dag::Vector<float>(objectsCount, 0.f)};
    generateRawEdges(boxRawLayout);
    generateAnchors(boxRawLayout);

    calculateObjectsSizes(boxRawLayout);
    calculateObjectsPositions(boxRawLayout);
    calculateAnchorsPositions(boxRawLayout);

    nodeBox.rect.size = boxRawLayout.size;
  }


  dag::Vector<NodeBoxId> nodeBoxIds = {nodeBoxes.keys().begin(), nodeBoxes.keys().end()};

  dag::Vector<dag::Vector<DependencyId>> nodeBoxOutDeps;
  for (const auto nodeBoxId : nodeBoxes.keys())
    nodeBoxOutDeps.push_back({nodeBoxes[nodeBoxId].outDeps.begin(), nodeBoxes[nodeBoxId].outDeps.end()});

  GraphLayout nodeBoxLayout = {};
  {
    TIME_PROFILE(ranking_node_boxes);
    AdjacencyLists boxesAdjLists(nodeBoxesCount);
    for (uint32_t nodeBoxIndex = 0; nodeBoxIndex < nodeBoxesCount; ++nodeBoxIndex)
    {
      boxesAdjLists[nodeBoxIndex].reserve(nodeBoxOutDeps[nodeBoxIndex].size());
      for (const auto depId : nodeBoxOutDeps[nodeBoxIndex])
        boxesAdjLists[nodeBoxIndex].push_back(NodeIdx(nodesNodeBox[registryDependencies[depId].to]));
    }
    nodeBoxLayout = layouter->layout(boxesAdjLists, get_reversed_topsort(boxesAdjLists), true);
  }
  const uint32_t objectsCount = nodeBoxLayout.adjacencyList.size();


  RawLayout rawCondensedLayout = {.nodeBoxIds = eastl::move(nodeBoxIds),
    .outDeps = eastl::move(nodeBoxOutDeps),
    .layouterResult = eastl::move(nodeBoxLayout),
    .rawObjects = dag::Vector<RawObject>(objectsCount, RawObject{}),
    .objectsOffsets = dag::Vector<ImVec2>(objectsCount, ImVec2{0.f, 0.f}),
    .objectsSizes = dag::Vector<ImVec2>(objectsCount, ImVec2{0.f, 0.f}),
    .objectsColumnSizes = dag::Vector<float>(objectsCount, 0.f)};
  generateRawEdges(rawCondensedLayout);
  generateAnchors(rawCondensedLayout);

  calculateObjectsSizes(rawCondensedLayout);
  calculateObjectsPositions(rawCondensedLayout);
  calculateAnchorsPositions(rawCondensedLayout);

  for (uint32_t nodeBoxIndex = 0; nodeBoxIndex < nodeBoxesCount; ++nodeBoxIndex)
  {
    const auto nodeBoxId = rawCondensedLayout.nodeBoxIds[nodeBoxIndex];
    const auto nodeBoxOffset = rawCondensedLayout.objectsOffsets[nodeBoxIndex];
    auto &nodeBoxRawLayout = boxesRawLayouts[nodeBoxId];

    nodeBoxes[nodeBoxId].rect = {nodeBoxOffset, nodeBoxRawLayout.size};
    for (auto &objOffset : nodeBoxRawLayout.objectsOffsets)
      objOffset += nodeBoxOffset;
    for (auto &anchor : nodeBoxRawLayout.anchors)
      anchor += nodeBoxOffset;

    calculateNodeRectangles(condensedLayout, nodeBoxRawLayout);
    calculateEdgeSplines(condensedLayout, nodeBoxRawLayout);
  }


  {
    TIME_PROFILE(search_anchors);

    for (auto &outerEdge : rawCondensedLayout.rawEdges)
    {
      const NodeIdx fromIdx = outerEdge.from;
      const NodeIdx toIdx = outerEdge.to;
      const auto outerDepId = outerEdge.carriedDeps.front();
      const auto outerResId = registryDependencies[outerDepId].resource;

      if (fromIdx < nodeBoxesCount)
      {
        const auto fromBoxId = rawCondensedLayout.nodeBoxIds[fromIdx];
        const auto &fromBoxRawLayout = boxesRawLayouts[fromBoxId];
        const NodeIdx fromBoxSinkIndex = fromBoxRawLayout.userNodeIds.size() + 1;

        for (const auto innerEdgeId : fromBoxRawLayout.rawObjects[fromBoxSinkIndex].inEdges)
        {
          const auto &innerEdge = fromBoxRawLayout.rawEdges[innerEdgeId];
          const auto innerDepId = innerEdge.carriedDeps.front();
          const auto innerResId = registryDependencies[innerDepId].resource;

          if (outerResId != ResourceId::Invalid && outerResId == innerResId ||
              outerResId == ResourceId::Invalid && outerDepId == innerDepId)
          {
            auto [newAnchorId, newAnchor] = rawCondensedLayout.anchors.appendNew();
            outerEdge.fromAnchor = newAnchorId;
            newAnchor = fromBoxRawLayout.anchors[innerEdge.toAnchor];
            break;
          }
        }
      }

      if (toIdx < nodeBoxesCount)
      {
        const auto toBoxId = rawCondensedLayout.nodeBoxIds[toIdx];
        const auto &toBoxRawLayout = boxesRawLayouts[toBoxId];
        const NodeIdx toBoxSourceIndex = toBoxRawLayout.userNodeIds.size() + 0;

        for (const auto innerEdgeId : toBoxRawLayout.rawObjects[toBoxSourceIndex].outEdges)
        {
          const auto &innerEdge = toBoxRawLayout.rawEdges[innerEdgeId];
          const auto innerDepId = innerEdge.carriedDeps.front();
          const auto innerResId = registryDependencies[innerDepId].resource;

          if (outerResId != ResourceId::Invalid && outerResId == innerResId ||
              outerResId == ResourceId::Invalid && outerDepId == innerDepId)
          {
            auto [newAnchorId, newAnchor] = rawCondensedLayout.anchors.appendNew();
            outerEdge.toAnchor = newAnchorId;
            newAnchor = toBoxRawLayout.anchors[innerEdge.fromAnchor];
            break;
          }
        }
      }
    }
  }

  calculateEdgeSplines(condensedLayout, rawCondensedLayout);
}


void Visualizer::generateRawEdges(RawLayout &raw_layout)
{
  TIME_PROFILE(generate_raw_edges);

  dag::Vector<RawEdgeId, framemem_allocator> edgesBetweenObjects;
  const auto rawEdgeSignComp = [&](const RawEdgeId edge_id, const DependencyId dep_id) {
    const auto &edgeDep = registryDependencies[raw_layout.rawEdges[edge_id].carriedDeps.front()];
    const auto &valDep = registryDependencies[dep_id];
    return edgeDep.type == valDep.type && edgeDep.resource == valDep.resource && edgeDep.resource != ResourceId::Invalid;
  };

  const auto &adjLists = raw_layout.layouterResult.adjacencyList;
  const auto &depMapping = raw_layout.layouterResult.edgeMapping;
  const auto &origDeps = raw_layout.outDeps;
  auto &objects = raw_layout.rawObjects;
  auto &edges = raw_layout.rawEdges;


  for (uint32_t objectIndex = 0; objectIndex < objects.size(); ++objectIndex)
    for (uint32_t edgeIndex = 0; edgeIndex < adjLists[objectIndex].size(); ++edgeIndex)
    {
      edgesBetweenObjects.clear();
      const uint32_t fromObjIdx = objectIndex;
      const uint32_t toObjIdx = adjLists[objectIndex][edgeIndex];
      const auto &[depSourceObjectIndex, dependenciesIndices] = depMapping[objectIndex][edgeIndex];
      for (const uint32_t dependencyIndex : dependenciesIndices)
      {
        const auto depId = origDeps[depSourceObjectIndex][dependencyIndex];
        auto it = eastl::find(edgesBetweenObjects.begin(), edgesBetweenObjects.end(), depId, rawEdgeSignComp);
        if (it == edgesBetweenObjects.end())
        {
          auto [newRawEdgeId, newRawEdge] = edges.appendNew(RawEdge{fromObjIdx, toObjIdx});
          newRawEdge.carriedDeps.push_back(depId);
          objects[fromObjIdx].outEdges.push_back(newRawEdgeId);
          objects[toObjIdx].inEdges.push_back(newRawEdgeId);
          edgesBetweenObjects.push_back(newRawEdgeId);
        }
        else
        {
          edges[*it].carriedDeps.push_back(depId);
        }
      }
    }
}

void Visualizer::generateAnchors(RawLayout &raw_layout)
{
  TIME_PROFILE(generate_anchors);

  const uint32_t objectsCount = raw_layout.rawObjects.size();

  const uint32_t userNodesCount = raw_layout.userNodeIds.size();
  const uint32_t nodeBoxesCount = raw_layout.nodeBoxIds.size();
  const uint32_t serviceNodesCount = raw_layout.outDeps.size() - (userNodesCount + nodeBoxesCount);
  const uint32_t fakeNodesCount = objectsCount - raw_layout.outDeps.size();

  const uint32_t userNodesIndexOffset = 0;
  const uint32_t nodeBoxesIndexOffset = userNodesIndexOffset + userNodesCount;
  const uint32_t serviceNodesIndexOffset = nodeBoxesIndexOffset + nodeBoxesCount;
  const uint32_t fakeNodesIndexOffset = serviceNodesIndexOffset + serviceNodesCount;

  dag::Vector<uint32_t, framemem_allocator> objectsSubranks(objectsCount);
  {
    const auto &ranks = raw_layout.layouterResult.nodeLayout;
    for (uint32_t rank = 0; rank < ranks.size(); ++rank)
      for (uint32_t subrank = 0; subrank < ranks[rank].size(); ++subrank)
        objectsSubranks[ranks[rank][subrank]] = subrank;
  }
  const auto rawEdgeBackOrder = [&](const RawEdgeId a, const RawEdgeId b) {
    return objectsSubranks[raw_layout.rawEdges[a].from] < objectsSubranks[raw_layout.rawEdges[b].from];
  };
  const auto rawEdgeForwOrder = [&](const RawEdgeId a, const RawEdgeId b) {
    return objectsSubranks[raw_layout.rawEdges[a].to] < objectsSubranks[raw_layout.rawEdges[b].to];
  };


  const uint32_t userResourceCount = userResources.size();
  IdIndexedMapping<ResourceId, AnchorId, framemem_allocator> resAnchors(userResourceCount);
  auto generateAnchorsOnSide = [&](dag::RelocatableFixedVector<RawEdgeId, 8> &edges_to_process,
                                 dag::RelocatableFixedVector<AnchorId, 8> &anchors_to_populate, bool in_edges) {
    resAnchors.clear();
    resAnchors.resize(userResourceCount, AnchorId::Invalid);
    for (const auto rawEdgeId : edges_to_process)
    {
      auto &edge = raw_layout.rawEdges[rawEdgeId];
      auto &targetEdgeAnchor = in_edges ? edge.toAnchor : edge.fromAnchor;
      const auto edgeResId = registryDependencies[edge.carriedDeps.front()].resource;
      if (edgeResId != ResourceId::Invalid)
      {
        AnchorId candidateAnchorId = resAnchors[edgeResId];
        if (candidateAnchorId == AnchorId::Invalid)
        {
          const auto newAnchorId = raw_layout.anchors.appendNew().first;
          resAnchors[edgeResId] = newAnchorId;
          anchors_to_populate.push_back(newAnchorId);
          candidateAnchorId = newAnchorId;
        }
        targetEdgeAnchor = candidateAnchorId;
      }
      else
      {
        const auto newAnchorId = raw_layout.anchors.appendNew().first;
        anchors_to_populate.push_back(newAnchorId);
        targetEdgeAnchor = newAnchorId;
      }
    }
  };


  for (uint32_t userNodeIndex = 0; userNodeIndex < userNodesCount; ++userNodeIndex)
  {
    auto &userNodeObject = raw_layout.rawObjects[userNodesIndexOffset + userNodeIndex];

    eastl::sort(userNodeObject.inEdges.begin(), userNodeObject.inEdges.end(), rawEdgeBackOrder);
    eastl::sort(userNodeObject.outEdges.begin(), userNodeObject.outEdges.end(), rawEdgeForwOrder);

    generateAnchorsOnSide(userNodeObject.inEdges, userNodeObject.inAnchors, true);
    generateAnchorsOnSide(userNodeObject.outEdges, userNodeObject.outAnchors, false);
  }

  if (serviceNodesCount != 0)
  {
    auto &sourceObject = raw_layout.rawObjects[serviceNodesIndexOffset + 0];
    auto &sinkObject = raw_layout.rawObjects[serviceNodesIndexOffset + 1];

    eastl::sort(sourceObject.outEdges.begin(), sourceObject.outEdges.end(), rawEdgeForwOrder);
    eastl::sort(sinkObject.inEdges.begin(), sinkObject.inEdges.end(), rawEdgeBackOrder);

    generateAnchorsOnSide(sourceObject.outEdges, sourceObject.outAnchors, false);
    generateAnchorsOnSide(sinkObject.inEdges, sinkObject.inAnchors, true);
  }

  for (uint32_t fakeNodeIndex = 0; fakeNodeIndex < fakeNodesCount; ++fakeNodeIndex)
  {
    auto &fakeNodeObject = raw_layout.rawObjects[fakeNodesIndexOffset + fakeNodeIndex];

    eastl::sort(fakeNodeObject.inEdges.begin(), fakeNodeObject.inEdges.end(), rawEdgeBackOrder);

    generateAnchorsOnSide(fakeNodeObject.inEdges, fakeNodeObject.inAnchors, true);

    for (const auto rawEdgeId : fakeNodeObject.outEdges)
    {
      auto &edge = raw_layout.rawEdges[rawEdgeId];
      const auto edgeResId = registryDependencies[edge.carriedDeps.front()].resource;
      if (edgeResId != ResourceId::Invalid)
        edge.fromAnchor = resAnchors[edgeResId];
      else
        for (const auto inRawEdgeId : fakeNodeObject.inEdges)
        {
          const auto &inEdge = raw_layout.rawEdges[inRawEdgeId];
          if (edge.carriedDeps.front() == inEdge.carriedDeps.front())
          {
            edge.fromAnchor = inEdge.toAnchor;
            break;
          }
        }
    }
  }
}


void Visualizer::calculateObjectsSizes(RawLayout &raw_layout)
{
  TIME_PROFILE(calculate_objects_sizes);

  const uint32_t objectsCount = raw_layout.rawObjects.size();
  if (objectsCount == 0)
    return;


  const uint32_t userNodesCount = raw_layout.userNodeIds.size();
  const uint32_t nodeBoxesCount = raw_layout.nodeBoxIds.size();
  const uint32_t fakeNodesCount = objectsCount - raw_layout.outDeps.size();

  const uint32_t userNodesIndexOffset = 0;
  const uint32_t nodeBoxesIndexOffset = userNodesIndexOffset + userNodesCount;
  const uint32_t fakeNodesIndexOffset = raw_layout.outDeps.size();


  const auto &objects = raw_layout.rawObjects;
  auto &objectsSizes = raw_layout.objectsSizes;
  auto &objectsColumnSizes = raw_layout.objectsColumnSizes;

  for (uint32_t userNodeIndex = 0; userNodeIndex < userNodesCount; ++userNodeIndex)
  {
    const auto &object = objects[userNodesIndexOffset + userNodeIndex];
    const auto &node = userNodes[raw_layout.userNodeIds[userNodeIndex]];
    const uint32_t maxAnchorsCount = max(object.inAnchors.size(), object.outAnchors.size());
    const uint32_t maxHistDepCount = max(node.inHistDeps.size(), node.outHistDeps.size());

    const float calcWidth = ImGui::CalcTextSize(getName(node.regId).data()).x + 2.f * NAMESPACE_BORDER_PADDING.x;
    const float calcHeight =
      NODE_Y_ANCHORS_START + 2.f * NODE_ANCHORS_MARGIN + 3.f * BASE_DEP_WIDTH * (maxAnchorsCount + 1 + maxHistDepCount);
    objectsSizes[userNodesIndexOffset + userNodeIndex] =
      ImMax(ImVec2{calcWidth, calcHeight}, ImVec2{NODE_MIN_X_SIZE, NODE_MIN_Y_SIZE});
  }

  for (uint32_t nodeBoxIndex = 0; nodeBoxIndex < nodeBoxesCount; ++nodeBoxIndex)
  {
    const auto &box = nodeBoxes[raw_layout.nodeBoxIds[nodeBoxIndex]];
    objectsSizes[nodeBoxesIndexOffset + nodeBoxIndex] = box.rect.size;
  }

  for (uint32_t fakeNodeIndex = 0; fakeNodeIndex < fakeNodesCount; ++fakeNodeIndex)
  {
    const auto &object = objects[fakeNodesIndexOffset + fakeNodeIndex];
    const uint32_t maxAnchorsCount = max(object.inAnchors.size(), object.outAnchors.size());
    objectsSizes[fakeNodesIndexOffset + fakeNodeIndex] =
      ImVec2{0.f, 2.f * NODE_ANCHORS_MARGIN + 3.f * BASE_DEP_WIDTH * maxAnchorsCount};
  }


  const auto &ranks = raw_layout.layouterResult.nodeLayout;
  const uint32_t rankCount = ranks.size();

  dag::Vector<float, framemem_allocator> columnSizes(rankCount, 0.f);
  for (uint32_t rank = 0; rank < rankCount; ++rank)
    for (const uint32_t objIndex : ranks[rank])
      columnSizes[rank] = max(columnSizes[rank], objectsSizes[objIndex].x);

  for (uint32_t rank = 0; rank < rankCount; ++rank)
    for (const uint32_t objIndex : ranks[rank])
      objectsColumnSizes[objIndex] = columnSizes[rank];
}


void Visualizer::calculateObjectsPositions(RawLayout &raw_layout)
{
  TIME_PROFILE(calculate_objects_positions);

  const uint32_t objectsCount = raw_layout.rawObjects.size();
  if (objectsCount == 0)
    return;


  const uint32_t fakeNodesIndexOffset = raw_layout.outDeps.size();

  const auto &ranks = raw_layout.layouterResult.nodeLayout;
  const uint32_t rankCount = ranks.size();
  dag::Vector<float, framemem_allocator> columnSizes(rankCount, 0.f);
  for (uint32_t columnIndex = 0; columnIndex < rankCount; ++columnIndex)
    columnSizes[columnIndex] = raw_layout.objectsColumnSizes[ranks[columnIndex][0]];

  const auto &objects = raw_layout.rawObjects;
  const auto &objectsSizes = raw_layout.objectsSizes;
  auto &objectsOffsets = raw_layout.objectsOffsets;
  const auto &edges = raw_layout.rawEdges;


  // now, we calculate object offsets
  {
    // here we count how many edges are between each pair
    // of adjacent columns, to make space between them wider if needed
    {
      dag::Vector<uint32_t, framemem_allocator> edgesBetween(rankCount, 0);
      for (const uint32_t objIndex : ranks[0])
        edgesBetween[0] = max(edgesBetween[0], objects[objIndex].outEdges.size());
      for (uint32_t rank = 1; rank < rankCount; ++rank)
        for (const uint32_t objIndex : ranks[rank])
        {
          edgesBetween[rank - 1] = max(edgesBetween[rank - 1], objects[objIndex].inEdges.size());
          edgesBetween[rank] = max(edgesBetween[rank], objects[objIndex].outEdges.size());
        }

      float horOffset = 0.f;
      float vertOffset = 0.f;
      for (uint32_t rank = 0; rank < rankCount; ++rank)
      {
        const auto &column = ranks[rank];
        vertOffset = 0.f;
        for (const uint32_t objIndex : column)
        {
          objectsOffsets[objIndex] = ImVec2{horOffset, vertOffset};
          vertOffset += objectsSizes[objIndex].y + OBJ_Y_BASE_MARGIN;
        }
        horOffset += columnSizes[rank] + OBJ_X_BASE_MARGIN * 0.1f * edgesBetween[rank];
      }
    }

    // after that, we need move objects up and down in columns to straighten edges, connecting them,
    // and lessen waves in long delendency lines
    auto equalizeHeights = [&](const uint32_t left_idx, const uint32_t right_idx, const uint32_t left_rank, const uint32_t right_rank,
                             bool forward) {
      // choose which object/column to move and what target center Y to aim for
      uint32_t rankToMove = 0;
      uint32_t objToMove = 0;
      uint32_t targetObj = 0;
      if (forward)
      {
        rankToMove = right_rank;
        objToMove = right_idx;
        targetObj = left_idx;
      }
      else
      {
        rankToMove = left_rank;
        objToMove = left_idx;
        targetObj = right_idx;
      }

      // find position of objToMove in its column
      const auto &column = raw_layout.layouterResult.nodeLayout[rankToMove];
      uint32_t objToMoveSubrank = 0;
      for (uint32_t subrank = 0; subrank < column.size(); ++subrank)
        if (column[subrank] == objToMove)
        {
          objToMoveSubrank = subrank;
          break;
        }

      // place the object at the target position
      const float targetCenterY = raw_layout.objectsOffsets[targetObj].y + raw_layout.objectsSizes[targetObj].y / 2.f;
      raw_layout.objectsOffsets[objToMove].y = targetCenterY - raw_layout.objectsSizes[objToMove].y / 2.f;

      // push objects below down if they would overlap
      for (uint32_t objIndex = objToMoveSubrank + 1; objIndex < column.size(); ++objIndex)
      {
        const uint32_t prev = column[objIndex - 1];
        const uint32_t cur = column[objIndex];
        const float minY = raw_layout.objectsOffsets[prev].y + raw_layout.objectsSizes[prev].y + OBJ_Y_BASE_MARGIN;
        if (raw_layout.objectsOffsets[cur].y < minY)
          raw_layout.objectsOffsets[cur].y = minY;
        else
          break;
      }

      // push objects above up if they would overlap
      for (uint32_t objIndex = objToMoveSubrank; objIndex > 0; --objIndex)
      {
        const uint32_t cur = column[objIndex - 1];
        const uint32_t next = column[objIndex];
        const float maxY = raw_layout.objectsOffsets[next].y - raw_layout.objectsSizes[cur].y - OBJ_Y_BASE_MARGIN;
        if (raw_layout.objectsOffsets[cur].y > maxY)
          raw_layout.objectsOffsets[cur].y = maxY;
        else
          break;
      }
    };

    // forward and backward run, trying to equalize fake nodes to keep long dependency edges straight
    for (uint32_t rank = 0; rank < rankCount - 1; ++rank)
      for (const auto from : ranks[rank])
        for (const auto rawEdgeId : objects[from].outEdges)
        {
          const NodeIdx to = edges[rawEdgeId].to;
          if (from >= fakeNodesIndexOffset && to >= fakeNodesIndexOffset)
          {
            equalizeHeights(from, to, rank, rank + 1, true);
            break;
          }
        }
    for (uint32_t rank = rankCount - 1; rank > 0; --rank)
      for (const auto to : ranks[rank])
        for (const auto rawEdgeId : objects[to].inEdges)
        {
          const NodeIdx from = edges[rawEdgeId].from;
          if (from >= fakeNodesIndexOffset && to >= fakeNodesIndexOffset)
          {
            equalizeHeights(from, to, rank - 1, rank, false);
            break;
          }
        }
  }


  // after, we shift all objects, so they all will have positive offsets
  {
    ImVec2 leftTop = objectsOffsets.front();
    ImVec2 rightBottom = leftTop;
    for (uint32_t objectIndex = 0; objectIndex < objectsCount; ++objectIndex)
    {
      leftTop = ImMin(leftTop, objectsOffsets[objectIndex]);
      rightBottom = ImMax(rightBottom, objectsOffsets[objectIndex] + objectsSizes[objectIndex]);
    }
    raw_layout.size = rightBottom - leftTop + NAMESPACE_BORDER_PADDING * 2;

    for (uint32_t objectIndex = 0; objectIndex < objectsCount; ++objectIndex)
      objectsOffsets[objectIndex] += NAMESPACE_BORDER_PADDING - leftTop;
  }
}

void Visualizer::calculateAnchorsPositions(RawLayout &raw_layout)
{
  TIME_PROFILE(calculate_anchors_positions);

  const uint32_t objectsCount = raw_layout.rawObjects.size();
  if (objectsCount == 0)
    return;


  const uint32_t userNodesCount = raw_layout.userNodeIds.size();
  const uint32_t nodeBoxesCount = raw_layout.nodeBoxIds.size();
  const uint32_t serviceNodesCount = raw_layout.outDeps.size() - (userNodesCount + nodeBoxesCount);

  const uint32_t userNodesIndexOffset = 0;
  const uint32_t nodeBoxesIndexOffset = userNodesIndexOffset + userNodesCount;
  const uint32_t serviceNodesIndexOffset = nodeBoxesIndexOffset + nodeBoxesCount;


  const auto &objects = raw_layout.rawObjects;
  const auto &objectsOffsets = raw_layout.objectsOffsets;
  const auto &objectsSizes = raw_layout.objectsSizes;
  auto &anchors = raw_layout.anchors;

  for (uint32_t objectIndex = 0; objectIndex < objectsCount; ++objectIndex)
  {
    const auto &object = objects[objectIndex];
    const ImVec2 offset = objectsOffsets[objectIndex];
    const ImVec2 size = objectsSizes[objectIndex];
    const float anchorsStartOffset = NODE_Y_ANCHORS_START + NODE_ANCHORS_MARGIN + 1.5f * BASE_DEP_WIDTH;

    float vertOffset = anchorsStartOffset;
    for (const auto anchorId : object.inAnchors)
    {
      anchors[anchorId] = {offset.x, offset.y + vertOffset};
      vertOffset += 3.f * BASE_DEP_WIDTH;
    }
    vertOffset = anchorsStartOffset;
    for (const auto anchorId : object.outAnchors)
    {
      anchors[anchorId] = {offset.x + size.x, offset.y + vertOffset};
      vertOffset += 3.f * BASE_DEP_WIDTH;
    }
  }

  if (serviceNodesCount != 0)
  {
    const auto &edges = raw_layout.rawEdges;
    const float rightBorder = raw_layout.size.x;

    for (const auto rawEdgeId : objects[serviceNodesIndexOffset + 0].outEdges)
    {
      const auto &edge = edges[rawEdgeId];
      anchors[edge.fromAnchor] = Anchor{0.f, anchors[edge.toAnchor].y};
    }

    for (const auto rawEdgeId : objects[serviceNodesIndexOffset + 1].inEdges)
    {
      const auto &edge = edges[rawEdgeId];
      anchors[edge.toAnchor] = Anchor{rightBorder, anchors[edge.fromAnchor].y};
    }
  }
}

void Visualizer::calculateNodeRectangles(CanvasLayout &layout, const RawLayout &raw_layout)
{
  TIME_PROFILE(calculate_node_rectangles);

  const uint32_t objectsCount = raw_layout.rawObjects.size();
  if (objectsCount == 0)
  {
    layout.size = NAMESPACE_BORDER_PADDING * 2;
    return;
  }

  auto &objectsOffsets = raw_layout.objectsOffsets;
  auto &objectsSizes = raw_layout.objectsSizes;

  layout.size = raw_layout.size;
  for (uint32_t userNodeIndex = 0; userNodeIndex < raw_layout.userNodeIds.size(); ++userNodeIndex)
    layout.nodes[raw_layout.userNodeIds[userNodeIndex]] = {objectsOffsets[userNodeIndex], objectsSizes[userNodeIndex]};
}

void Visualizer::calculateEdgeSplines(CanvasLayout &layout, const RawLayout &raw_layout)
{
  TIME_PROFILE(calculate_edge_splines);

  const uint32_t edgesCount = raw_layout.rawEdges.size();
  if (edgesCount == 0)
    return;


  layout.edges.reserve(edgesCount);

  uint32_t backEdgesDrawn = 0;
  for (const auto &rawEdge : raw_layout.rawEdges)
  {
    auto [edgeId, edge] = layout.edges.appendNew();
    edge.carriedDeps = rawEdge.carriedDeps;


    ImVec2 leftEdgeAnchor = raw_layout.anchors[rawEdge.fromAnchor];
    ImVec2 rightEdgeAnchor = raw_layout.anchors[rawEdge.toAnchor];

    // if starting object is narrower that column it is in,
    // we make additional spline to not collide with other objects in this column
    // and move edge start to the column end
    const NodeIdx from = rawEdge.from;
    if (raw_layout.objectsSizes[from].x < raw_layout.objectsColumnSizes[from])
    {
      const ImVec2 anchorDelta = ImVec2{raw_layout.objectsColumnSizes[from] - raw_layout.objectsSizes[from].x, 0.f};
      edge.splines.push_back({leftEdgeAnchor, leftEdgeAnchor, leftEdgeAnchor + anchorDelta, leftEdgeAnchor + anchorDelta});
      leftEdgeAnchor += anchorDelta;
    }

    // if the start of the edge is to the left of the end, we draw regular spline
    const float bezierOffset = abs(rightEdgeAnchor.x - leftEdgeAnchor.x) / 4;
    if (leftEdgeAnchor.x <= rightEdgeAnchor.x)
    {
      edge.splines.push_back(
        {leftEdgeAnchor, leftEdgeAnchor + ImVec2{bezierOffset, 0.f}, rightEdgeAnchor - ImVec2{bezierOffset, 0.f}, rightEdgeAnchor});
    }
    // but if the start of the edge is to the right of the end, we have backward edge
    // in that case we make 3 splines, to guide edge over the other objects
    else
    {
      const float horizontalOffset = bezierOffset * (backEdgesDrawn + 1);
      const float verticalOffset = OBJ_Y_BASE_MARGIN * (backEdgesDrawn + 1);

      edge.splines.push_back({
        .P0 = leftEdgeAnchor,
        .P1 = leftEdgeAnchor + ImVec2{horizontalOffset, 0.f},
        .P2 = ImVec2{leftEdgeAnchor.x + horizontalOffset, -verticalOffset},
        .P3 = ImVec2{leftEdgeAnchor.x, -verticalOffset},
      });
      edge.splines.push_back({
        .P0 = ImVec2{leftEdgeAnchor.x, -verticalOffset},
        .P1 = ImVec2{leftEdgeAnchor.x, -verticalOffset},
        .P2 = ImVec2{rightEdgeAnchor.x, -verticalOffset},
        .P3 = ImVec2{rightEdgeAnchor.x, -verticalOffset},
      });
      edge.splines.push_back({
        .P0 = ImVec2{rightEdgeAnchor.x, -verticalOffset},
        .P1 = ImVec2{rightEdgeAnchor.x - horizontalOffset, -verticalOffset},
        .P2 = rightEdgeAnchor - ImVec2{horizontalOffset, 0.f},
        .P3 = rightEdgeAnchor,
      });

      ++backEdgesDrawn;
    }
  }
}


void Visualizer::drawTree(const dafg::NameSpaceNameId &namespace_id, int &res_idx, int &counter, bool &selected_by_mouse,
  ImGuiDagor::ComboInfo &info, eastl::string &input, float text_width, float button_offset)
{
  const auto &knownNames = registry.knownNames;
  auto &nsContent = nameSpaces[namespace_id];

  eastl::string imguiLabel = namespace_id == knownNames.root() ? "" : knownNames.getShortName(namespace_id);
  const float freeWidthAfterText = eastl::max(text_width - ImGui::GetCursorPosX() - ImGui::CalcTextSize(imguiLabel.data()).x, 0.f);
  const int spaceCount = static_cast<int>(freeWidthAfterText / ImGui::CalcTextSize(" ").x);
  imguiLabel.append(spaceCount, ' ');
  imguiLabel.append_sprintf("##ns_%d", static_cast<int>(namespace_id));

  const ImGuiTreeNodeFlags commonFlags = ImGuiTreeNodeFlags_SpanLabelWidth | ImGuiTreeNodeFlags_FramePadding;
  const ImGuiTreeNodeFlags subTreeFlags = ImGuiTreeNodeFlags_DefaultOpen | commonFlags;

  const bool active = ImGui::GetStateStorage()->GetInt(ImGui::GetID(imguiLabel.data()), 1) != 0;
  const uint16_t totalResourcesInSubtree = nsContent.totalResourcesInSubtree;
  const bool needOpenSubTree = counter <= res_idx && res_idx < counter + totalResourcesInSubtree; // to show focused res
  bool closeSubTree = !active && needOpenSubTree;
  if (!active && needOpenSubTree)
    ImGui::TreeNodeSetOpen(ImGui::GetID(imguiLabel.data()), true);

  const bool opened = ImGui::TreeNodeEx(imguiLabel.data(), subTreeFlags);
  const bool hiddenNameSpace = nsContent.visibleResourcesInSubtree == 0;
  eastl::string buttonLabel = hiddenNameSpace ? "Show" : "Hide";
  buttonLabel.append_sprintf("##bns_%d", static_cast<int>(namespace_id));
  ImGui::SameLine();
  ImGui::SetCursorPosX(button_offset);
  if (ImGui::Button(buttonLabel.data()))
  {
    auto cit = nsResNameIds.begin() + counter;
    for (uint16_t i = 0; i < totalResourcesInSubtree; ++i, ++cit)
      userResources[regResRepresent[*cit]].hidden = !hiddenNameSpace;
    if (hiddenNameSpace)
      showResourcesInNameSpace(namespace_id);
    else
      hideResourcesInNameSpace(namespace_id);
  }

  if (opened)
  {
    for (const auto &childNS : nsContent.subNameSpaces)
      if (!selected_by_mouse)
        drawTree(childNS, res_idx, counter, selected_by_mouse, info, input, text_width, button_offset);
    ImGui::TreePop();
    if (closeSubTree)
      ImGui::TreeNodeSetOpen(ImGui::GetID(imguiLabel.data()), false);
  }
  else
  {
    counter += totalResourcesInSubtree;
    return;
  }

  const ImGuiTreeNodeFlags leafFlags = commonFlags | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Leaf;
  for (const auto &resData : nsContent.resources)
  {
    const bool isSelected = res_idx == counter;
    const bool hidden = userResources[regResRepresent[resData]].hidden;
    if (isSelected && (ImGui::IsWindowAppearing() || info.selectionChanged))
      ImGui::SetScrollHereY();
    if (isSelected && info.arrowScroll)
      ImGui::SetScrollHereY();

    eastl::string imguiLabel = knownNames.getShortName(resData);
    const float freeWidthAfterText = eastl::max(text_width - ImGui::GetCursorPosX() - ImGui::CalcTextSize(imguiLabel.data()).x, 0.f);
    const int spaceCount = static_cast<int>(freeWidthAfterText / ImGui::CalcTextSize(" ").x);
    imguiLabel.append(spaceCount, ' ');
    imguiLabel.append_sprintf("##res_%d", static_cast<int>(resData));

    const ImGuiTreeNodeFlags selectedFlag = isSelected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;
    if (ImGui::TreeNodeEx(imguiLabel.data(), leafFlags | selectedFlag))
    {
      if (!hidden && ImGui::IsItemClicked(ImGuiMouseButton_Left))
      {
        info.selectionChanged = res_idx != counter;
        res_idx = counter;
        input = knownNames.getName(resData);
        ImGui::CloseCurrentPopup();
        selected_by_mouse = true;
      }
      if (hidden && isSelected)
      {
        info.selectionChanged = true;
        res_idx = -1;
        input.clear();
        selected_by_mouse = true;
      }
    }

    eastl::string buttonLabel = hidden ? "Show" : "Hide";
    buttonLabel.append_sprintf("##bres_%d", static_cast<int>(resData));
    ImGui::SameLine();
    ImGui::SetCursorPosX(button_offset);
    if (ImGui::Button(buttonLabel.data()))
    {
      userResources[regResRepresent[resData]].hidden = !userResources[regResRepresent[resData]].hidden;
      dafg::NameSpaceNameId updatedNS = namespace_id;
      bool rootNameSpace = false;
      while (!rootNameSpace)
      {
        nameSpaces[updatedNS].visibleResourcesInSubtree += hidden ? 1 : -1;
        rootNameSpace = updatedNS == registry.knownNames.root();
        updatedNS = registry.knownNames.getParent(updatedNS);
      }
    }

    ++counter;
  }
}

bool Visualizer::treeWithFilter(const char *label, const dag::Vector<eastl::string_view> &data, int &current_idx, eastl::string &input,
  bool return_on_arrows, const char *hint, ImGuiComboFlags flags)
{
  if (data.empty())
  {
    ImGui::TextColored({1, 1, 0, 1}, "No resources");
    return false;
  }

  float textWidth = 0.f;
  eastl::for_each(data.begin(), data.end(),
    [&max = textWidth](const auto &b) { max = eastl::max(max, ImGui::CalcTextSize(b.data()).x); });
  const float buttonWidth = eastl::max(ImGui::CalcTextSize("Show").x, ImGui::CalcTextSize("Hide").x);
  const float padding = ImGui::GetStyle().FramePadding.x + ImGui::GetStyle().WindowPadding.x;
  const float width = textWidth + buttonWidth + ImGui::GetStyle().ScrollbarSize + 2.0f * padding;

  auto res = ImGuiDagor::BeginComboWithFilter(label, data, current_idx, input, width, hint, flags);

  if (!res.has_value())
    return false;

  auto &info = res.value();

  if (info.inputTextChanged && !info.arrowScroll)
    ImGui::CloseCurrentPopup();

  int counter = 0;
  bool selectedByMouse = false;
  const float buttonOffset =
    ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize - buttonWidth - ImGui::GetStyle().FramePadding.x;
  drawTree(registry.knownNames.root(), current_idx, counter, selectedByMouse, info, input, textWidth, buttonOffset);
  info.arrowScroll = info.arrowScroll && return_on_arrows;
  return ImGuiDagor::EndComboWithFilter(label, data, current_idx, input, info, selectedByMouse);
}

} // namespace visualization::usergraph

} // namespace dafg

static bool dafg_show_fg_tex_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("dafg", "show_fg_tex", 1, DebugTexOverlay::MAX_CONSOLE_ARGS_CNT)
  {
    if (dafg::Runtime::isInitialized())
    {
      // TODO: awful disgusting hack, remove asap
      auto &registry = dafg::Runtime::get().getInternalRegistry();
      const auto &depData = dafg::Runtime::get().getDependencyData();

      if (argc > 1)
      {
        // I don't care about perf here
        eastl::string name = argv[1];
        if (name[0] != '/')
          name = "/" + name;

        auto resId = registry.knownNames.getNameId<dafg::ResNameId>(name);
        if (resId == dafg::ResNameId::Invalid)
        {
          console::print_d("daFG: resource %s not found.", name.c_str());
          return found;
        }

        eastl::string concatenatedParams;
        for (int i = 2; i < argc; ++i)
        {
          concatenatedParams += argv[i];
          if (i + 1 < argc)
            concatenatedParams += " ";
        }
        set_manual_showtex_params(concatenatedParams.c_str());

        update_fg_debug_tex(Selection{ReadTimePoint{}, resId}, registry, depData);
      }
      else
      {
        update_fg_debug_tex({}, registry, depData);
      }
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(dafg_show_fg_tex_console_handler);