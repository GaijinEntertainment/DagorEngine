// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if DAGOR_DBGLEVEL > 0
#include "debug_ui.h"
#include <gui/dag_imgui.h>
#include <imgui.h>
#include <implot.h>
#include <dag/dag_vector.h>
#include <util/dag_string.h>
#include "globals.h"
#include "device_memory.h"
#include "resource_manager.h"
#include "render_work.h"
#include "timelines.h"
#include "temp_buffers.h"
#include "device_context.h"

using namespace drv3d_vulkan;

namespace
{

dag::Vector<String> const_info_lines;

constexpr uint64_t USAGE_HISTORY_LEN = 256;
dag::Vector<UiPlotScrollBuffer<USAGE_HISTORY_LEN, ImU64>> heapUsagePlots;
uint64_t maxHeapLimitMb = 0;

UiPlotScrollBuffer<USAGE_HISTORY_LEN, ImU64> budgetPlot;
UiPlotScrollBuffer<USAGE_HISTORY_LEN, ImU64> imgUploadPlot;
UiPlotScrollBuffer<USAGE_HISTORY_LEN, ImU64> bufUploadPlot;

UiPlotScrollBuffer<USAGE_HISTORY_LEN, ImU64> frameMemAllocPlot;
UiPlotScrollBuffer<USAGE_HISTORY_LEN, ImU64> frameMemUsagePlot;

UiPlotScrollBuffer<USAGE_HISTORY_LEN, ImU64> totalVkAllocsPlot;

uint64_t trackingFrameIdx = 0;
bool data_initialized = false;

UiPlotScrollBuffer<USAGE_HISTORY_LEN, ImU64> tempBufferAllocPlot;

} // anonymous namespace

#define DEV_MEM()                      \
  WinAutoLock lk(Globals::Mem::mutex); \
  DeviceMemoryPool &devMem = Globals::Mem::pool;

void initData()
{
  DEV_MEM();

  const char *memCfg = "host shared manual";
  const char *memBudgeting = "budget unaware";
#if !_TARGET_C3
  if (devMem.getMemoryConfiguration() == DeviceMemoryConfiguration::DEDICATED_DEVICE_MEMORY)
    memCfg = "dedicated device memory";
  else if (devMem.getMemoryConfiguration() == DeviceMemoryConfiguration::HOST_SHARED_MEMORY_AUTO)
    memCfg = "host shared auto";
#endif

  const PhysicalDeviceSet &devInfo = Globals::VK::phy;
#if VK_EXT_memory_budget
  if (devInfo.memoryBudgetInfoAvailable)
    memBudgeting = "budget aware";
#endif

  const_info_lines.push_back(String(64, "Config: %s | %s", memCfg, memBudgeting));

  devMem.iterateHeaps([](const DeviceMemoryPool::Heap &heap) {
    if (maxHeapLimitMb < heap.limit)
      maxHeapLimitMb = heap.limit;

    const_info_lines.push_back(String(128, "Heap %u: %s, limit %s | %s", heap.index, byte_size_unit(heap.size),
      byte_size_unit(heap.limit), (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "device local" : "host local"));
    heapUsagePlots.push_back();
  });

  maxHeapLimitMb >>= 20; // convert to Mb

  devMem.iterateTypes([](const DeviceMemoryPool::Type &type, size_t index) {
    const_info_lines.push_back(
      String(128, "Type %u: heap %u, %s(0x%08lX)", index, type.heap->index, formatMemoryTypeFlags(type.flags), type.flags));
  });

  data_initialized = true;
}

void setPlotLimitsVarY(uint64_t default_max)
{
  ImPlot::SetNextAxisLimits(ImAxis_X1, trackingFrameIdx < USAGE_HISTORY_LEN ? 0 : trackingFrameIdx - USAGE_HISTORY_LEN,
    trackingFrameIdx, ImGuiCond_Always);
  ImPlot::SetNextAxisLimits(ImAxis_Y1, 0, default_max, ImGuiCond_Once);
}

void setPlotLimits() { setPlotLimitsVarY(maxHeapLimitMb); }

void drawConstInfo()
{
  for (const String &i : const_info_lines)
    ImGui::TextUnformatted(i.begin(), i.end());
}

void drawUsage()
{
  DEV_MEM();

  devMem.iterateHeaps([](const DeviceMemoryPool::Heap &heap) {
    String inUseS = byte_size_unit(heap.inUse);
    String limitS = byte_size_unit(heap.limit);
    ImGui::Text("Heap %u: %llu%% %s of %s", heap.index, heap.inUse * 100ull / heap.limit, inUseS.c_str(), limitS.c_str());
    heapUsagePlots[heap.index].addPoint(trackingFrameIdx, heap.inUse >> 20);
  });

  setPlotLimits();
  if (ImPlot::BeginPlot("##Heap mem usage", nullptr, "Usage, Mb"))
  {
    for (int i = 0; i < heapUsagePlots.size(); ++i)
      ImPlot::PlotLine(String(32, "Heap %u", i), &heapUsagePlots[i].xData[0], &heapUsagePlots[i].yData[0],
        heapUsagePlots[i].xData.size(), ImPlotLineFlags_None, heapUsagePlots[i].offset);
    ImPlot::EndPlot();
  }
  ImGui::Checkbox("Clean memory every work item", &RenderWork::cleanUpMemoryEveryWorkItem);
  if (ImGui::Button("Trigger soft OOM signal"))
  {
    Globals::Mem::res.triggerOutOfMemorySignal();
  }
}

void drawBudget()
{
  uint32_t freeDeviceLocalMemoryKb = Globals::Mem::pool.getCurrentAvailableDeviceKb();
  if (!freeDeviceLocalMemoryKb)
  {
    ImGui::Text("Budget is not available");
    return;
  }

  budgetPlot.addPoint(trackingFrameIdx, freeDeviceLocalMemoryKb >> 10);

  setPlotLimits();
  if (ImPlot::BeginPlot("##GPU memory budget", nullptr, "Free memory, Mb"))
  {
    ImPlot::PlotLine("GPU mem", &budgetPlot.xData[0], &budgetPlot.yData[0], budgetPlot.xData.size(), ImPlotLineFlags_None,
      budgetPlot.offset);
    ImPlot::EndPlot();
  }
}

void drawUploadInfo()
{
  // find and use one of last finished work items with offset that guarantie we not using this work
  const size_t finishedWorkOffset = MAX_PENDING_REPLAY_ITEMS + 1;
  const uint64_t defaultYLim = 100;
  size_t maxId = 0;
  Globals::timelines.get<TimelineManager::CpuReplay>().enumerate([&maxId](size_t, const RenderWork &ctx) //
    {
      if (maxId < ctx.id)
        maxId = ctx.id;
    });
  const RenderWork *lastWork = nullptr;
  if (maxId < finishedWorkOffset)
    return;
  Globals::timelines.get<TimelineManager::CpuReplay>().enumerate(
    [maxId, finishedWorkOffset, &lastWork](size_t, const RenderWork &ctx) //
    {
      if (ctx.id == (maxId - finishedWorkOffset))
        lastWork = &ctx;
    });

  bufUploadPlot.addPoint(trackingFrameIdx, lastWork->bufferUploadCopies.size());
  imgUploadPlot.addPoint(trackingFrameIdx, lastWork->imageUploadCopies.size());

  setPlotLimitsVarY(defaultYLim);
  if (ImPlot::BeginPlot("##Uploads amounts", nullptr, "Uploads, count"))
  {
    ImPlot::PlotLine("Buf", &bufUploadPlot.xData[0], &bufUploadPlot.yData[0], bufUploadPlot.xData.size(), ImPlotLineFlags_None,
      bufUploadPlot.offset);
    ImPlot::PlotLine("Img", &imgUploadPlot.xData[0], &imgUploadPlot.yData[0], imgUploadPlot.xData.size(), ImPlotLineFlags_None,
      imgUploadPlot.offset);
    ImPlot::EndPlot();
  }
}

void drawFrameMemInfo()
{
  uint32_t alloc, usage;
  Frontend::frameMemBuffers.getMemUsageInfo(alloc, usage);
  frameMemAllocPlot.addPoint(trackingFrameIdx, alloc >> 10);
  frameMemUsagePlot.addPoint(trackingFrameIdx, usage >> 10);

  const uint64_t defaultYLim = 10;
  setPlotLimitsVarY(defaultYLim);
  if (ImPlot::BeginPlot("##Framemem usage & alloc", nullptr, "FrameMem, Kb"))
  {
    ImPlot::PlotLine("Alloc", &frameMemAllocPlot.xData[0], &frameMemAllocPlot.yData[0], frameMemAllocPlot.xData.size(),
      ImPlotLineFlags_None, frameMemAllocPlot.offset);
    ImPlot::PlotLine("Usage, approx", &frameMemUsagePlot.xData[0], &frameMemUsagePlot.yData[0], frameMemUsagePlot.xData.size(),
      ImPlotLineFlags_None, frameMemUsagePlot.offset);
    ImPlot::EndPlot();
  }

  if (ImGui::Button("Purge framemem"))
    Frontend::frameMemBuffers.purge();
}

void drawTempBuffersInfo()
{
  tempBufferAllocPlot.addPoint(trackingFrameIdx, Frontend::tempBuffers.getLastAllocSize() >> 10);

  const uint64_t defaultYLim = 10;
  setPlotLimitsVarY(defaultYLim);
  if (ImPlot::BeginPlot("##TempBuffers alloc", nullptr, "TempBuffers alloc, Kb"))
  {
    ImPlot::PlotLine("Update", &tempBufferAllocPlot.xData[0], &tempBufferAllocPlot.yData[0], tempBufferAllocPlot.xData.size(),
      ImPlotLineFlags_None, tempBufferAllocPlot.offset);
    ImPlot::EndPlot();
  }
}

void drawVkMemAllocsInfo()
{
  DEV_MEM();

  uint32_t newPoint = devMem.getAllocationCounter();
  totalVkAllocsPlot.addPoint(trackingFrameIdx, newPoint);
  ImPlot::SetNextAxisLimits(ImAxis_X1, trackingFrameIdx < USAGE_HISTORY_LEN ? 0 : trackingFrameIdx - USAGE_HISTORY_LEN,
    trackingFrameIdx, ImGuiCond_Always);
  const uint32_t viewDelta = 100;
  ImPlot::SetNextAxisLimits(ImAxis_Y1, newPoint > viewDelta ? (newPoint - viewDelta) : 0, newPoint + viewDelta, ImGuiCond_Always);
  if (ImPlot::BeginPlot("##VkMemAllocsCalls", nullptr, "VK mem alloc calls"))
  {
    ImPlot::PlotLine("Allocs", &totalVkAllocsPlot.xData[0], &totalVkAllocsPlot.yData[0], totalVkAllocsPlot.xData.size(),
      ImPlotLineFlags_None, totalVkAllocsPlot.offset);
    ImPlot::EndPlot();
  }
}

void drv3d_vulkan::debug_ui_memory()
{
  if (!data_initialized)
    initData();

  if (ImGui::TreeNode("Info##1"))
  {
    drawConstInfo();
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Usage##2"))
  {
    drawUsage();
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Budget##3"))
  {
    drawBudget();
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Upload##4"))
  {
    drawUploadInfo();
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("FrameMem##5"))
  {
    drawFrameMemInfo();
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("TempBuffers##6"))
  {
    drawTempBuffersInfo();
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("VkAllocs"))
  {
    drawVkMemAllocsInfo();
    ImGui::TreePop();
  }

  trackingFrameIdx++;
}

#endif