// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/visualization/structuresIntermediate.h>

#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <imgui-node-editor/imgui_canvas.h>


namespace dafg::visualization::irgraph
{

class Visualizer
{
public:
  Visualizer(const intermediate::Graph &ir_graph, const PassColoring &coloring);

  void draw();
  void updateVisualization();


private:
  // draw functions
  void drawUI();
  void drawCanvas(ImDrawList *draw_list);

  void drawEdge(ImDrawList *draw_list, const EdgeId id, const eastl::pair<ImVec2, ImVec2> from_to);
  void drawNode(ImDrawList *draw_list, const NodeId id, const ImVec2 grid_position);
  void drawResource(ImDrawList *draw_list, const ResId id, const eastl::pair<ImVec2, ImVec2> grid_bounds);

  void drawTextnOnNode(const NodeId id, const ImVec2 grid_position);
  void drawTextnOnResource(const ResId id, const eastl::pair<ImVec2, ImVec2> grid_bounds);

  // control functions
  void checkHovering();
  void generateHoverMsg(ElementID element_id);
  void generatePopupMsg(ElementID element_id);
  void processInfoMsg();

  void processInput();

  // update functions
  void clearData();

  void updateResourses();
  void updateNodes();
  void updateEdges();


  void updateWholeGraphView();
  void updateFocusGraphView();

  void generateAllElementsSet(GraphView &graph_view);
  void generateFocusedSet(GraphView &graph_view);

  void placeWithGeneralLayout(GraphView &graph_view);
  void placeWithFocusedLayout(GraphView &graph_view);
  void placeResourcesByLines(GraphView &graph_view);
  void updateEdgesLayout(GraphView &graph_view);


  // data sources
  const intermediate::Graph &intermediateGraph;
  const PassColoring &passColoring;


  // gathered data
  IdIndexedMapping<NodeId, IntermediateNode> nodes;
  IdIndexedMapping<ResId, IntermediateResource> resources;
  IdIndexedMapping<EdgeId, IntermediateEdge> edges;

  IdIndexedMapping<intermediate::NodeIndex, NodeId> irNodeIndexToVisNodeId;
  IdIndexedMapping<intermediate::ResourceIndex, ResId> irResIndexToVisResId;


  // canvas
  ImGuiEx::Canvas canvas;

  GraphView wholeGraphView{{
    .initCanvasScaleId = 7,
    .canvasScales = {0.01f, 0.025f, 0.05f, 0.1f, 0.25f, 0.5f, 0.75f, 1.0f, 1.25f, 1.50f},
  }};

  GraphView focusedGraphView{{
    .initCanvasScaleId = 4,
    .canvasScales = {0.05f, 0.1f, 0.5f, 0.75f, 1.0f, 1.25f, 1.50f, 2.0f, 2.5f, 3.0f},
  }};

  String hoverMessage;
  String popupMessage;


  // focusing
  ElementID focusedElementId;
  ElementID nextFocusedElementId;
  bool focusOnly = false;
  bool compactView = false;

  void updateVisibilityFlags();

  void resetFocusElement();
  void setFocusElement(ElementID id);
};

} // namespace dafg::visualization::irgraph