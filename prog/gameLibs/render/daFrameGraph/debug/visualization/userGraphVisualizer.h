// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/visualization/structuresUser.h>

#include <frontend/internalRegistry.h>
#include <frontend/nodeTracker.h>
#include <frontend/nameResolver.h>
#include <frontend/dependencyData.h>

#include <backend/intermediateRepresentation.h>

#include <debug/textureVisualization.h>

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <imgui-node-editor/imgui_canvas.h>
#include <ska_hash_map/flat_hash_map2.hpp>


namespace dafg::visualization::usergraph
{

class Visualizer
{
public:
  Visualizer(InternalRegistry &int_registry, const NameResolver &name_resolver, const DependencyData &dep_data,
    const intermediate::Graph &ir_graph, const PassColoring &coloring);

  void draw();

  void drawUI();

  void drawCanvas();
  enum CanvasChannels
  {
    SUSPEND = 0,
    NAMESPACES,
    EDGES,
    NODES,
    TEXTS,
    COUNT
  };

  void drawNodeBox(ImDrawList *draw_list, const NodeBoxId node_box_id, const ImVec2 offset);
  void drawNode(ImDrawList *draw_list, const NodeNameId node_id, const ImVec2 node_offset, const ImVec2 node_size);
  void drawEdges(ImDrawList *draw_list, const Layout &layout, const ImVec2 offset);

  void processPopup();
  void processCentering();
  void processInput();
  void processTextureDebug();


  void updateVisualization();

  void updateIRInfo();
  void updateNameSpaces();
  void updateDependencies();

  void checkCycles();
  void condenseGraph();

  void performLayout();
  void performBoxLayout(const NodeBox &node_box, Layout &node_box_layout);
  void performCondensedLayout();
  void calculatePositions(Layout &layout, const eastl::span<NodeNameId> user_nodes, const eastl::span<NodeBoxId> node_boxes,
    const bool has_io);


  // data sources
  const NameResolver &resolver;

private:
  InternalRegistry &registry;
  const DependencyData &depData;

  const intermediate::Graph &intermediateGraph;
  const PassColoring &intermediatePassColoring;


  // gathered data
private:
  // from InternalRegistry
  IdIndexedMapping<NameSpaceNameId, NameSpace> nameSpaces; // Used for nodes grouping
  dag::Vector<NodeNameId> nodeIds;                         // Node ids in namespace order
  dag::Vector<ResNameId> resIds;                           // Resource ids in namespace order
  dag::Vector<eastl::string_view> nodeNames;               // Node names in namespace order for ImGuiDagor::ComboWithFilter()
  dag::Vector<eastl::string_view> resNames;                // Resource names in namespace order for ImGuiDagor::ComboWithFilter()

  IdIndexedMapping<DependencyId, Dependency> registryDependencies;  // List of all dependencies, stated in InternalRegistry
  IdIndexedMapping<NodeNameId, NodeDependencies> nodesDependencies; // Lists of dependencies per user node
  dag::Vector<DependencyId> historyDependencies;                    // List of history reads, not used for layouting
  dag::Vector<DependencyId> disabledDependencies;                   // Dependencies, that were disabled during check for cycles

  // from intermediate::Graph
  IdIndexedMapping<NodeNameId, PassColor> passColors;   // Info for visualizator to show node pass colors
  IdIndexedMapping<NodeNameId, uint32_t> executionTime; // Info for layouter to cpecify modification chain orders


  // generated data
private:
  IdIndexedMapping<NodeBoxId, NodeBox> nodeBoxes;
  IdIndexedMapping<NodeBoxDependencyId, NodeBoxDependency> nodeBoxDependencies;
  IdIndexedMapping<NodeNameId, NodeBoxId> nodesNodeBox;
  IdIndexedMapping<NodeBoxId, NameSpaceNameId> nodeBoxesNameSpace;

  IdIndexedMapping<NodeBoxId, Layout> nodeBoxesLayouts;

  IdIndexedFlags<NodeNameId> nodesCycled;    // Flags to indicate, that node is participate in cycle
  IdIndexedFlags<ResNameId> hiddenResources; // Flags to hide/show resources in resource tree view
  Layout wholeGraphLayout;                   // Non-hierarchical layout, containing all user-defined nodes
  Layout condensedLayout;                    // Hierarchical layout with all boxes in condensed graph
  bool layoutReady = false;
  bool hierarchicalView = false;


private:
  ImGuiEx::Canvas canvas;

  CanvasCamera canvasCamera{
    .initCanvasScaleId = 7,
    .canvasScales = {0.1f, 0.15f, 0.20f, 0.25f, 0.33f, 0.5f, 0.75f, 1.0f, 1.25f, 1.50f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f,
      8.0f},
  };

  ImVec2 focusedResLeftEnd;
  ImVec2 focusedResRightEnd;


private:
  bool searchBoxHovered = false;

  eastl::string nodeSearchInput;
  int focusedNodeIndex = UNKNOWN_INDEX;
  struct
  {
    NodeNameId id = NodeNameId::Invalid;
    bool wasChanged = false;

    inline bool valid() const { return id != NodeNameId::Invalid; }
  } focusedNode;

  eastl::string resourceSearchInput;
  int focusedResourceIndex = UNKNOWN_INDEX;
  struct
  {
    ResNameId id = ResNameId::Invalid;
    bool wasChanged = false;
    bool hasRenames;
    ResourceFocusType type;

    inline bool valid() const { return id != ResNameId::Invalid; }
  } focusedResource;


private:
  inline bool isNodeFake(NodeIdx nodeidx) { return nodeidx >= registry.nodes.size(); }

  void setFocusedNode(NodeNameId node_id);
  inline void resetFocusedNode()
  {
    setFocusedNode(NodeNameId::Invalid);
    nodeSearchInput.clear();
    focusedNodeIndex = UNKNOWN_INDEX;
  };

  void setFocusedResource(ResNameId res_id, ResourceFocusType focus_type = ResourceFocusType::All);
  inline void resetFocusedResource()
  {
    setFocusedResource(ResNameId::Invalid);
    resourceSearchInput.clear();
    focusedResourceIndex = UNKNOWN_INDEX;
  };


  // vars from old visualizator
private:
  bool canvasHovered = false;

  String tooltipMessage;
  ImVec2 centerView;

  eastl::unique_ptr<IGraphLayouter> layouter = make_default_layouter();

  ImVec2 prevFocusedNodePos;

  ColorationType coloration = ColorationType::None;
  eastl::array<eastl::pair<const char *, ColorationType>, 3> colorationTypes = {
    eastl::pair{(const char *)"None", ColorationType::None},
    eastl::pair{(const char *)"Color by framebuffers", ColorationType::Framebuffer},
    eastl::pair{(const char *)"Color by passes", ColorationType::Pass}};

  dafg::ResNameId selectedResId = dafg::ResNameId::Invalid;
  bool shouldUpdateVisualization = false;

  dag::Vector<const Edge *> hoverEdges = {};
  dag::Vector<const Edge *> popupEdges = {};


  // functions from old visualizator
private:
  template <class EnumType>
  inline NameSpaceNameId getParent(EnumType id)
  {
    return registry.knownNames.getParent(id);
  }

  template <class EnumType>
  inline eastl::string_view getName(EnumType res_id)
  {
    return registry.knownNames.getName(res_id);
  }

  template <class EnumType>
  inline dag::Vector<eastl::string_view> gatherNames(eastl::span<const EnumType> ids)
  {
    dag::Vector<eastl::string_view> names;
    names.reserve(ids.size());
    for (auto id : ids)
      names.push_back(getName(id));
    return names;
  }

  inline bool isVisibleRename(dafg::ResNameId res_id) const
  {
    return focusedResource.type != ResourceFocusType::Resource &&
           depData.renamingRepresentatives[res_id] == depData.renamingRepresentatives[focusedResource.id];
  }


  void hideResourcesInSubTree(NameSpaceNameId name_space_id);

  void hideResourcesInNameSpace(NameSpaceNameId namespace_id);

  void showResourcesInSubTree(NameSpaceNameId namespace_id);

  void showResourcesInNameSpace(NameSpaceNameId namespace_id);

  void drawTree(const dafg::NameSpaceNameId &namespace_id, int &res_idx, int &counter, bool &selected_by_mouse,
    ImGuiDagor::ComboInfo &info, eastl::string &input, float text_width, float button_offset);

  bool treeWithFilter(const char *label, const dag::Vector<eastl::string_view> &data, int &current_idx, eastl::string &input,
    bool return_on_arrows = true, const char *hint = "", ImGuiComboFlags flags = 0);
};

} // namespace dafg::visualization::usergraph