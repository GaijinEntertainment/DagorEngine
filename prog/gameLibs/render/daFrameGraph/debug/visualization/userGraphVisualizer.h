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
  Visualizer(InternalRegistry &int_registry, const DependencyData &dep_data, const intermediate::Graph &ir_graph,
    const PassColoring &coloring);

  void draw();
  void updateVisualization(const IdIndexedFlags<NodeNameId, framemem_allocator> &nodes_changed);


  // draw functions
private:
  void drawUI();
  void drawCanvas();

  void drawNodes(ImDrawList *draw_list, const CanvasLayout &layout);
  void drawEdges(ImDrawList *draw_list, const CanvasLayout &layout);
  void drawNodeBoxes(ImDrawList *draw_list);

  // control functions
private:
  void checkHovering();
  void processInput();
  void processPopup();
  void processTextureDebug();

  // update functions
private:
  void updateNodesRess();
  void updateIRInfo();
  void updateNameSpaces();
  void updateDependencies();

  void calculateNodesColors();
  void checkCycles();
  void condenseGraph();

  // layout functions
private:
  void performLayout();
  void performCondensedLayout();

  void generateRawEdges(RawLayout &raw_layout);
  void generateAnchors(RawLayout &raw_layout);

  void calculateObjectsSizes(RawLayout &raw_layout);
  void calculateObjectsPositions(RawLayout &raw_layout);
  void calculateAnchorsPositions(RawLayout &raw_layout);

  void calculateNodeRectangles(CanvasLayout &layout, const RawLayout &raw_layout);
  void calculateEdgeSplines(CanvasLayout &layout, const RawLayout &raw_layout);


  // data sources
private:
  InternalRegistry &registry;
  const DependencyData &depData;

  const intermediate::Graph &intermediateGraph;
  const PassColoring &intermediatePassColoring;


  // gathered data
private:
  IdIndexedFlags<NodeNameId> lastChangedNodes;  // We store nodes, changed during last recompilation
  IdIndexedFlags<NodeNameId> accumChangedNodes; // and accumulate them, if graph was recompiled multiple times

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

  IdIndexedMapping<NodeBoxId, NodeBox> nodeBoxes;
  IdIndexedMapping<NodeId, NodeBoxId> nodesNodeBox;


  // layouts
private:
  eastl::unique_ptr<IGraphLayouter> layouter = make_default_layouter();
  CanvasLayout generalLayout;   // Non-hierarchical layout, nodes are placed individually
  CanvasLayout condensedLayout; // Hierarchical layout, nodes are grouped by name spaces
  bool hierarchicalView = false;
  bool updateNeeded = true;


private:
  ImGuiEx::Canvas canvas;
  enum CanvasChannels
  {
    SUSPEND = 0,
    NAMESPACES,
    EDGES,
    NODES,
    TEXTS,
    COUNT
  };

  CanvasCamera canvasCamera{};

  NodeColorationType coloration = NodeColorationType::None;


  // hovering
private:
  struct
  {
    bool window = false;
    bool searchBox = false;
    bool canvas = false;

    NodeId node = NodeId::Invalid;
    ResourceId resource = ResourceId::Invalid;
    dag::Vector<DependencyId> deps = {};

    String tooltip;

    void reset()
    {
      window = false;
      searchBox = false;
      canvas = false;

      node = NodeId::Invalid;
      resource = ResourceId::Invalid;
      deps.clear();

      tooltip.clear();
    }
  } hoverState;
  NodeId popupNode = NodeId::Invalid;
  dag::Vector<DependencyId> popupDeps = {};


  // focusing
private:
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

  void setFocusedNode(NodeId node_id);
  void centerOnFocusedNode();
  inline void resetFocusedNode()
  {
    nodeSearchInput.clear();
    focusedNodeIndex = UNKNOWN_INDEX;
    focusedNode = {};
  };

  void setFocusedResource(ResourceId res_id, ResourceFocusType focus_type = ResourceFocusType::All);
  void centerOnFocusedRes();
  inline void resetFocusedResource()
  {
    resourceSearchInput.clear();
    focusedResourceIndex = UNKNOWN_INDEX;
    focusedResource = {};
  };


  // inspecting
private:
  struct
  {
    DependencyId id = DependencyId::Invalid;
    bool wasChanged = false;

    inline void set(const DependencyId dep_id)
    {
      id = dep_id;
      wasChanged = true;
    }
    inline void reset() { set(DependencyId::Invalid); }
  } inspectedDependency;


  // misc functions
private:
  template <class EnumType>
  inline NameSpaceNameId getParent(EnumType id) const
  {
    return registry.knownNames.getParent(id);
  }

  template <class EnumType>
  inline eastl::string_view getName(EnumType res_id) const
  {
    return registry.knownNames.getName(res_id);
  }

  template <class EnumType>
  inline eastl::string_view getShortName(EnumType res_id) const
  {
    return registry.knownNames.getShortName(res_id);
  }

  template <class EnumType>
  inline dag::Vector<eastl::string_view> gatherNames(eastl::span<const EnumType> ids) const
  {
    dag::Vector<eastl::string_view> names;
    names.reserve(ids.size());
    for (auto id : ids)
      names.push_back(getName(id));
    return names;
  }

  inline bool isResourceVisible(const ResourceId res_id) const
  {
    return res_id == ResourceId::Invalid ||
           !userResources[res_id].hidden && (!focusedResource.valid() || focusedResource.type == ResourceFocusType::All ||
                                              focusedResource.type == ResourceFocusType::Resource && focusedResource.id == res_id ||
                                              focusedResource.type == ResourceFocusType::ResourceAndRenames &&
                                                depData.renamingRepresentatives[nameIdByResId(res_id)] ==
                                                  depData.renamingRepresentatives[nameIdByResId(focusedResource.id)]);
  }


  // tree view functions
private:
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