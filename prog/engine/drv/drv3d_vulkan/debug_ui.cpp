// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if DAGOR_DBGLEVEL > 0
#include "debug_ui.h"

#include <EASTL/sort.h>
#include <dag/dag_vector.h>
#include <gui/dag_imgui.h>
#include <perfMon/dag_graphStat.h>

#include "imgui.h"
#include "implot.h"
#include "globals.h"
#include "pipeline/manager.h"
#include "device_context.h"
#include "execution_timings.h"
#include "backend.h"
#include "backend_interop.h"

using namespace drv3d_vulkan;

namespace
{
constexpr uint32_t TOTAL_TIMINGS = 5;
constexpr uint32_t MAX_TIMELINES = 10;

constexpr uint32_t FRAME_TIMINGS_HISTORY = 256;
enum
{
  TIMING_PRESENT_TO_PRESENT = 0,
  TIMING_PRESENT_TO_WORKER_PRESENT = 1,
  TIMING_WAIT_FOR_WORKER = 2,
  TIMING_WORKER_WAIT = 3,
  TIMING_GPU_FENCE_WAIT = 4,
  TIMING_PRESENT_SUBMIT = 5,
  TIMING_SWAPCHAIN_ACQUIRE = 6,
  TIMING_LATENCY_GPU_WAIT = 7,
  FRAME_TIMINGS = 8
};

const char *timingsStrings[FRAME_TIMINGS] = {"PresentToPresent", "PresentToWorkerPresent", "WaitForWorker", "WorkerWait",
  "GPUFenceWait", "PresentSubmit", "SwapchainAcquire", "LatencyGPUWait"};
UiPlotScrollBuffer<FRAME_TIMINGS_HISTORY, ImU64> frameTimingsGraphs[FRAME_TIMINGS];
uint64_t trackingFrameIdx = 0;

ImS64 timelineCpuGraph[TOTAL_TIMINGS * MAX_TIMELINES];
ImS64 timelineGpuGraph[TOTAL_TIMINGS * MAX_TIMELINES];

struct FrameHashHistoryEntry
{
  uint64_t hash;
  uint64_t cnt;
};
Tab<FrameHashHistoryEntry> frameHashesHistory;
bool freezeFrameHashHistory = false;
constexpr uint32_t MAX_FRAME_HASHES_HISTORY_SIZE = 64;

DrawStatSingle stat3dRecord = {};

namespace pipeline_ui
{
struct DebugInfo
{
  dag::Vector<String> shaderNames;
  int64_t totalCompilationTime;
  size_t variantCount;
  bool enabled;
  ProgramID program;
};

dag::Vector<DebugInfo> debugInfos; // guarded by mutex
WinCritSec mutex;

eastl::array<char, 512> searchString{};

struct ProgramTypeData
{
  static constexpr uint32_t TYPE_NONE = static_cast<uint32_t>(-1);

  const char *displayText = "?";
  uint32_t type = TYPE_NONE;
  bool show = true;
};

eastl::array<ProgramTypeData, 1 << program_type_bits> progTypeDatas{
  ProgramTypeData{"G", program_type_graphics},
  ProgramTypeData{"C", program_type_compute},
};
} // namespace pipeline_ui

const char *timeline_plot_legend[] = {"acquire", "submit", "process", "wait", "cleanup"};

void drawFrameHashesHistory()
{
  if (!freezeFrameHashHistory)
  {
    uint64_t currentHashes[GPU_TIMELINE_HISTORY_SIZE];

    Globals::timelines.get<TimelineManager::GpuExecute>().enumerate([&](size_t index, const FrameInfo &ctx) //
      { currentHashes[index] = ctx.execTracker.getPrevJobHash(); });

    for (uint64_t &itr : currentHashes)
      for (FrameHashHistoryEntry &cmp : frameHashesHistory)
      {
        if (cmp.hash == itr)
        {
          itr = 0;
          ++cmp.cnt;
          break;
        }
      }

    for (int i = 0; i < GPU_TIMELINE_HISTORY_SIZE; ++i)
      for (int j = i + 1; j < GPU_TIMELINE_HISTORY_SIZE; ++j)
      {
        if (currentHashes[i] == currentHashes[j])
          currentHashes[j] = 0;
      }

    eastl::sort(eastl::begin(frameHashesHistory), eastl::end(frameHashesHistory),
      [](const FrameHashHistoryEntry &l, const FrameHashHistoryEntry &r) //
      { return l.cnt > r.cnt; });

    if (frameHashesHistory.size() < MAX_FRAME_HASHES_HISTORY_SIZE)
    {
      for (uint64_t itr : currentHashes)
      {
        if (itr == 0)
          continue;
        frameHashesHistory.push_back({itr, 1});
      }
    }
    else
      ImGui::Text("Max hash history size reached!");
  }

  ImGui::Checkbox("Freeze", &freezeFrameHashHistory);

  if (ImGui::Button("Clear hash history"))
    clear_and_shrink(frameHashesHistory);
  for (FrameHashHistoryEntry &itr : frameHashesHistory)
    ImGui::Text("GPU job hash %" PRIX64 " cnt: %zu", itr.hash, itr.cnt);
}

void drawTimings()
{
  const Drv3dTimings &timings = Frontend::timings.get(0);

  float msTimingVal[FRAME_TIMINGS] = {profile_usec_from_ticks_delta(timings.frontendUpdateScreenInterval) / 1000.0f,
    profile_usec_from_ticks_delta(timings.frontendToBackendUpdateScreenLatency) / 1000.0f,
    profile_usec_from_ticks_delta(timings.frontendBackendWaitDuration) / 1000.0f,
    profile_usec_from_ticks_delta(timings.backendFrontendWaitDuration) / 1000.0f,
    profile_usec_from_ticks_delta(timings.gpuWaitDuration) / 1000.0f, profile_usec_from_ticks_delta(timings.presentDuration) / 1000.0f,
    profile_usec_from_ticks_delta(timings.backbufferAcquireDuration) / 1000.0f,
    profile_usec_from_ticks_delta(timings.frontendWaitForSwapchainDuration) / 1000.0f};
  for (int i = 0; i < FRAME_TIMINGS; ++i)
  {
    ImGui::Text("%s %0.2f ms", timingsStrings[i], msTimingVal[i]);
    frameTimingsGraphs[i].addPoint(trackingFrameIdx, msTimingVal[i]);
  }

  ImPlot::SetNextAxisLimits(ImAxis_X1, trackingFrameIdx < FRAME_TIMINGS_HISTORY ? 0 : trackingFrameIdx - FRAME_TIMINGS_HISTORY,
    trackingFrameIdx, ImGuiCond_Always);
  if (ImPlot::BeginPlot("##Frame timings plot", nullptr, "Period, ms"))
  {
    for (int i = 0; i < FRAME_TIMINGS; ++i)
      ImPlot::PlotLine(timingsStrings[i], &frameTimingsGraphs[i].xData[0], &frameTimingsGraphs[i].yData[0],
        frameTimingsGraphs[i].xData.size(), ImPlotLineFlags_None, frameTimingsGraphs[i].offset);
    ImPlot::EndPlot();
  }
}

} // namespace

void drv3d_vulkan::debug_ui_frame()
{
  Stat3D::stopStatSingle(stat3dRecord);
  ImGui::Text("DP: %u", stat3dRecord.val[DRAWSTAT_DP]);
  ImGui::Text("Tris: %llu", (unsigned long long)stat3dRecord.tri);
  ImGui::Text("Buffer locks: %u", stat3dRecord.val[DRAWSTAT_LOCKVIB]);
  ImGui::Text("RT: %u", stat3dRecord.val[DRAWSTAT_RT]);
  ImGui::Text("PS: %u", stat3dRecord.val[DRAWSTAT_PS]);
  ImGui::Text("INST: %u", stat3dRecord.val[DRAWSTAT_INS]);
  ImGui::Text("Logical render pass count: %u", stat3dRecord.val[DRAWSTAT_RENDERPASS_LOGICAL]);

  if (ImGui::TreeNode("FrameHashes##1"))
  {
    drawFrameHashesHistory();
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("FrameTimings##1"))
  {
    drawTimings();
    ImGui::TreePop();
  }

  trackingFrameIdx++;

  Stat3D::startStatSingle(stat3dRecord);
}

void timelineGraphFill(TimelineHistoryIndex idx, const uint64_t *timestamps, ImS64 *graph)
{
  G_ASSERT(MAX_TIMELINES > idx);
  const uint32_t ST_CNT = (uint32_t)TimelineHistoryState::COUNT;
  static ImS64 tsBuf[ST_CNT];
  static ImS64 timingsBuf[ST_CNT];

  // copy from changing source
  memcpy(&tsBuf[0], &timestamps[0], sizeof(uint64_t) * ST_CNT);

  // init to acq -> acquire time
  timingsBuf[0] = tsBuf[(uint32_t)TimelineHistoryState::ACQUIRED] - tsBuf[(uint32_t)TimelineHistoryState::INIT];
  // acq to ready -> submit time
  timingsBuf[1] = tsBuf[(uint32_t)TimelineHistoryState::READY] - tsBuf[(uint32_t)TimelineHistoryState::ACQUIRED];
  // ready to progress -> process time
  timingsBuf[2] = tsBuf[(uint32_t)TimelineHistoryState::IN_PROGRESS] - tsBuf[(uint32_t)TimelineHistoryState::READY];
  // progress to done -> wait time
  timingsBuf[3] = tsBuf[(uint32_t)TimelineHistoryState::DONE] - tsBuf[(uint32_t)TimelineHistoryState::IN_PROGRESS];
  // done to init -> cleanup time
  timingsBuf[4] = tsBuf[(uint32_t)TimelineHistoryState::INIT] - tsBuf[(uint32_t)TimelineHistoryState::DONE];

  for (int i = 0; i < TOTAL_TIMINGS; ++i)
  {
    if (timingsBuf[i] > 0)
      graph[i * MAX_TIMELINES + idx] = ref_time_delta_to_usec(timingsBuf[i]) / 1000;
  }
}

void drv3d_vulkan::debug_ui_timeline()
{
  TimelineManager &tman = Globals::timelines;

  if (ImPlot::BeginPlot("##Timeline CPU replay", nullptr, "Delta, ms"))
  {
    TimelineHistoryIndex maxIdx = 0;
    tman.get<TimelineManager::CpuReplay>().enumerateTimestamps([&maxIdx](TimelineHistoryIndex idx, const uint64_t *timestamps) {
      if (maxIdx < idx)
        maxIdx = idx;
      timelineGraphFill(idx, timestamps, &timelineCpuGraph[0]);
    });
    for (int i = 0; i < TOTAL_TIMINGS; ++i)
    {
      String label(64, "CPU st %s", timeline_plot_legend[i]);
      ImPlot::PlotLine(label.c_str(), &timelineCpuGraph[MAX_TIMELINES * i], maxIdx + 1);
    }
    ImPlot::EndPlot();
  }
  if (ImPlot::BeginPlot("##Timeline GPU execute", nullptr, "Delta, ms"))
  {
    TimelineHistoryIndex maxIdx = 0;
    tman.get<TimelineManager::GpuExecute>().enumerateTimestamps([&maxIdx](TimelineHistoryIndex idx, const uint64_t *timestamps) {
      if (maxIdx < idx)
        maxIdx = idx;
      timelineGraphFill(idx, timestamps, &timelineGpuGraph[0]);
    });
    for (int i = 0; i < TOTAL_TIMINGS; ++i)
    {
      String label(64, "GPU st %s", timeline_plot_legend[i]);
      ImPlot::PlotLine(label.c_str(), &timelineGpuGraph[MAX_TIMELINES * i], maxIdx + 1);
    }
    ImPlot::EndPlot();
  }
}

namespace
{
struct UniversalPipeInfoCb
{
  template <typename PipeType>
  void operator()(PipeType &pipe, ProgramID program)
  {
    pipeline_ui::DebugInfo result;

    // dumpShaderInfos can return any container
    auto shaderInfos = pipe.dumpShaderInfos();
    result.shaderNames.reserve(shaderInfos.size());
    for (auto &info : shaderInfos)
      result.shaderNames.emplace_back(eastl::move(info.name));

    result.totalCompilationTime = pipe.dumpCompilationTime();
    result.variantCount = pipe.dumpVariantCount();
    result.enabled = pipe.isUsageAllowed();
    result.program = program;

    pipeline_ui::debugInfos.emplace_back(eastl::move(result));
  }
};
} // namespace

void drv3d_vulkan::debug_ui_update_pipelines_data()
{
  WinAutoLock lock(pipeline_ui::mutex);
  pipeline_ui::debugInfos.clear();
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  PipelineManager &pipeMan = Globals::pipelines;

  pipeMan.enumerate<ComputePipeline>(UniversalPipeInfoCb{});
  pipeMan.enumerate<VariatedGraphicsPipeline>(UniversalPipeInfoCb{});
#endif
}

void drv3d_vulkan::debug_ui_pipelines()
{
#if !VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  ImGui::Text("Pipeline debug info requires %s", #VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA);
#else

  if (ImGui::Button("Update"))
    Globals::ctx.updateDebugUIPipelinesData();

  ImGui::SameLine();
  ImGui::SetNextItemWidth(250);
  // -1 is important here so that we always have a null terminator
  ImGui::InputText("Search by shader", pipeline_ui::searchString.data(), pipeline_ui::searchString.size() - 1);


  for (auto &type : pipeline_ui::progTypeDatas)
  {
    if (type.type == pipeline_ui::ProgramTypeData::TYPE_NONE)
    {
      continue;
    }

    // Type ids should go in order, without any holes
    G_ASSERT(type.type == eastl::addressof(type) - pipeline_ui::progTypeDatas.data());

    ImGui::SameLine();
    ImGui::Checkbox(type.displayText, &type.show);
  }

  dag::Vector<pipeline_ui::DebugInfo> pipelineInfos;
  {
    WinAutoLock lock(pipeline_ui::mutex);
    pipelineInfos = pipeline_ui::debugInfos;
  }

  {
    auto success =
      ImGui::BeginTable("PipelinesDebugInfo", 7, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY);
    if (!success)
      return;
  }
  ImGui::TableSetupColumn("E", ImGuiTableColumnFlags_NoSort);
  ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoSort);
  ImGui::TableSetupColumn("ProgID");
  ImGui::TableSetupColumn("Shaders", ImGuiTableColumnFlags_NoSort);
  ImGui::TableSetupColumn("Variations");
  ImGui::TableSetupColumn("Comp. Time, us");
  ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_NoSort);

  ImGui::TableHeadersRow();

  ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs();

  if (sortSpecs && sortSpecs->SpecsCount > 0)
  {
    // properly supporting multisort is harder and probably not worth it
    eastl::sort(pipelineInfos.begin(), pipelineInfos.end(),
      [col = sortSpecs->Specs[0].ColumnIndex](const pipeline_ui::DebugInfo &l, const pipeline_ui::DebugInfo &r) {
        switch (col)
        {
          case 2: return l.program.get() < r.program.get();
          case 3: return l.shaderNames.size() < r.shaderNames.size();
          case 4: return l.variantCount < r.variantCount;
          case 5: return l.totalCompilationTime < r.totalCompilationTime;
          default: return true; // don't sort
        }
      });

    if (sortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Descending)
      eastl::reverse(pipelineInfos.begin(), pipelineInfos.end());
  }

  // TODO: There should be an engine-wide utility that does this
  auto formatList = [](const auto &list, auto toString) {
    String result("[");
    for (auto &element : list)
    {
      // toString might return any type of string (even const char*), therefore auto
      auto elementStr = toString(element);
      if (elementStr.empty())
        continue;
      result += elementStr;
      result += ", ";
    }
    result.chop(2);
    result += "]";
    return result;
  };

  for (auto &info : pipelineInfos)
  {
    if (!pipeline_ui::progTypeDatas[get_program_type(info.program)].show)
      continue;

    bool searchSuccessful = false;
    for (auto &name : info.shaderNames)
    {
      if (name.find(pipeline_ui::searchString.data()) != nullptr)
      {
        searchSuccessful = true;
        break;
      }
    }

    if (!searchSuccessful)
      continue;

    ImGui::TableNextRow();

    // Pipeline enable/disable checkbox
    ImGui::TableNextColumn();
    if (get_program_type(info.program) == program_type_graphics)
    {
      // TODO: When we update imgui, we can use disabled checkboxes so that it looks prettier.
      // See https://github.com/ocornut/imgui/issues/211#issuecomment-902751145
      if (ImGui::Checkbox(String(0, "##program%d", info.program.get()).c_str(), &info.enabled))
      {
        Globals::ctx.setPipelineUsability(info.program, info.enabled);
        Globals::ctx.updateDebugUIPipelinesData();
      }
    }

    ImGui::TableNextColumn();
    ImGui::TextUnformatted(pipeline_ui::progTypeDatas[get_program_type(info.program)].displayText);

    ImGui::TableNextColumn();
    ImGui::Text("%x", info.program.get());

    ImGui::TableNextColumn();
    ImGui::TextUnformatted(formatList(info.shaderNames, [](const String &e) { return e; }).c_str());

    ImGui::TableNextColumn();
    ImGui::Text("%zu", info.variantCount);

    ImGui::TableNextColumn();
    ImGui::Text("%" PRId64, info.totalCompilationTime);


    // TODO: Details view
    String popup_id(16, "Details for ProgID %d", info.program.get());

    ImGui::TableNextColumn();
    if (ImGui::Button(String(16, "Details##%d", info.program.get()).c_str()))
    {
      ImGui::OpenPopup(popup_id.c_str());
    }

    if (ImGui::BeginPopupContextWindow(popup_id.c_str()))
    {
      ImGui::TextUnformatted("TODO!");
      ImGui::EndPopup();
    }
  }
  ImGui::EndTable();
#endif
}

void drv3d_vulkan::debug_ui_misc()
{
  if (ImGui::Button("Generate fault report"))
    Globals::ctx.generateFaultReportAtFrameEnd();

  ImGui::TextUnformatted(Globals::ctx.isWorkerRunning() ? "Execution: threaded" : "Execution: deferred");

  if (ImGui::Button("Switch execution mode"))
    Globals::ctx.toggleWorkerThread();

  size_t multiQueueOverride = Backend::interop.toggleMultiQueueSubmit.load(std::memory_order_relaxed);
  if (Globals::cfg.bits.allowMultiQueue || multiQueueOverride)
  {
    ImGui::TextUnformatted(multiQueueOverride % 2 ? "MultiQueueSubmit: no" : "MultiQueueSubmit: yes");
    if (ImGui::Button("Toggle multi queue submit"))
      ++Backend::interop.toggleMultiQueueSubmit;
  }
}

#endif
