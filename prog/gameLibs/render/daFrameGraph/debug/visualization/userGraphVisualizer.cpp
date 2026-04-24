// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "userGraphVisualizer.h"

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


// Originally inspired by mr ocornut
// https://gist.github.com/ocornut/7e9b3ec566a333d725d4


constexpr float NODE_X_SIZE = 200.f;
constexpr float NODE_Y_SIZE = 100.f;
constexpr float FAKE_NODE_Y_SIZE = 50.f;
constexpr float COL_X_BASE_MARGIN = 2.f * NODE_X_SIZE;
constexpr float OBJ_Y_BASE_MARGIN = 0.6f * NODE_Y_SIZE;

constexpr ImVec2 NAMESPACE_BORDER_PADDING = ImVec2{50.f, 50.f};

constexpr float NODE_BORDER_WIDTH = 2.f;
constexpr ImVec2 NODE_BORDER_PADDING = ImVec2{10.f, 10.f};
constexpr ImU32 NODE_REGULAR_COLOR = IM_COL32(100, 100, 100, 255);   // rgb(100, 100, 100)
constexpr ImU32 NODE_SELECTION_COLOR = IM_COL32(250, 250, 150, 255); // rgb(250, 250, 150)

constexpr ImU32 DEP_COLOR_PREV = IM_COL32(255, 0, 0, 255);   // rgb(255, 0, 0)
constexpr ImU32 DEP_COLOR_FOLLOW = IM_COL32(0, 0, 255, 255); // rgb(0, 0, 255)
constexpr ImU32 DEP_COLOR_MOD = IM_COL32(255, 255, 0, 255);  // rgb(255, 255, 0)
constexpr ImU32 DEP_COLOR_READ = IM_COL32(0, 255, 0, 255);   // rgb(0, 255, 0)
constexpr ImU32 DEP_COLOR_CONS = IM_COL32(255, 0, 255, 255); // rgb(255, 0, 255)

constexpr float EXPLICIT_DEP_WIDTH = 3.f;
constexpr float RESOURCE_DEP_WIDTH = 5.f;

constexpr float RES_SELECTION_WIDTH_FRAC = 2.f;
constexpr ImU32 RES_SELECTION_COLOR = IM_COL32(255, 128, 0, 255); // rgb(255, 128, 0)


constexpr ImVec2 MAX_IMVEC2 = ImVec2{eastl::numeric_limits<float>::max(), eastl::numeric_limits<float>::max()};

template <class T, class... Ts>
static void hash_pack(size_t &hash, T first, const Ts &...other)
{
  hash ^= eastl::hash<T>{}(first) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  (hash_pack(hash, other), ...);
}


namespace dafg
{

extern ConVarT<bool, false> recompile_graph;

void show_dependencies_window() { imgui_window_set_visible(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME, true); }

namespace visualization::usergraph
{

static eastl::array<DependencyTypeInfo, static_cast<size_t>(DependencyType::COUNT)> dependencyInfoByTypeArray{
  DependencyTypeInfo{"Precedes", DEP_COLOR_PREV},
  DependencyTypeInfo{"Follows", DEP_COLOR_FOLLOW},
  DependencyTypeInfo{"Modifies", DEP_COLOR_MOD},
  DependencyTypeInfo{"Reads", DEP_COLOR_READ},
  DependencyTypeInfo{"Consumes", DEP_COLOR_CONS},
};

static DependencyTypeInfo &dependency_info_by_type(const DependencyType type)
{
  return dependencyInfoByTypeArray[static_cast<size_t>(type)];
}

static const char *res_focus_name_by_type(ResourceFocusType type)
{
  switch (type)
  {
    case ResourceFocusType::All: return "Show all";
    case ResourceFocusType::Resource: return "Focus on resource";
    case ResourceFocusType::ResourceAndRenames: return "Focus on resource and renames";
  }
  return "";
}

static ImU32 color_by_hash(size_t hash)
{
  constexpr size_t HUE_COUNT = 256;

  // Hash some more to get different colors for close numbers
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = (hash >> 16) ^ hash;
  float hue = static_cast<float>((6607 * hash) % HUE_COUNT) / static_cast<float>(HUE_COUNT - 1);

  ImVec4 color(0, 0, 0, 1);
  ImGui::ColorConvertHSVtoRGB(hue, 1.f, 1.f, color.x, color.y, color.z);
  return ImGui::ColorConvertFloat4ToU32(color);
};

static ImU32 apply_alpha_coeff(ImU32 color, float k = 1.f)
{
  return ImGui::ColorConvertFloat4ToU32(ImGui::ColorConvertU32ToFloat4(color) * ImVec4{1.f, 1.f, 1.f, k});
}

// sometimes we have to get dirty to stay clean...
static const Edge *selectedEdge = nullptr;


Visualizer::Visualizer(InternalRegistry &int_registry, const NameResolver &name_resolver, const DependencyData &dep_data,
  const intermediate::Graph &ir_graph, const PassColoring &coloring) :
  registry(int_registry), resolver(name_resolver), depData(dep_data), intermediateGraph(ir_graph), intermediatePassColoring(coloring)
{
  REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP_FG2, IMGUI_USG_WIN_NAME, [&]() { this->draw(); });
}

void Visualizer::draw()
{
  if (ImGui::IsWindowCollapsed() || !layoutReady)
    return;

  focusedNode.wasChanged = false;
  focusedResource.wasChanged = false;
  searchBoxHovered = !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);


  drawUI();

  drawCanvas();


  processPopup();

  processCentering();

  processInput();

  processTextureDebug();
}

void Visualizer::drawUI()
{
  // first line
  {
    if (ImGuiDagor::ComboWithFilter("Node Search", nodeNames, focusedNodeIndex, nodeSearchInput, false, true, "Search for node...") &&
        focusedNodeIndex != UNKNOWN_INDEX)
    {
      resetFocusedResource();
      setFocusedNode(nodeIds[focusedNodeIndex]);
    }
    searchBoxHovered = searchBoxHovered || ImGui::IsItemHovered();
    ImGui::SameLine();
    if (ImGui::Button("Reset node focus"))
      resetFocusedNode();

    ImGui::SameLine();
    if (treeWithFilter("FG Managed Resource Search", resNames, focusedResourceIndex, resourceSearchInput, true,
          "Search for resource...") &&
        focusedResourceIndex != UNKNOWN_INDEX)
    {
      resetFocusedNode();
      setFocusedResource(resIds[focusedResourceIndex]);
    }
    searchBoxHovered = searchBoxHovered || ImGui::IsItemHovered();
    ImGui::SameLine();
    if (ImGui::Button("Reset resource focus"))
      resetFocusedResource();

    if (focusedResource.valid())
    {
      ImGui::SameLine();
      ImGuiDagor::EnumCombo("##resourceRenamesCombo", ResourceFocusType::All,
        focusedResource.hasRenames ? ResourceFocusType::ResourceAndRenames : ResourceFocusType::Resource, focusedResource.type,
        &res_focus_name_by_type, ImGuiComboFlags_WidthFitPreview);
    }
  }

  // second line
  {
    ImGui::Text("Edge types: ");

    for (auto &info : dependencyInfoByTypeArray)
    {
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(info.imguiColor));
      ImGui::Checkbox(info.name.c_str(), &info.visible);
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

void Visualizer::drawCanvas()
{
  if (!canvas.Begin("##scrolling_region", ImVec2(0, 0)))
    return;

  canvasHovered = canvas.ViewRect().Contains(ImGui::GetIO().MousePos);
  centerView = ImVec2(0.0f, 0.0f);
  focusedResLeftEnd = MAX_IMVEC2;
  focusedResRightEnd = MAX_IMVEC2;
  tooltipMessage.clear();
  hoverEdges.clear();

  // here, we will perform draw, based on precalculated positions
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  drawList->ChannelsSplit(CanvasChannels::COUNT);
  {
    const ImVec2 offset = ImVec2{0.f, -wholeGraphLayout.size.y / 2.f};

    for (uint32_t userNodeIndex = 0; userNodeIndex < registry.nodes.size(); ++userNodeIndex)
      drawNode(drawList, NodeNameId(userNodeIndex), offset + wholeGraphLayout.objectsOffsets[userNodeIndex],
        wholeGraphLayout.objectsSizes[userNodeIndex]);

    drawEdges(drawList, wholeGraphLayout, offset);
  }
  drawList->ChannelsMerge();

  canvas.End();
}

void Visualizer::drawNode(ImDrawList *draw_list, const NodeNameId node_id, const ImVec2 node_offset, const ImVec2 node_size)
{
  const auto nodeName = getName(node_id);
  auto &nodeData = registry.nodes[node_id];

  // draw node rectangle and borders
  {
    draw_list->ChannelsSetCurrent(CanvasChannels::NODES);

    const ImVec2 leftTopPos = node_offset;
    const ImVec2 rightBottomPos = node_offset + node_size;
    const bool nodeBoxHovered = ImGui::IsMouseHoveringRect(leftTopPos, rightBottomPos, true);

    ImU32 nodeFillColor = NODE_REGULAR_COLOR;
    ImU32 nodeBorderColor = apply_alpha_coeff(nodeFillColor, 0.8f);
    float nodeBorderThickness = NODE_BORDER_WIDTH;

    if (nodeBoxHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
      resetFocusedResource();
      setFocusedNode(node_id);
      focusedNodeIndex = eastl::find(nodeIds.begin(), nodeIds.end(), node_id) - nodeIds.begin();
      nodeSearchInput = nodeName;
    }

    if (node_id == focusedNode.id)
    {
      nodeBorderColor = NODE_SELECTION_COLOR;
      nodeBorderThickness *= 2.f;
      centerView = (leftTopPos + rightBottomPos) / 2.f;
    }

    switch (coloration)
    {
      case ColorationType::None: break;
      case ColorationType::Framebuffer:
      {
        if (nodeData.renderingRequirements)
        {
          size_t hash = 0;

          const auto &pass = *nodeData.renderingRequirements;
          hash_pack(hash, pass.depthReadOnly);
          for (auto color : pass.colorAttachments)
            hash_pack(hash, color.nameId, color.mipLevel, color.layer);
          hash_pack(hash, pass.depthAttachment.nameId, pass.depthAttachment.mipLevel, pass.depthAttachment.layer);

          nodeFillColor = color_by_hash(hash);

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
      }
      case ColorationType::Pass:
      {
        nodeFillColor = color_by_hash(static_cast<size_t>(passColors[node_id]));

        if (nodeBoxHovered)
          tooltipMessage.aprintf(0, "Pass color: %d\n", eastl::to_underlying(passColors[node_id]));

        break;
      }
    }

    draw_list->AddRectFilled(node_offset, rightBottomPos, nodeFillColor, 4.0f);
    draw_list->AddRect(node_offset, rightBottomPos, nodeBorderColor, 4.0f, 0, nodeBorderThickness);
  }

  // draw texts and buttons
  {
    draw_list->ChannelsSetCurrent(CanvasChannels::TEXTS);

    ImGui::SetCursorScreenPos(node_offset + NODE_BORDER_PADDING);
    ImGui::BeginGroup();
    {
      ImGui::TextUnformatted(nodeName.data());

      if (!nodeData.nodeSource.empty())
      {
        if (ImGui::IsItemHovered())
        {
          // Tooltips doesn't work inside canvas and we can't use Suspend()/Resume() because of interleaving
          // with ChannelsSetCurrent() api. So just remember desired message and show it later.
          tooltipMessage.printf(0, "right click to copy:\n%s", nodeData.nodeSource);
          if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            ImGui::SetClipboardText(nodeData.nodeSource.c_str());
        }

#if _TARGET_PC_WIN // there are some problems with calling system on not win platform
        if (ImGui::Button(String(0, "open source file##%s", nodeName)))
        {
          // Open source file via VSCode
          // https://code.visualstudio.com/docs/editor/command-line
          String cmd(0, "code -g %s", nodeData.nodeSource.c_str());
          system(cmd.c_str());
        }
#endif
      }
      else
      {
        ImGui::Text("no provided source file");
      }

      ImGui::Checkbox(String(0, "Enable node##%s", nodeName), &nodeData.enabled);
    }
    ImGui::EndGroup();
  }
}

void Visualizer::drawEdges(ImDrawList *draw_list, const Layout &layout, const ImVec2 offset)
{
  draw_list->ChannelsSetCurrent(CanvasChannels::EDGES);

  for (uint32_t edgeIndex = 0; edgeIndex < layout.edgesCount; ++edgeIndex)
  {
    auto &edge = layout.edges[edgeIndex];
    auto &edgeSplines = layout.edgesSplines[edgeIndex];
    // edge can carry a lot of dependencies, which would make it really wide
    // they can be without any resource, or one resource would have many dependencies
    // to keep graph readable, we will look only on unique resources

    // get list of explicit dependencies witout resources and list of unique resources
    // and calcutale line widths
    dag::Vector<DependencyId, framemem_allocator> explicitDeps;
    dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> carriedResources;
    IdIndexedMapping<ResNameId, DependencyType, framemem_allocator> resourcesInfos;
    explicitDeps.reserve(edge.carriedDeps.size());
    carriedResources.reserve(edge.carriedDeps.size());
    for (const auto depId : edge.carriedDeps)
    {
      const auto &dep = registryDependencies[depId];
      if (dep.resource == ResNameId::Invalid)
        explicitDeps.push_back(depId);
      else if (carriedResources.insert(dep.resource).second)
        resourcesInfos.set(dep.resource, dep.type);
    }

    const float edgeWidth = RESOURCE_DEP_WIDTH * min(carriedResources.size(), static_cast<uint32_t>(15));
    const float dependencyWidth = edgeWidth / max(carriedResources.size(), static_cast<uint32_t>(1));

    auto drawDepsAlongSpline = [&](const CubicBezierWithNormals &spline) {
      draw_list->ChannelsSetCurrent(CanvasChannels::EDGES);

      // draw explicit dependencies first as they are less important
      for (const auto explicitDepId : explicitDeps)
      {
        const auto &info = dependency_info_by_type(registryDependencies[explicitDepId].type);
        if (info.visible)
          draw_list->AddBezierCubic(offset + spline.P0, offset + spline.P1, offset + spline.P2, offset + spline.P3, info.imguiColor,
            EXPLICIT_DEP_WIDTH);
      }

      // now draw implicit dependencies
      bool visibleEdge = false;
      float dependencyOffset = (dependencyWidth - edgeWidth) * 0.5f;
      for (const auto resId : carriedResources)
      {
        const auto depType = resourcesInfos[resId];
        const auto &[_, depColor, depVisible] = dependency_info_by_type(depType);
        const bool resourceIsFocused = focusedResource.valid() && focusedResource.id == resId;

        if (depVisible && (!focusedResource.valid() || (focusedResource.valid() && focusedResource.type == ResourceFocusType::All) ||
                            (focusedResource.valid() && focusedResource.id == resId) ||
                            (focusedResource.valid() && focusedResource.id != resId && isVisibleRename(resId))))
        {
          visibleEdge = true;

          auto addBezierToDrawList = [&](ImU32 color, float line_thickness) {
            draw_list->AddBezierCubic(offset + spline.P0 + dependencyOffset * spline.norm0,
              offset + spline.P1 + dependencyOffset * spline.norm0, offset + spline.P2 + dependencyOffset * spline.norm1,
              offset + spline.P3 + dependencyOffset * spline.norm1, color, line_thickness);
          };

          if (resourceIsFocused)
            addBezierToDrawList(RES_SELECTION_COLOR, RES_SELECTION_WIDTH_FRAC * dependencyWidth);

          addBezierToDrawList(depColor, dependencyWidth);
        }

        // check to perform camera focusing on resource
        if (resourceIsFocused && focusedResource.wasChanged && spline.P0.x < focusedResLeftEnd.x)
        {
          focusedResLeftEnd = spline.P0;
          focusedResRightEnd = spline.P3;
        }

        dependencyOffset += dependencyWidth;
      }

      draw_list->ChannelsSetCurrent(CanvasChannels::SUSPEND);

      // check mouse hovering above current spline
      if (visibleEdge && canvasHovered)
      {
        auto hit = ImProjectOnCubicBezier(ImGui::GetIO().MousePos,
          {offset + spline.P0, offset + spline.P1, offset + spline.P2, offset + spline.P3});
        if (hit.Distance < edgeWidth * 0.5)
        {
          hoverEdges.push_back(&edge);

          for (const auto resId : carriedResources)
          {
            const auto depType = resourcesInfos[resId];
            const auto &[_, depColor, depVisible] = dependency_info_by_type(depType);
            if (
              depVisible && (!focusedResource.valid() || (focusedResource.valid() && focusedResource.type == ResourceFocusType::All) ||
                              (focusedResource.valid() && focusedResource.id == resId) ||
                              (focusedResource.valid() && focusedResource.id != resId && isVisibleRename(resId))))
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
    };

    for (const auto &spline : edgeSplines)
      drawDepsAlongSpline(spline);
  }
}

void Visualizer::processPopup()
{
  if (ImGui::BeginPopup("edge_resources"))
  {
    if (ImGui::Selectable("<None>", true))
    {
      selectedResId = dafg::ResNameId::Invalid;
      shouldUpdateVisualization = true;
    }
    for (const auto edgePtr : popupEdges)
      for (const auto depId : edgePtr->carriedDeps)
      {
        const auto &dep = registryDependencies[depId];
        if (dep.resource != ResNameId::Invalid)
        {
          auto label = getName(dep.resource).data();
          if (ImGui::Selectable(label))
          {
            selectedResId = dep.resource;
            selectedEdge = edgePtr;

            resetFocusedNode();
            resetFocusedResource();
            shouldUpdateVisualization = true;
          }
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

void Visualizer::processCentering()
{
  if (focusedNode.wasChanged && focusedNode.valid())
  {
    canvas.CenterView(centerView);
    canvasCamera.canvasOffset = canvas.ViewOrigin();
    prevFocusedNodePos = centerView;
  }
  else if (focusedResource.wasChanged && focusedResource.valid())
  {
    centerView = (focusedResLeftEnd + focusedResRightEnd) / 2.f;
    canvas.CenterView(centerView);
    canvasCamera.canvasOffset = canvas.ViewOrigin();
  }
  else if (!focusedNode.wasChanged && focusedNode.valid())
  {
    // here centerView is focused node position
    ImVec2 offset = centerView - prevFocusedNodePos;
    ImVec2 prevCenter = -canvas.ToLocalV(canvas.ViewOrigin() - canvas.CalcCenterView(ImVec2{}).Origin);

    canvas.CenterView(prevCenter + offset);
    canvasCamera.canvasOffset = canvas.ViewOrigin();
    prevFocusedNodePos = centerView;
  }
}

void Visualizer::processInput()
{
  if (ImGui::IsWindowHovered() && canvasHovered)
  {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
    {
      canvasCamera.canvasOffset += ImGui::GetIO().MouseDelta;
      canvas.SetView(canvasCamera.canvasOffset, canvasCamera.getZoom());
    }
    if (abs(ImGui::GetIO().MouseWheel) > 0.1 && !searchBoxHovered)
    {
      ImGui::GetIO().MouseWheel > 0 ? canvasCamera.zoomIn() : canvasCamera.zoomOut();

      ImVec2 oldPos = canvas.ToLocal(ImGui::GetIO().MousePos);
      canvas.SetView(canvasCamera.canvasOffset, canvasCamera.getZoom());
      ImVec2 newPos = canvas.ToLocal(ImGui::GetIO().MousePos);

      canvasCamera.canvasOffset += canvas.FromLocalV(newPos - oldPos);
      canvas.SetView(canvasCamera.canvasOffset, canvasCamera.getZoom());
    }
  }
}

void Visualizer::processTextureDebug()
{
  if (eastl::exchange(shouldUpdateVisualization, false))
  {
    eastl::optional<Selection> resSelection = eastl::nullopt;

    if (selectedEdge != nullptr && selectedResId != dafg::ResNameId::Invalid)
      for (const auto depId : selectedEdge->carriedDeps)
      {
        const auto &dep = registryDependencies[depId];
        if (dep.resource == selectedResId)
        {
          resSelection = Selection{PreciseTimePoint{dep.from, dep.to}, selectedResId};
          break;
        }
      }

    update_fg_debug_tex(resSelection, registry, depData);
  }
}

void Visualizer::hideResourcesInSubTree(NameSpaceNameId name_space_id)
{
  nameSpaces[name_space_id].visibleResourcesInSubtree = 0;
  for (const auto &subNameSpace : nameSpaces[name_space_id].subNameSpaces)
    hideResourcesInSubTree(subNameSpace);
}

void Visualizer::hideResourcesInNameSpace(NameSpaceNameId namespace_id)
{
  const uint16_t old = nameSpaces[namespace_id].visibleResourcesInSubtree;
  hideResourcesInSubTree(namespace_id);
  while (namespace_id != registry.knownNames.root())
  {
    namespace_id = registry.knownNames.getParent(namespace_id);
    nameSpaces[namespace_id].visibleResourcesInSubtree -= old;
  }
}

void Visualizer::showResourcesInSubTree(NameSpaceNameId namespace_id)
{
  nameSpaces[namespace_id].visibleResourcesInSubtree = nameSpaces[namespace_id].totalResourcesInSubtree;
  for (const auto &subNameSpace : nameSpaces[namespace_id].subNameSpaces)
    showResourcesInSubTree(subNameSpace);
}

void Visualizer::showResourcesInNameSpace(NameSpaceNameId namespace_id)
{
  const uint16_t diff = nameSpaces[namespace_id].totalResourcesInSubtree - nameSpaces[namespace_id].visibleResourcesInSubtree;
  showResourcesInSubTree(namespace_id);
  while (namespace_id != registry.knownNames.root())
  {
    namespace_id = registry.knownNames.getParent(namespace_id);
    nameSpaces[namespace_id].visibleResourcesInSubtree += diff;
  }
}


void Visualizer::setFocusedNode(NodeNameId node_id)
{
  focusedNode = {
    .id = node_id,
    .wasChanged = true,
  };
}

void Visualizer::setFocusedResource(ResNameId res_id, ResourceFocusType focus_type)
{
  focusedResource = {
    res_id,
    true,
    false,
    focus_type,
  };

  if (focusedResource.valid())
    for (const auto &[from, to] : depData.renamingChains.enumerate())
    {
      focusedResource.hasRenames =
        (from != focusedResource.id && to == focusedResource.id) || (from == focusedResource.id && to != focusedResource.id);
      if (focusedResource.hasRenames)
        return;
    }
}


void Visualizer::updateVisualization()
{
  TIME_PROFILE(update_usergraph_visualization);

  if (!imgui_window_is_visible(nullptr, IMGUI_USG_WIN_NAME) || imgui_window_is_collapsed(nullptr, IMGUI_USG_WIN_NAME))
    return;


  selectedEdge = nullptr;
  layoutReady = false;

  updateIRInfo();

  updateNameSpaces();
  updateDependencies();

  performLayout();

  layoutReady = true;


  update_external_texture_visualization(registry, depData);
}

void Visualizer::updateIRInfo()
{
  TIME_PROFILE(update_ir_info);

  passColors.clear();
  passColors.resize(registry.nodes.size(), UNKNOWN_PASS_COLOR);
  executionTime.clear();
  executionTime.resize(registry.nodes.size(), CULLED_OUT_NODE);


  uint32_t currExecTime = 0;
  for (auto [idx, irNode] : intermediateGraph.nodes.enumerate())
    if (irNode.multiplexingIndex == 0 && irNode.frontendNode)
    {
      const auto nodeNameId = *irNode.frontendNode;
      passColors[nodeNameId] = intermediatePassColoring[idx];
      executionTime[nodeNameId] = currExecTime;
      ++currExecTime;
    }
}

void Visualizer::updateNameSpaces()
{
  TIME_PROFILE(update_name_spaces);

  nameSpaces.clear();
  nameSpaces.reserve(registry.knownNames.nameCount<NameSpaceNameId>());

  nodeIds.clear();
  resIds.clear();
  hiddenResources.clear();

  nodeIds.reserve(registry.nodes.size());
  resIds.reserve(registry.resources.size());
  hiddenResources.resize(registry.resources.size(), false);

  nodeNames.clear();
  resNames.clear();


  // populate name spaces map
  {
    auto insertNS = [&](const NameSpaceNameId namespace_id) { nameSpaces.set(namespace_id, {}); };
    insertNS(registry.knownNames.root());
    registry.knownNames.iterateSubTree<NameSpaceNameId>(registry.knownNames.root(), insertNS);
  }

  // collect sub namespaces
  for (const auto namespaceId : nameSpaces.keys())
    if (namespaceId != registry.knownNames.root())
      nameSpaces[registry.knownNames.getParent(namespaceId)].subNameSpaces.push_back(namespaceId);

  // collect nodes and resources ids
  for (const auto nodeId : registry.nodes.keys())
    nameSpaces[registry.knownNames.getParent(nodeId)].nodes.push_back(nodeId);
  for (const auto resId : registry.resources.keys())
    nameSpaces[registry.knownNames.getParent(resId)].resources.push_back(resId);

  // calculate contents
  {
    auto nameComparator = [&names = registry.knownNames](const auto &fst, const auto &snd) {
      return eastl::string_view(names.getShortName(fst)) < eastl::string_view(names.getShortName(snd));
    };

    dag::FixedMoveOnlyFunction<24, void(const NameSpaceNameId)> processTree = [&](const NameSpaceNameId name_space_id) {
      NameSpace &currNameSpace = nameSpaces[name_space_id];
      currNameSpace.totalNodesInSubtree = currNameSpace.nodes.size();
      currNameSpace.totalResourcesInSubtree = currNameSpace.resources.size();

      stlsort::sort(currNameSpace.subNameSpaces.begin(), currNameSpace.subNameSpaces.end(), nameComparator);
      stlsort::sort(currNameSpace.nodes.begin(), currNameSpace.nodes.end(), nameComparator);
      stlsort::sort(currNameSpace.resources.begin(), currNameSpace.resources.end(), nameComparator);

      for (const auto &subNameSpaceId : currNameSpace.subNameSpaces)
      {
        processTree(subNameSpaceId);

        const NameSpace &subNameSpace = nameSpaces[subNameSpaceId];
        currNameSpace.totalNodesInSubtree += subNameSpace.totalNodesInSubtree;
        currNameSpace.totalResourcesInSubtree += subNameSpace.totalResourcesInSubtree;
      }

      currNameSpace.visibleResourcesInSubtree = currNameSpace.totalResourcesInSubtree;

      nodeIds.insert(nodeIds.end(), currNameSpace.nodes.begin(), currNameSpace.nodes.end());
      resIds.insert(resIds.end(), currNameSpace.resources.begin(), currNameSpace.resources.end());
    };

    processTree(registry.knownNames.root());
  }

  // get names
  nodeNames = gatherNames<NodeNameId>(nodeIds);
  resNames = gatherNames<ResNameId>(resIds);
}

void Visualizer::updateDependencies()
{
  TIME_PROFILE(update_dependencies);

  registryDependencies.clear();
  dependenciesLists.clear();
  dependenciesLists.resize(registry.nodes.size(), {});
  for (auto list : dependenciesLists)
    list.reserve(16);

  auto addDependency = [&](const NodeNameId from, const NodeNameId to, const DependencyType dep,
                         const ResNameId res = ResNameId::Invalid) {
    auto [newDepId, newDep] = registryDependencies.appendNew();
    dependenciesLists[from].push_back(newDepId);
    newDep = {dep, from, to, res};
  };


  // filling explicit dependencies
  for (auto [nodeId, nodeData] : registry.nodes.enumerate())
  {
    for (const NodeNameId prevId : nodeData.precedingNodeIds)
      addDependency(prevId, nodeId, DependencyType::EXPLICIT_PREVIOUS);

    for (const NodeNameId nextId : nodeData.followingNodeIds)
      addDependency(nodeId, nextId, DependencyType::EXPLICIT_FOLLOW);
  }

  // filling resource dependencies
  for (auto [resId, lifetime] : depData.resourceLifetimes.enumerate())
  {
    /*
    consumer is dependent on every reader
    if there are no readers, consumer is dependent on last modifier
    if there are no modifiers and no readers, consumer is dependent on introducer

    every reader is dependent on last modifier
    if there are no modifiers, every reader is dependent on introducer

    first modifier is dependent on introducer

    // clang-format off
    so, if we have modifiers, we get such dependencies:

              M           R
            / |         / | \
           /  M        /  R  \
          /   |       / / | \ \
         /    ~      / /  ~  \ \
        I           M===~   ~===C
              ~    / \ \  ~  / /
              |   /   \ \ | / /
              M  /     \  R  /
              | /       \ | /
              M           R

    or, if we dont have modifiers:

              R
            / | \
           /  R  \
          / / | \ \
         / /  ~  \ \
        I===~   ~===C
         \ \  ~  / /
          \ \ | / /
           \  R  /
            \ | /
              R

    // clang-format on
    */

    const NodeNameId introducerId = lifetime.introducedBy;
    const NodeNameId consumerId = lifetime.consumedBy;
    NodeNameId readSourceId = introducerId;

    const bool resHasIntroducer = introducerId != NodeNameId::Invalid;
    const bool resHasModifiers = !lifetime.modificationChain.empty();
    const bool resHasReaders = !lifetime.readers.empty();
    const bool resHasConsumer = consumerId != NodeNameId::Invalid;

    if (resHasModifiers)
    {
      // we sort modifiers as we can
      // nodes, which were used in intermediate graph, will go first in execution order
      // all other nodes will be considered culled away and will go in random order after exucutable nodes
      dag::Vector<NodeNameId, framemem_allocator> modifiersOrder(lifetime.modificationChain.begin(), lifetime.modificationChain.end());
      eastl::sort(modifiersOrder.begin(), modifiersOrder.end(),
        [&](const NodeNameId first, const NodeNameId second) { return executionTime[first] < executionTime[second]; });

      if (resHasIntroducer)
        addDependency(introducerId, modifiersOrder.front(), DependencyType::IMPLICIT_RES_MODIFY, resId);

      for (uint32_t modifierIdx = 1; modifierIdx < modifiersOrder.size(); ++modifierIdx)
        addDependency(modifiersOrder[modifierIdx - 1], modifiersOrder[modifierIdx], DependencyType::IMPLICIT_RES_MODIFY, resId);

      readSourceId = modifiersOrder.back();
    }

    const bool readSourceValid = readSourceId != NodeNameId::Invalid;

    for (const auto readerId : lifetime.readers)
    {
      if (readSourceValid)
        addDependency(readSourceId, readerId, DependencyType::IMPLICIT_RES_READ, resId);
      if (resHasConsumer)
        addDependency(readerId, consumerId, DependencyType::IMPLICIT_RES_CONSUME, resId);
    }
    if (!resHasReaders && readSourceValid && resHasConsumer)
      addDependency(readSourceId, consumerId, DependencyType::IMPLICIT_RES_CONSUME, resId);
  }
}

static dag::Vector<NodeIdx> get_reversed_topsort(const AdjacencyLists &graph)
{
  dag::Vector<NodeIdx> res;
  res.reserve(graph.size());

  // we will use DFS to sort indicies in topoligical order
  dag::Vector<bool, framemem_allocator> indexVisited;
  indexVisited.resize(graph.size(), false);

  dag::FixedMoveOnlyFunction<32, void(const NodeIdx)> dfsNode = [&](const NodeIdx node_index) {
    indexVisited[node_index] = true;

    for (const auto to : graph[node_index])
      if (!indexVisited[to])
        dfsNode(to);

    res.push_back(node_index);
  };

  // iterate on every index and we will get reversed topsort
  for (uint32_t i = 0; i < graph.size(); ++i)
    if (!indexVisited[i])
      dfsNode(i);

  return res;
}

void Visualizer::performLayout()
{
  TIME_PROFILE(generate_layout);

  wholeGraphLayout = {};


  const uint32_t userNodesCount = dependenciesLists.size();
  if (userNodesCount > 0)
  {
    // we generate adjacency lists of graph and layout it
    AdjacencyLists graphAdjLists(userNodesCount);
    for (auto [nodeId, nodeDependencies] : dependenciesLists.enumerate())
    {
      const auto nodeIdx = NodeIdx(nodeId);
      graphAdjLists[nodeIdx].reserve(nodeDependencies.size());
      for (const auto depId : nodeDependencies)
        graphAdjLists[nodeIdx].push_back(NodeIdx(registryDependencies[depId].to));
    }
    auto layout = layouter->layout(graphAdjLists, get_reversed_topsort(graphAdjLists), true);


    // after layouting, we collect nodes ranks and generated edges
    wholeGraphLayout = {
      .objectsCount = layout.adjacencyList.size(),
      .edgesCount = 0,
      .objectsRanks = eastl::move(layout.nodeLayout),
      .edges = {},
    };

    wholeGraphLayout.edges.reserve(wholeGraphLayout.objectsCount);
    for (uint32_t curNodeIdx = 0; curNodeIdx < wholeGraphLayout.objectsCount; ++curNodeIdx)
    {
      for (uint32_t curEdgeIdx = 0; curEdgeIdx < layout.adjacencyList[curNodeIdx].size(); ++curEdgeIdx)
      {
        Edge &newEdge = wholeGraphLayout.edges.emplace_back();
        newEdge.from = curNodeIdx;
        newEdge.to = layout.adjacencyList[curNodeIdx][curEdgeIdx];

        const auto [depSourceNodeIdx, depsIndicies] = layout.edgeMapping[curNodeIdx][curEdgeIdx];
        newEdge.carriedDeps.reserve(depsIndicies.size());
        for (uint32_t curDepInx : depsIndicies)
          newEdge.carriedDeps.push_back(dependenciesLists[NodeNameId(depSourceNodeIdx)][curDepInx]);
      }
    }
    wholeGraphLayout.edgesCount = wholeGraphLayout.edges.size();
  }


  dag::Vector<NodeNameId, framemem_allocator> userNodeIds(dependenciesLists.keys().begin(), dependenciesLists.keys().end());
  calculatePositions(wholeGraphLayout, userNodeIds);
}

void Visualizer::calculatePositions(Layout &layout, const eastl::span<NodeNameId> user_nodes)
{
  TIME_PROFILE(calculate_positions);

  auto &[objectsCount, edgesCount, ranks, edges, layoutSize, objectsOffsets, objectsSizes, edgesSplines] = layout;

  if (objectsCount == 0)
  {
    layoutSize = NAMESPACE_BORDER_PADDING * 2;
    objectsOffsets = {};
    objectsSizes = {};
    edgesSplines = {};
    return;
  }

  const uint32_t userNodesCount = user_nodes.size();
  const uint32_t fakeNodesCount = objectsCount - userNodesCount;

  const uint32_t userNodesIndexOffset = 0;
  const uint32_t fakeNodesIndexOffset = userNodesIndexOffset + userNodesCount;

  // for every column we need its size
  // and mapping "object -> column size"
  const uint32_t rankCount = ranks.size();
  dag::Vector<float, framemem_allocator> columnSizes;
  columnSizes.resize(rankCount, 0.f);
  dag::Vector<float, framemem_allocator> objectsColumnSizes;
  objectsColumnSizes.resize(objectsCount, 0.f);


  // firstly, we fill objects sizes
  {
    objectsSizes.resize(objectsCount, ImVec2{0.f, 0.f});

    for (uint32_t userNodeIndex = 0; userNodeIndex < userNodesCount; ++userNodeIndex)
      objectsSizes[userNodesIndexOffset + userNodeIndex] = {NODE_X_SIZE, NODE_Y_SIZE};
    for (uint32_t fakeNodeIndex = 0; fakeNodeIndex < fakeNodesCount; ++fakeNodeIndex)
      objectsSizes[fakeNodesIndexOffset + fakeNodeIndex] = {0.f, FAKE_NODE_Y_SIZE};

    for (uint32_t rank = 0; rank < rankCount; ++rank)
      for (const uint32_t objIndex : ranks[rank])
        columnSizes[rank] = max(columnSizes[rank], objectsSizes[objIndex].x);
    for (uint32_t rank = 0; rank < rankCount; ++rank)
      for (const uint32_t objIndex : ranks[rank])
        objectsColumnSizes[objIndex] = columnSizes[rank];
  }


  // next step, we calculate object offsets
  {
    objectsOffsets.resize(objectsCount, ImVec2{0.f, 0.f});

    // for every object we get lists of incoming and outcoming edges
    struct LayoutObjectEdges
    {
      dag::Vector<const Edge *, framemem_allocator> in;
      dag::Vector<const Edge *, framemem_allocator> out;
    };
    dag::Vector<LayoutObjectEdges, framemem_allocator> objectsEdges;
    objectsEdges.resize(objectsCount, {});
    {
      uint32_t maxColumnSize = 0;
      for (const auto &column : ranks)
        maxColumnSize = max(maxColumnSize, column.size());

      for (auto objEdges = objectsEdges.rbegin(); objEdges != objectsEdges.rend(); ++objEdges)
      {
        objEdges->in.reserve(maxColumnSize);
        objEdges->out.reserve(maxColumnSize);
      }

      for (const auto &edge : edges)
      {
        objectsEdges[edge.from].out.push_back(&edge);
        objectsEdges[edge.to].in.push_back(&edge);
      }
    }

    {
      // we count how many edges are between each pair
      // of adjacent columns, to make space between them wider if needed
      dag::Vector<uint32_t, framemem_allocator> edgesBetween;
      edgesBetween.resize(rankCount, 0);
      for (const uint32_t objIndex : ranks[0])
        edgesBetween[0] = max(edgesBetween[0], objectsEdges[objIndex].out.size());

      for (uint32_t rank = 1; rank < rankCount; ++rank)
        for (const uint32_t objIndex : ranks[rank])
        {
          edgesBetween[rank - 1] = max(edgesBetween[rank - 1], objectsEdges[objIndex].in.size());
          edgesBetween[rank] = max(edgesBetween[rank], objectsEdges[objIndex].out.size());
        }

      float horOffset = 0.f;
      float vertOffset = 0.f;
      for (uint32_t rank = 0; rank < rankCount; ++rank)
      {
        const auto &column = ranks[rank];
        vertOffset = 0.f;
        for (const uint32_t objIndex : column)
        {
          objectsOffsets[objIndex] = ImVec2{horOffset, vertOffset};
          vertOffset += objectsSizes[objIndex].y + OBJ_Y_BASE_MARGIN;
        }
        horOffset += columnSizes[rank] + COL_X_BASE_MARGIN / (1.f + exp(-0.1f * edgesBetween[rank]));
      }
    }

    // after that, we need move objects up and down in columns to straighten edges, connecting them,
    // and lessen waves in long delendency lines
    auto equalizeHeights = [&](const uint32_t left_idx, const uint32_t right_idx, const uint32_t left_rank, const uint32_t right_rank,
                             bool forward) {
      // choose which object/column to move and what target center Y to aim for
      uint32_t rankToMove = 0;
      uint32_t objToMove = 0;
      uint32_t targetObj = 0;
      if (forward)
      {
        rankToMove = right_rank;
        objToMove = right_idx;
        targetObj = left_idx;
      }
      else
      {
        rankToMove = left_rank;
        objToMove = left_idx;
        targetObj = right_idx;
      }

      // find position of objToMove in its column
      const auto &column = layout.objectsRanks[rankToMove];
      uint32_t objToMoveSubrank = 0;
      for (uint32_t subrank = 0; subrank < column.size(); ++subrank)
        if (column[subrank] == objToMove)
        {
          objToMoveSubrank = subrank;
          break;
        }

      // place the object at the target position
      const float targetCenterY = layout.objectsOffsets[targetObj].y + layout.objectsSizes[targetObj].y / 2.f;
      layout.objectsOffsets[objToMove].y = targetCenterY - layout.objectsSizes[objToMove].y / 2.f;

      // push objects below down if they would overlap
      for (uint32_t objIndex = objToMoveSubrank + 1; objIndex < column.size(); ++objIndex)
      {
        const uint32_t prev = column[objIndex - 1];
        const uint32_t cur = column[objIndex];
        const float minY = layout.objectsOffsets[prev].y + layout.objectsSizes[prev].y + OBJ_Y_BASE_MARGIN;
        if (layout.objectsOffsets[cur].y < minY)
          layout.objectsOffsets[cur].y = minY;
        else
          break;
      }

      // push objects above up if they would overlap
      for (uint32_t objIndex = objToMoveSubrank; objIndex > 0; --objIndex)
      {
        const uint32_t cur = column[objIndex - 1];
        const uint32_t next = column[objIndex];
        const float maxY = layout.objectsOffsets[next].y - layout.objectsSizes[cur].y - OBJ_Y_BASE_MARGIN;
        if (layout.objectsOffsets[cur].y > maxY)
          layout.objectsOffsets[cur].y = maxY;
        else
          break;
      }
    };

    // forward and backward run, trying to equalize fake nodes to keep long dependency edges straight
    for (uint32_t rank = 0; rank < rankCount - 1; ++rank)
      for (const auto from : ranks[rank])
        for (const auto edge : objectsEdges[from].out)
        {
          const NodeIdx to = edge->to;
          if (from >= fakeNodesIndexOffset && to >= fakeNodesIndexOffset)
          {
            equalizeHeights(from, to, rank, rank + 1, true);
            break;
          }
        }
    for (uint32_t rank = rankCount - 1; rank > 0; --rank)
      for (const auto to : ranks[rank])
        for (const auto edge : objectsEdges[to].in)
        {
          const NodeIdx from = edge->from;
          if (from >= fakeNodesIndexOffset && to >= fakeNodesIndexOffset)
          {
            equalizeHeights(from, to, rank - 1, rank, false);
            break;
          }
        }

    // after moving all objects, we calculate size of layout,
    // move objects to keep margin and place service nodes if layout has them
    {
      ImVec2 leftTop = objectsOffsets.front();
      ImVec2 rightBottom = leftTop;

      for (uint32_t objectIndex = 0; objectIndex < objectsCount; ++objectIndex)
      {
        leftTop = ImMin(leftTop, objectsOffsets[objectIndex]);
        rightBottom = ImMax(rightBottom, objectsOffsets[objectIndex] + objectsSizes[objectIndex]);
      }
      layoutSize = rightBottom - leftTop + NAMESPACE_BORDER_PADDING * 2;

      for (uint32_t objectIndex = 0; objectIndex < objectsCount; ++objectIndex)
        objectsOffsets[objectIndex] += NAMESPACE_BORDER_PADDING - leftTop;
    }
  }


  // and finally, we generate edge splines
  {
    edgesSplines.resize(edges.size());

    uint32_t backEdgesDrawn = 0;
    for (uint32_t edgeIndex = 0; edgeIndex < edges.size(); ++edgeIndex)
    {
      const auto &edge = edges[edgeIndex];
      auto &edgeSplines = edgesSplines[edgeIndex];
      const NodeIdx from = edge.from;
      const NodeIdx to = edge.to;

      ImVec2 leftEdgeAnchor = objectsOffsets[from] + ImVec2{objectsSizes[from].x, objectsSizes[from].y / 2.f};
      ImVec2 rightEdgeAnchor = objectsOffsets[to] + ImVec2{0, objectsSizes[to].y / 2.f};
      constexpr ImVec2 NORM_UP = ImVec2{0.f, 1.f};
      constexpr ImVec2 NORM_DOWN = ImVec2{0.f, -1.f};
      constexpr float BEZIER_OFFSET = NODE_X_SIZE * 0.5f;

      // if starting object is narrower that column it is in,
      // we make additional spline to not collide with other objects in this column
      // and move edge start to the column end
      if (objectsSizes[from].x < objectsColumnSizes[from])
      {
        const ImVec2 anchorDelta = ImVec2{objectsColumnSizes[from] - objectsSizes[from].x, 0.f};
        edgeSplines.push_back(
          {leftEdgeAnchor, leftEdgeAnchor, leftEdgeAnchor + anchorDelta, leftEdgeAnchor + anchorDelta, NORM_UP, NORM_UP});
        leftEdgeAnchor += anchorDelta;
      }
      // if the start of the edge is to the left of the end, we draw regular spline
      if (leftEdgeAnchor.x <= rightEdgeAnchor.x)
      {
        edgeSplines.push_back({leftEdgeAnchor, leftEdgeAnchor + ImVec2{BEZIER_OFFSET, 0.f},
          rightEdgeAnchor - ImVec2{BEZIER_OFFSET, 0.f}, rightEdgeAnchor, NORM_UP, NORM_UP});
      }
      // but if the start of the edge is to the right of the end, we have backward edge
      // in that case we make 3 splines, to guide edge over the other objects
      else
      {
        const float horizontalOffset = BEZIER_OFFSET * (backEdgesDrawn + 1);
        const float verticalOffset = OBJ_Y_BASE_MARGIN * (backEdgesDrawn + 1);

        edgeSplines.push_back({
          .P0 = leftEdgeAnchor,
          .P1 = leftEdgeAnchor + ImVec2{horizontalOffset, 0.f},
          .P2 = ImVec2{leftEdgeAnchor.x + horizontalOffset, -verticalOffset},
          .P3 = ImVec2{leftEdgeAnchor.x, -verticalOffset},
          .norm0 = NORM_UP,
          .norm1 = NORM_DOWN,
        });
        edgeSplines.push_back({
          .P0 = ImVec2{leftEdgeAnchor.x, -verticalOffset},
          .P1 = ImVec2{leftEdgeAnchor.x, -verticalOffset},
          .P2 = ImVec2{rightEdgeAnchor.x, -verticalOffset},
          .P3 = ImVec2{rightEdgeAnchor.x, -verticalOffset},
          .norm0 = NORM_DOWN,
          .norm1 = NORM_DOWN,
        });
        edgeSplines.push_back({
          .P0 = ImVec2{rightEdgeAnchor.x, -verticalOffset},
          .P1 = ImVec2{rightEdgeAnchor.x - horizontalOffset, -verticalOffset},
          .P2 = rightEdgeAnchor - ImVec2{horizontalOffset, 0.f},
          .P3 = rightEdgeAnchor,
          .norm0 = NORM_DOWN,
          .norm1 = NORM_UP,
        });

        ++backEdgesDrawn;
      }
    }
  }
}


void Visualizer::drawTree(const dafg::NameSpaceNameId &namespace_id, int &res_idx, int &counter, bool &selected_by_mouse,
  ImGuiDagor::ComboInfo &info, eastl::string &input, float text_width, float button_offset)
{
  const auto &knownNames = registry.knownNames;
  auto &nsContent = nameSpaces[namespace_id];

  eastl::string imguiLabel = namespace_id == knownNames.root() ? "" : knownNames.getShortName(namespace_id);
  const float freeWidthAfterText = eastl::max(text_width - ImGui::GetCursorPosX() - ImGui::CalcTextSize(imguiLabel.data()).x, 0.f);
  const int spaceCount = static_cast<int>(freeWidthAfterText / ImGui::CalcTextSize(" ").x);
  imguiLabel.append(spaceCount, ' ');
  imguiLabel.append_sprintf("##ns_%d", static_cast<int>(namespace_id));

  const ImGuiTreeNodeFlags commonFlags = ImGuiTreeNodeFlags_SpanLabelWidth | ImGuiTreeNodeFlags_FramePadding;
  const ImGuiTreeNodeFlags subTreeFlags = ImGuiTreeNodeFlags_DefaultOpen | commonFlags;

  const bool active = ImGui::GetStateStorage()->GetInt(ImGui::GetID(imguiLabel.data()), 1) != 0;
  const uint16_t totalResourcesInSubtree = nsContent.totalResourcesInSubtree;
  const bool needOpenSubTree = counter <= res_idx && res_idx < counter + totalResourcesInSubtree; // to show focused res
  bool closeSubTree = !active && needOpenSubTree;
  if (!active && needOpenSubTree)
    ImGui::TreeNodeSetOpen(ImGui::GetID(imguiLabel.data()), true);

  const bool opened = ImGui::TreeNodeEx(imguiLabel.data(), subTreeFlags);
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

  const ImGuiTreeNodeFlags leafFlags = commonFlags | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Leaf;
  for (const auto &resData : nsContent.resources)
  {
    const bool isSelected = res_idx == counter;
    const bool hidden = hiddenResources[resData];
    if (isSelected && (ImGui::IsWindowAppearing() || info.selectionChanged))
      ImGui::SetScrollHereY();
    if (isSelected && info.arrowScroll)
      ImGui::SetScrollHereY();

    eastl::string imguiLabel = knownNames.getShortName(resData);
    const float freeWidthAfterText = eastl::max(text_width - ImGui::GetCursorPosX() - ImGui::CalcTextSize(imguiLabel.data()).x, 0.f);
    const int spaceCount = static_cast<int>(freeWidthAfterText / ImGui::CalcTextSize(" ").x);
    imguiLabel.append(spaceCount, ' ');
    imguiLabel.append_sprintf("##res_%d", static_cast<int>(resData));

    const ImGuiTreeNodeFlags selectedFlag = isSelected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;
    if (ImGui::TreeNodeEx(imguiLabel.data(), leafFlags | selectedFlag))
    {
      if (!hidden && ImGui::IsItemClicked(ImGuiMouseButton_Left))
      {
        info.selectionChanged = res_idx != counter;
        res_idx = counter;
        input = knownNames.getName(resData);
        ImGui::CloseCurrentPopup();
        selected_by_mouse = true;
      }
      if (hidden && isSelected)
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

bool Visualizer::treeWithFilter(const char *label, const dag::Vector<eastl::string_view> &data, int &current_idx, eastl::string &input,
  bool return_on_arrows, const char *hint, ImGuiComboFlags flags)
{
  if (data.empty())
  {
    ImGui::TextColored({1, 1, 0, 1}, "No resources");
    return false;
  }

  float textWidth = 0.f;
  eastl::for_each(data.begin(), data.end(),
    [&max = textWidth](const auto &b) { max = eastl::max(max, ImGui::CalcTextSize(b.data()).x); });
  const float buttonWidth = eastl::max(ImGui::CalcTextSize("Show").x, ImGui::CalcTextSize("Hide").x);
  const float padding = ImGui::GetStyle().FramePadding.x + ImGui::GetStyle().WindowPadding.x;
  const float width = textWidth + buttonWidth + ImGui::GetStyle().ScrollbarSize + 2.0f * padding;

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
  drawTree(registry.knownNames.root(), current_idx, counter, selectedByMouse, info, input, textWidth, buttonOffset);
  info.arrowScroll = info.arrowScroll && return_on_arrows;
  return ImGuiDagor::EndComboWithFilter(label, data, current_idx, input, info, selectedByMouse);
}

} // namespace visualization::usergraph

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
      const auto &depData = dafg::Runtime::get().getDependencyData();

      dafg::visualization::usergraph::selectedEdge = nullptr;

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

        update_fg_debug_tex(Selection{ReadTimePoint{}, resId}, registry, depData);
      }
      else
      {
        update_fg_debug_tex({}, registry, depData);
      }
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(dafg_show_fg_tex_console_handler);