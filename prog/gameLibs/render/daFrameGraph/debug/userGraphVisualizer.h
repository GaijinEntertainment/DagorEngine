// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dafgVisualizerStructuresUser.h>

#include <debug/textureVisualization.h>

#include <frontend/internalRegistry.h>
#include <frontend/nodeTracker.h>
#include <frontend/nameResolver.h>
#include <frontend/dependencyData.h>

#include <backend/intermediateRepresentation.h>

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <imgui-node-editor/imgui_canvas.h>
#include <ska_hash_map/flat_hash_map2.hpp>


namespace dafg::visualization
{

class UserGraphVisualizer
{
public:
  UserGraphVisualizer(InternalRegistry &intRegistry, const NameResolver &nameResolver, const DependencyData &depData,
    const intermediate::Graph &irGraph, const PassColoring &coloring);

  void draw();

  void drawUI();

  void drawCanvas();

  void drawAllNodes(ImDrawList *drawList);
  void drawEdge(ImDrawList *drawList, VisualizedEdge &edge, uint32_t from, uint32_t to);

  void processPopup();
  void processCentering();
  void processInput();


  void updateVisualization();

  // sometimes we have to get dirty to stay clean...
  static VisualizedEdge *selectedEdge;

private:
  InternalRegistry &registry;
  const NameResolver &resolver;
  const DependencyData &depData;
  DebugPassColoration passColoring;

  const intermediate::Graph &intermediateGraph;
  const PassColoring &intermediatePassColoring;

  ImGuiEx::Canvas canvas;

  bool focusedNodeChanged = false;
  bool focusedResourceChanged = false;
  bool searchBoxHovered = false;

  bool canvasHovered = false;

  String tooltipMessage;
  ImVec2 centerView;
  ImVec2 focusedResourceLeftmostEdgeStart;
  ImVec2 focusedResourceLeftmostEdgeEnd;

  dag::Vector<ImVec2> nodeLeftAnchors;
  dag::Vector<ImVec2> nodeRightAnchors;


  bool hasResources;


  // vars from old visualizator
private:
  // from global
  eastl::array<EdgeTypeDebugInfo, static_cast<size_t>(EdgeType::VISIBLE_COUNT)> edge_type_debug_infos{
    EdgeTypeDebugInfo{"EXPLICIT_PREVIOUS", "Precedes", "red", IM_COL32(255, 0, 0, 255)},
    EdgeTypeDebugInfo{"EXPLICIT_FOLLOW", "Follows", "blue", IM_COL32(0, 0, 255, 255)},
    EdgeTypeDebugInfo{"IMPLICIT_RES_DEP", "Uses Resource Of", "purple", IM_COL32(255, 0, 255, 255)},
    EdgeTypeDebugInfo{"IMPLICIT_MOD_CHAIN", "Modification Chain", "darkviolet", IM_COL32(148, 0, 211, 255)}};

  dag::Vector<dag::Vector<EdgeData>> labeled_graph;
  dag::Vector<NodeNameId> nodeIds;
  dag::Vector<ResNameId> resIds;
  dag::Vector<bool> nodeHasExplicitOrderings;
  dag::Vector<eastl::string_view> nodeNames;
  dag::Vector<eastl::string_view> resNames;

  // Used for imgui visualization
  dag::Vector<dag::Vector<uint32_t>> node_table;
  // We need fake nodes and fake edges when visualizing with imgui,
  // which changes the adjacency list.
  dag::Vector<dag::Vector<eastl::pair<uint32_t, VisualizedEdge>>> visualized_adjacency_list;


  // from visualizer
  ska::flat_hash_map<NameSpaceNameId, NameSpaceContents> nameSpaces{};
  IdIndexedFlags<ResNameId> hiddenResources{};


  // from global
  eastl::unique_ptr<IGraphLayouter> layouter = make_default_layouter();

  const float scale_levels[18] = {
    0.1f, 0.15f, 0.20f, 0.25f, 0.33f, 0.5f, 0.75f, 1.0f, 1.25f, 1.50f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
  int current_scale_level = 7; // 1.0 scale

  int focusedNodeId = -1;
  eastl::optional<FocusedResource> focusedResource{};
  eastl::string nodeSearchInput;
  ImVec2 prevFocusedNodePos;
  eastl::string resourceSearchInput;

  int focusedResourceNo = -1;

  ColorationType coloration = ColorationType::None;
  eastl::array<eastl::pair<const char *, ColorationType>, 3> colorationTypes = {
    eastl::pair{(const char *)"None", ColorationType::None},
    eastl::pair{(const char *)"Color by framebuffers", ColorationType::Framebuffer},
    eastl::pair{(const char *)"Color by passes", ColorationType::Pass}};

  dafg::ResNameId selectedResId = dafg::ResNameId::Invalid;
  bool shouldUpdateVisualization = false;

  ImVec2 viewOrigin = ImVec2(0.0f, 0.0f);
  dag::Vector<VisualizedEdge *> hoverEdges = {};
  dag::Vector<VisualizedEdge *> popupEdges = {};


  // functions from old visualizator
private:
  // from global
  const EdgeTypeDebugInfo &infoForEdgeType(EdgeType type) { return edge_type_debug_infos[static_cast<size_t>(type)]; }

  bool visualizationDataGenerated() { return visualized_adjacency_list.size() >= nodeIds.size(); }

  bool nodeIsFake(uint32_t node) { return node >= nodeIds.size(); }

  const uint32_t CULLED_OUT_NODE = static_cast<uint32_t>(-1);
  void addDebugEdge(uint32_t from, uint32_t to, EdgeType type, ResNameId res_id)
  {
    if (from != CULLED_OUT_NODE && to != CULLED_OUT_NODE)
      labeled_graph[from].push_back({to, type, res_id});
  }


  // from visualizer
  void collectValidNodes(eastl::span<const NodeNameId> node_execution_order);

  void generateDependencyEdges(eastl::span<const NodeNameId> node_execution_order);

  void updateNameSpaces();

  void hideResourcesInSubTree(NameSpaceNameId name_space_id);

  void hideResourcesInNameSpace(NameSpaceNameId namespace_id);

  void showResourcesInSubTree(NameSpaceNameId namespace_id);

  void showResourcesInNameSpace(NameSpaceNameId namespace_id);

  void processTreeContents(NameSpaceNameId namespace_id);

  template <class EnumType>
  eastl::string_view getName(EnumType res_id);

  template <class EnumType>
  dag::Vector<eastl::string_view> gatherNames(eastl::span<const EnumType> ids);

  template <class EnumType>
  NameSpaceNameId getParent(EnumType id);


  // from global
  void generatePlanarizedLayout();

  void invalidateGraphVisualization();

  void resetTextureVisualization() { clear_visualization_node(); }

  void showDependenciesWindow() { imgui_window_set_visible(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME, true); }

  void setFocusedResource(dafg::ResNameId resId, FocusedResourceAction focusedResourceAction = FocusedResourceAction::ShowAll)
  {
    focusedResource = eastl::optional<FocusedResource>({{
      .id = resId,
      .hasRenames = false,
      .action = focusedResourceAction,
    }});

    G_ASSERTF(focusedResource->id != dafg::ResNameId::Invalid, "invalid resource id");
    for (const auto &[from, to] : depData.renamingChains.enumerate())
    {
      focusedResource->hasRenames =
        (from != focusedResource->id && to == focusedResource->id) || (from == focusedResource->id && to != focusedResource->id);
      if (focusedResource->hasRenames)
        return;
    }
  }

  bool isVisibleRename(dafg::ResNameId resId) const
  {
    return focusedResource->action != FocusedResourceAction::FocusOnResource &&
           depData.renamingRepresentatives[resId] == depData.renamingRepresentatives[focusedResource->id];
  }

  void drawTree(const dafg::NameSpaceNameId &namespace_id, int &res_idx, int &counter, bool &selected_by_mouse,
    ImGuiDagor::ComboInfo &info, eastl::string &input, float text_width, float button_offset);

  bool treeWithFilter(const char *label, const dag::Vector<eastl::string_view> &data, int &current_idx, eastl::string &input,
    bool return_on_arrows = true, const char *hint = "", ImGuiComboFlags flags = 0);
};

} // namespace dafg::visualization