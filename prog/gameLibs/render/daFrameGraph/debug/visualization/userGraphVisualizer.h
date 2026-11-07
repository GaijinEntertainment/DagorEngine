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
  void drawExplorerPanel();
  void drawCanvasPanel();

  void drawSearchBar();
  void drawNameSpaceTree(NameSpaceNameId namespace_id);

  void drawControls();
  void drawCanvas();

  void drawNodes(ImDrawList *draw_list, const CanvasLayout &layout);
  void drawEdges(ImDrawList *draw_list, const CanvasLayout &layout);
  void drawNodeBoxes(ImDrawList *draw_list);

  void drawGpuCaptureWindow();

  // tree view functions
private:
  void hideResource(ResNameId res_name_id);
  void showResource(ResNameId res_name_id);
  void hideResourcesInSubTree(NameSpaceNameId namespace_id);
  void hideResourcesInNameSpace(NameSpaceNameId namespace_id);
  void showResourcesInSubTree(NameSpaceNameId namespace_id);
  void showResourcesInNameSpace(NameSpaceNameId namespace_id);

  // control functions
private:
  void checkHovering();
  void processInput();
  void processPopup();
  void processTextureDebug();

  // update functions
private:
  void updateNodesRess();
  void updateSearchItems();
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

  IdIndexedMapping<NameSpaceNameId, NameSpace> nameSpaces; // Information about sub namespaces, nodes and resources for tree view

  IdIndexedMapping<DependencyId, Dependency> registryDependencies; // List of all dependencies, stated in InternalRegistry
  dag::Vector<DependencyId> disabledDependencies;                  // Dependencies, disabled during check for cycles

  IdIndexedMapping<NodeBoxId, NodeBox> nodeBoxes;
  IdIndexedMapping<NodeId, NodeBoxId> nodesNodeBox;

  bool updateNeeded = true;


  // layouts
private:
  eastl::unique_ptr<IGraphLayouter> layouter = make_default_layouter();
  CanvasLayout generalLayout;   // Non-hierarchical layout, nodes are placed individually
  CanvasLayout condensedLayout; // Hierarchical layout, nodes are grouped by name spaces


  // view
private:
  NodeColorationType nodeColoration = NodeColorationType::None;
  bool hierarchicalView = false;
  bool smoothCamera = true;
  bool canvasGrid = true;
  bool macroView = false;


  // canvas
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


  // search
private:
  struct SearchItem
  {
    NodeId nodeId = NodeId::Invalid;
    ResourceId resId = ResourceId::Invalid;
    float rating = 0;

    eastl::string_view shortName = {};
    eastl::string_view fullName = {};
    eastl::string_view parentName = {};

    dag::Vector<eastl::pair<uint16_t, uint16_t>> shortNameMatches = {};
    dag::Vector<eastl::pair<uint16_t, uint16_t>> fullNameMatches = {};

    bool isNode() const { return nodeId != NodeId::Invalid; }
  };
  struct SearchState
  {
    eastl::string input;
    eastl::string lastRatedInput;

    dag::Vector<SearchItem> searchItems;
    dag::Vector<uint32_t> sortedIndices;
    int selectedIndex = 0;

    bool needRating = false;

    void reset(const uint32_t search_items_count)
    {
      searchItems.clear();
      sortedIndices.clear();

      searchItems.reserve(search_items_count);
      sortedIndices.reserve(search_items_count);

      needRating = true;
    }

    void rateSearch()
    {
      needRating |= lastRatedInput != input;
      if (eastl::exchange(needRating, false))
      {
        lastRatedInput = input;
        sortedIndices.clear();
        selectedIndex = 0;

        for (uint32_t itemIdx = 0; itemIdx < searchItems.size(); ++itemIdx)
        {
          auto &item = searchItems[itemIdx];
          item.rating = fuzzy_search(input, item.shortName, item.fullName, item.shortNameMatches, item.fullNameMatches);
          if (input.empty() || item.rating > 0.f)
            sortedIndices.push_back(itemIdx);
        }

        if (!input.empty())
          eastl::sort(sortedIndices.begin(), sortedIndices.end(),
            [this](uint32_t a, uint32_t b) { return searchItems[a].rating > searchItems[b].rating; });
      }
    };
  } searchState;


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

  void clearFocus() { focusState = {}; }

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

  void centerOnFocus()
  {
    if (focusState.nodeValid())
    {
      const auto nodeRect = (hierarchicalView ? condensedLayout : generalLayout).nodes[focusState.nodeId];
      canvasCamera.targetCamera(nodeRect.offset + nodeRect.size / 2.f);
    }

    if (focusState.resValid())
    {
      constexpr ImVec2 MAX_IMVEC2 = ImVec2{eastl::numeric_limits<float>::max(), eastl::numeric_limits<float>::max()};

      ImVec2 leftPos = MAX_IMVEC2;
      ImVec2 rightPos = MAX_IMVEC2;
      for (const auto &edge : (hierarchicalView ? condensedLayout : generalLayout).edges.values())
        for (const auto depId : edge.carriedDeps)
          if (registryDependencies[depId].resource == focusState.resId)
            for (const auto &spline : edge.splines)
              if (spline.P0.x < leftPos.x)
              {
                leftPos = spline.P0;
                rightPos = spline.P3;
                break;
              }

      if (leftPos == MAX_IMVEC2 || rightPos == MAX_IMVEC2)
        return;

      canvasCamera.targetCamera((leftPos + rightPos) / 2.f);
    }
  }


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

  void clearInspectedDependency()
  {
    inspectedDependency = {
      .wasChanged = true,
    };
  }

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
  dag::Vector<NodeNameId> nsNodeNameIds;
  dag::Vector<eastl::string_view> nsNodeNames;
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

  inline bool isResourceVisible(const ResourceId res_id) const
  {
    return res_id == ResourceId::Invalid ||
           !userResources[res_id].hidden &&
             (!focusState.resValid() || focusState.focusType == ResourceFocusType::All ||
               focusState.focusType == ResourceFocusType::Resource && res_id == focusState.resId ||
               focusState.focusType == ResourceFocusType::ResourceAndRenames &&
                 depData.renamingRepresentatives[getNameId(res_id)] == depData.renamingRepresentatives[getNameId(focusState.resId)]);
  }
};

} // namespace dafg::visualization::usergraph