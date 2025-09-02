// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "irGraphVisualizer.h"

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <imgui-node-editor/imgui_canvas.h>
#include <memory/dag_framemem.h>
#include <util/dag_convar.h>


constexpr float GRID_HORISONTAL_SIZE = 400.f;
constexpr float GRID_YX_RATIO = 0.5f;
constexpr float TEXT_FRACTION_PADDING = 0.1f;
constexpr float BORDER_FRACTION_THICKNESS = 0.01f;

constexpr ImVec2 GRID_SIZE = {GRID_HORISONTAL_SIZE, GRID_HORISONTAL_SIZE *GRID_YX_RATIO};
constexpr ImVec2 TEXT_PADDING = {TEXT_FRACTION_PADDING, TEXT_FRACTION_PADDING / GRID_YX_RATIO};
constexpr ImVec2 BORDER_PADDING = {BORDER_FRACTION_THICKNESS, BORDER_FRACTION_THICKNESS / GRID_YX_RATIO};
static ImVec2 gridToVec(ImVec2 gridCoords) { return gridCoords * GRID_SIZE; }

constexpr float NODE_VERT_OFFSET = 3.f;
constexpr float PASS_HOR_OFFSET = 2.f;

constexpr float RES_LINE_START = -4.f;
constexpr float RES_LINE_OFFSET = -3.f;


namespace dafg
{

extern ConVarT<bool, false> recompile_graph;

namespace visualization
{

IRGraphVisualizer::IRGraphVisualizer(const intermediate::Graph &irGraph, const PassColoring &coloring) :
  intermediateGraph{irGraph}, passColoring(coloring)
{
  REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP_FG2, IMGUI_IRG_WIN_NAME, [&]() { this->draw(); });
}

void IRGraphVisualizer::draw()
{
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  // drawing ui

  // first line
  {
    const bool focusedOnNode = eastl::holds_alternative<NodeId>(focusedElementId);
    const NodeId focusedNodeId = focusedOnNode ? eastl::get<NodeId>(focusedElementId) : NodeId::INVALID;

    if (ImGui::BeginCombo("##node_search", focusedOnNode ? nodes[focusedNodeId].name.c_str() : "Node Search"))
    {
      for (auto [nodeId, node] : nodes.enumerate())
      {
        bool selected = nodeId == focusedNodeId;
        if (ImGui::Selectable(node.name.c_str(), selected))
          setFocusElement(nodeId);
        if (selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
  }

  // second line
  {
    const bool focusedOnResource = eastl::holds_alternative<ResId>(focusedElementId);
    const ResId focusedResourceId = focusedOnResource ? eastl::get<ResId>(focusedElementId) : ResId::INVALID;

    if (ImGui::BeginCombo("##resource_search", focusedOnResource ? resources[focusedResourceId].name.c_str() : "Resource Search"))
    {
      for (auto [resId, resource] : resources.enumerate())
      {
        bool selected = resId == focusedResourceId;
        if (ImGui::Selectable(resource.name.c_str(), selected))
          setFocusElement(resId);
        if (selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
  }

  // third line
  {
    if (ImGui::Button("Recompile"))
    {
      wholeGraphView.resetView();
      canvas.SetView(wholeGraphView.getOffset(), wholeGraphView.getZoom());

      recompile_graph.set(true);
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset view"))
    {
      GraphView &curGraphView = compactView ? focusedGraphView : wholeGraphView;

      curGraphView.resetView();
      canvas.SetView(curGraphView.getOffset(), curGraphView.getZoom());
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset focus"))
    {
      focusOnly = false;
      compactView = false;

      canvas.SetView(wholeGraphView.getOffset(), wholeGraphView.getZoom());

      resetFocusElement();
    }

    ImGui::SameLine();
    if (ImGui::Checkbox("Focus Only", &focusOnly))
    {
      updateVisibilityFlags();
    }

    ImGui::SameLine();
    if (ImGui::Checkbox("Compact View", &compactView))
    {
      GraphView &curGraphView = compactView ? focusedGraphView : wholeGraphView;

      canvas.SetView(curGraphView.getOffset(), curGraphView.getZoom());
    }
  }

  // forth line
  {
    ImGui::TextUnformatted("double left click on element to set focus");
  }

  // drawing canvas
  drawCanvas(drawList);

  // tooltip and popup
  processInfoMsg();

  // zoom and pan
  processInput();
}

void IRGraphVisualizer::drawCanvas(ImDrawList *drawList)
{
  if (!canvas.Begin("##scrolling_region", ImVec2(0, 0)))
    return;

  const GraphView &curGraphView = compactView ? focusedGraphView : wholeGraphView;
  const GraphSubset &graphSubset = curGraphView.elementsSet;
  const SetLayout &graphLayout = curGraphView.elementsLayout;

  // Layer 0 for edges, 1 for nodes, 2 for text
  drawList->ChannelsSplit(3);

  // drawing ...
  {
    drawList->ChannelsSetCurrent(0);

    // ... edges
    for (const EdgeId edgeId : graphSubset.edges)
      drawEdge(drawList, edgeId, graphLayout.edgesFrTo[edgeId]);

    drawList->ChannelsSetCurrent(1);

    // ... nodes
    for (const NodeId nodeId : graphSubset.nodes)
      drawNode(drawList, nodeId, graphLayout.nodesGridCoords[nodeId]);

    // ... and resourses
    for (const ResId resId : graphSubset.resources)
      drawResource(drawList, resId, graphLayout.resGridBounds[resId]);
  }

  // drawing text ...
  {
    drawList->ChannelsSetCurrent(2);

    // ... on nodes
    for (const NodeId nodeId : graphSubset.nodes)
      drawTextnOnNode(nodeId, graphLayout.nodesGridCoords[nodeId]);

    // ... on resourses
    for (const ResId resId : graphSubset.resources)
      drawTextnOnResource(resId, graphLayout.resGridBounds[resId]);
  }

  // canvas.Suspend() can only be called with channel 0
  // and switching it during drawing elements kinda odd
  drawList->ChannelsSetCurrent(0);
  checkHovering();

  drawList->ChannelsMerge();
  canvas.End();
}

void IRGraphVisualizer::drawEdge(ImDrawList *drawList, const EdgeId id, const eastl::pair<ImVec2, ImVec2> fromTo)
{
  const auto &edge = edges[id];

  if (edge.isVisible)
  {
    const ImVec2 edgeStart = gridToVec(fromTo.first);
    const ImVec2 edgeEnd = gridToVec(fromTo.second);
    const int focusedColor = edge.focused ? 192 : 127;
    const ImColor edgeColor = {focusedColor, 255, focusedColor, edge.outOfFocus ? 63 : 255};

    drawList->AddLine(edgeStart, edgeEnd, edgeColor);
  }
}

void IRGraphVisualizer::drawNode(ImDrawList *drawList, const NodeId id, const ImVec2 gridPosition)
{
  const auto &node = nodes[id];

  if (node.isVisible)
  {
    const ImVec2 nodeStart = gridToVec(gridPosition);
    const ImVec2 nodeEnd = gridToVec(gridPosition + ImVec2(1.f, 1.f));
    const ImColor nodeColor = {255, 127, 127, node.outOfFocus ? 63 : 255};

    if (node.focused)
    {
      const ImVec2 borderStart = gridToVec(gridPosition - BORDER_PADDING);
      const ImVec2 borderEnd = gridToVec(gridPosition + ImVec2(1.f, 1.f) + BORDER_PADDING);

      drawList->AddRectFilled(borderStart, borderEnd, IM_COL32_WHITE);
    }

    drawList->AddRectFilled(nodeStart, nodeEnd, nodeColor);
  }
}

void IRGraphVisualizer::drawResource(ImDrawList *drawList, const ResId id, const eastl::pair<ImVec2, ImVec2> gridBounds)
{
  const auto resource = resources[id];

  if (resource.isVisible)
  {
    const ImVec2 resourceStart = gridToVec(gridBounds.first);
    const ImVec2 resourceEnd = gridToVec(gridBounds.second + ImVec2(1.f, 1.f));
    const ImColor resourceColor = {127, 127, 255, resource.outOfFocus ? 63 : 255};

    if (resource.focused)
    {
      const ImVec2 borderStart = gridToVec(gridBounds.first - BORDER_PADDING);
      const ImVec2 borderEnd = gridToVec(gridBounds.second + ImVec2(1.f, 1.f) + BORDER_PADDING);

      drawList->AddRectFilled(borderStart, borderEnd, IM_COL32_WHITE);
    }

    drawList->AddRectFilled(resourceStart, resourceEnd, resourceColor);
  }
}

void IRGraphVisualizer::drawTextnOnNode(const NodeId id, const ImVec2 gridPosition)
{
  const auto &node = nodes[id];

  if (node.isVisible)
  {
    ImGui::SetCursorScreenPos(gridToVec(gridPosition + TEXT_PADDING));
    ImGui::TextUnformatted(node.name.c_str());
    ImGui::SetCursorScreenPos(gridToVec(gridPosition + ImVec2(1.0f, 1.0f) - TEXT_PADDING));
    ImGui::Text("x%u", node.multiplexingCount);
  }
}

void IRGraphVisualizer::drawTextnOnResource(const ResId id, const eastl::pair<ImVec2, ImVec2> gridBounds)
{
  const auto &resource = resources[id];

  if (resource.isVisible)
  {
    const auto [startPos, _] = gridBounds;

    ImGui::SetCursorScreenPos(gridToVec(startPos + TEXT_PADDING));
    ImGui::TextUnformatted(resource.name.c_str());
    ImGui::SetCursorScreenPos(gridToVec(startPos + ImVec2(1.0f, 1.0f) - TEXT_PADDING));
    ImGui::Text("x%u", resource.multiplexingCount);
  }
}

void IRGraphVisualizer::checkHovering()
{
  const GraphView &curGraphView = compactView ? focusedGraphView : wholeGraphView;
  const GraphSubset &graphSubset = curGraphView.elementsSet;
  const SetLayout &graphLayout = curGraphView.elementsLayout;

  const ImVec2 mouseGridPosition = ImGui::GetIO().MousePos / GRID_SIZE;

  const bool doubleLeftClick = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
  const bool rightClick = ImGui::IsMouseDown(ImGuiMouseButton_Right);

  hoverMessage.clear();

  // ... for nodes
  for (const NodeId nodeId : graphSubset.nodes)
  {
    const ImVec2 position = graphLayout.nodesGridCoords[nodeId];

    if (ImRect{position, position + ImVec2(1.f, 1.f)}.Contains(mouseGridPosition))
    {
      generateHoverMsg(nodeId);

      if (doubleLeftClick)
        nextFocusedElementId = nodeId;

      if (rightClick)
      {
        generatePopupMsg(nodeId);

        canvas.Suspend();
        ImGui::OpenPopup("element_info");
        canvas.Resume();
      }
    }
  }

  // ... for resourses
  for (const ResId resId : graphSubset.resources)
  {
    const auto [start, end] = graphLayout.resGridBounds[resId];

    if (ImRect{start, end + ImVec2(1.f, 1.f)}.Contains(mouseGridPosition))
    {
      generateHoverMsg(resId);

      if (doubleLeftClick)
        nextFocusedElementId = resId;

      if (rightClick)
      {
        generatePopupMsg(resId);

        canvas.Suspend();
        ImGui::OpenPopup("element_info");
        canvas.Resume();
      }
    }
  }

  if (doubleLeftClick)
  {
    if (eastl::holds_alternative<eastl::monostate>(nextFocusedElementId))
      resetFocusElement();
    else
    {
      setFocusElement(nextFocusedElementId);
      nextFocusedElementId = {};
    }
  }
}

void IRGraphVisualizer::generateHoverMsg(ElementID id)
{
  if (eastl::holds_alternative<NodeId>(id))
  {
    const NodeId nodeId = eastl::get<NodeId>(id);
    const auto &node = nodes[nodeId];

    hoverMessage = "Node\n\n";
    hoverMessage += "Name: " + String(node.name.c_str()) + "\n";
    hoverMessage += "Multiplexing: x\n";
  }
  else if (eastl::holds_alternative<ResId>(id))
  {
    const ResId resId = eastl::get<ResId>(id);
    const auto &resource = resources[resId];

    hoverMessage = "Resource\n\n";
    hoverMessage += "Name: " + String(resource.name.c_str()) + "\n";
    hoverMessage += "Multiplexing: x\n";
  }

  hoverMessage += "\nright click for more...";
}

void IRGraphVisualizer::generatePopupMsg(ElementID id)
{
  if (eastl::holds_alternative<NodeId>(id))
  {
    // const NodeId nodeId = eastl::get<NodeId>(id);
    // const auto &node = nodes[nodeId];

    popupMessage = "Node Info:\n\n";
  }
  else if (eastl::holds_alternative<ResId>(id))
  {
    // const ResId resId = eastl::get<ResId>(id);
    // const auto &resource = resources[resId];

    popupMessage = "Resource Info:\n\n";
  }
}

void IRGraphVisualizer::processInfoMsg()
{
  if (ImGui::BeginPopup("element_info"))
  {
    ImGui::TextUnformatted(popupMessage);
    ImGui::EndPopup();
  }
  else if (!hoverMessage.empty())
  {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(hoverMessage);
    ImGui::EndTooltip();
  }
}

void IRGraphVisualizer::processInput()
{
  GraphView &curGraphView = compactView ? focusedGraphView : wholeGraphView;

  const bool canvasHovered = canvas.ViewRect().Contains(canvas.ToLocal(ImGui::GetIO().MousePos));
  if (ImGui::IsWindowHovered() && canvasHovered)
  {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
    {
      curGraphView.getOffset() += ImGui::GetIO().MouseDelta;
      canvas.SetView(curGraphView.getOffset(), curGraphView.getZoom());
    }
    if (abs(ImGui::GetIO().MouseWheel) > 0.1)
    {
      ImGui::GetIO().MouseWheel > 0 ? curGraphView.zoomIn() : curGraphView.zoomOut();

      ImVec2 oldPos = canvas.ToLocal(ImGui::GetIO().MousePos);
      canvas.SetView(curGraphView.getOffset(), curGraphView.getZoom());
      ImVec2 newPos = canvas.ToLocal(ImGui::GetIO().MousePos);

      curGraphView.getOffset() += canvas.FromLocalV(newPos - oldPos);
      canvas.SetView(curGraphView.getOffset(), curGraphView.getZoom());
    }
  }
}


void IRGraphVisualizer::updateVisualization()
{
  clearData();

  updateResourses();
  updateNodes();
  updateEdges();

  updateWholeGraphView();
}


void IRGraphVisualizer::clearData()
{
  nodes.clear();
  resources.clear();
  edges.clear();
  irNodeIndexToVisNodeId.clear();
  irResIndexToVisResId.clear();

  focusOnly = false;
  compactView = false;
  resetFocusElement();
}

void IRGraphVisualizer::updateResourses()
{
  for (const auto [irResourceIndex, irResource] : intermediateGraph.resources.enumerate())
    if (irResource.multiplexingIndex == 0)
    {
      auto [newResId, newResourse] = resources.appendNew();
      irResIndexToVisResId.set(irResourceIndex, newResId);

      newResourse.name = intermediateGraph.resourceNames[irResourceIndex].c_str();
      newResourse.frontendResources = irResource.frontendResources;
      newResourse.intermediateResource = irResourceIndex;
      newResourse.multiplexingCount = 1;
    }
}

void IRGraphVisualizer::updateNodes()
{
  for (const auto [irNodeIndex, irNode] : intermediateGraph.nodes.enumerate())
    if (irNode.multiplexingIndex == 0)
    {
      auto [newNodeId, newNode] = nodes.appendNew();
      irNodeIndexToVisNodeId.set(irNodeIndex, newNodeId);

      newNode.name = intermediateGraph.nodeNames[irNodeIndex].c_str();
      newNode.frontendNode = irNode.frontendNode;
      newNode.intermediateNode = irNodeIndex;
      newNode.multiplexingCount = 1;

      for (const auto predIrNodeIndex : irNode.predecessors)
      {
        const NodeId predNodeId = irNodeIndexToVisNodeId[predIrNodeIndex];

        newNode.previousNodes.push_back(predNodeId);
        nodes[predNodeId].followingNodes.push_back(newNodeId);
      }

      for (const auto &request : irNode.resourceRequests)
      {
        const ResId curResId = irResIndexToVisResId[request.resource];
        IntermediateResource &curRes = resources[curResId];

        newNode.resourceUsages.push_back(curResId);
        resources[curResId].requestedBy.push_back(newNodeId);

        if (curRes.firstUsage != NodeId::INVALID)
        {
          curRes.firstUsage = newNodeId < curRes.firstUsage ? newNodeId : curRes.firstUsage;
          curRes.lastUsage = newNodeId > curRes.lastUsage ? newNodeId : curRes.lastUsage;
        }
        else
        {
          curRes.firstUsage = newNodeId;
          curRes.lastUsage = newNodeId;
        }
      }
    }
}

void IRGraphVisualizer::updateEdges()
{
  for (auto [nodeId, node] : nodes.enumerate())
  {
    for (const auto prevNodeId : node.previousNodes)
    {
      auto [newEdgeId, newEdge] = edges.appendNew();
      newEdge.fromNode = prevNodeId;
      newEdge.toNode = nodeId;
    }

    for (const auto resId : node.resourceUsages)
    {
      auto [newEdgeId, newEdge] = edges.appendNew();
      newEdge.fromNode = nodeId;
      newEdge.toRes = resId;
    }
  }
}


void IRGraphVisualizer::updateWholeGraphView()
{
  generateAllElementsSet(wholeGraphView);

  placeWithGeneralLayout(wholeGraphView);
}

void IRGraphVisualizer::updateFocusGraphView()
{
  generateFocusedSet(focusedGraphView);

  placeWithFocusedLayout(focusedGraphView);
}

void IRGraphVisualizer::generateAllElementsSet(GraphView &graphView)
{
  GraphSubset &subset = graphView.elementsSet;

  subset.reset();

  subset.nodes.reserve(nodes.size());
  subset.resources.reserve(resources.size());
  subset.edges.reserve(edges.size());

  for (auto nodeId : nodes.keys())
    subset.nodes.insert(nodeId);

  for (auto resId : resources.keys())
    subset.resources.insert(resId);

  for (auto edgeId : edges.keys())
    subset.edges.insert(edgeId);
}

void IRGraphVisualizer::generateFocusedSet(GraphView &graphView)
{
  GraphSubset &subset = graphView.elementsSet;

  subset.reset();

  // focused on node
  if (eastl::holds_alternative<NodeId>(focusedElementId))
  {
    const NodeId focusedNodeId = eastl::get<NodeId>(focusedElementId);

    subset.nodes.insert(focusedNodeId);

    // picking corresponded elements
    const IntermediateNode &node = nodes[focusedNodeId];

    for (const auto nodeId : node.previousNodes)
      subset.nodes.insert(nodeId);

    for (const auto nodeId : node.followingNodes)
      subset.nodes.insert(nodeId);

    for (const auto resId : node.resourceUsages)
      subset.resources.insert(resId);

    for (auto [edgeId, edge] : edges.enumerate())
      if (edge.fromNode == focusedNodeId || edge.toNode == focusedNodeId)
        subset.edges.insert(edgeId);
  }
  // focused on resource
  else if (eastl::holds_alternative<ResId>(focusedElementId))
  {
    const ResId focusedResourceId = eastl::get<ResId>(focusedElementId);

    subset.resources.insert(focusedResourceId);

    // picking corresponded elements
    const IntermediateResource &res = resources[focusedResourceId];

    for (const auto nodeId : res.requestedBy)
      subset.nodes.insert(nodeId);

    for (auto [edgeId, edge] : edges.enumerate())
      if (edge.toRes == focusedResourceId)
        subset.edges.insert(edgeId);
  }
}

void IRGraphVisualizer::placeWithGeneralLayout(GraphView &graphView)
{
  const GraphSubset &subset = graphView.elementsSet;
  SetLayout &layout = graphView.elementsLayout;

  // placing nodes ...
  {
    uint32_t currentPassSize = 0;
    uint32_t currentPassNumber = 0;
    PassColor previousColor = PassColor{eastl::underlying_type_t<PassColor>(-1)};

    for (const auto nodeId : subset.nodes)
    {
      // ... by passes
      auto &node = nodes[nodeId];
      const auto currentColor = passColoring[node.intermediateNode];

      if (previousColor != currentColor)
      {
        ++currentPassNumber;
        currentPassSize = 0;
        previousColor = currentColor;
      }

      node.renderPassNumber = currentPassNumber - 1;

      // ... on grid
      layout.nodesGridCoords.set(nodeId, ImVec2(node.renderPassNumber * PASS_HOR_OFFSET, currentPassSize * NODE_VERT_OFFSET));

      ++currentPassSize;
    }
  }

  // placing resources ...
  {
    // ... by lines
    placeResourcesByLines(graphView);

    // ... on grid
    for (const auto resId : subset.resources)
    {
      const auto &resource = resources[resId];
      const ImVec2 &firstNodePos = layout.nodesGridCoords[resource.firstUsage];
      const ImVec2 &lastNodePos = layout.nodesGridCoords[resource.lastUsage];

      layout.resGridBounds.set(resId, {{firstNodePos.x, RES_LINE_START + RES_LINE_OFFSET * resource.line},
                                        {lastNodePos.x, RES_LINE_START + RES_LINE_OFFSET * resource.line}});
    }
  }

  // placing edges
  updateEdgesLayout(graphView);
}

void IRGraphVisualizer::placeResourcesByLines(GraphView &graphView)
{
  /*
  we represent each resource as a segment on an integer line, where the ends are the first and last use
  (the location in memory is not taken into account yet, only in time)
  then we sort these segments and begin to lay them out along lines,
  choosing for each segment the smallest line in order where it can be placed
  so we get the layout of resources along lines without intersections and with the smallest number of lines
  */

  FRAMEMEM_VALIDATE;

  const GraphSubset &subset = graphView.elementsSet;

  struct Lifetime
  {
    size_t startPass;
    size_t endPass;
    ResId resId;
  };

  dag::Vector<Lifetime, framemem_allocator> times;
  times.reserve(subset.resources.size());
  for (const auto resId : subset.resources)
  {
    const auto resource = resources[resId];
    const IntermediateNode &firstNode = nodes[resource.firstUsage];
    const IntermediateNode &lastNode = nodes[resource.lastUsage];

    times.push_back({firstNode.renderPassNumber, lastNode.renderPassNumber, resId});
  }

  eastl::sort(times.begin(), times.end(), [](const Lifetime &a, const Lifetime &b) {
    return a.startPass == b.startPass ? a.endPass < b.endPass : a.startPass < b.startPass;
  });

  dag::Vector<uint32_t, framemem_allocator> lastTimeInLine;

  for (const Lifetime &time : times)
  {
    size_t lineToPlace = 0;

    while (lineToPlace < lastTimeInLine.size() && lastTimeInLine[lineToPlace] >= time.startPass)
      ++lineToPlace;

    if (lineToPlace >= lastTimeInLine.size())
      lastTimeInLine.push_back(0);

    resources[time.resId].line = lineToPlace;
    lastTimeInLine[lineToPlace] = time.endPass;
  }
}

void IRGraphVisualizer::placeWithFocusedLayout(GraphView &graphView)
{
  const GraphSubset &subset = graphView.elementsSet;
  SetLayout &layout = graphView.elementsLayout;

  layout.reset();

  // focused on node
  if (eastl::holds_alternative<NodeId>(focusedElementId))
  {
    const NodeId focusedNodeId = eastl::get<NodeId>(focusedElementId);
    const IntermediateNode &focusedNode = nodes[focusedNodeId];

    {
      const float prevGridMiddle = (NODE_VERT_OFFSET * (focusedNode.previousNodes.size() - 1) + 1) / 2;
      const float follGridMiddle = (NODE_VERT_OFFSET * (focusedNode.followingNodes.size() - 1) + 1) / 2;
      const float halfVerticalGridSpan = max(prevGridMiddle, follGridMiddle);
      const float prevGridOffset = halfVerticalGridSpan - prevGridMiddle;
      const float follGridOffset = halfVerticalGridSpan - follGridMiddle;

      // placing nodes
      for (size_t i = 0; i < focusedNode.previousNodes.size(); ++i)
        layout.nodesGridCoords.set(focusedNode.previousNodes[i], ImVec2(0.0f, prevGridOffset + i * NODE_VERT_OFFSET));

      layout.nodesGridCoords.set(focusedNodeId, ImVec2(PASS_HOR_OFFSET * 2, halfVerticalGridSpan - 0.5f));

      for (size_t i = 0; i < focusedNode.followingNodes.size(); ++i)
        layout.nodesGridCoords.set(focusedNode.followingNodes[i],
          ImVec2(2 * PASS_HOR_OFFSET * 2, follGridOffset + i * NODE_VERT_OFFSET));

      // placing resources
      for (size_t i = 0; i < subset.resources.size(); ++i)
        layout.resGridBounds.set(subset.resources[i],
          {{0.0, RES_LINE_START + i * RES_LINE_OFFSET}, {2 * PASS_HOR_OFFSET * 2, RES_LINE_START + i * RES_LINE_OFFSET}});
    }

    // placing edges
    updateEdgesLayout(graphView);
  }
  // focused on resource
  else if (eastl::holds_alternative<ResId>(focusedElementId))
  {
    const ResId focusedResourceId = eastl::get<ResId>(focusedElementId);
    const IntermediateResource &focusedResource = resources[focusedResourceId];

    {
      const float horGridSpan = PASS_HOR_OFFSET * (focusedResource.requestedBy.size() - 1) + 1;

      // placing resource
      layout.resGridBounds.set(focusedResourceId, {{0.0, 0.0}, {horGridSpan, 0.0}});

      // placing nodes
      for (size_t i = 0; i < focusedResource.requestedBy.size(); ++i)
        layout.nodesGridCoords.set(focusedResource.requestedBy[i], ImVec2(i * PASS_HOR_OFFSET, -RES_LINE_START));
    }

    // placing edges
    updateEdgesLayout(graphView);
  }
}

void IRGraphVisualizer::updateEdgesLayout(GraphView &graphView)
{
  const GraphSubset &subset = graphView.elementsSet;
  SetLayout &layout = graphView.elementsLayout;

  for (const auto edgeId : subset.edges)
  {
    const IntermediateEdge &edge = edges[edgeId];

    if (edge.toRes == ResId::INVALID)
    {
      const ImVec2 &fromNodePos = layout.nodesGridCoords[edge.fromNode];
      const ImVec2 &toNodePos = layout.nodesGridCoords[edge.toNode];

      layout.edgesFrTo.set(edgeId, {{fromNodePos.x + 1.0f, fromNodePos.y + 0.5f}, {toNodePos.x, toNodePos.y + 0.5f}});
    }
    else
    {
      const ImVec2 &fromNodePos = layout.nodesGridCoords[edge.fromNode];
      const ImVec2 &toResPos = layout.resGridBounds[edge.toRes].first;

      layout.edgesFrTo.set(edgeId, {{fromNodePos.x + 0.5f, fromNodePos.y}, {fromNodePos.x + 0.5f, toResPos.y + 1.0f}});
    }
  }
}


void IRGraphVisualizer::updateVisibilityFlags()
{
  const bool anyFocus = !eastl::holds_alternative<eastl::monostate>(focusedElementId);

  auto resetFocuses = [this, anyFocus](auto &array) {
    for (auto &elem : array)
    {
      elem.focused = false;
      elem.outOfFocus = anyFocus;
      elem.isVisible = !focusOnly || !anyFocus;
    }
  };

  resetFocuses(nodes);
  resetFocuses(resources);
  resetFocuses(edges);


  if (eastl::holds_alternative<NodeId>(focusedElementId))
  {
    const NodeId focusedNodeId = eastl::get<NodeId>(focusedElementId);

    for (const auto nodeId : nodes[focusedNodeId].previousNodes)
    {
      nodes[nodeId].outOfFocus = false;
      nodes[nodeId].isVisible = true;
    }
    for (const auto nodeId : nodes[focusedNodeId].followingNodes)
    {
      nodes[nodeId].outOfFocus = false;
      nodes[nodeId].isVisible = true;
    }
    for (const auto resId : nodes[focusedNodeId].resourceUsages)
    {
      resources[resId].outOfFocus = false;
      resources[resId].isVisible = true;
    }

    nodes[focusedNodeId].focused = true;
    nodes[focusedNodeId].outOfFocus = false;
    nodes[focusedNodeId].isVisible = true;

    for (auto &edge : edges)
      if (edge.fromNode == focusedNodeId || edge.toNode == focusedNodeId)
      {
        edge.focused = true;
        edge.outOfFocus = false;
        edge.isVisible = true;
      }
  }

  if (eastl::holds_alternative<ResId>(focusedElementId))
  {
    const ResId focusedResourceId = eastl::get<ResId>(focusedElementId);

    for (const auto nodeId : resources[focusedResourceId].requestedBy)
    {
      nodes[nodeId].outOfFocus = false;
      nodes[nodeId].isVisible = true;
    }

    resources[focusedResourceId].focused = true;
    resources[focusedResourceId].outOfFocus = false;
    resources[focusedResourceId].isVisible = true;

    for (auto &edge : edges)
      if (edge.toRes == focusedResourceId)
      {
        edge.focused = true;
        edge.outOfFocus = false;
        edge.isVisible = true;
      }
  }
}

void IRGraphVisualizer::resetFocusElement()
{
  focusedElementId = {};

  updateVisibilityFlags();

  updateFocusGraphView();
}


void IRGraphVisualizer::setFocusElement(ElementID id)
{
  focusedElementId = id;

  updateVisibilityFlags();

  updateFocusGraphView();
}

} // namespace visualization

} // namespace dafg