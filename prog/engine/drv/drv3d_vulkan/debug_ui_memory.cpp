#if DAGOR_DBGLEVEL > 0
#include "debug_ui.h"
#include <gui/dag_imgui.h>
#include <imgui.h>
#include <implot.h>
#include "device.h"
#include <dag/dag_vector.h>
#include <util/dag_string.h>

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

uint64_t trackingFrameIdx = 0;
bool data_initialized = false;

} // anonymous namespace

#define DEV_MEM()                          \
  Device &drvDev = get_device();           \
  GuardedObjectAutoLock lk(drvDev.memory); \
  DeviceMemoryPool &devMem = drvDev.memory.get();

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

  const PhysicalDeviceSet &devInfo = drvDev.getDeviceProperties();
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
  ImPlot::SetNextPlotLimitsX(trackingFrameIdx < USAGE_HISTORY_LEN ? 0 : trackingFrameIdx - USAGE_HISTORY_LEN, trackingFrameIdx,
    ImGuiCond_Always);
  ImPlot::SetNextPlotLimitsY(0, default_max, ImGuiCond_Once);
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
        heapUsagePlots[i].xData.size(), heapUsagePlots[i].offset);
    ImPlot::EndPlot();
  }
  ImGui::Checkbox("Clean memory every work item", &RenderWork::cleanUpMemoryEveryWorkItem);
  if (ImGui::Button("Trigger soft OOM signal"))
  {
    drvDev.resources.triggerOutOfMemorySignal();
  }
}

void drawBudget()
{
  Device &drvDev = get_device();

  uint32_t freeDeviceLocalMemoryKb = drvDev.getCurrentAvailableMemoryKb();
  if (!freeDeviceLocalMemoryKb)
  {
    ImGui::Text("Budget is not available");
    return;
  }

  budgetPlot.addPoint(trackingFrameIdx, freeDeviceLocalMemoryKb >> 10);

  setPlotLimits();
  if (ImPlot::BeginPlot("##GPU memory budget", nullptr, "Free memory, Mb"))
  {
    ImPlot::PlotLine("GPU mem", &budgetPlot.xData[0], &budgetPlot.yData[0], budgetPlot.xData.size(), budgetPlot.offset);
    ImPlot::EndPlot();
  }
}

void drawUploadInfo()
{
  // find and use one of last finished work items with offset that guarantie we not using this work
  const size_t finishedWorkOffset = MAX_PENDING_WORK_ITEMS + 1;
  const uint64_t defaultYLim = 100;
  Device &drvDev = get_device();
  size_t maxId = 0;
  drvDev.timelineMan.get<TimelineManager::CpuReplay>().enumerate([&maxId](size_t, const RenderWork &ctx) //
    {
      if (maxId < ctx.id)
        maxId = ctx.id;
    });
  const RenderWork *lastWork = nullptr;
  if (maxId < finishedWorkOffset)
    return;
  drvDev.timelineMan.get<TimelineManager::CpuReplay>().enumerate(
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
    ImPlot::PlotLine("Buf", &bufUploadPlot.xData[0], &bufUploadPlot.yData[0], bufUploadPlot.xData.size(), bufUploadPlot.offset);
    ImPlot::PlotLine("Img", &imgUploadPlot.xData[0], &imgUploadPlot.yData[0], imgUploadPlot.xData.size(), imgUploadPlot.offset);
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

  if (ImGui::TreeNode("Upload##3"))
  {
    drawUploadInfo();
    ImGui::TreePop();
  }

  trackingFrameIdx++;
}

#endif