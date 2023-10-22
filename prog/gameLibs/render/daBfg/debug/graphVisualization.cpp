#include "backendDebug.h"
#include "textureVisualization.h"

#include <EASTL/array.h>
#include <EASTL/string.h>
#include <EASTL/queue.h>
#include <EASTL/numeric.h>
#include <dag/dag_vector.h>
#include <dag/dag_vectorSet.h>

#include <backend.h>
#include <nodes/nodeTracker.h>

#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_console.h>
#include <util/dag_string.h>
#include <util/dag_convar.h>
#include <util/dag_stlqsort.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <graphLayouter/graphLayouter.h>
#include <imgui.h>
#include <folders/folders.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui-node-editor/imgui_canvas.h>
#include <imgui-node-editor/imgui_bezier_math.h>

constexpr auto IMGUI_WINDOW_GROUP = "FRAMEGRAPH";
constexpr auto IMGUI_VISUALIZE_DEPENDENCIES_WINDOW = "Node Dependencies##FRAMEGRAPH-dependencies";

static dabfg::NodeHandle debugTextureCopyNode;
static UniqueTex copiedTexture;

namespace dabfg
{
enum class EdgeType
{
  EXPLICIT_PREVIOUS,
  EXPLICIT_FOLLOW,
  IMPLICIT_RES_DEP,
  IMPLICIT_MOD_CHAIN,
  VISIBLE_COUNT,
  INVISIBLE = VISIBLE_COUNT, // Use for some implicit ordering in visualization
  COUNT
};

struct EdgeTypeDebugInfo
{
  eastl::string idName;
  eastl::string readableName;
  // Color string as per dotfile format
  eastl::string dotColor;
  uint32_t imguiColor;
  bool visible = true;
};

static eastl::array<EdgeTypeDebugInfo, static_cast<size_t>(EdgeType::VISIBLE_COUNT)> edge_type_debug_infos{
  EdgeTypeDebugInfo{"EXPLICIT_PREVIOUS", "Precedes", "red", IM_COL32(255, 0, 0, 255)},
  EdgeTypeDebugInfo{"EXPLICIT_FOLLOW", "Follows", "blue", IM_COL32(0, 0, 255, 255)},
  EdgeTypeDebugInfo{"IMPLICIT_RES_DEP", "Uses Resource Of", "purple", IM_COL32(255, 0, 255, 255)},
  EdgeTypeDebugInfo{"IMPLICIT_MOD_CHAIN", "Modification Chain", "darkviolet", IM_COL32(148, 0, 211, 255)}};

static const EdgeTypeDebugInfo &infoForEdgeType(EdgeType type) { return edge_type_debug_infos[static_cast<size_t>(type)]; }

struct EdgeData
{
  NodeIdx destination;
  EdgeType type;
  ResNameId resId;
};

static dag::Vector<dag::Vector<EdgeData>> labeled_graph;
static dag::Vector<NodeNameId> nodeIds;
static dag::Vector<ResNameId> resIds;
static dag::Vector<eastl::string_view> nodeNames;
static dag::Vector<eastl::string_view> resNames;

// Used for imgui visualization
static dag::Vector<dag::Vector<uint32_t>> node_table;
// We need fake nodes and fake edges when visualizing with imgui,
// which changes the adjacency list.
struct VisualizedEdge
{
  float thickness = 0.0f;
  eastl::fixed_vector<EdgeType, 4> types;
  eastl::fixed_vector<ResNameId, 8> resIds;
  NodeIdx source;
  NodeIdx destination;
};
static dag::Vector<dag::Vector<eastl::pair<uint32_t, VisualizedEdge>>> visualized_adjacency_list;

bool visualization_data_generated() { return visualized_adjacency_list.size() >= nodeIds.size(); }

static bool node_is_fake(uint32_t node) { return node >= nodeIds.size(); }

static constexpr uint32_t CULLED_OUT_NODE = static_cast<uint32_t>(-1);
static void add_debug_edge(uint32_t from, uint32_t to, EdgeType type, ResNameId res_id)
{
  if (from != CULLED_OUT_NODE && to != CULLED_OUT_NODE)
    labeled_graph[from].push_back({to, type, res_id});
}

static struct DebugVisualizer
{
  const NodeTracker *nodeTracker = nullptr;

  void collectValidNodes(eastl::span<const NodeNameId> node_execution_order)
  {
    nodeIds.assign(node_execution_order.begin(), node_execution_order.end());
  }

  void generateDependencyEdges(eastl::span<const NodeNameId> node_execution_order)
  {
    resIds.clear();
    labeled_graph.clear();
    labeled_graph.resize(nodeIds.size());
    if (!nodeTracker)
      return;

    IdIndexedMapping<NodeNameId, uint32_t> executionTimeForNode(nodeTracker->registry.nodes.size(), CULLED_OUT_NODE);
    for (size_t i = 0; i < node_execution_order.size(); ++i)
      executionTimeForNode[node_execution_order[i]] = i;

    for (auto [nodeId, nodeData] : nodeTracker->registry.nodes.enumerate())
    {
      for (const NodeNameId prevId : nodeData.precedingNodeIds)
        if (executionTimeForNode.isMapped(prevId))
          add_debug_edge(executionTimeForNode[prevId], executionTimeForNode[nodeId], EdgeType::EXPLICIT_PREVIOUS, ResNameId::Invalid);

      for (const NodeNameId nextId : nodeData.followingNodeIds)
        if (executionTimeForNode.isMapped(nextId))
          add_debug_edge(executionTimeForNode[nodeId], executionTimeForNode[nextId], EdgeType::EXPLICIT_FOLLOW, ResNameId::Invalid);
    }

    for (auto [resId, lifetime] : nodeTracker->resourceLifetimes.enumerate())
    {
      if (lifetime.introducedBy == NodeNameId::Invalid || executionTimeForNode[lifetime.introducedBy] == CULLED_OUT_NODE)
        continue;

      eastl::fixed_vector<NodeNameId, 32> modifiers(lifetime.modificationChain.begin(), lifetime.modificationChain.end());

      eastl::erase_if(modifiers, [&](NodeNameId id) { return executionTimeForNode[id] == CULLED_OUT_NODE; });

      eastl::sort(modifiers.begin(), modifiers.end(), [&executionTimeForNode](NodeNameId first, NodeNameId second) {
        return executionTimeForNode[first] < executionTimeForNode[second];
      });

      if (!modifiers.empty())
        add_debug_edge(executionTimeForNode[lifetime.introducedBy], executionTimeForNode[modifiers.front()],
          EdgeType::IMPLICIT_MOD_CHAIN, resId);

      for (size_t i = 0; i + 1 < modifiers.size(); ++i)
        add_debug_edge(executionTimeForNode[modifiers[i]], executionTimeForNode[modifiers[i + 1]], EdgeType::IMPLICIT_MOD_CHAIN,
          resId);

      auto finalModifier = !modifiers.empty() ? modifiers.back() : lifetime.introducedBy;

      if (lifetime.consumedBy != NodeNameId::Invalid)
      {
        add_debug_edge(executionTimeForNode[finalModifier], executionTimeForNode[lifetime.consumedBy], EdgeType::IMPLICIT_MOD_CHAIN,
          resId);
        for (auto reader : lifetime.readers)
          add_debug_edge(executionTimeForNode[reader], executionTimeForNode[lifetime.consumedBy], EdgeType::INVISIBLE, resId);
      }


      for (auto reader : lifetime.readers)
        add_debug_edge(executionTimeForNode[finalModifier], executionTimeForNode[reader], EdgeType::IMPLICIT_RES_DEP, resId);

      if (!modifiers.empty() || lifetime.consumedBy != NodeNameId::Invalid || !lifetime.readers.empty())
        resIds.push_back(resId);
    }
    stlsort::sort(resIds.begin(), resIds.end(),
      [&names = nodeTracker->registry.knownNames](ResNameId fst, ResNameId snd) { return names.getName(fst) < names.getName(snd); });
    nodeNames = gatherNames<NodeNameId>(nodeIds);
    resNames = gatherNames<ResNameId>(resIds);
  }

  template <class EnumType>
  eastl::string_view getName(EnumType res_id)
  {
    return nodeTracker->registry.knownNames.getName(res_id);
  }

  template <class EnumType>
  dag::Vector<eastl::string_view> gatherNames(eastl::span<const EnumType> ids)
  {
    dag::Vector<eastl::string_view> names;
    names.reserve(ids.size());
    for (auto id : ids)
      names.push_back(getName(id));
    return names;
  }
} visualizer;

static auto layouter = make_default_layouter();

static void generate_planarized_layout()
{
  if (imgui_window_is_visible(IMGUI_WINDOW_GROUP, IMGUI_VISUALIZE_DEPENDENCIES_WINDOW))
  {
    node_table.clear();
    visualized_adjacency_list.clear();

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

    node_table = std::move(layout.nodeLayout);

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
            visEdge.types.push_back(edge.type);
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
}

void update_graph_visualization(const NodeTracker *node_tracker, eastl::span<const NodeNameId> node_execution_order)
{
  visualizer.nodeTracker = node_tracker;
  if (node_tracker && debugTextureCopyNode)
  {
    dag::Vector<NodeNameId> new_node_execution_order;
    new_node_execution_order.assign(node_execution_order.begin(), node_execution_order.end());
    NodeNameId debugNodeNameId =
      node_tracker->registry.knownNames.getNameId<NodeNameId>(node_tracker->registry.knownNames.root(), "debug_texture_copy_node");
    erase_item_by_value(new_node_execution_order, debugNodeNameId);
    visualizer.collectValidNodes(new_node_execution_order);
    visualizer.generateDependencyEdges(new_node_execution_order);
  }
  else
  {
    visualizer.collectValidNodes(node_execution_order);
    visualizer.generateDependencyEdges(node_execution_order);
  }
  generate_planarized_layout();
}

void invalidate_graph_visualization()
{
  update_graph_visualization(nullptr, {});
  copiedTexture.close();
}

void reset_texture_visualization() { debugTextureCopyNode = {}; }

void Backend::dumpGraph(const eastl::string &filename) const
{
  dag::Vector<String> nodeLabels;
  nodeLabels.reserve(labeled_graph.size());
  for (size_t i = 0; i < labeled_graph.size(); ++i)
    nodeLabels.emplace_back(String(0, "node_%d", i));


  FullFileSaveCB cb(filename.c_str(), DF_WRITE | DF_CREATE);

  const eastl::string GRAPH_BEGIN = "digraph dabfg {\n";
  cb.write(GRAPH_BEGIN.data(), GRAPH_BEGIN.size());

  for (size_t i = 0; i < labeled_graph.size(); ++i)
  {
    String nodeDecl(0, "  %s [label=\"%s\"]\n", nodeLabels[i], registry.knownNames.getName(nodeIds[i]));
    cb.write(nodeDecl.str(), nodeDecl.size() - 1);
  }


  for (size_t i = 0; i < labeled_graph.size(); ++i)
  {
    for (size_t j = 0; j < labeled_graph[i].size(); ++j)
    {
      String edge(0, "  %s -> %s [color=\"%s\"]\n", nodeLabels[i], nodeLabels[labeled_graph[i][j].destination],
        infoForEdgeType(labeled_graph[i][j].type).dotColor);

      cb.write(edge.str(), edge.size() - 1);
    }
  }

  // Hacky (graphviz-wise) way to add a legend to a graph.
  {
    const eastl::string LEGEND_BEGIN = "  subgraph cluster_01 {\n"
                                       "    label = \"Legend\";\n"
                                       "    node [shape=point] {\n"
                                       "      rank=same;";
    cb.write(LEGEND_BEGIN.data(), LEGEND_BEGIN.size());

    for (const auto &edgeTypeInfo : edge_type_debug_infos)
    {
      for (size_t i = 0; i < 2; ++i)
      {
        String node(0, "      %s%d [style = invis];\n", edgeTypeInfo.idName, i);
        cb.write(node.str(), node.size() - 1);
      }
    }

    const eastl::string LEGEND_MIDDLE = "    }\n";
    cb.write(LEGEND_MIDDLE.data(), LEGEND_MIDDLE.size());

    for (const auto &edgeTypeInfo : edge_type_debug_infos)
    {
      String edge(0, "    %s0 -> %s1 [label=\"%s\" color=\"%s\"]\n", edgeTypeInfo.idName, edgeTypeInfo.idName,
        edgeTypeInfo.readableName, edgeTypeInfo.dotColor);
      cb.write(edge.str(), edge.size() - 1);
    }

    const eastl::string LEGEND_END = "  }\n";
    cb.write(LEGEND_END.data(), LEGEND_END.size());
  }

  const eastl::string GRAPH_END = "}\n";
  cb.write(GRAPH_END.data(), GRAPH_END.size() - 1);
}
} // namespace dabfg

static bool frame_graph_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("dabfg", "dump_graph", 1, 1)
  {
    const eastl::string FILENAME = "dabfg.dot";
    dabfg::Backend::get().dumpGraph(FILENAME);
    console::print_d("Frame graph saved to %s", FILENAME.c_str());
  }
  return found;
}

static constexpr float NODE_WINDOW_PADDING = 10.f;
static constexpr float NODE_HORIZONTAL_MARGIN = 80.f;

extern ConVarT<bool, false> recompile_graph;

static const float scale_levels[] = {
  0.1f, 0.15f, 0.20f, 0.25f, 0.33f, 0.5f, 0.75f, 1.0f, 1.25f, 1.50f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
static int current_scale_level = 7; // 1.0 scale

static constexpr ImU32 SELECTION_COLOR = IM_COL32(255, 225, 150, 255);

static void visualize_framegraph_dependencies()
{
  if (!dabfg::visualization_data_generated())
    recompile_graph.set(true);

  // TODO: This code can be generalized and reused elsewhere

  ImDrawList *drawList = ImGui::GetWindowDrawList();

  // A lot of code comes from mr ocornut himself
  // https://gist.github.com/ocornut/7e9b3ec566a333d725d4

  if (ImGui::Button("Recompile"))
    recompile_graph.set(true);

  bool focusedNodeChanged = false;
  bool focusedResourceChanged = false;
  static int focusedNodeId = -1;
  static dabfg::ResNameId focusedResId = dabfg::ResNameId::Invalid;
  static eastl::string nodeSearchInput;
  static eastl::string resourceSearchInput;
  bool searchBoxHovered = !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

  ImGui::SameLine();
  if (ImGuiDagor::ComboWithFilter("Execution Order / Node Search", dabfg::nodeNames, focusedNodeId, nodeSearchInput, true, true,
        "Search for node..."))
  {
    focusedNodeChanged = true;
    resourceSearchInput.clear();
    focusedResId = dabfg::ResNameId::Invalid;
  }
  searchBoxHovered = searchBoxHovered || ImGui::IsItemHovered();

  ImGui::SameLine();
  static int focusedResourceNo = -1;
  if (ImGuiDagor::ComboWithFilter("FG Managed Resource Search", dabfg::resNames, focusedResourceNo, resourceSearchInput, false, true,
        "Search for resource..."))
  {
    focusedResourceChanged = true;
    nodeSearchInput.clear();
    focusedNodeId = -1;
    focusedResId = focusedResourceNo > -1 ? dabfg::resIds[focusedResourceNo] : dabfg::ResNameId::Invalid;
  }
  searchBoxHovered = searchBoxHovered || ImGui::IsItemHovered();

  ImGui::SameLine();
  ImGui::Text("Use mouse wheel button to pan. Edge types: ");

  for (auto &info : dabfg::edge_type_debug_infos)
  {
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(info.imguiColor));
    ImGui::Checkbox(info.readableName.c_str(), &info.visible);
    ImGui::PopStyleColor();
  }

  static dabfg::VisualizedEdge *selectedEdge = nullptr;
  static dabfg::NodeNameId selectedEdgePrecedingNode = dabfg::NodeNameId::Invalid;
  static dabfg::ResNameId selectedResId = dabfg::ResNameId::Invalid;

  if (!dabfg::visualizer.nodeTracker)
    return;

  auto &registry = dabfg::visualizer.nodeTracker->registry;
  fg_texture_visualization_imgui_line(selectedResId, registry, selectedEdgePrecedingNode);

  static ImGuiEx::Canvas canvas;
  static ImVec2 viewOrigin = ImVec2(0.0f, 0.0f);
  ImVec2 centerView = ImVec2(0.0f, 0.0f);
  String tooltipMessage;

  const ImVec2 canvasMousePos = canvas.ToLocal(ImGui::GetIO().MousePos);
  const bool canvasHovered = canvas.ViewRect().Contains(canvasMousePos);

  if (!canvas.Begin("##scrolling_region", ImVec2(0, 0)))
    return;

  // Layer 0 for edges, 1 for nodes, 2 for text
  drawList->ChannelsSplit(3);

  // (with fake nodes)
  const size_t node_count = dabfg::visualized_adjacency_list.size();

  // Used further below to draw edges
  dag::Vector<ImVec2> nodeLeftAnchors(node_count);
  dag::Vector<ImVec2> nodeRightAnchors(node_count);

  float currentHorizontalOffset = 0;

  for (const auto &column : dabfg::node_table)
  {
    float currentMaxWidth = 0;
    for (uint32_t subrank = 0; subrank < column.size(); ++subrank)
    {
      auto nodeId = column[subrank];

      const auto verticalPos = column.size() == 1 ? 0 : ((static_cast<float>(subrank) / static_cast<float>(column.size() - 1)) - 0.5f);
      const auto nodeRectMin = ImVec2(currentHorizontalOffset, verticalPos * 1000.f);

      const bool real = !dabfg::node_is_fake(nodeId);

      if (real)
      {
        drawList->ChannelsSetCurrent(2);

        ImGui::SetCursorScreenPos(nodeRectMin + ImVec2(NODE_WINDOW_PADDING, NODE_WINDOW_PADDING));
        ImGui::BeginGroup();

        auto nodeName = registry.knownNames.getName(dabfg::nodeIds[nodeId]);
        auto &nodeData = registry.nodes[dabfg::nodeIds[nodeId]];

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

        ImU32 nodeBorderColor = IM_COL32(100, 100, 100, 255);
        float nodeBorderThickness = 1.f;
        if (nodeId == focusedNodeId)
        {
          nodeBorderColor = SELECTION_COLOR;
          nodeBorderThickness = 4.f;
          centerView = nodeRectMin + nodeSize / 2.f;
        }

        drawList->ChannelsSetCurrent(1);
        drawList->AddRectFilled(nodeRectMin, nodeRectMax, IM_COL32(75, 75, 75, 255), 4.0f);
        drawList->AddRect(nodeRectMin, nodeRectMax, nodeBorderColor, 4.0f, 0, nodeBorderThickness);

        nodeLeftAnchors[nodeId] = nodeRectMin + ImVec2(0, nodeSize.y / 2);
        nodeRightAnchors[nodeId] = nodeRectMax - ImVec2(0, nodeSize.y / 2);

        currentMaxWidth = eastl::max(currentMaxWidth, nodeSize.x);
      }
      else
      {
        nodeLeftAnchors[nodeId] = nodeRightAnchors[nodeId] = nodeRectMin;
      }
    }
    currentHorizontalOffset += currentMaxWidth + NODE_HORIZONTAL_MARGIN;
  }

  ImVec2 focusedResourceLeftmostEdgeStart = ImVec2(HUGE_VALF, HUGE_VALF);
  ImVec2 focusedResourceLeftmostEdgeEnd = ImVec2(HUGE_VALF, HUGE_VALF);
  drawList->ChannelsSetCurrent(0);
  for (uint32_t from = 0; from < dabfg::visualized_adjacency_list.size(); ++from)
  {
    for (auto &[to, edge] : dabfg::visualized_adjacency_list[from])
    {
      ImCubicBezierPoints spline = {nodeRightAnchors[from], nodeRightAnchors[from] + ImVec2(NODE_HORIZONTAL_MARGIN, 0),
        nodeLeftAnchors[to] - ImVec2(NODE_HORIZONTAL_MARGIN, 0), nodeLeftAnchors[to]};

      bool hasFocusedResId = false;
      if (focusedResId != dabfg::ResNameId::Invalid)
        hasFocusedResId = eastl::find(edge.resIds.begin(), edge.resIds.end(), focusedResId) != edge.resIds.end();
      if (focusedResourceChanged && hasFocusedResId && spline.P0.x < focusedResourceLeftmostEdgeStart.x)
      {
        focusedResourceLeftmostEdgeStart = spline.P0;
        focusedResourceLeftmostEdgeEnd = spline.P3;
      }

      float thickness = edge.thickness / edge.types.size();
      ImVec2 splineOffset = ImVec2(0.0, thickness * (edge.types.size() - 1) * 0.5);
      for (dabfg::EdgeType type : edge.types)
      {
        auto &info = infoForEdgeType(type);
        if (!info.visible)
          continue;
        auto addBezierToDrawList = [&](ImU32 color, float line_thickness) {
          drawList->AddBezierCubic(splineOffset + spline.P0, splineOffset + spline.P1, splineOffset + spline.P2,
            splineOffset + spline.P3, color, line_thickness);
        };

        if (hasFocusedResId)
          addBezierToDrawList(SELECTION_COLOR, 1.2f * thickness);
        addBezierToDrawList(info.imguiColor, thickness);
        if (hasFocusedResId)
          addBezierToDrawList(SELECTION_COLOR, 0.5f * thickness + 0.3f);

        splineOffset -= ImVec2(0.0, thickness);
      }

      if (canvasHovered && canvasMousePos.x >= spline.P0.x && canvasMousePos.x <= spline.P3.x)
      {
        auto hit = ImProjectOnCubicBezier(canvasMousePos, spline);
        if (hit.Distance < edge.thickness * 0.5)
        {
          if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
          {
            selectedEdge = &edge;
            canvas.Suspend();
            ImGui::OpenPopup("edge_resources");
            canvas.Resume();
            selectedResId = dabfg::ResNameId::Invalid;
            selectedEdgePrecedingNode = dabfg::nodeIds[edge.source];
          }

          if (!selectedEdge)
            for (auto resId : edge.resIds)
              tooltipMessage.aprintf(0, "%s\n", dabfg::visualizer.getName(resId));
        }
      }
    }
  }

  drawList->ChannelsMerge();
  canvas.End();

  if (ImGui::BeginPopup("edge_resources"))
  {
    if (ImGui::Selectable("<None>", true))
    {
      selectedResId = dabfg::ResNameId::Invalid;
      selectedEdgePrecedingNode = dabfg::NodeNameId::Invalid;
    }
    for (size_t i = 0; i < selectedEdge->resIds.size(); ++i)
    {
      auto tmpResId = selectedEdge->resIds[i];
      auto label = dabfg::visualizer.getName(tmpResId).data();
      if (ImGui::Selectable(label))
      {
        selectedResId = tmpResId;
        nodeSearchInput.clear();
        resourceSearchInput.clear();
        focusedNodeId = -1;
        focusedResId = dabfg::ResNameId::Invalid;
      }
    }
    ImGui::EndPopup();
  }
  else
  {
    selectedEdge = nullptr;
    if (tooltipMessage.length() && !searchBoxHovered)
    {
      ImGui::BeginTooltip();
      ImGui::TextUnformatted(tooltipMessage);
      ImGui::EndTooltip();
    }
  }

  dabfg::NodeNameId *selectedEdgeFollowingNode = nullptr;
  if (selectedEdge)
    selectedEdgeFollowingNode = &dabfg::nodeIds[selectedEdge->destination];
  update_fg_debug_tex(selectedEdgePrecedingNode, selectedEdgeFollowingNode, selectedResId, copiedTexture, registry,
    debugTextureCopyNode);

  if (focusedNodeChanged && focusedNodeId > -1)
  {
    canvas.CenterView(centerView);
    viewOrigin = canvas.ViewOrigin();
  }
  else if (focusedResourceChanged && focusedResId != dabfg::ResNameId::Invalid)
  {
    centerView = (focusedResourceLeftmostEdgeStart + focusedResourceLeftmostEdgeEnd) / 2.f;
    canvas.CenterView(centerView);
    viewOrigin = canvas.ViewOrigin();
  }

  if (canvasHovered)
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

REGISTER_CONSOLE_HANDLER(frame_graph_console_handler);

REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP, IMGUI_VISUALIZE_DEPENDENCIES_WINDOW, visualize_framegraph_dependencies);
