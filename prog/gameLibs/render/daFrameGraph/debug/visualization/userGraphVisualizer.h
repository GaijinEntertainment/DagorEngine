// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/visualization/structuresUser.h>
#include <debug/visualization/gpuCaptureWindow.h>

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
    const PassColoring &coloring) :
    registry(int_registry), depData(dep_data), intermediateGraph(ir_graph), intermediatePassColoring(coloring)
  {
    REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME, [&]() { this->draw(); });
    REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP_FG2, IMGUI_GPU_CAPTURE_WIN_NAME, [&]() { this->drawGpuCaptureWindow(); });
  }

  void draw();
  void updateVisualization(const IdIndexedFlags<NodeNameId, framemem_allocator> &nodes_changed);
  void receiveBlobData(NodeNameId node_id, ResNameId res_id, const BlobView &blob_view);


  // draw functions
private:
  void drawUI();
  void drawGpuCaptureWindow();
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

  bool isFrontend(const NodeId id) const { return id != NodeId::Invalid && userNodes[id].regId != NodeNameId::Invalid; }
  bool isFrontend(const ResourceId id) const { return id != ResourceId::Invalid && userResources[id].regId != ResNameId::Invalid; }
  NodeNameId getNameId(const NodeId id) const { return id != NodeId::Invalid ? userNodes[id].regId : NodeNameId::Invalid; }
  ResNameId getNameId(const ResourceId id) const { return id != ResourceId::Invalid ? userResources[id].regId : ResNameId::Invalid; }

  bool isPresented(const NodeNameId id) const { return id != NodeNameId::Invalid && regNodesRepresent[id] != NodeId::Invalid; }
  bool isPresented(const ResNameId id) const { return id != ResNameId::Invalid && regResRepresent[id] != ResourceId::Invalid; }
  NodeId getRepresentId(const NodeNameId id) const { return id != NodeNameId::Invalid ? regNodesRepresent[id] : NodeId::Invalid; }
  ResourceId getRepresentId(const ResNameId id) const { return id != ResNameId::Invalid ? regResRepresent[id] : ResourceId::Invalid; }

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
  bool macroView = false;

  NodeColorationType coloration = NodeColorationType::None;


  // hovering
private:
  struct HoverState
  {
    bool window = false;
    bool canvas = false;

    NodeId node = NodeId::Invalid;
    ResourceId resource = ResourceId::Invalid;
    dag::Vector<DependencyId> deps = {};
    // History edges are not a part of the layout, so they are hovered while they are drawn
    bool historyEdge = false;

    String tooltip;

    void reset()
    {
      window = false;
      canvas = false;

      node = NodeId::Invalid;
      resource = ResourceId::Invalid;
      deps.clear();
      historyEdge = false;

      tooltip.clear();
    }
    bool isActive() { return window && canvas; }
  } hoverState;


  // popup
private:
  struct PopupState
  {
    NodeId nodeId = NodeId::Invalid;
    ResourceId resourceId = ResourceId::Invalid;

    NodeNameId nodeNameId = NodeNameId::Invalid;
    ResNameId resNameId = ResNameId::Invalid;

    dag::Vector<DependencyId> deps = {};

    dag::Vector<eastl::pair<NodeNameId, NodeNameId>> fromToNameIds = {};

    bool history = false;

    bool nodeValid() const { return nodeId != NodeId::Invalid; }
    bool resValid() const { return resourceId != ResourceId::Invalid; }
  } popupState;

  void setPopupState(const HoverState &hover_state)
  {
    popupState.nodeId = hover_state.node;
    popupState.resourceId = hover_state.resource;
    popupState.deps = hover_state.deps;
    popupState.history = hover_state.historyEdge;

    popupState.nodeNameId = getNameId(hover_state.node);
    popupState.resNameId = getNameId(hover_state.resource);

    popupState.fromToNameIds.clear();
    for (const auto depId : hover_state.deps)
      popupState.fromToNameIds.push_back(
        eastl::pair<NodeNameId, NodeNameId>{getNameId(registryDependencies[depId].from), getNameId(registryDependencies[depId].to)});
  }

  void resolvePopupState()
  {
    popupState.nodeId = getRepresentId(popupState.nodeNameId);
    popupState.resourceId = getRepresentId(popupState.resNameId);

    popupState.deps.clear();
    if (popupState.resNameId != ResNameId::Invalid && !popupState.fromToNameIds.empty())
      for (auto &&[depId, dep] : registryDependencies.enumerate())
        if (!dep.disabled && (dep.type == DependencyType::IMPLICIT_RES_HIST) == popupState.history &&
            popupState.resNameId == getNameId(dep.resource) &&
            eastl::find(popupState.fromToNameIds.begin(), popupState.fromToNameIds.end(),
              eastl::pair<NodeNameId, NodeNameId>{getNameId(dep.from), getNameId(dep.to)}) != popupState.fromToNameIds.end())
          popupState.deps.push_back(depId);
  }


  // focusing
private:
  struct FocusState
  {
    NodeId nodeId = NodeId::Invalid;
    ResourceId resId = ResourceId::Invalid;

    NodeNameId nodeNameId = NodeNameId::Invalid;
    ResNameId resNameId = ResNameId::Invalid;

    ResourceFocusType focusType = ResourceFocusType::All;
    bool hasRenames = false;

    bool nodeValid() const { return nodeId != NodeId::Invalid; }
    bool resValid() const { return resId != ResourceId::Invalid; }
  } focusState;

  eastl::string nodeSearchInput;
  int focusedNodeIndex = UNKNOWN_INDEX;

  eastl::string resourceSearchInput;
  int focusedResourceIndex = UNKNOWN_INDEX;

  void clearFocus()
  {
    focusState = {};
    nodeSearchInput.clear();
    focusedNodeIndex = UNKNOWN_INDEX;
    resourceSearchInput.clear();
    focusedResourceIndex = UNKNOWN_INDEX;
  }

  void setFocus(NodeNameId id)
  {
    clearFocus();

    focusState = {
      .nodeId = getRepresentId(id),
      .resId = ResourceId::Invalid,

      .nodeNameId = id,
      .resNameId = ResNameId::Invalid,

      .focusType = ResourceFocusType::All,
      .hasRenames = false,
    };

    if (auto it = eastl::find(nsNodeNameIds.begin(), nsNodeNameIds.end(), id); it != nsNodeNameIds.end())
    {
      focusedNodeIndex = int(it - nsNodeNameIds.begin());
      nodeSearchInput = nsNodeNames[focusedNodeIndex];
    }
  }

  void setFocus(ResNameId id, ResourceFocusType focus_type = ResourceFocusType::All)
  {
    clearFocus();

    focusState = {
      .nodeId = NodeId::Invalid,
      .resId = getRepresentId(id),

      .nodeNameId = NodeNameId::Invalid,
      .resNameId = id,

      .focusType = focus_type,
      .hasRenames = false,
    };

    if (auto it = eastl::find(nsResNameIds.begin(), nsResNameIds.end(), id); it != nsResNameIds.end())
    {
      focusedResourceIndex = int(it - nsResNameIds.begin());
      resourceSearchInput = nsResNames[focusedResourceIndex];
    }

    if (focusState.resValid())
      for (const auto [from, to] : depData.renamingChains.enumerate())
        if ((from != id && to == id) || (from == id && to != id))
        {
          focusState.hasRenames = true;
          break;
        }
  }

  void setFocus(NodeId id) { setFocus(getNameId(id)); }

  void setFocus(ResourceId id, ResourceFocusType focus_type = ResourceFocusType::All) { setFocus(getNameId(id), focus_type); }

  void resolveFocusState()
  {
    if (focusState.nodeNameId != NodeNameId::Invalid)
      setFocus(focusState.nodeNameId);
    if (focusState.resNameId != ResNameId::Invalid)
      setFocus(focusState.resNameId, focusState.focusType);
  }

  void centerOnFocus();


  // inspecting
private:
  struct InspectedDependency
  {
    NodeNameId fromNameId = NodeNameId::Invalid;
    NodeNameId toNameId = NodeNameId::Invalid;
    ResNameId resNameId = ResNameId::Invalid;

    DependencyId depId = DependencyId::Invalid;
    bool history = false;
    bool wasChanged = false;
  } inspectedDependency;

  void setInspectedDependency(const DependencyId dep_id)
  {
    const auto &dep = registryDependencies[dep_id];
    inspectedDependency = {
      .fromNameId = getNameId(dep.from),
      .toNameId = getNameId(dep.to),
      .resNameId = getNameId(dep.resource),
      .depId = dep_id,
      .history = dep.type == DependencyType::IMPLICIT_RES_HIST,
      .wasChanged = true,
    };
  }

  void clearInspectedDependency()
  {
    inspectedDependency = {
      .wasChanged = true,
    };
  }

  void resolveInspectedDependency()
  {
    for (auto &&[depId, dep] : registryDependencies.enumerate())
      if (!dep.disabled && inspectedDependency.resNameId == getNameId(dep.resource) &&
          inspectedDependency.fromNameId == getNameId(dep.from) && inspectedDependency.toNameId == getNameId(dep.to) &&
          (dep.type == DependencyType::IMPLICIT_RES_HIST) == inspectedDependency.history)
      {
        inspectedDependency.depId = depId;
        inspectedDependency.wasChanged = true;
        return;
      }

    inspectedDependency = {
      .wasChanged = true,
    };
  }

  struct BlobInstance
  {
    DependencyId depId = DependencyId::Invalid;
    dafg::ResourceSubtypeTag tag = dafg::ResourceSubtypeTag::Invalid;
    bool operator==(const BlobInstance &other) const = default;
  };
  struct InspectedBlob
  {
    BlobInstance inst = {};
    BlobInstance storedInst = {};
    dag::Vector<uint8_t> buffer;
    bool dataStored = false;

    ~InspectedBlob();

    inline void *set(const DependencyId dep_id, const dafg::ResourceSubtypeTag new_tag)
    {
      inst = BlobInstance{dep_id, new_tag};

      if (storedInst == inst && dataStored)
        return buffer.data();
      else
        return nullptr;
    }
    inline void reset() { set(DependencyId::Invalid, dafg::ResourceSubtypeTag::Invalid); }
  } inspectedBlob;


  // gpu capture
private:
  GpuCapture gpuCapture;
  void setCaptureBoundary(CaptureBoundary &boundary, NodeNameId id);


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
           !userResources[res_id].hidden &&
             (!focusState.resValid() || focusState.focusType == ResourceFocusType::All ||
               focusState.focusType == ResourceFocusType::Resource && res_id == focusState.resId ||
               focusState.focusType == ResourceFocusType::ResourceAndRenames &&
                 depData.renamingRepresentatives[getNameId(res_id)] == depData.renamingRepresentatives[getNameId(focusState.resId)]);
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