// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dafgVisualizerStructuresIntermediate.h>

#include <backend/intermediateRepresentation.h>
#include <backend/passColoring.h>

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <imgui-node-editor/imgui_canvas.h>


namespace dafg::visualization
{

class IRGraphVisualizer
{
public:
  IRGraphVisualizer(const intermediate::Graph &irGraph, const PassColoring &coloring);

  void draw();

  void drawCanvas(ImDrawList *drawList);

  void drawEdge(ImDrawList *drawList, const EdgeId id, const eastl::pair<ImVec2, ImVec2> fromTo);
  void drawNode(ImDrawList *drawList, const NodeId id, const ImVec2 gridPosition);
  void drawResource(ImDrawList *drawList, const ResId id, const eastl::pair<ImVec2, ImVec2> gridBounds);

  void drawTextnOnNode(const NodeId id, const ImVec2 gridPosition);
  void drawTextnOnResource(const ResId id, const eastl::pair<ImVec2, ImVec2> gridBounds);

  void checkHovering();
  void generateHoverMsg(ElementID elementId);
  void generatePopupMsg(ElementID elementId);
  void processInfoMsg();

  void processInput();


  void updateVisualization();

  void clearData();

  void updateResourses();
  void updateNodes();
  void updateEdges();


  void updateWholeGraphView();
  void updateFocusGraphView();

  void generateAllElementsSet(GraphView &graphView);
  void generateFocusedSet(GraphView &graphView);

  void placeWithGeneralLayout(GraphView &graphView);
  void placeWithFocusedLayout(GraphView &graphView);
  void placeResourcesByLines(GraphView &graphView);
  void updateEdgesLayout(GraphView &graphView);


  void updateVisibilityFlags();

  void resetFocusElement();
  void setFocusElement(ElementID id);


private:
  const intermediate::Graph &intermediateGraph;
  const PassColoring &passColoring;

  IdIndexedMapping<NodeId, IntermediateNode> nodes;
  IdIndexedMapping<ResId, IntermediateResource> resources;
  IdIndexedMapping<EdgeId, IntermediateEdge> edges;

  IdIndexedMapping<intermediate::NodeIndex, NodeId> irNodeIndexToVisNodeId;
  IdIndexedMapping<intermediate::ResourceIndex, ResId> irResIndexToVisResId;

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

  ElementID focusedElementId;
  ElementID nextFocusedElementId;
  bool focusOnly = false;
  bool compactView = false;
};

} // namespace dafg::visualization