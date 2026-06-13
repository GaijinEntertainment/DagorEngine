// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/visualization/structuresIntermediate.h>

#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>
#include <backend/nodeStateDeltas.h>

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <imgui-node-editor/imgui_canvas.h>


namespace dafg::visualization::irgraph
{

class Visualizer
{
public:
  Visualizer(const InternalRegistry &int_registry, const intermediate::Graph &ir_graph, const PassColoring &coloring,
    const sd::NodeStateDeltas &state_deltas);

  void draw();
  void updateVisualization();


private:
  // draw functions
  void drawUI();
  void drawCanvas();

  void drawNodes(ImDrawList *draw_list, const CanvasLayout &layout);
  void drawResources(ImDrawList *draw_list, const CanvasLayout &layout);
  void drawOrderings(ImDrawList *draw_list, const CanvasLayout &layout);
  void drawUsages(ImDrawList *draw_list, const CanvasLayout &layout);

  // control functions
  void checkHovering(const CanvasLayout &layout);
  void processInput();
  void processPopup();

  // update functions
  void updateNodes();
  void updateResourses();
  void updateConnections();

  void performLayout();
  void placeNodesByPasses(CanvasLayout &layout);
  void placeResourcesByLines(CanvasLayout &layout);
  void placeOrderings(CanvasLayout &layout);
  void placeUsages(CanvasLayout &layout);


  // data sources
  const InternalRegistry &registry;
  const intermediate::Graph &intermediateGraph;
  const PassColoring &passColoring;
  const sd::NodeStateDeltas &stateDeltas;


  // gathered data
  IdIndexedMapping<NodeId, Node> nodes;
  IdIndexedMapping<ResourceId, Resource> resources;
  IdIndexedMapping<intermediate::NodeIndex, NodeId> irNodeRepresent;
  IdIndexedMapping<intermediate::ResourceIndex, ResourceId> irResRepresent;
  inline intermediate::NodeIndex irIndexByNodeId(const NodeId node_id) const { return nodes[node_id].irIndex; };
  inline intermediate::ResourceIndex irIndexByResId(const ResourceId resource_id) const { return resources[resource_id].irIndex; }

  IdIndexedMapping<OrderingId, Ordering> orderings;
  IdIndexedMapping<UsageId, Usage> usages;


  // canvas
  ImGuiEx::Canvas canvas;
  enum CanvasChannels
  {
    SUSPEND = 0,
    ORDERINGS,
    USAGES,
    NODES,
    RESOURCES = NODES,
    TEXTS,
    COUNT
  };

  CanvasCamera canvasCamera{};

  CanvasLayout generalLayout;
  bool updateNeeded = true;


  // hovering
  struct
  {
    bool window = false;
    bool searchBox = false;
    bool canvas = false;

    NodeId node = NodeId::Invalid;
    ResourceId resource = ResourceId::Invalid;

    String tooltip;

    void reset()
    {
      window = false;
      searchBox = false;
      canvas = false;

      node = NodeId::Invalid;
      resource = ResourceId::Invalid;

      tooltip.clear();
    }
  } hoverState;
  ResourceId popupResource = ResourceId::Invalid;


  // focusing
  const struct
  {
    mutable NodeId node = NodeId::Invalid;
    mutable ResourceId resource = ResourceId::Invalid;


    inline bool nodeValid() const { return node != NodeId::Invalid; }
    inline bool resValid() const { return resource != ResourceId::Invalid; }

    inline void set(NodeId id) const
    {
      node = id;
      resource = ResourceId::Invalid;
    }
    inline void set(ResourceId id) const
    {
      node = NodeId::Invalid;
      resource = id;
    }
    inline void reset() const
    {
      node = NodeId::Invalid;
      resource = ResourceId::Invalid;
    };
  } focusState;
};

} // namespace dafg::visualization::irgraph