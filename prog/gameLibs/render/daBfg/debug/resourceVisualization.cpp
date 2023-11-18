#include "backendDebug.h"

#include <frontend/internalRegistry.h>

#include <EASTL/array.h>
#include <EASTL/string.h>
#include <EASTL/queue.h>
#include <dag/dag_vector.h>
#include <dag/dag_vectorSet.h>
#include <humanInput/dag_hiKeybIds.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_console.h>
#include <util/dag_string.h>
#include <util/dag_convar.h>
#include <gui/dag_imgui.h>
#include <graphLayouter/graphLayouter.h>
#include <imgui.h>


// NOTE: ImGui does not expose it's math functions for vecs.
// This is ridiculous, but the author himself simply redefines them for demos :(
static inline ImVec2 operator+(const ImVec2 &lhs, const ImVec2 &rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2 &lhs, const ImVec2 &rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
static inline ImVec2 operator/(const ImVec2 &lhs, float rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }

constexpr auto IMGUI_WINDOW_GROUP = "FRAMEGRAPH";
constexpr auto IMGUI_VISUALIZE_LIFETIMES_WINDOW = "Resource Lifetimes##FRAMEGRAPH-lifetimes";


constexpr eastl::pair<ResourceBarrier, const char *> BARRIER_NAMES[]{
  {RB_RW_RENDER_TARGET, "RB_RW_RENDER_TARGET"},
  {RB_RW_UAV, "RB_RW_UAV"},
  {RB_RW_COPY_DEST, "RB_RW_COPY_DEST"},
  {RB_RW_BLIT_DEST, "RB_RW_BLIT_DEST"},
  {RB_RO_SRV, "RB_RO_SRV"},
  {RB_RO_CONSTANT_BUFFER, "RB_RO_CONSTANT_BUFFER"},
  {RB_RO_VERTEX_BUFFER, "RB_RO_VERTEX_BUFFER"},
  {RB_RO_INDEX_BUFFER, "RB_RO_INDEX_BUFFER"},
  {RB_RO_INDIRECT_BUFFER, "RB_RO_INDIRECT_BUFFER"},
  {RB_RO_VARIABLE_RATE_SHADING_TEXTURE, "RB_RO_VARIABLE_RATE_SHADING_TEXTURE"},
  {RB_RO_COPY_SOURCE, "RB_RO_COPY_SOURCE"},
  {RB_RO_BLIT_SOURCE, "RB_RO_BLIT_SOURCE"},
  {RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE, "RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE"},
  {RB_FLAG_RELEASE_PIPELINE_OWNERSHIP, "RB_FLAG_RELEASE_PIPELINE_OWNERSHIP"},
  {RB_FLAG_ACQUIRE_PIPELINE_OWNERSHIP, "RB_FLAG_ACQUIRE_PIPELINE_OWNERSHIP"},
  {RB_FLAG_SPLIT_BARRIER_BEGIN, "RB_FLAG_SPLIT_BARRIER_BEGIN"},
  {RB_FLAG_SPLIT_BARRIER_END, "RB_FLAG_SPLIT_BARRIER_END"},
  {RB_STAGE_VERTEX, "RB_STAGE_VERTEX"},
  {RB_STAGE_PIXEL, "RB_STAGE_PIXEL"},
  {RB_STAGE_COMPUTE, "RB_STAGE_COMPUTE"},
  {RB_STAGE_RAYTRACE, "RB_STAGE_RAYTRACE"},
  {RB_FLUSH_UAV, "RB_FLUSH_UAV"},
  {RB_FLAG_DONT_PRESERVE_CONTENT, "RB_FLAG_DONT_PRESERVE_CONTENT"},
  {RB_SOURCE_STAGE_VERTEX, "RB_SOURCE_STAGE_VERTEX"},
  {RB_SOURCE_STAGE_PIXEL, "RB_SOURCE_STAGE_PIXEL"},
  {RB_SOURCE_STAGE_COMPUTE, "RB_SOURCE_STAGE_COMPUTE"},
  {RB_SOURCE_STAGE_RAYTRACE, "RB_SOURCE_STAGE_RAYTRACE"},
};
static eastl::string nameForBarrier(ResourceBarrier barrier)
{
  eastl::string result;
  for (auto [flag, name] : BARRIER_NAMES)
  {
    if (flag == (barrier & flag))
    {
      result += name;
      result += " | ";
    }
  }

  if (!result.empty())
    result.resize(result.size() - 3);

  return result;
}

static const eastl::vector_map<dabfg::Access, const char *> RES_USAGE_ACCESS_NAMES{
  {dabfg::Access::UNKNOWN, "UNKNOWN"},       //
  {dabfg::Access::READ_ONLY, "READ_ONLY"},   //
  {dabfg::Access::READ_WRITE, "READ_WRITE"}, //
};
static const eastl::vector_map<dabfg::Usage, const char *> RES_USAGE_TYPE_NAMES{
  {dabfg::Usage::UNKNOWN, "UNKNOWN"},
  {dabfg::Usage::COLOR_ATTACHMENT, "COLOR_ATTACHMENT"},
  {dabfg::Usage::INPUT_ATTACHMENT, "INPUT_ATTACHMENT"},
  {dabfg::Usage::DEPTH_ATTACHMENT, "DEPTH_ATTACHMENT"},
  {dabfg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE, "DEPTH_ATTACHMENT_AND_SHADER_RESOURCE"},
  {dabfg::Usage::RESOLVE_ATTACHMENT, "RESOLVE_ATTACHMENT"},
  {dabfg::Usage::SHADER_RESOURCE, "SHADER_RESOURCE"},
  {dabfg::Usage::CONSTANT_BUFFER, "CONSTANT_BUFFER"},
  {dabfg::Usage::INDEX_BUFFER, "INDEX_BUFFER"},
  {dabfg::Usage::VERTEX_BUFFER, "VERTEX_BUFFER"},
  {dabfg::Usage::COPY, "COPY"},
  {dabfg::Usage::BLIT, "BLIT"},
  {dabfg::Usage::INDIRECTION_BUFFER, "INDIRECTION_BUFFER"},
  {dabfg::Usage::VRS_RATE_TEXTURE, "VRS_RATE_TEXTURE"},
};
constexpr eastl::pair<dabfg::Stage, const char *> RES_USAGE_STAGE_NAMES[]{
  {dabfg::Stage::PRE_RASTER, "PRE_RASTER"},   //
  {dabfg::Stage::POST_RASTER, "POST_RASTER"}, //
  {dabfg::Stage::COMPUTE, "COMPUTE"},         //
  {dabfg::Stage::TRANSFER, "TRANSFER"},       //
  {dabfg::Stage::RAYTRACE, "RAYTRACE"},       //
};
static eastl::string name_for_usage_stage(dabfg::Stage stage)
{
  eastl::string result;
  for (auto [flag, name] : RES_USAGE_STAGE_NAMES)
  {
    if (flag == (stage & flag))
    {
      result += name;
      result += " | ";
    }
  }

  if (!result.empty())
    result.resize(result.size() - 3);

  return result;
}

namespace dabfg
{
static constexpr int RES_RECORD_WINDOW = 2; // must be in sync with scheduler, but it's ok if it's not

struct ResourceInfo
{
  eastl::string name;

  int firstUsageNodeIdx = eastl::numeric_limits<int>::max();
  int lastUsageNodeIdx = eastl::numeric_limits<int>::min();

  int heapIndex = -1;
  int offset = 0;
  int size = 0;

  bool createdByRename = false;
  bool consumedByRename = false;

  // Node execution index -> usage
  dag::VectorMap<int, ResourceUsage> usageTimeline;

  eastl::fixed_vector<eastl::pair<int, ResourceBarrier>, 32> barrierEvents;
};

static eastl::array<ska::flat_hash_map<ResNameId, ResourceInfo>, RES_RECORD_WINDOW> per_resource_infos;

static dag::Vector<int> resource_heap_sizes;

static dag::Vector<eastl::string> node_names;

void update_resource_visualization(const InternalRegistry &registry, eastl::span<const NodeNameId> node_execution_order)
{
  for (auto &v : per_resource_infos)
    v.clear();
  resource_heap_sizes.clear();
  node_names.clear();
  node_names.reserve(node_execution_order.size());
  for (auto nodeId : node_execution_order)
    node_names.emplace_back(registry.knownNames.getName(nodeId));

  for (int nodeExecIdx = 0; nodeExecIdx < node_execution_order.size(); ++nodeExecIdx)
  {
    const auto &node = registry.nodes[node_execution_order[nodeExecIdx]];

    for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
    {
      // "Consumer" node is implicit in most cases, we need to recover it
      const auto recordUsage = //
        [frame, nodeExecIdx](ResNameId res) {
          auto &lifetime = per_resource_infos[frame][res];
          lifetime.lastUsageNodeIdx = eastl::max(lifetime.lastUsageNodeIdx, nodeExecIdx);
        };

      for (auto resId : node.createdResources)
      {
        recordUsage(resId);
        per_resource_infos[frame][resId].firstUsageNodeIdx = nodeExecIdx;
      }

      for (auto [to, from] : node.renamedResources)
      {
        per_resource_infos[frame][from].lastUsageNodeIdx = nodeExecIdx;
        recordUsage(to);
        per_resource_infos[frame][to].firstUsageNodeIdx = nodeExecIdx;
        per_resource_infos[frame][from].consumedByRename = true;
        per_resource_infos[frame][to].createdByRename = true;
      }

      for (auto resId : node.readResources)
        recordUsage(resId);

      for (auto resId : node.modifiedResources)
        recordUsage(resId);

      for (auto &[resId, _] : node.historyResourceReadRequests)
      {
        auto &lifetime = per_resource_infos[frame][resId];
        // This is a hack: next frame reads are represented as node ids bigger
        // than the total amount of executed nodes
        const int fakeExecId = nodeExecIdx + node_execution_order.size();
        lifetime.lastUsageNodeIdx = eastl::max(lifetime.lastUsageNodeIdx, fakeExecId);
      }
    }
  }

  for (auto &lifetimes : per_resource_infos)
    for (auto &[id, lifetime] : lifetimes)
    {
      lifetime.name = registry.knownNames.getName(id);

      // Non-allocated resources may have invalid lifetimes
      if (lifetime.heapIndex < 0)
        continue;

      lifetime.usageTimeline.reserve(lifetime.lastUsageNodeIdx - lifetime.firstUsageNodeIdx);
    }

  for (int nodeExecIdx = 0; nodeExecIdx < node_execution_order.size(); ++nodeExecIdx)
  {
    const auto &node = registry.nodes[node_execution_order[nodeExecIdx]];

    for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
    {
      for (const auto &[resId, req] : node.resourceRequests)
        per_resource_infos[frame][resId].usageTimeline[nodeExecIdx] = req.usage;

      for (const auto &[resId, req] : node.historyResourceReadRequests)
        per_resource_infos[frame][resId].usageTimeline[nodeExecIdx + node_execution_order.size()] = req.usage;
    }
  }
}

void debug_rec_resource_placement(ResNameId id, int frame, int heap, int offset, int size)
{
  if (frame >= RES_RECORD_WINDOW)
    return;

  auto &res = per_resource_infos[frame][id];
  res.heapIndex = heap;
  res.offset = offset;
  res.size = size;

  if (resource_heap_sizes.size() <= heap)
    resource_heap_sizes.resize(heap + 1, 0);
  resource_heap_sizes[heap] = eastl::max(resource_heap_sizes[heap], offset + size);
}

void debug_rec_resource_barrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier)
{
  if (res_frame >= RES_RECORD_WINDOW)
    return;
  auto &rep = per_resource_infos[res_frame][res_id];
  const auto frameDelta = (exec_frame - res_frame + RES_RECORD_WINDOW) % RES_RECORD_WINDOW;
  rep.barrierEvents.emplace_back(exec_time + frameDelta * node_names.size(), barrier);
}
} // namespace dabfg

static constexpr float NODE_WINDOW_PADDING = 10.f;

extern ConVarT<bool, false> recompile_graph;

static void visualize_resource_lifetimes()
{
  const bool showTooltips = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
  const size_t nodeCount = dabfg::node_names.size();

  if (nodeCount == 0)
    recompile_graph.set(true);

  static constexpr ImU32 LINE_COLOR = IM_COL32(100, 100, 100, 255);
  static constexpr float VERTICAL_PADDING = 25.0f;
  static constexpr float NODE_HORIZONTAL_HALFSIZE = 50.0f;
  static constexpr float NODE_HORIZONTAL_SIZE = 2.0f * NODE_HORIZONTAL_HALFSIZE;

  static ImVec2 viewScrolling = ImVec2(NODE_HORIZONTAL_HALFSIZE, 0.0f);
  static float viewScaling = 1.0f;
  static float verticalScaling = 400.0f;

  ImDrawList *drawList = ImGui::GetWindowDrawList();

  if (ImGui::Button("Recompile"))
    recompile_graph.set(true);
  ImGui::SameLine();
  if (ImGui::Button("Reset View"))
  {
    viewScrolling = ImVec2(NODE_HORIZONTAL_HALFSIZE, 0.0f);
    viewScaling = 1.0f;
  }
  ImGui::SameLine();
  static bool showBarriers = false;
  ImGui::Checkbox("Show barriers", &showBarriers);
  ImGui::SameLine();

  ImGui::TextUnformatted("Use mouse wheel to pan/zoom. Hold LSHIFT to show usages or change vertical scale.");

  ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true,
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
  ImGui::PushItemWidth(120.0f);

  drawList->PushClipRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), false);

  const ImVec2 initialPos = ImGui::GetCursorScreenPos();
  const ImVec2 viewOffset = initialPos + viewScrolling;

  constexpr float GAP_BETWEEN_FRAMES_SIZE = 400.0f;
  constexpr float GAP_BETWEEN_HEAPS_SIZE = 100.0f;

  const float viewWidth =
    dabfg::RES_RECORD_WINDOW * (NODE_HORIZONTAL_SIZE * nodeCount + GAP_BETWEEN_FRAMES_SIZE) - GAP_BETWEEN_FRAMES_SIZE;

  dag::Vector<float> heapSizeScaling(dabfg::resource_heap_sizes.size());
  for (size_t i = 0; i < heapSizeScaling.size(); ++i)
    heapSizeScaling[i] = verticalScaling / float(dabfg::resource_heap_sizes[i]);


  // Draw background
  for (int frame = 0; frame < dabfg::RES_RECORD_WINDOW; ++frame)
  {
    const float frameOffsetX = frame * (NODE_HORIZONTAL_SIZE * nodeCount + GAP_BETWEEN_FRAMES_SIZE);
    for (int i = 0; i < nodeCount; ++i)
    {
      const auto &nodeName = dabfg::node_names[i];

      const auto nodeRectCenter = ImVec2(viewOffset.x + (frameOffsetX + NODE_HORIZONTAL_SIZE * i) * viewScaling,
        initialPos.y + ImGui::GetWindowHeight() - 2.0f * NODE_WINDOW_PADDING);

      drawList->AddLine(ImVec2(nodeRectCenter.x - NODE_HORIZONTAL_HALFSIZE * viewScaling, initialPos.y),
        ImVec2(nodeRectCenter.x - NODE_HORIZONTAL_HALFSIZE * viewScaling,
          initialPos.y + ImGui::GetWindowHeight() - NODE_WINDOW_PADDING),
        LINE_COLOR);

      if (i == nodeCount - 1)
        drawList->AddLine(ImVec2(nodeRectCenter.x + NODE_HORIZONTAL_HALFSIZE * viewScaling, initialPos.y),
          ImVec2(nodeRectCenter.x + NODE_HORIZONTAL_HALFSIZE * viewScaling,
            initialPos.y + ImGui::GetWindowHeight() - NODE_WINDOW_PADDING),
          LINE_COLOR);

      {
        ImVec2 textSize = ImGui::CalcTextSize(nodeName.c_str());
        textSize.x = eastl::min(textSize.x, NODE_HORIZONTAL_SIZE * viewScaling);

        const ImVec2 nodeRectMin = nodeRectCenter - 0.5f * ImVec2(textSize.x, 0);
        const ImVec2 nodeRectMax = nodeRectMin + textSize;

        ImGui::PushClipRect(nodeRectMin, nodeRectMax, true);
        ImGui::SetCursorScreenPos(nodeRectMin);
        ImGui::TextUnformatted(nodeName.c_str());
        ImGui::PopClipRect();

        drawList->AddRectFilled(nodeRectMin, nodeRectMax, IM_COL32(75, 75, 75, 255), 4.0f);
        drawList->AddRect(nodeRectMin, nodeRectMax, IM_COL32(100, 100, 100, 255), 4.0f);
      }

      {
        const auto numberRectCenter = ImVec2(nodeRectCenter.x, initialPos.y + 2.0f * NODE_WINDOW_PADDING);

        eastl::string num = eastl::to_string(i);
        ImVec2 textSize = ImGui::CalcTextSize(num.c_str());
        textSize.x = eastl::min(textSize.x, NODE_HORIZONTAL_SIZE * viewScaling);

        ImGui::SetCursorScreenPos(numberRectCenter - textSize / 2);
        ImGui::TextUnformatted(num.c_str());
      }
    }

    float verticalOffset = viewOffset.y + VERTICAL_PADDING;
    for (int i = 0; i < dabfg::resource_heap_sizes.size() && frame == 0; ++i)
    {
      drawList->AddLine(ImVec2(initialPos.x, verticalOffset), ImVec2(initialPos.x + ImGui::GetWindowWidth(), verticalOffset),
        LINE_COLOR);
      ImGui::SetCursorScreenPos(ImVec2(initialPos.x, verticalOffset));
      ImGui::Text("heap %i (size %i)", i, dabfg::resource_heap_sizes[i]);

      verticalOffset += dabfg::resource_heap_sizes[i] * heapSizeScaling[i] + GAP_BETWEEN_HEAPS_SIZE;

      if (i == dabfg::resource_heap_sizes.size() - 1)
        drawList->AddLine(ImVec2(initialPos.x, verticalOffset), ImVec2(initialPos.x + ImGui::GetWindowWidth(), verticalOffset),
          LINE_COLOR);
    }
  }

  dabfg::ResNameId hoveredResourceId = dabfg::ResNameId::Invalid;
  int hoveredResourceOwnerFrame = 0;

  // Draw resource rects
  for (int frame = 0; frame < dabfg::RES_RECORD_WINDOW; ++frame)
  {
    const float frameOffsetX = frame * (NODE_HORIZONTAL_SIZE * nodeCount + GAP_BETWEEN_FRAMES_SIZE);

    for (const auto &[resId, info] : dabfg::per_resource_infos[frame])
    {
      // NOTE: some resources might have been skipped when scheduling,
      // don't display them.
      if (info.heapIndex < 0)
        continue;

      float resOffsetY = viewOffset.y + VERTICAL_PADDING;
      for (int i = 0; i < info.heapIndex; ++i)
        resOffsetY += dabfg::resource_heap_sizes[i] * heapSizeScaling[i] + GAP_BETWEEN_HEAPS_SIZE;
      resOffsetY += info.offset * heapSizeScaling[info.heapIndex];

      const float startOffsetForReadability = info.createdByRename ? 0 : -.45f;
      const auto resRectMin = ImVec2(
        viewOffset.x + viewScaling * (frameOffsetX + NODE_HORIZONTAL_SIZE * (info.firstUsageNodeIdx + startOffsetForReadability)),
        resOffsetY);


      // history resources need to span across gaps
      const uint32_t gapSkips = nodeCount > 0 ? info.lastUsageNodeIdx / nodeCount * GAP_BETWEEN_FRAMES_SIZE : 0;
      const float endOffsetForReadability = info.consumedByRename ? 0 : .45f;
      const auto resRectMax =
        ImVec2(viewOffset.x +
                 (frameOffsetX + NODE_HORIZONTAL_SIZE * (info.lastUsageNodeIdx + endOffsetForReadability) + gapSkips) * viewScaling,
          resOffsetY + info.size * heapSizeScaling[info.heapIndex]);

      auto drawRect =
        [&drawList, &hoveredResourceId, &hoveredResourceOwnerFrame, resId = resId, frame, nameCstr = info.name.c_str()] //
        (ImVec2 min, ImVec2 max)                                                                                        //
      {
        ImGui::PushClipRect(min, max, true);
        ImGui::SetCursorScreenPos(min + ImVec2(4.0f, 4.0f));
        ImGui::Text("%s (%i)", nameCstr, frame);
        ImGui::PopClipRect();

        if (ImGui::IsMouseHoveringRect(min, max))
        {
          hoveredResourceId = resId;
          hoveredResourceOwnerFrame = frame;
        }

        drawList->AddRectFilled(min, max, IM_COL32(75, 75, 75, 128), 2.0f);
        drawList->AddRect(min, max, IM_COL32(30, 161, 161, 255), 2.0f);
      };

      if (frame + 1 < dabfg::RES_RECORD_WINDOW || info.lastUsageNodeIdx < nodeCount)
      {
        drawRect(resRectMin, resRectMax);
      }
      else
      {
        // Wraparound resource, broken into two separate rects
        drawRect(ImVec2(viewOffset.x - NODE_HORIZONTAL_SIZE * viewScaling, resRectMin.y),
          ImVec2(resRectMax.x - (viewWidth + GAP_BETWEEN_FRAMES_SIZE) * viewScaling, resRectMax.y));
        drawRect(ImVec2(resRectMin.x, resRectMin.y), ImVec2(viewOffset.x + viewWidth * viewScaling, resRectMax.y));
      }
    }
  }

  if (showBarriers)
  {
    for (int frame = 0; frame < dabfg::RES_RECORD_WINDOW; ++frame)
    {
      const float frameOffsetX = frame * (NODE_HORIZONTAL_SIZE * nodeCount + GAP_BETWEEN_FRAMES_SIZE);

      for (const auto &[name, info] : dabfg::per_resource_infos[frame])
      {
        if (info.heapIndex < 0)
          continue;

        float resOffsetY = viewOffset.y + VERTICAL_PADDING;
        for (int i = 0; i < info.heapIndex; ++i)
          resOffsetY += dabfg::resource_heap_sizes[i] * heapSizeScaling[i] + GAP_BETWEEN_HEAPS_SIZE;
        resOffsetY += info.offset * heapSizeScaling[info.heapIndex];

        for (auto [time, barrier] : info.barrierEvents)
        {
          const uint32_t gapSkips = nodeCount > 0 ? time / nodeCount * GAP_BETWEEN_FRAMES_SIZE : 0;

          const float frameOffsetXWithHistory =
            (frame + 1 < dabfg::RES_RECORD_WINDOW || time < nodeCount) ? frameOffsetX + gapSkips : -NODE_HORIZONTAL_SIZE * nodeCount;

          const float rectMiddle = viewOffset.x + (frameOffsetXWithHistory + NODE_HORIZONTAL_SIZE * (time - .5f)) * viewScaling;
          const float rectBottom = resOffsetY + eastl::max(4, info.size * heapSizeScaling[info.heapIndex]);

          const auto min = ImVec2(rectMiddle - 3.f, resOffsetY);
          const auto max = ImVec2(rectMiddle + 3.f, rectBottom);

          if (showTooltips && ImGui::IsMouseHoveringRect(min, max))
          {
            hoveredResourceId = dabfg::ResNameId::Invalid;
            ImGui::SetTooltip("%s", nameForBarrier(barrier).c_str());
          }

          drawList->AddRectFilled(min, max, IM_COL32(75, 75, 75, 128), 2.0f);
          drawList->AddRect(min, max, IM_COL32(161, 120, 30, 255), 2.0f);
        }
      }
    }
  }

  if (showTooltips && hoveredResourceId != dabfg::ResNameId::Invalid)
  {
    const auto &info = dabfg::per_resource_infos[hoveredResourceOwnerFrame][hoveredResourceId];
    if (ImGui::IsKeyDown(HumanInput::DKEY_LSHIFT))
    {
      const float mouseX = (ImGui::GetMousePos().x - viewOffset.x) / viewScaling + NODE_HORIZONTAL_HALFSIZE;
      const float totalFrameWidth = nodeCount * NODE_HORIZONTAL_SIZE + GAP_BETWEEN_FRAMES_SIZE;
      const int hoverFrame = static_cast<int>(floor(mouseX / totalFrameWidth));
      const int hoverNodeIdx = static_cast<int>(floor((mouseX - hoverFrame * totalFrameWidth) / NODE_HORIZONTAL_SIZE));

      if (hoverFrame < 0 || hoverFrame >= dabfg::RES_RECORD_WINDOW || hoverNodeIdx < 0 || hoverNodeIdx >= nodeCount)
      {
        ImGui::SetTooltip("\"%s\"", info.name.c_str());
      }
      else
      {
        const auto &usageTimeline = dabfg::per_resource_infos[hoveredResourceOwnerFrame][hoveredResourceId].usageTimeline;

        const auto nodeIdx = hoverNodeIdx + (hoveredResourceOwnerFrame != hoverFrame ? nodeCount : 0);

        auto it = usageTimeline.find(nodeIdx);

        if (it == usageTimeline.end())
        {
          ImGui::SetTooltip("\"%s\"\n"
                            "Not used",
            info.name.c_str());
        }
        else
        {
          ImGui::SetTooltip("\"%s\"\n"
                            "Access: %s\n"
                            "Stage: %s\n"
                            "Type: %s\n",
            info.name.c_str(), RES_USAGE_ACCESS_NAMES.find(it->second.access)->second, name_for_usage_stage(it->second.stage).c_str(),
            RES_USAGE_TYPE_NAMES.find(it->second.type)->second);
        }
      }
    }
    else
    {
      ImGui::SetTooltip("\"%s\", frame %i, heap %i, offset %i, size %i", info.name.c_str(), hoveredResourceOwnerFrame, info.heapIndex,
        info.offset, info.size);
    }
  }

  if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive())
  {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
      viewScrolling = viewScrolling + ImGui::GetIO().MouseDelta;

    if (ImGui::GetIO().MouseWheel != 0)
    {
      if (ImGui::IsKeyDown(HumanInput::DKEY_LSHIFT))
      {
        const float prevVertScaling = verticalScaling;
        verticalScaling = eastl::clamp(verticalScaling + 20.f * ImGui::GetIO().MouseWheel, 100.f, 2000.f);

        const float prevAnchor = (ImGui::GetMousePos().y - viewOffset.y) / prevVertScaling;
        const float nextAnchor = (ImGui::GetMousePos().y - viewOffset.y) / verticalScaling;
        viewScrolling.y = viewScrolling.y + verticalScaling * (nextAnchor - prevAnchor);
      }
      else
      {
        const float prevScaling = viewScaling;
        viewScaling = eastl::clamp(viewScaling + 0.05f * ImGui::GetIO().MouseWheel, 0.25f, 2.0f);

        const float prevAnchor = (ImGui::GetMousePos().x - viewOffset.x) / prevScaling;
        const float nextAnchor = (ImGui::GetMousePos().x - viewOffset.x) / viewScaling;
        viewScrolling.x = viewScrolling.x + viewScaling * (nextAnchor - prevAnchor);
      }
    }
  }

  ImGui::EndChild();
}

REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP, IMGUI_VISUALIZE_LIFETIMES_WINDOW, visualize_resource_lifetimes);
