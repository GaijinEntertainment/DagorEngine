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

constexpr uint64_t HEAP_USAGE_HISTORY_LEN = 256;
dag::Vector<UiPlotScrollBuffer<HEAP_USAGE_HISTORY_LEN, ImU64>> heapUsagePlots;
uint64_t maxHeapLimitMb = 0;

constexpr uint64_t BUDGET_USAGE_HISTORY_LEN = 256;
UiPlotScrollBuffer<BUDGET_USAGE_HISTORY_LEN, ImU64> budgetPlot;

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

void setPlotLimits()
{
  ImPlot::SetNextPlotLimitsX(trackingFrameIdx < HEAP_USAGE_HISTORY_LEN ? 0 : trackingFrameIdx - HEAP_USAGE_HISTORY_LEN,
    trackingFrameIdx, ImGuiCond_Always);
  ImPlot::SetNextPlotLimitsY(0, maxHeapLimitMb, ImGuiCond_Once);
}

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

  trackingFrameIdx++;
}

#endif