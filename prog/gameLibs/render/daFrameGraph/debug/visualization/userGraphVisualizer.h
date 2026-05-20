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
  void drawNode(ImDrawList *draw_list, const NodeId node_id, const ImVec2 node_offset, const ImVec2 node_size);
  void drawEdges(ImDrawList *draw_list, const Layout &layout, const ImVec2 offset);

  void processPopup();
  void processCentering();
  void processInput();
  void processTextureDebug();


  void updateVisualization();

  void updateNodesRess();
  void updateIRInfo();
  void updateNameSpaces();
  void updateDependencies();

  void checkCycles();
  void condenseGraph();

  void performLayout();
  void performBoxLayout(const NodeBox &node_box, Layout &node_box_layout);
  void performCondensedLayout();
  void calculatePositions(Layout &layout, const eastl::span<NodeId> user_nodes, const eastl::span<NodeBoxId> node_boxes,
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
  IdIndexedMapping<NodeId, Node> userNodes;               // Nodes and resources,
  IdIndexedMapping<ResourceId, Resource> userResources;   // filtered of debug instances
  IdIndexedMapping<NodeNameId, NodeId> regNodesRepresent; // and mappings registry id -> represantative id
  IdIndexedMapping<ResNameId, ResourceId> regResRepresent;
  inline NodeNameId nameIdByNodeId(const NodeId node_id) const { return userNodes[node_id].regId; };
  inline ResNameId nameIdByResId(const ResourceId resource_id) const { return userResources[resource_id].regId; }
  inline bool nodeIsPresented(NodeNameId id) const { return regNodesRepresent[id] != NodeId::Invalid; }
  inline bool resIsPresented(ResNameId id) const { return regResRepresent[id] != ResourceId::Invalid; }

  IdIndexedMapping<NameSpaceNameId, NameSpace> nameSpaces; // Information about sub namespaces, nodes and resources for
  dag::Vector<NodeNameId> nsNodeNameIds;                   // ImGuiDagor::ComboWithFilter() and treeWithFilter()
  dag::Vector<ResNameId> nsResNameIds;
  dag::Vector<eastl::string_view> nsNodeNames;
  dag::Vector<eastl::string_view> nsResNames;

  IdIndexedMapping<DependencyId, Dependency> registryDependencies; // List of all dependencies, stated in InternalRegistry
  dag::Vector<DependencyId> disabledDependencies;                  // Dependencies, disabled during check for cycles


  // generated data
private:
  IdIndexedMapping<NodeBoxId, NodeBox> nodeBoxes;
  IdIndexedMapping<NodeId, NodeBoxId> nodesNodeBox;
  IdIndexedMapping<NodeBoxDependencyId, NodeBoxDependency> nodeBoxDependencies;

  IdIndexedMapping<NodeBoxId, Layout> nodeBoxesLayouts;

  Layout wholeGraphLayout; // Non-hierarchical layout, containing all user-defined nodes
  Layout condensedLayout;  // Hierarchical layout with all boxes in condensed graph
  bool layoutReady = false;
  bool hierarchicalView = false;
  bool updateNeeded = true;


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
    NodeId id = NodeId::Invalid;
    bool wasChanged = false;

    inline bool valid() const { return id != NodeId::Invalid; }
  } focusedNode;

  eastl::string resourceSearchInput;
  int focusedResourceIndex = UNKNOWN_INDEX;
  struct
  {
    ResourceId id = ResourceId::Invalid;
    bool wasChanged = false;
    bool hasRenames = false;
    ResourceFocusType type = ResourceFocusType::All;

    inline bool valid() const { return id != ResourceId::Invalid; }
  } focusedResource;


private:
  void setFocusedNode(NodeId node_id);
  inline void resetFocusedNode()
  {
    nodeSearchInput.clear();
    focusedNodeIndex = UNKNOWN_INDEX;
    focusedNode = {};
  };

  void setFocusedResource(ResourceId res_id, ResourceFocusType focus_type = ResourceFocusType::All);
  inline void resetFocusedResource()
  {
    resourceSearchInput.clear();
    focusedResourceIndex = UNKNOWN_INDEX;
    focusedResource = {};
  };


private:
  inline bool isVisibleRename(ResourceId res_id) const
  {
    return focusedResource.type != ResourceFocusType::Resource && depData.renamingRepresentatives[nameIdByResId(res_id)] ==
                                                                    depData.renamingRepresentatives[nameIdByResId(focusedResource.id)];
  }


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
  inline eastl::string_view getShortName(EnumType res_id)
  {
    return registry.knownNames.getShortName(res_id);
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