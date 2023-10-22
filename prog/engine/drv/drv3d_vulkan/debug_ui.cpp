#if DAGOR_DBGLEVEL > 0
#include "debug_ui.h"
#include <EASTL/sort.h>
#include <dag/dag_vector.h>
#include <gui/dag_imgui.h>
#include "device.h"
#include "imgui.h"
#include "implot.h"

using namespace drv3d_vulkan;

namespace
{
constexpr uint32_t TOTAL_TIMINGS = 5;
constexpr uint32_t MAX_TIMELINES = 10;

ImS64 timelineCpuGraph[TOTAL_TIMINGS * MAX_TIMELINES];
ImS64 timelineGpuGraph[TOTAL_TIMINGS * MAX_TIMELINES];

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

eastl::array<ProgramTypeData, 1 << program_type_bits> progTypeDatas
{
  ProgramTypeData{"G", program_type_graphics}, ProgramTypeData{"C", program_type_compute},
#if D3D_HAS_RAY_TRACING
    ProgramTypeData{"RT", program_type_raytrace},
#endif
};
} // namespace pipeline_ui

const char *timeline_plot_legend[] = {"acquire", "submit", "process", "wait", "cleanup"};

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

  Stat3D::startStatSingle(stat3dRecord);
}

void timelineGraphFill(TimelineHistoryIndex idx, const uint64_t *timestamps, ImS64 *graph)
{
  G_ASSERT(MAX_TIMELINES > idx);
  const uint32_t ST_CNT = (uint32_t)TimelineHisotryState::COUNT;
  static ImS64 tsBuf[ST_CNT];
  static ImS64 timingsBuf[ST_CNT];

  // copy from changing source
  memcpy(&tsBuf[0], &timestamps[0], sizeof(uint64_t) * ST_CNT);

  // init to acq -> acquire time
  timingsBuf[0] = tsBuf[(uint32_t)TimelineHisotryState::ACQUIRED] - tsBuf[(uint32_t)TimelineHisotryState::INIT];
  // acq to ready -> submit time
  timingsBuf[1] = tsBuf[(uint32_t)TimelineHisotryState::READY] - tsBuf[(uint32_t)TimelineHisotryState::ACQUIRED];
  // ready to progress -> process time
  timingsBuf[2] = tsBuf[(uint32_t)TimelineHisotryState::IN_PROGRESS] - tsBuf[(uint32_t)TimelineHisotryState::READY];
  // progress to done -> wait time
  timingsBuf[3] = tsBuf[(uint32_t)TimelineHisotryState::DONE] - tsBuf[(uint32_t)TimelineHisotryState::IN_PROGRESS];
  // done to init -> cleanup time
  timingsBuf[4] = tsBuf[(uint32_t)TimelineHisotryState::INIT] - tsBuf[(uint32_t)TimelineHisotryState::DONE];

  for (int i = 0; i < TOTAL_TIMINGS; ++i)
  {
    if (timingsBuf[i] > 0)
      graph[i * MAX_TIMELINES + idx] = ref_time_delta_to_usec(timingsBuf[i]) / 1000;
  }
}

void drv3d_vulkan::debug_ui_timeline()
{
  TimelineManager &tman = get_device().timelineMan;

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
  PipelineManager &pipeMan = get_device().pipeMan;

  pipeMan.enumerate<ComputePipeline>(UniversalPipeInfoCb{});
  pipeMan.enumerate<VariatedGraphicsPipeline>(UniversalPipeInfoCb{});
#if D3D_HAS_RAY_TRACING
  pipeMan.enumerate<RaytracePipeline>(UniversalPipeInfoCb{});
#endif
#endif
}

void drv3d_vulkan::debug_ui_pipelines()
{
#if !VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  ImGui::Text("Pipeline debug info requires %s", #VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA);
#else

  if (ImGui::Button("Update"))
    get_device().getContext().updateDebugUIPipelinesData();

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
        get_device().getContext().setPipelineUsability(info.program, info.enabled);
        get_device().getContext().updateDebugUIPipelinesData();
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
    get_device().getContext().generateFaultReportAtFrameEnd();
}

#endif
