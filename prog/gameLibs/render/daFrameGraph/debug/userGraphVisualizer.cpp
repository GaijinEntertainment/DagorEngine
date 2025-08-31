// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "userGraphVisualizer.h"

#include "EASTL/numeric.h"

#include <debug/textureVisualization.h>

#include <runtime/runtime.h>
#include <frontend/nodeTracker.h>
#include <frontend/nameResolver.h>

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <imgui-node-editor/imgui_canvas.h>
#include <imgui-node-editor/imgui_bezier_math.h>

#include <util/dag_convar.h>
#include <util/dag_console.h>
#include <render/debugTexOverlay.h>
#include <perfMon/dag_statDrv.h>


constexpr float NODE_WINDOW_PADDING = 10.f;
constexpr float NODE_HORIZONTAL_MARGIN = 80.f;
constexpr ImU32 SELECTION_COLOR = IM_COL32(255, 225, 150, 255);

template <class T, class... Ts>
static void hashPack(size_t &hash, T first, const Ts &...other)
{
  hash ^= eastl::hash<T>{}(first) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  (hashPack(hash, other), ...);
}


namespace dafg
{

extern ConVarT<bool, false> recompile_graph;

void show_dependencies_window() { imgui_window_set_visible(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME, true); }

namespace visualization
{

VisualizedEdge *UserGraphVisualizer::selectedEdge = nullptr;

UserGraphVisualizer::UserGraphVisualizer(InternalRegistry &intRegistry, const NameResolver &nameResolver,
  const DependencyData &_depData, const intermediate::Graph &irGraph, const PassColoring &coloring) :
  registry(intRegistry), resolver(nameResolver), depData(_depData), intermediateGraph(irGraph), intermediatePassColoring(coloring)
{
  REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME, [&]() { this->draw(); });
}

static const char *get_string(FocusedResourceAction action)
{
  switch (action)
  {
    case FocusedResourceAction::ShowAll: return "None";
    case FocusedResourceAction::FocusOnResourceAndRenames: return "Focus on resource and renames";
    case FocusedResourceAction::FocusOnResource: return "Focus on resource";
    default: return "Unobtainable";
  }
}

void UserGraphVisualizer::draw()
{
  if (ImGui::IsWindowCollapsed())
    return;

  if (!visualizationDataGenerated())
    dafg::recompile_graph.set(true);

  // TODO: This code can be generalized and reused elsewhere

  // A lot of code comes from mr ocornut himself
  // https://gist.github.com/ocornut/7e9b3ec566a333d725d4

  focusedNodeChanged = false;
  focusedResourceChanged = false;
  searchBoxHovered = !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

  // if we have focused node and graph has changed,
  // we find that node by name and assume that focused node hasn't been changed
  if (focusedNodeId > -1)
  {
    const auto iter = eastl::find(nodeNames.begin(), nodeNames.end(), nodeSearchInput);
    focusedNodeId = iter == nodeNames.end() ? -1 : iter - nodeNames.begin();
  }

  drawUI();

  drawCanvas();

  processPopup();

  processCentering();

  processInput();

  if (eastl::exchange(shouldUpdateVisualization, false))
  {
    if (selectedEdge != nullptr && selectedResId != dafg::ResNameId::Invalid)
    {
      Selection selection{PreciseTimePoint{nodeIds[selectedEdge->source], nodeIds[selectedEdge->destination]}, selectedResId};
      update_fg_debug_tex(selection, registry);
    }
    else
    {
      update_fg_debug_tex({}, registry);
    }
  }
}

void UserGraphVisualizer::drawUI()
{
  // first line
  {
    if (ImGuiDagor::ComboWithFilter("Execution Order / Node Search", nodeNames, focusedNodeId, nodeSearchInput, true, true,
          "Search for node..."))
    {
      focusedNodeChanged = true;
      resourceSearchInput.clear();
      focusedResource.reset();
    }
    searchBoxHovered = searchBoxHovered || ImGui::IsItemHovered();

    ImGui::SameLine();
    if (treeWithFilter("FG Managed Resource Search", resNames, focusedResourceNo, resourceSearchInput, true, "Search for resource..."))
    {
      focusedResourceChanged = true;
      nodeSearchInput.clear();
      focusedNodeId = -1;
      if (focusedResourceNo > -1)
        setFocusedResource(resIds[focusedResourceNo]);
      else
        focusedResource = {};
    }
    searchBoxHovered = searchBoxHovered || ImGui::IsItemHovered();

    if (focusedResource.has_value())
    {
      ImGui::SameLine();
      ImGuiDagor::EnumCombo("##resourceRenamesCombo", FocusedResourceAction::ShowAll,
        focusedResource->hasRenames ? FocusedResourceAction::FocusOnResourceAndRenames : FocusedResourceAction::FocusOnResource,
        focusedResource->action, &get_string, ImGuiComboFlags_WidthFitPreview);
    }
  }

  // second line
  {
    ImGui::Text("Edge types: ");

    for (auto &info : edge_type_debug_infos)
    {
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(info.imguiColor));
      ImGui::Checkbox(info.readableName.c_str(), &info.visible);
      ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    if (ImGui::BeginCombo("##coloration", colorationTypes[(size_t)coloration].first))
    {
      for (auto [name, type] : colorationTypes)
      {
        bool selected = coloration == type;
        if (ImGui::Selectable(name, selected))
          coloration = type;
        if (selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
  }


  // third line
  {
    if (ImGui::Button("Recompile"))
      dafg::recompile_graph.set(true);

    ImGui::SameLine();
    if (deselect_button("Deselect"))
    {
      selectedEdge = nullptr;
      selectedResId = dafg::ResNameId::Invalid;
      shouldUpdateVisualization = true;
    }

    fg_texture_visualization_imgui_line(registry);

    ImGui::SameLine();
    overlay_checkbox("Overlay mode");
  }

  // forth line
  {
    ImGui::Text("Use mouse wheel button to pan.");
  }
}

void UserGraphVisualizer::drawCanvas()
{
  if (!canvas.Begin("##scrolling_region", ImVec2(0, 0)))
    return;

  centerView = ImVec2(0.0f, 0.0f);
  tooltipMessage.clear();
  hoverEdges.clear();

  canvasHovered = canvas.ViewRect().Contains(ImGui::GetIO().MousePos);

  ImDrawList *drawList = ImGui::GetWindowDrawList();

  // Layer 0 for edges, 1 for nodes, 2 for text
  drawList->ChannelsSplit(3);

  drawAllNodes(drawList);

  focusedResourceLeftmostEdgeStart = ImVec2(HUGE_VALF, HUGE_VALF);
  focusedResourceLeftmostEdgeEnd = ImVec2(HUGE_VALF, HUGE_VALF);
  drawList->ChannelsSetCurrent(0);

  for (uint32_t from = 0; from < visualized_adjacency_list.size(); ++from)
    for (auto &[to, edge] : visualized_adjacency_list[from])
      drawEdge(drawList, edge, from, to);

  drawList->ChannelsMerge();
  canvas.End();
}

void UserGraphVisualizer::drawAllNodes(ImDrawList *drawList)
{
  // (with fake nodes)
  const size_t node_count = visualized_adjacency_list.size();

  // Used further below to draw edges
  nodeLeftAnchors.clear();
  nodeRightAnchors.clear();
  nodeLeftAnchors.resize(node_count);
  nodeRightAnchors.resize(node_count);

  float currentHorizontalOffset = 0;
  hasResources = !hiddenResources.empty();

  for (const auto &column : node_table)
  {
    float currentMaxWidth = 0;
    for (uint32_t subrank = 0; subrank < column.size(); ++subrank)
    {
      auto nodeId = column[subrank];

      const auto verticalPos = column.size() == 1 ? 0 : ((static_cast<float>(subrank) / static_cast<float>(column.size() - 1)) - 0.5f);
      const auto nodeRectMin = ImVec2(currentHorizontalOffset, verticalPos * 1000.f);

      const bool real = !nodeIsFake(nodeId);

      if (real)
      {
        drawList->ChannelsSetCurrent(2);

        ImGui::SetCursorScreenPos(nodeRectMin + ImVec2(NODE_WINDOW_PADDING, NODE_WINDOW_PADDING));
        ImGui::BeginGroup();

        auto nodeName = registry.knownNames.getName(nodeIds[nodeId]);
        auto &nodeData = registry.nodes[nodeIds[nodeId]];

        bool inactiveNode = !nodeHasExplicitOrderings[nodeId];
        if (inactiveNode && hasResources)
        {
          const bool ignoreHiddenResources = focusedResource.has_value() && focusedResource->action != FocusedResourceAction::ShowAll;
          if (focusedResource.has_value() && nodeData.renamedResources.contains(focusedResource->id))
            inactiveNode = false;
          else
          {
            for (const auto &r : nodeData.resourceRequests)
            {
              const auto resolvedId = resolver.resolve(r.first);
              if (ignoreHiddenResources)
              {
                if (resolvedId == focusedResource->id || isVisibleRename(r.first))
                {
                  inactiveNode = false;
                  break;
                }
              }
              else
              {
                const auto cbegin = resIds.cbegin();
                const auto cend = resIds.cend();
                if (!hiddenResources[resolvedId] && eastl::find(cbegin, cend, resolvedId) != cend)
                {
                  inactiveNode = false;
                  break;
                }
              }
            }
          }
        }

        ImGui::TextUnformatted(nodeName);

        if (!nodeData.nodeSource.empty() && ImGui::IsItemHovered())
        {
          // Tooltips doesn't work inside canvas and we can't use Suspend()/Resume() because of interleaving
          // with ChannelsSetCurrent() api. So just remember desired message and show it later.
          tooltipMessage.printf(0, "right click to copy %s", nodeData.nodeSource);
          if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            ImGui::SetClipboardText(nodeData.nodeSource.c_str());
        }
        ImGui::SameLine();
        ImGui::Checkbox(String(0, "##%s", nodeName), &nodeData.enabled);

#if _TARGET_PC_WIN // there are some problems with calling system on not win platform
        if (!nodeData.nodeSource.empty())
        {
          if (ImGui::Button(String(0, "open source file##%s", nodeName)))
          {
            // Open source file via VSCode
            // https://code.visualstudio.com/docs/editor/command-line
            String cmd(0, "code -g %s", nodeData.nodeSource.c_str());
            system(cmd.c_str());
          }
        }
        else
        {
          ImGui::Text("no provided source file");
        }
#endif
        ImGui::EndGroup();

        const auto nodeSize = ImGui::GetItemRectSize() + ImVec2(2 * NODE_WINDOW_PADDING, 2 * NODE_WINDOW_PADDING);

        const auto nodeRectMax = nodeRectMin + nodeSize;

        const bool nodeBoxHovered = ImGui::IsMouseHoveringRect(nodeRectMin, nodeRectMax, true);

        ImU32 nodeBorderColor = IM_COL32(100, 100, 100, 255);
        float nodeBorderThickness = 1.f;

        if (nodeBoxHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
          focusedNodeId = nodeId;
          nodeSearchInput = nodeName;
          focusedNodeChanged = true;
          resourceSearchInput.clear();
          focusedResource.reset();
        }

        if (nodeId == focusedNodeId)
        {
          nodeBorderColor = SELECTION_COLOR;
          nodeBorderThickness = 4.f;
          centerView = nodeRectMin + nodeSize / 2.f;
        }

        drawList->ChannelsSetCurrent(1);

        ImU32 nodeFillColor = IM_COL32(75, 75, 75, 255);

        auto setColorFromHash = [&nodeFillColor](size_t hash) {
          // Hash some more to get different colors for close numbers
          hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
          hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
          hash = (hash >> 16) ^ hash;
          constexpr size_t HUE_COUNT = 256;
          float hue = static_cast<float>((6607 * hash) % HUE_COUNT) / static_cast<float>(HUE_COUNT - 1);

          ImVec4 color(0, 0, 0, 1);
          ImGui::ColorConvertHSVtoRGB(hue, 1.f, 1.f, color.x, color.y, color.z);
          nodeFillColor = ImGui::ColorConvertFloat4ToU32(color);
        };

        switch (coloration)
        {
          case ColorationType::None: break;
          case ColorationType::Framebuffer:
            if (const auto &node = registry.nodes[nodeIds[nodeId]]; node.renderingRequirements)
            {
              size_t hash = 0;

              const auto &pass = *node.renderingRequirements;

              hashPack(hash, pass.depthReadOnly);
              for (auto color : pass.colorAttachments)
                hashPack(hash, color.nameId, color.mipLevel, color.layer);
              hashPack(hash, pass.depthAttachment.nameId, pass.depthAttachment.mipLevel, pass.depthAttachment.layer);

              setColorFromHash(hash);

              if (nodeBoxHovered)
              {
                tooltipMessage.aprintf(0, "Color attachments:\n");
                for (const auto &color : pass.colorAttachments)
                  if (color.nameId != dafg::ResNameId::Invalid)
                    tooltipMessage.aprintf(0, "  %s\n", getName(color.nameId));
                  else
                    tooltipMessage.aprintf(0, "  <none>\n");
                if (pass.depthAttachment.nameId != dafg::ResNameId::Invalid)
                  tooltipMessage.aprintf(0, "Depth attachment: %s (%s)\n", getName(pass.depthAttachment.nameId),
                    pass.depthReadOnly ? "RO" : "RW");
              }
            }
            break;
          case ColorationType::Pass:
            setColorFromHash(static_cast<size_t>(passColoring[nodeIds[nodeId]]));
            if (nodeBoxHovered)
              tooltipMessage.aprintf(0, "Pass color: %d\n", eastl::to_underlying(passColoring[nodeIds[nodeId]]));
            break;
        }

        if (inactiveNode)
        {
          nodeFillColor = ImGui::ColorConvertFloat4ToU32(ImGui::ColorConvertU32ToFloat4(nodeFillColor) * ImVec4{1.f, 1.f, 1.f, 0.1f});
          nodeBorderColor =
            ImGui::ColorConvertFloat4ToU32(ImGui::ColorConvertU32ToFloat4(nodeBorderColor) * ImVec4{1.f, 1.f, 1.f, 0.1f});
        }

        drawList->AddRectFilled(nodeRectMin, nodeRectMax, nodeFillColor, 4.0f);
        drawList->AddRect(nodeRectMin, nodeRectMax, nodeBorderColor, 4.0f, 0, nodeBorderThickness);

        nodeLeftAnchors[nodeId] = nodeRectMin + ImVec2(0, nodeSize.y / 2);
        nodeRightAnchors[nodeId] = nodeRectMax - ImVec2(0, nodeSize.y / 2);

        currentMaxWidth = eastl::max(currentMaxWidth, nodeSize.x);
      }
      else
        nodeLeftAnchors[nodeId] = nodeRightAnchors[nodeId] = nodeRectMin;
    }
    currentHorizontalOffset += currentMaxWidth + NODE_HORIZONTAL_MARGIN;
  }
}

void UserGraphVisualizer::drawEdge(ImDrawList *drawList, VisualizedEdge &edge, uint32_t from, uint32_t to)
{
  uint32_t visible_resource_count =
    focusedResource.has_value() && focusedResource->action != FocusedResourceAction::ShowAll ? 0 : edge.resIds.size();

  if (hasResources && visible_resource_count > 0)
  {
    for (const auto &resId : edge.resIds)
      if (hiddenResources[resId])
        --visible_resource_count;
  }
  else if (visible_resource_count == 0 && !edge.resIds.empty())
  {
    if (focusedResource->action == FocusedResourceAction::FocusOnResource &&
        eastl::find(edge.resIds.begin(), edge.resIds.end(), focusedResource->id) != edge.resIds.end())
      visible_resource_count = 1;
    else
      for (auto resId : edge.resIds)
        if (isVisibleRename(resId))
          ++visible_resource_count;
  }

  ImCubicBezierPoints spline = {nodeRightAnchors[from], nodeRightAnchors[from] + ImVec2(NODE_HORIZONTAL_MARGIN, 0),
    nodeLeftAnchors[to] - ImVec2(NODE_HORIZONTAL_MARGIN, 0), nodeLeftAnchors[to]};

  bool edgeHasFocusedResId = false;
  if (focusedResource.has_value())
    edgeHasFocusedResId = eastl::find(edge.resIds.begin(), edge.resIds.end(), focusedResource->id) != edge.resIds.end();
  if (focusedResourceChanged && edgeHasFocusedResId && spline.P0.x < focusedResourceLeftmostEdgeStart.x)
  {
    focusedResourceLeftmostEdgeStart = spline.P0;
    focusedResourceLeftmostEdgeEnd = spline.P3;
  }

  const float informationEdgeThickness = edge.thickness / (edge.types.size() * eastl::max(edge.resIds.size(), static_cast<size_t>(1)));
  const float resourceEdgeThickness = informationEdgeThickness * visible_resource_count;

  bool visibleEdge = false;
  ImVec2 splineOffset = ImVec2(0.0, eastl::max(informationEdgeThickness, resourceEdgeThickness) * (edge.types.size() - 1) * 0.5);
  for (EdgeType type : edge.types)
  {
    auto &info = infoForEdgeType(type);
    if (!info.visible)
      continue;

    visibleEdge = true;
    const float thickness =
      type == EdgeType::EXPLICIT_FOLLOW || type == EdgeType::EXPLICIT_PREVIOUS ? informationEdgeThickness : resourceEdgeThickness;

    if (thickness > 0.001f)
    {
      auto addBezierToDrawList = [&](ImU32 color, float line_thickness) {
        drawList->AddBezierCubic(splineOffset + spline.P0, splineOffset + spline.P1, splineOffset + spline.P2,
          splineOffset + spline.P3, color, line_thickness);
      };

      if (edgeHasFocusedResId)
        addBezierToDrawList(SELECTION_COLOR, 1.2f * thickness);
      addBezierToDrawList(info.imguiColor, thickness);
      if (edgeHasFocusedResId)
        addBezierToDrawList(SELECTION_COLOR, 0.5f * thickness + 0.3f);

      splineOffset -= ImVec2(0.0, thickness);
    }
  }

  const ImVec2 canvasMousePos = ImGui::GetIO().MousePos;

  if (visibleEdge && hasResources && canvasHovered && canvasMousePos.x >= spline.P0.x && canvasMousePos.x <= spline.P3.x)
  {
    auto hit = ImProjectOnCubicBezier(canvasMousePos, spline);
    if (hit.Distance < edge.thickness * 0.5)
    {
      hoverEdges.push_back(&edge);

      for (auto resId : edge.resIds)
      {
        const bool alwaysVisible = !focusedResource.has_value() || focusedResource->action == FocusedResourceAction::ShowAll;
        const bool focusedRes = focusedResource.has_value() && resId == focusedResource->id;
        const bool visibleRename = focusedResource.has_value() && isVisibleRename(resId);
        if ((alwaysVisible && !hiddenResources[resId]) || focusedRes || visibleRename)
          tooltipMessage.aprintf(0, "%s\n", getName(resId));
      }

      if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
      {
        popupEdges = hoverEdges;

        canvas.Suspend();
        ImGui::OpenPopup("edge_resources");
        canvas.Resume();
        selectedResId = dafg::ResNameId::Invalid;
      }
    }
  }
}

void UserGraphVisualizer::processPopup()
{
  if (ImGui::BeginPopup("edge_resources"))
  {
    if (ImGui::Selectable("<None>", true))
    {
      selectedResId = dafg::ResNameId::Invalid;
      shouldUpdateVisualization = true;
    }
    for (const auto edgePtr : popupEdges)
      for (const auto resId : edgePtr->resIds)
      {
        auto label = getName(resId).data();
        if (ImGui::Selectable(label))
        {
          selectedResId = resId;
          selectedEdge = edgePtr;

          nodeSearchInput.clear();
          resourceSearchInput.clear();
          focusedNodeId = -1;
          focusedResource.reset();
          shouldUpdateVisualization = true;
        }
      }
    ImGui::EndPopup();
  }
  else if (tooltipMessage.length() && !searchBoxHovered)
  {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(tooltipMessage);
    ImGui::EndTooltip();
  }
}

void UserGraphVisualizer::processCentering()
{
  if (focusedNodeChanged && focusedNodeId > -1)
  {
    canvas.CenterView(centerView);
    viewOrigin = canvas.ViewOrigin();
    prevFocusedNodePos = centerView;
  }
  else if (focusedResourceChanged && focusedResource.has_value())
  {
    centerView = (focusedResourceLeftmostEdgeStart + focusedResourceLeftmostEdgeEnd) / 2.f;
    canvas.CenterView(centerView);
    viewOrigin = canvas.ViewOrigin();
  }
  else if (!focusedNodeChanged && focusedNodeId > -1)
  {
    // here centerView is focused node position
    ImVec2 offset = centerView - prevFocusedNodePos;
    ImVec2 prevCenter = -canvas.ToLocalV(canvas.ViewOrigin() - canvas.CalcCenterView(ImVec2{}).Origin);

    canvas.CenterView(prevCenter + offset);
    viewOrigin = canvas.ViewOrigin();
    prevFocusedNodePos = centerView;
  }
}

void UserGraphVisualizer::processInput()
{
  if (ImGui::IsWindowHovered() && canvasHovered)
  {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
    {
      viewOrigin += ImGui::GetIO().MouseDelta;
      canvas.SetView(viewOrigin, scale_levels[current_scale_level]);
    }
    if (abs(ImGui::GetIO().MouseWheel) > 0.1 && !searchBoxHovered)
    {
      int delta = ImGui::GetIO().MouseWheel > 0 ? 1 : -1;
      current_scale_level = clamp<int>(current_scale_level + delta, 0, countof(scale_levels) - 1);
      ImVec2 oldPos = canvas.ToLocal(ImGui::GetIO().MousePos);
      canvas.SetView(viewOrigin, scale_levels[current_scale_level]);
      ImVec2 newPos = canvas.ToLocal(ImGui::GetIO().MousePos);
      viewOrigin += canvas.FromLocalV(newPos - oldPos);
      canvas.SetView(viewOrigin, scale_levels[current_scale_level]);
    }
  }
}


void UserGraphVisualizer::collectValidNodes(eastl::span<const NodeNameId> node_execution_order)
{
  nodeIds.assign(node_execution_order.begin(), node_execution_order.end());
}

void UserGraphVisualizer::generateDependencyEdges(eastl::span<const NodeNameId> node_execution_order)
{
  resIds.clear();
  hiddenResources.clear();
  labeled_graph.clear();
  labeled_graph.resize(nodeIds.size());

  IdIndexedMapping<NodeNameId, uint32_t> executionTimeForNode(registry.nodes.size(), CULLED_OUT_NODE);
  for (size_t i = 0; i < node_execution_order.size(); ++i)
    executionTimeForNode[node_execution_order[i]] = i;

  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    for (const NodeNameId prevId : nodeData.precedingNodeIds)
      if (executionTimeForNode.isMapped(prevId))
        addDebugEdge(executionTimeForNode[prevId], executionTimeForNode[nodeId], EdgeType::EXPLICIT_PREVIOUS, ResNameId::Invalid);

    for (const NodeNameId nextId : nodeData.followingNodeIds)
      if (executionTimeForNode.isMapped(nextId))
        addDebugEdge(executionTimeForNode[nodeId], executionTimeForNode[nextId], EdgeType::EXPLICIT_FOLLOW, ResNameId::Invalid);
  }

  size_t resCounter = 0;
  for (auto [resId, lifetime] : depData.resourceLifetimes.enumerate())
  {
    if (lifetime.introducedBy == NodeNameId::Invalid || executionTimeForNode[lifetime.introducedBy] == CULLED_OUT_NODE)
      continue;

    eastl::fixed_vector<NodeNameId, 32> modifiers(lifetime.modificationChain.begin(), lifetime.modificationChain.end());

    eastl::erase_if(modifiers, [&](NodeNameId id) { return executionTimeForNode[id] == CULLED_OUT_NODE; });

    eastl::sort(modifiers.begin(), modifiers.end(), [&executionTimeForNode](NodeNameId first, NodeNameId second) {
      return executionTimeForNode[first] < executionTimeForNode[second];
    });

    if (!modifiers.empty())
      addDebugEdge(executionTimeForNode[lifetime.introducedBy], executionTimeForNode[modifiers.front()], EdgeType::IMPLICIT_MOD_CHAIN,
        resId);

    for (size_t i = 0; i + 1 < modifiers.size(); ++i)
      addDebugEdge(executionTimeForNode[modifiers[i]], executionTimeForNode[modifiers[i + 1]], EdgeType::IMPLICIT_MOD_CHAIN, resId);

    auto finalModifier = !modifiers.empty() ? modifiers.back() : lifetime.introducedBy;

    if (lifetime.consumedBy != NodeNameId::Invalid)
    {
      addDebugEdge(executionTimeForNode[finalModifier], executionTimeForNode[lifetime.consumedBy], EdgeType::IMPLICIT_MOD_CHAIN,
        resId);
      for (auto reader : lifetime.readers)
        addDebugEdge(executionTimeForNode[reader], executionTimeForNode[lifetime.consumedBy], EdgeType::INVISIBLE, resId);
    }


    for (auto reader : lifetime.readers)
      addDebugEdge(executionTimeForNode[finalModifier], executionTimeForNode[reader], EdgeType::IMPLICIT_RES_DEP, resId);

    if (!modifiers.empty() || lifetime.consumedBy != NodeNameId::Invalid || !lifetime.readers.empty())
    {
      nameSpaces[getParent(resId)].resources.push_back(resId);
      ++resCounter;
    }
  }

  resIds.reserve(resCounter);

  if (resCounter)
  {
    for (auto &[namespaceId, content] : nameSpaces)
    {
      const bool emptyNS = content.subNameSpaces.empty() && content.resources.empty();
      if (registry.knownNames.root() != namespaceId && !emptyNS)
        nameSpaces[getParent(namespaceId)].subNameSpaces.push_back(namespaceId);
    }

    processTreeContents(registry.knownNames.root());
    hiddenResources.resize(static_cast<size_t>(*eastl::max_element(resIds.cbegin(), resIds.cend())) + 1, false);
  }
  else
    nameSpaces.clear();

  nodeNames = gatherNames<NodeNameId>(nodeIds);
  resNames = gatherNames<ResNameId>(resIds);
}

void UserGraphVisualizer::updateNameSpaces()
{
  nameSpaces.clear();

  auto insertNS = [&](NameSpaceNameId namespaceId) { nameSpaces.insert({namespaceId, {}}); };

  insertNS(registry.knownNames.root());
  registry.knownNames.iterateSubTree<NameSpaceNameId>(registry.knownNames.root(), insertNS);
}

void UserGraphVisualizer::hideResourcesInSubTree(NameSpaceNameId name_space_id)
{
  nameSpaces[name_space_id].visibleResourcesInSubtree = 0;
  for (const auto &subNameSpace : nameSpaces[name_space_id].subNameSpaces)
    hideResourcesInSubTree(subNameSpace);
}

void UserGraphVisualizer::hideResourcesInNameSpace(NameSpaceNameId namespace_id)
{
  const uint16_t old = nameSpaces[namespace_id].visibleResourcesInSubtree;
  hideResourcesInSubTree(namespace_id);
  while (namespace_id != registry.knownNames.root())
  {
    namespace_id = registry.knownNames.getParent(namespace_id);
    nameSpaces[namespace_id].visibleResourcesInSubtree -= old;
  }
}

void UserGraphVisualizer::showResourcesInSubTree(NameSpaceNameId namespace_id)
{
  nameSpaces[namespace_id].visibleResourcesInSubtree = nameSpaces[namespace_id].totalResourcesInSubtree;
  for (const auto &subNameSpace : nameSpaces[namespace_id].subNameSpaces)
    showResourcesInSubTree(subNameSpace);
}

void UserGraphVisualizer::showResourcesInNameSpace(NameSpaceNameId namespace_id)
{
  const uint16_t diff = nameSpaces[namespace_id].totalResourcesInSubtree - nameSpaces[namespace_id].visibleResourcesInSubtree;
  showResourcesInSubTree(namespace_id);
  while (namespace_id != registry.knownNames.root())
  {
    namespace_id = registry.knownNames.getParent(namespace_id);
    nameSpaces[namespace_id].visibleResourcesInSubtree += diff;
  }
}

void UserGraphVisualizer::processTreeContents(NameSpaceNameId namespace_id)
{
  nameSpaces[namespace_id].totalResourcesInSubtree = nameSpaces[namespace_id].resources.size();
  auto comp = [&names = registry.knownNames](const auto &fst, const auto &snd) {
    return eastl::string_view(names.getShortName(fst)) < eastl::string_view(names.getShortName(snd));
  };
  stlsort::sort(nameSpaces[namespace_id].subNameSpaces.begin(), nameSpaces[namespace_id].subNameSpaces.end(), comp);
  stlsort::sort(nameSpaces[namespace_id].resources.begin(), nameSpaces[namespace_id].resources.end(), comp);
  for (const auto &subfolder : nameSpaces[namespace_id].subNameSpaces)
  {
    processTreeContents(subfolder);
    nameSpaces[namespace_id].totalResourcesInSubtree += nameSpaces[subfolder].totalResourcesInSubtree;
  }
  nameSpaces[namespace_id].visibleResourcesInSubtree = nameSpaces[namespace_id].totalResourcesInSubtree;
  resIds.insert(resIds.end(), nameSpaces[namespace_id].resources.begin(), nameSpaces[namespace_id].resources.end());
};

template <class EnumType>
eastl::string_view UserGraphVisualizer::getName(EnumType res_id)
{
  return registry.knownNames.getName(res_id);
}

template <class EnumType>
dag::Vector<eastl::string_view> UserGraphVisualizer::gatherNames(eastl::span<const EnumType> ids)
{
  dag::Vector<eastl::string_view> names;
  names.reserve(ids.size());
  for (auto id : ids)
    names.push_back(getName(id));
  return names;
}

template <class EnumType>
NameSpaceNameId UserGraphVisualizer::getParent(EnumType id)
{
  return registry.knownNames.getParent(id);
}


// from global
void UserGraphVisualizer::generatePlanarizedLayout()
{
  selectedEdge = nullptr;

  node_table.clear();
  visualized_adjacency_list.clear();

  if (imgui_get_state() == ImGuiState::OFF)
    return;

  if (!imgui_window_is_visible(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME))
    return;

  if (imgui_window_is_collapsed(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME))
    return;

  TIME_PROFILE(generate_planarized_layout);

  AdjacencyLists adjList(labeled_graph.size());
  for (uint32_t from = 0; from < labeled_graph.size(); ++from)
  {
    for (auto edge : labeled_graph[from])
      adjList[from].push_back(edge.destination);
  }

  // NOTE: the graph is already submitted in topsort order to debug vis
  dag::Vector<NodeIdx> reversedTopsort(nodeIds.size());
  eastl::iota(reversedTopsort.rbegin(), reversedTopsort.rend(), 0);
  auto layout = layouter->layout(adjList, reversedTopsort, true);

  node_table = eastl::move(layout.nodeLayout);

  nodeHasExplicitOrderings.resize(layout.adjacencyList.size(), false);

  visualized_adjacency_list.resize(layout.adjacencyList.size());
  for (uint32_t from = 0; from < layout.adjacencyList.size(); ++from)
  {
    for (uint32_t i = 0; i < layout.adjacencyList[from].size(); ++i)
    {
      auto to = layout.adjacencyList[from][i];
      auto &[prevFrom, prevEdges] = layout.edgeMapping[from][i];
      VisualizedEdge visEdge;
      visEdge.source = prevFrom;
      visEdge.destination = labeled_graph[prevFrom][prevEdges[0]].destination;
      for (uint32_t edgeIdx : prevEdges)
      {
        EdgeData edge = labeled_graph[prevFrom][edgeIdx];
        if (edge.type == EdgeType::INVISIBLE)
          continue;
        if (eastl::find(visEdge.types.begin(), visEdge.types.end(), edge.type) == visEdge.types.end())
        {
          visEdge.types.push_back(edge.type);
          if (edge.type == EdgeType::EXPLICIT_FOLLOW || edge.type == EdgeType::EXPLICIT_PREVIOUS)
          {
            nodeHasExplicitOrderings[from] = true;
            nodeHasExplicitOrderings[to] = true;
          }
        }
        if (edge.resId != ResNameId::Invalid &&
            eastl::find(visEdge.resIds.begin(), visEdge.resIds.end(), edge.resId) == visEdge.resIds.end())
        {
          visEdge.thickness += 5.0;
          visEdge.resIds.push_back(edge.resId);
        }
        visEdge.destination = edge.destination;
      }
      if (visEdge.types.empty())
        continue;
      if (visEdge.resIds.empty())
        visEdge.thickness = 5.0;
      visualized_adjacency_list[from].emplace_back(to, visEdge);
    }
  }
}

void UserGraphVisualizer::updateVisualization()
{
  TIME_PROFILE(update_graph_visualization);

  passColoring.clear();
  passColoring.reserve(registry.nodes.size());

  // Debug graph visualization works with not multiplexed nodes
  dag::Vector<NodeNameId, framemem_allocator> demultiplexedNodeExecutionOrder;
  demultiplexedNodeExecutionOrder.reserve(registry.nodes.size());
  for (auto [idx, irNode] : intermediateGraph.nodes.enumerate())
    if (irNode.multiplexingIndex == 0 && irNode.frontendNode)
    {
      demultiplexedNodeExecutionOrder.emplace_back(*irNode.frontendNode);
      passColoring.set(*irNode.frontendNode, intermediatePassColoring[idx]);
    }

  updateNameSpaces();

  auto new_node_execution_order = filter_out_debug_node(demultiplexedNodeExecutionOrder, registry);
  collectValidNodes(new_node_execution_order);
  generateDependencyEdges(new_node_execution_order);

  generatePlanarizedLayout();
  update_external_texture_visualization(registry, depData);
}

void UserGraphVisualizer::invalidateGraphVisualization()
{
  nameSpaces = {};
  hiddenResources = {};

  collectValidNodes({});
  generateDependencyEdges({});
  generatePlanarizedLayout();

  close_visualization_texture();
}

void UserGraphVisualizer::drawTree(const dafg::NameSpaceNameId &namespace_id, int &res_idx, int &counter, bool &selected_by_mouse,
  ImGuiDagor::ComboInfo &info, eastl::string &input, float text_width, float button_offset)
{
  const auto &knownNames = registry.knownNames;
  auto &nsContent = nameSpaces[namespace_id];

  eastl::string imguiLabel = namespace_id == knownNames.root() ? "" : knownNames.getShortName(namespace_id);
  const float freeWidthAfterText = eastl::max(text_width - ImGui::GetCursorPosX() - ImGui::CalcTextSize(imguiLabel.data()).x, 0.f);
  const int spaceCount = static_cast<int>(freeWidthAfterText / ImGui::CalcTextSize(" ").x);
  imguiLabel.append(spaceCount, ' ');
  imguiLabel.append_sprintf("##ns_%d", static_cast<int>(namespace_id));

  const ImGuiTreeNodeFlags COMMON_FLAGS = ImGuiTreeNodeFlags_SpanTextWidth | ImGuiTreeNodeFlags_FramePadding;
  const ImGuiTreeNodeFlags SUB_TREE_FLAGS = ImGuiTreeNodeFlags_DefaultOpen | COMMON_FLAGS;

  const bool active = ImGui::GetStateStorage()->GetInt(ImGui::GetID(imguiLabel.data()), 1) != 0;
  const uint16_t totalResourcesInSubtree = nsContent.totalResourcesInSubtree;
  const bool needOpenSubTree = counter <= res_idx && res_idx < counter + totalResourcesInSubtree; // to show focused res
  bool closeSubTree = !active && needOpenSubTree;
  if (!active && needOpenSubTree)
    ImGui::TreeNodeSetOpen(ImGui::GetID(imguiLabel.data()), true);

  const bool opened = ImGui::TreeNodeEx(imguiLabel.data(), SUB_TREE_FLAGS);
  const bool hiddenNameSpace = nsContent.visibleResourcesInSubtree == 0;
  eastl::string buttonLabel = hiddenNameSpace ? "Show" : "Hide";
  buttonLabel.append_sprintf("##bns_%d", static_cast<int>(namespace_id));
  ImGui::SameLine();
  ImGui::SetCursorPosX(button_offset);
  if (ImGui::Button(buttonLabel.data()))
  {
    auto cit = resIds.begin() + counter;
    for (uint16_t i = 0; i < totalResourcesInSubtree; ++i, ++cit)
      hiddenResources[*cit] = !hiddenNameSpace;
    if (hiddenNameSpace)
      showResourcesInNameSpace(namespace_id);
    else
      hideResourcesInNameSpace(namespace_id);
  }

  if (opened)
  {
    for (const auto &childNS : nsContent.subNameSpaces)
      if (!selected_by_mouse)
        drawTree(childNS, res_idx, counter, selected_by_mouse, info, input, text_width, button_offset);
    ImGui::TreePop();
    if (closeSubTree)
      ImGui::TreeNodeSetOpen(ImGui::GetID(imguiLabel.data()), false);
  }
  else
  {
    counter += totalResourcesInSubtree;
    return;
  }

  const ImGuiTreeNodeFlags LEAF_FLAGS = COMMON_FLAGS | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Leaf;
  for (const auto &resData : nsContent.resources)
  {
    const bool is_selected = res_idx == counter;
    const bool hidden = hiddenResources[resData];
    if (is_selected && (ImGui::IsWindowAppearing() || info.selectionChanged))
      ImGui::SetScrollHereY();
    if (is_selected && info.arrowScroll)
      ImGui::SetScrollHereY();

    eastl::string imguiLabel = knownNames.getShortName(resData);
    const float freeWidthAfterText = eastl::max(text_width - ImGui::GetCursorPosX() - ImGui::CalcTextSize(imguiLabel.data()).x, 0.f);
    const int spaceCount = static_cast<int>(freeWidthAfterText / ImGui::CalcTextSize(" ").x);
    imguiLabel.append(spaceCount, ' ');
    imguiLabel.append_sprintf("##res_%d", static_cast<int>(resData));

    const ImGuiTreeNodeFlags selectedFlag = is_selected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;
    if (ImGui::TreeNodeEx(imguiLabel.data(), LEAF_FLAGS | selectedFlag))
    {
      if (!hidden && ImGui::IsItemClicked(ImGuiMouseButton_Left))
      {
        info.selectionChanged = res_idx != counter;
        res_idx = counter;
        input = knownNames.getName(resData);
        ImGui::CloseCurrentPopup();
        selected_by_mouse = true;
      }
      if (hidden && is_selected)
      {
        info.selectionChanged = true;
        res_idx = -1;
        input.clear();
        selected_by_mouse = true;
      }
    }

    eastl::string buttonLabel = hidden ? "Show" : "Hide";
    buttonLabel.append_sprintf("##bres_%d", static_cast<int>(resData));
    ImGui::SameLine();
    ImGui::SetCursorPosX(button_offset);
    if (ImGui::Button(buttonLabel.data()))
    {
      hiddenResources[resData] = !hiddenResources[resData];
      dafg::NameSpaceNameId updatedNS = namespace_id;
      bool rootNameSpace = false;
      while (!rootNameSpace)
      {
        nameSpaces[updatedNS].visibleResourcesInSubtree += hidden ? 1 : -1;
        rootNameSpace = updatedNS == registry.knownNames.root();
        updatedNS = registry.knownNames.getParent(updatedNS);
      }
    }

    ++counter;
  }
}

bool UserGraphVisualizer::treeWithFilter(const char *label, const dag::Vector<eastl::string_view> &data, int &current_idx,
  eastl::string &input, bool return_on_arrows, const char *hint, ImGuiComboFlags flags)
{
  if (data.empty())
  {
    ImGui::TextColored({1, 1, 0, 1}, "No resources");
    return false;
  }

  float text_width = 0.f;
  eastl::for_each(data.begin(), data.end(),
    [&max = text_width](const auto &b) { max = eastl::max(max, ImGui::CalcTextSize(b.data()).x); });
  const float buttonWidth = eastl::max(ImGui::CalcTextSize("Show").x, ImGui::CalcTextSize("Hide").x);
  const float padding = ImGui::GetStyle().FramePadding.x + ImGui::GetStyle().WindowPadding.x;
  const float width = text_width + buttonWidth + ImGui::GetStyle().ScrollbarSize + 2.0f * padding;

  auto res = ImGuiDagor::BeginComboWithFilter(label, data, current_idx, input, width, hint, flags);

  if (!res.has_value())
    return false;

  auto &info = res.value();

  if (info.inputTextChanged && !info.arrowScroll)
    ImGui::CloseCurrentPopup();

  int counter = 0;
  bool selectedByMouse = false;
  const float buttonOffset =
    ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize - buttonWidth - ImGui::GetStyle().FramePadding.x;
  drawTree(registry.knownNames.root(), current_idx, counter, selectedByMouse, info, input, text_width, buttonOffset);
  info.arrowScroll = info.arrowScroll && return_on_arrows;
  return ImGuiDagor::EndComboWithFilter(label, data, current_idx, input, info, selectedByMouse);
}

} // namespace visualization

} // namespace dafg

static bool dafg_show_fg_tex_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("dafg", "show_fg_tex", 1, DebugTexOverlay::MAX_CONSOLE_ARGS_CNT)
  {
    if (dafg::Runtime::isInitialized())
    {
      // TODO: awful disgusting hack, remove asap
      auto &registry = dafg::Runtime::get().getInternalRegistry();

      dafg::visualization::UserGraphVisualizer::selectedEdge = nullptr;

      if (argc > 1)
      {
        // I don't care about perf here
        eastl::string name = argv[1];
        if (name[0] != '/')
          name = "/" + name;

        auto resId = registry.knownNames.getNameId<dafg::ResNameId>(name);
        if (resId == dafg::ResNameId::Invalid)
        {
          console::print_d("daFG: resource %s not found.", name.c_str());
          return found;
        }

        eastl::string concatenatedParams;
        for (int i = 2; i < argc; ++i)
        {
          concatenatedParams += argv[i];
          if (i + 1 < argc)
            concatenatedParams += " ";
        }
        set_manual_showtex_params(concatenatedParams.c_str());

        update_fg_debug_tex(Selection{ReadTimePoint{}, resId}, registry);
      }
      else
      {
        update_fg_debug_tex({}, registry);
      }
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(dafg_show_fg_tex_console_handler);