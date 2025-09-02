// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "resourceVisualizer.h"

#include <imgui.h>
#include <gui/dag_imgui.h>
#include <memory/dag_framemem.h>
#include <util/dag_convar.h>


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

static constexpr float NODE_WINDOW_PADDING = 10.f;

constexpr ImU32 LINE_COLOR = IM_COL32(100, 100, 100, 255);
constexpr float VERTICAL_PADDING = 25.0f;

constexpr float GAP_BETWEEN_FRAMES_SIZE = 400.0f;
constexpr float GAP_BETWEEN_HEAPS_SIZE = 100.0f;


namespace dafg
{

extern ConVarT<bool, false> recompile_graph;

namespace visualization
{

ResourseVisualizer::ResourseVisualizer(const InternalRegistry &intRegistry, const intermediate::Graph &irGraph) :
  registry(intRegistry), intermediateGraph(irGraph)
{
  REGISTER_IMGUI_WINDOW(IMGUI_WINDOW_GROUP_FG2, IMGUI_RES_WIN_NAME, [&]() { this->draw(); });
}

void ResourseVisualizer::draw()
{
  if (ImGui::IsWindowCollapsed())
    return;

  showTooltips = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

  if (nodeCount == 0)
    dafg::recompile_graph.set(true);

  drawUI();

  drawCanvas();

  processPopup();
}

void ResourseVisualizer::drawUI()
{
  if (ImGui::Button("Recompile"))
    dafg::recompile_graph.set(true);

  ImGui::SameLine();
  if (ImGui::Button("Reset View"))
  {
    viewScrolling = ImVec2(NODE_HORIZONTAL_HALFSIZE, 0.0f);
    viewScaling = 1.0f;
  }

  ImGui::SameLine();
  ImGui::Checkbox("Show barriers", &showBarriers);

  ImGui::SameLine();
  ImGui::TextUnformatted("Use mouse wheel to pan/zoom. Hold LSHIFT to show usages or change vertical scale.");
}

void ResourseVisualizer::drawCanvas()
{
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  ImGui::BeginChild("scrolling_region", ImVec2(0, 0), ImGuiChildFlags_Border,
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
  ImGui::PushItemWidth(120.0f);

  drawList->PushClipRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), false);

  initialPos = ImGui::GetCursorScreenPos();
  viewOffset = initialPos + viewScrolling;
  viewWidth = RES_RECORD_WINDOW * (NODE_HORIZONTAL_SIZE * nodeCount + GAP_BETWEEN_FRAMES_SIZE) - GAP_BETWEEN_FRAMES_SIZE;

  dag::Vector<float> heapSizeScaling(resourceHeapSizes.size());
  for (size_t i = 0; i < heapSizeScaling.size(); ++i)
    heapSizeScaling[i] = verticalScaling / float(resourceHeapSizes[i]);

  // Draw background
  for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
  {
    const float frameOffsetX = frame * (NODE_HORIZONTAL_SIZE * nodeCount + GAP_BETWEEN_FRAMES_SIZE);
    for (int i = 0; i < nodeCount; ++i)
    {
      const auto &nodeName = nodeNames[i];

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
    for (int i = 0; i < resourceHeapSizes.size() && frame == 0; ++i)
    {
      drawList->AddLine(ImVec2(initialPos.x, verticalOffset), ImVec2(initialPos.x + ImGui::GetWindowWidth(), verticalOffset),
        LINE_COLOR);
      ImGui::SetCursorScreenPos(ImVec2(initialPos.x, verticalOffset));
      ImGui::Text("heap %i (size %i)", i, resourceHeapSizes[i]);

      verticalOffset += resourceHeapSizes[i] * heapSizeScaling[i] + GAP_BETWEEN_HEAPS_SIZE;

      if (i == resourceHeapSizes.size() - 1)
        drawList->AddLine(ImVec2(initialPos.x, verticalOffset), ImVec2(initialPos.x + ImGui::GetWindowWidth(), verticalOffset),
          LINE_COLOR);
    }
  }

  hoveredResourceId = dafg::ResNameId::Invalid;
  hoveredResourceOwnerFrame = 0;

  // Draw resource rects
  for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
  {
    const float frameOffsetX = frame * (NODE_HORIZONTAL_SIZE * nodeCount + GAP_BETWEEN_FRAMES_SIZE);

    for (const auto &[resId, info] : perResourceInfos[frame])
    {
      // NOTE: some resources might have been skipped when scheduling,
      // don't display them.
      if (info.heapIndex < 0)
        continue;

      float resOffsetY = viewOffset.y + VERTICAL_PADDING;
      for (int i = 0; i < info.heapIndex; ++i)
        resOffsetY += resourceHeapSizes[i] * heapSizeScaling[i] + GAP_BETWEEN_HEAPS_SIZE;
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

      auto drawRect = [this, &drawList, resId = resId, frame, nameCstr = info.name.c_str()] //
        (ImVec2 min, ImVec2 max)                                                            //
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

      if (frame + 1 < RES_RECORD_WINDOW || info.lastUsageNodeIdx < nodeCount)
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
    for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
    {
      const float frameOffsetX = frame * (NODE_HORIZONTAL_SIZE * nodeCount + GAP_BETWEEN_FRAMES_SIZE);

      for (const auto &[name, info] : perResourceInfos[frame])
      {
        if (info.heapIndex < 0)
          continue;

        float resOffsetY = viewOffset.y + VERTICAL_PADDING;
        for (int i = 0; i < info.heapIndex; ++i)
          resOffsetY += resourceHeapSizes[i] * heapSizeScaling[i] + GAP_BETWEEN_HEAPS_SIZE;
        resOffsetY += info.offset * heapSizeScaling[info.heapIndex];

        for (auto [time, barrier] : info.barrierEvents)
        {
          const uint32_t gapSkips = nodeCount > 0 ? time / nodeCount * GAP_BETWEEN_FRAMES_SIZE : 0;

          const float frameOffsetXWithHistory =
            (frame + 1 < RES_RECORD_WINDOW || time < nodeCount) ? frameOffsetX + gapSkips : -NODE_HORIZONTAL_SIZE * nodeCount;

          const float rectMiddle = viewOffset.x + (frameOffsetXWithHistory + NODE_HORIZONTAL_SIZE * (time - .5f)) * viewScaling;
          const float rectBottom = resOffsetY + eastl::max(4, info.size * heapSizeScaling[info.heapIndex]);

          const auto min = ImVec2(rectMiddle - 3.f, resOffsetY);
          const auto max = ImVec2(rectMiddle + 3.f, rectBottom);

          if (showTooltips && ImGui::IsMouseHoveringRect(min, max))
          {
            hoveredResourceId = dafg::ResNameId::Invalid;
            ImGui::SetTooltip("%s", nameForBarrier(barrier).c_str());
          }

          drawList->AddRectFilled(min, max, IM_COL32(75, 75, 75, 128), 2.0f);
          drawList->AddRect(min, max, IM_COL32(161, 120, 30, 255), 2.0f);
        }
      }
    }
  }

  processInput();

  ImGui::EndChild();
}

void ResourseVisualizer::processPopup()
{
  if (showTooltips && hoveredResourceId != dafg::ResNameId::Invalid)
  {
    const auto &info = perResourceInfos[hoveredResourceOwnerFrame][hoveredResourceId];
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
    {
      const float mouseX = (ImGui::GetMousePos().x - viewOffset.x) / viewScaling + NODE_HORIZONTAL_HALFSIZE;
      const float totalFrameWidth = nodeCount * NODE_HORIZONTAL_SIZE + GAP_BETWEEN_FRAMES_SIZE;
      const int hoverFrame = static_cast<int>(floor(mouseX / totalFrameWidth));
      const int hoverNodeIdx = static_cast<int>(floor((mouseX - hoverFrame * totalFrameWidth) / NODE_HORIZONTAL_SIZE));

      if (hoverFrame < 0 || hoverFrame >= RES_RECORD_WINDOW || hoverNodeIdx < 0 || hoverNodeIdx >= nodeCount)
      {
        ImGui::SetTooltip("\"%s\"", info.name.c_str());
      }
      else
      {
        const auto &usageTimeline = perResourceInfos[hoveredResourceOwnerFrame][hoveredResourceId].usageTimeline;

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
            info.name.c_str(), dafg::to_string(it->second.access), dafg::to_string(it->second.stage).c_str(),
            dafg::to_string(it->second.type).c_str());
        }
      }
    }
    else
    {
      ImGui::SetTooltip("\"%s\", frame %i, heap %i, offset %i, size %i", info.name.c_str(), hoveredResourceOwnerFrame, info.heapIndex,
        info.offset, info.size);
    }
  }
}

void ResourseVisualizer::processInput()
{
  if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive())
  {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
      viewScrolling = viewScrolling + ImGui::GetIO().MouseDelta;

    if (ImGui::GetIO().MouseWheel != 0)
    {
      if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
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
}

void ResourseVisualizer::updateVisualization()
{
  dag::Vector<NodeNameId, framemem_allocator> nodeExecutionOrder;
  for (const intermediate::Node &irNode : intermediateGraph.nodes)
    if (irNode.frontendNode)
      nodeExecutionOrder.emplace_back(*irNode.frontendNode);
    else
      nodeExecutionOrder.emplace_back(NodeNameId::Invalid);

  for (auto &v : perResourceInfos)
    v.clear();
  resourceHeapSizes.clear();
  nodeNames.clear();
  nodeNames.reserve(nodeExecutionOrder.size());
  for (auto nodeId : nodeExecutionOrder)
    if (nodeId != NodeNameId::Invalid)
      nodeNames.emplace_back(registry.knownNames.getName(nodeId));
    else
      nodeNames.emplace_back("<internal>");

  for (int nodeExecIdx = 0; nodeExecIdx < nodeExecutionOrder.size(); ++nodeExecIdx)
  {
    const auto nodeNameId = nodeExecutionOrder[nodeExecIdx];
    if (nodeNameId == NodeNameId::Invalid)
      continue;

    const auto &node = registry.nodes[nodeNameId];

    for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
    {
      // "Consumer" node is implicit in most cases, we need to recover it
      const auto recordUsage = //
        [this, frame, nodeExecIdx](ResNameId res) {
          auto &lifetime = perResourceInfos[frame][res];
          lifetime.lastUsageNodeIdx = eastl::max(lifetime.lastUsageNodeIdx, nodeExecIdx);
        };

      for (auto resId : node.createdResources)
      {
        recordUsage(resId);
        perResourceInfos[frame][resId].firstUsageNodeIdx = nodeExecIdx;
      }

      for (auto [to, from] : node.renamedResources)
      {
        perResourceInfos[frame][from].lastUsageNodeIdx = nodeExecIdx;
        recordUsage(to);
        perResourceInfos[frame][to].firstUsageNodeIdx = nodeExecIdx;
        perResourceInfos[frame][from].consumedByRename = true;
        perResourceInfos[frame][to].createdByRename = true;
      }

      for (auto resId : node.readResources)
        recordUsage(resId);

      for (auto resId : node.modifiedResources)
        recordUsage(resId);

      for (auto &[resId, _] : node.historyResourceReadRequests)
      {
        auto &lifetime = perResourceInfos[frame][resId];
        // This is a hack: next frame reads are represented as node ids bigger
        // than the total amount of executed nodes
        const int fakeExecId = nodeExecIdx + nodeExecutionOrder.size();
        lifetime.lastUsageNodeIdx = eastl::max(lifetime.lastUsageNodeIdx, fakeExecId);
      }
    }
  }

  for (auto &lifetimes : perResourceInfos)
    for (auto &[id, lifetime] : lifetimes)
    {
      lifetime.name = registry.knownNames.getName(id);

      // Non-allocated resources may have invalid lifetimes
      if (lifetime.heapIndex < 0)
        continue;

      lifetime.usageTimeline.reserve(lifetime.lastUsageNodeIdx - lifetime.firstUsageNodeIdx);
    }

  for (int nodeExecIdx = 0; nodeExecIdx < nodeExecutionOrder.size(); ++nodeExecIdx)
  {
    const auto nodeNameId = nodeExecutionOrder[nodeExecIdx];
    if (nodeNameId == NodeNameId::Invalid)
      continue;

    const auto &node = registry.nodes[nodeNameId];

    for (int frame = 0; frame < RES_RECORD_WINDOW; ++frame)
    {
      for (const auto &[resId, req] : node.resourceRequests)
        perResourceInfos[frame][resId].usageTimeline[nodeExecIdx] = req.usage;

      for (const auto &[resId, req] : node.historyResourceReadRequests)
        perResourceInfos[frame][resId].usageTimeline[nodeExecIdx + nodeExecutionOrder.size()] = req.usage;
    }
  }

  for (const auto [id, frame, heap, offset, size] : resourcePlacemantEntries)
  {
    if (frame >= RES_RECORD_WINDOW)
      continue;

    auto &res = perResourceInfos[frame][id];
    res.heapIndex = heap;
    res.offset = offset;
    res.size = size;

    if (resourceHeapSizes.size() <= heap)
      resourceHeapSizes.resize(heap + 1, 0);
    resourceHeapSizes[heap] = eastl::max(resourceHeapSizes[heap], offset + size);
  }

  for (const auto [res_id, res_frame, exec_time, exec_frame, barrier] : resourceBarrierEntries)
  {
    if (res_frame >= RES_RECORD_WINDOW)
      continue;
    auto &rep = perResourceInfos[res_frame][res_id];
    const auto frameDelta = (exec_frame - res_frame + RES_RECORD_WINDOW) % RES_RECORD_WINDOW;
    rep.barrierEvents.emplace_back(exec_time + frameDelta * nodeNames.size(), barrier);
  }

  nodeCount = nodeNames.size();
}

void ResourseVisualizer::clearResourcePlacements() { resourcePlacemantEntries.clear(); }

void ResourseVisualizer::recResourcePlacement(ResNameId id, int frame, int heap, int offset, int size)
{
  resourcePlacemantEntries.push_back({id, frame, heap, offset, size});
}

void ResourseVisualizer::clearResourceBarriers() { resourceBarrierEntries.clear(); }

void ResourseVisualizer::recResourceBarrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier)
{
  resourceBarrierEntries.push_back({res_id, res_frame, exec_time, exec_frame, barrier});
}

} // namespace visualization

} // namespace dafg