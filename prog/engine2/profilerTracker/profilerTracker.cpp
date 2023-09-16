#include <3d/dag_profilerTracker.h>
#include "profilerTrackerInternal.h"

#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>

#include <debug/dag_assert.h>
#include <gui/dag_imgui.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <util/dag_string.h>
#include <imgui/imgui.h>
#include <imgui/implot.h>

#include <mutex>

static bool d3d_markers_window = false;
static bool help_window = false;
static bool hidden_group_window = false;

static eastl::unique_ptr<Tracker> tracker;
static std::mutex mutex;
static char profilerInputBuffer[256] = {0};
static bool loaded = false;
// profiler values are not updated if the window is closed
static uint16_t window_closed_counter = 0;
static uint64_t dump_history_counter = 0;
static uint32_t frame_id = 0;

// History dump is called once in every <D3D_MARKER_UPDATE_FREQUENCY> frames
// This value needs to be no more than the history size of TimeIntervalProfiler
#define D3D_MARKER_UPDATE_FREQUENCY 120

static const char *FILTERS[] = {"None", "Avg", "Min", "Max"};

enum Filter
{
  NONE = 0,
  AVG,
  MIN,
  MAX,
  COUNT
};

static Filter filter = NONE;
static int filter_size = 5;

static void load()
{
  if (loaded)
    return;
  loaded = tracker->load();
}

void profiler_tracker::init()
{
  std::lock_guard<std::mutex> lock(mutex);
  loaded = false;
#if PROFILER_TRACKER_ENABLED
  // history size (frames); max skipped frames
  tracker = eastl::make_unique<Tracker>(8 * 64); // ~8.5 seconds with
                                                 // 60 fps
  load();
#endif
}

void profiler_tracker::close()
{
  std::lock_guard<std::mutex> lock(mutex);
  if (tracker)
  {
    tracker.reset();
  }
}

static void record_value_impl(const eastl::string &group, const eastl::string &label, float value, uint32_t frameOffset)
{
  if (tracker)
    tracker->updateRecord(group, label, value, frameOffset);
}

static void record_value_impl(const char *group, const char *label, float value, uint32_t frameOffset)
{
  if (tracker)
    tracker->updateRecord(group, label, value, frameOffset);
}


static void update_records(int threadId, TimeIntervalNode *, TimeIntervalNode *, const char *name, int depth, const float *cpu,
  const float *gpu, const DrawStatSingle *, int cnt)
{
  const auto &tracked = tracker->getTrackedProfilers();
  auto itr = tracked.find_as(name);
  if (itr == tracked.end())
    return;
  for (uint32_t i = 0; i < min(cnt, D3D_MARKER_UPDATE_FREQUENCY); ++i)
    record_value_impl("D3D markers CPU", name, cpu[i], D3D_MARKER_UPDATE_FREQUENCY - i - 1);
  if (gpu)
    for (uint32_t i = 0; i < min(cnt, D3D_MARKER_UPDATE_FREQUENCY); ++i)
      record_value_impl("D3D markers GPU", name, gpu[i], D3D_MARKER_UPDATE_FREQUENCY - i - 1);
}

void profiler_tracker::start_frame()
{
  std::lock_guard<std::mutex> lock(mutex);
  frame_id++;
  if (tracker)
  {
    if (window_closed_counter < 3)
    {
      window_closed_counter++;
      if (++dump_history_counter >= D3D_MARKER_UPDATE_FREQUENCY && tracker->getTrackedProfilers().size() > 0)
      {
        dump_history_counter = 0;
        TimeIntervalProfiler::DumpHistory(update_records);
      }
    }
    tracker->startFrame();
  }
}

void profiler_tracker::record_value(const eastl::string &group, const eastl::string &label, float value, uint32_t frameOffset)
{
  std::lock_guard<std::mutex> lock(mutex);
  record_value_impl(group, label, value, frameOffset);
}

void profiler_tracker::record_value(const char *group, const char *label, float value, uint32_t frameOffset)
{
  std::lock_guard<std::mutex> lock(mutex);
  record_value_impl(group, label, value, frameOffset);
}

void profiler_tracker::clear()
{
  std::lock_guard<std::mutex> lock(mutex);
  if (tracker)
    tracker->clear();
}

Tracker::Tracker(uint32_t history_size) : historySize(history_size)
{
  // List here groups, that should be displayed in the available group list
  // the stored bool is the default value for visibility of the group (true = visible)
  visibleGroups = {{"Latency", false}, {"App", true}};
}

void Tracker::setPaused(bool p) { paused = p; }

void Tracker::startFrame()
{
  if (!isPaused())
  {
    for (auto &group : records)
    {
      const eastl::string *toErase = nullptr;
      for (auto &itr : group.second)
      {
        if (itr.second.skippedCount++ >= historySize)
        {
          // erase only 1 value per group at a time
          toErase = &itr.first;
        }
        else
        {
          if (itr.second.values.size() < itr.second.values.capacity())
            itr.second.values.push_back(0);
          else
          {
            itr.second.valuesEnd = itr.second.valuesEnd % itr.second.values.size(); // new pos
            itr.second.values[itr.second.valuesEnd] = 0;
          }
          // valuesEnd = values.size() is allowed
          itr.second.valuesEnd++;
        }
      }
      if (toErase != nullptr)
        group.second.erase(*toErase);
    }
  }
}

void Tracker::updateRecord(const char *group, const char *label, float value, uint32_t frame_offset)
{
  if (isPaused())
    return;
  if (frame_offset > historySize)
  {
    logerr("[profiler_tracker] frame_offest <%d> exceeded history size <%d>", frame_offset, historySize);
    return;
  }
  // this could be improved
  updateRecord(eastl::string(group), eastl::string(label), value, frame_offset);
}

void Tracker::updateRecord(const eastl::string &group, const eastl::string &label, float value, uint32_t frame_offset)
{
  if (isPaused())
    return;
  SubgroupMap &groupMap = records[group];
  auto itr = groupMap.find(label);
  if (itr == groupMap.end())
  {
    eastl::vector<float> vec;
    vec.reserve(historySize);
    groupMap[label] = Record{group, label, eastl::move(vec), 0, 0};
    itr = groupMap.find(label);
  }
  itr->second.skippedCount = eastl::min(static_cast<uint16_t>(frame_offset), itr->second.skippedCount);
  if (itr->second.values.size() == 0)
  {
    itr->second.values.push_back(value);
    itr->second.valuesEnd++;
  }
  else
  {
    uint32_t index = (itr->second.valuesEnd + itr->second.values.size() - frame_offset - 1) % itr->second.values.size();
    itr->second.values[index] += value;
  }
}

void Tracker::clear() { records.clear(); }

Tracker::PlotData Tracker::getValues(const eastl::string &group, const eastl::string &label) const
{
  PlotData ret;
  auto groupMap = records.find(group);
  if (groupMap == records.end())
    return ret;
  auto itr = groupMap->second.find(label);
  if (itr == groupMap->second.end())
    return ret;
  ret.skipped = itr->second.skippedCount;
  if (itr->second.valuesEnd == itr->second.values.size())
  {
    ret.size1 = itr->second.values.size();
    ret.values1 = itr->second.values.data();
    if (ret.size1 > 0)
      ret.size1--; // last value is not filled yet
  }
  else
  {
    ret.size1 = itr->second.values.size() - itr->second.valuesEnd;
    ret.values1 = &itr->second.values[itr->second.valuesEnd];
    ret.size2 = itr->second.valuesEnd;
    ret.values2 = itr->second.values.data();
    if (ret.size2 > 0)
      ret.size2--; // last value is not filled yet
  }
  return ret;
}

profiler_tracker::Timer::Timer() : stamp(::ref_time_ticks()) {}

int profiler_tracker::Timer::timeUSec() const { return ::get_time_usec(stamp); }

void Tracker::trackProfiler(eastl::string &&label, bool need_save)
{
  trackedProfilers.insert(eastl::move(label));
  if (need_save)
    save();
}

void Tracker::untrackProfiler(const eastl::string &label)
{
  trackedProfilers.erase(label);
  save();
}

bool Tracker::save() const
{
  if (!imgui_get_blk())
    return false;
  DataBlock *blk = imgui_get_blk()->addBlock("profiler_tracker");
  blk->setInt("filterSize", filter_size);
  blk->setInt("filter", static_cast<int>(filter));
  DataBlock *trackedBlk = blk->addBlock("tracked");
  trackedBlk->clearData();
  trackedBlk->addInt("count", trackedProfilers.size());
  int i = 0;
  for (const auto &label : trackedProfilers)
  {
    const String id(8, "%d", i++);
    trackedBlk->addStr(id.c_str(), label.c_str());
  }
  DataBlock *visibleBlk = blk->addBlock("visible");
  visibleBlk->clearData();
  visibleBlk->addInt("count", visibleGroups.size());
  i = 0;
  for (const auto &group : visibleGroups)
  {
    const String groupId(16, "group_%d", i);
    visibleBlk->setStr(groupId.c_str(), group.first.c_str());
    const String valueId(16, "value_%d", i++);
    visibleBlk->setBool(valueId.c_str(), group.second);
  }
  imgui_save_blk();
  return true;
}

bool Tracker::load()
{
  if (!imgui_get_blk())
    return false;
  const DataBlock *blk = imgui_get_blk()->getBlockByName("profiler_tracker");
  if (!blk)
    return true; // no need to try again
  filter_size = blk->getInt("filterSize", filter_size);
  filter = static_cast<Filter>(blk->getInt("filter", static_cast<int>(filter)));
  const DataBlock *trackedBlk = blk->getBlockByName("tracked");
  if (trackedBlk)
  {
    uint32_t count = trackedBlk->getInt("count", 0);
    for (uint32_t i = 0; i < count; ++i)
    {
      String id(8, "%d", i);
      const char *s = trackedBlk->getStr(id.c_str(), nullptr);
      if (s)
        trackProfiler(eastl::string(s), false);
    }
  }
  const DataBlock *visibleBlk = blk->getBlockByName("visible");
  if (visibleBlk)
  {
    uint32_t count = visibleBlk->getInt("count", 0);
    for (uint32_t i = 0; i < count; ++i)
    {
      const String groupId(16, "group_%d", i);
      const String valueId(16, "value_%d", i);

      const char *group = visibleBlk->getStr(groupId.c_str(), nullptr);
      bool value = visibleBlk->getBool(valueId.c_str(), false);
      if (group)
      {
        auto itr = visibleGroups.find(eastl::string(group));
        if (itr != visibleGroups.end())
          itr->second = value;
      }
    }
  }
  return true;
}

static float get_data_point(const Tracker::PlotData *dataPtr, int idx)
{
  float value = idx < dataPtr->size1 ? dataPtr->values1[idx] : dataPtr->values2[idx - dataPtr->size1];
  return value;
}

static bool window_open = true;

static void show_help_window(bool *p_open)
{
  ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Help", p_open))
  {
    ImGui::End();
    return;
  }

  ImGui::Text("Creating a marker:");
  ImGui::BulletText("TRACK_SCOPE_TIME(groupName, labelName);");
  ImGui::BulletText("profiler_tracker::record_value(group, name, sum, "
                    "frame_offset);");
  ImGui::TextWrapped("The TRACK_SCOPE_TIME basically creates an object, which measures time spent "
                     "in that scope and uses record_value automatically.");
  ImGui::TextWrapped("If several values are recorded with the same name and group in one frame, "
                     "they will be summed up.");
  ImGui::TextWrapped("The purpose of the frame_offset parameter is to allow periodically updated "
                     "value. For example a code only updates the measured values every 10 frames, "
                     "but then it wants to update all 10 values. In this case, values can be "
                     "updated in a for loop with decreasing frame offset to fill in all previous "
                     "frames.");
  ImGui::Separator();
  ImGui::Text("Tracking D3D profiler markers:");
  ImGui::TextWrapped("D3D markers can be selected in the 'D3D markers' / 'Track markers' menu "
                     "option. By default only cpu is measured, but gpu profiling can be enabled in "
                     "the same menu. These values are updated every 120 frames (the history size "
                     "of the profiler). The query can take several milliseconds, that's why it's "
                     "not updated every frame. Selected markers are saved locally in the imgui "
                     "config file.");
  ImGui::End();
}

static void show_d3d_markers(bool *p_open)
{
  ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("D3D markers", p_open))
  {
    ImGui::End();
    return;
  }
  const Tracker::Map &map = tracker->getMap();
  ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
  if (ImGui::InputTextWithHint("Name", "Enter label from a TIME_D3D_PROFILE", profilerInputBuffer, IM_ARRAYSIZE(profilerInputBuffer),
        input_text_flags))
  {
    char *s = profilerInputBuffer;
    tracker->trackProfiler(eastl::string(s));
    s[0] = 0;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add"))
  {
    char *s = profilerInputBuffer;
    tracker->trackProfiler(eastl::string(s));
    s[0] = 0;
  }
  ImGui::Separator();
  const auto &tracked = tracker->getTrackedProfilers();
  int id = 0;
  for (const eastl::string &label : tracked)
  {
    ImGui::PushID(id++);
    ImGui::Text("%s", label.c_str());
    ImGui::SameLine();
    if (ImGui::Button("X"))
      tracker->untrackProfiler(label);
    ImGui::PopID();
  }
  ImGui::End();
}

static void show_group_hiding_window(bool *p_open)
{
  // getHideMap
  ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Registered markers", p_open))
  {
    ImGui::End();
    return;
  }
  ImGui::TextWrapped("If you want to commit a marker for other to use, register it to this "
                     "list. It is recommended to hide it by default");
  ImGui::Text("prog/engine2/profilerTracker/profilerTracker.cpp");
  ImGui::Text("Tracker::Tracker()");
  ImGui::Separator();
  for (auto &group : tracker->getVisibleMap())
    if (ImGui::Checkbox(group.first.c_str(), &group.second))
      tracker->save();
  ImGui::End();
}

static void draw_window()
{
  if (!tracker)
    return;
  g_enable_time_profiler = true;
  window_closed_counter = 0;
  load();
  if (d3d_markers_window)
    show_d3d_markers(&d3d_markers_window);
  if (help_window)
    show_help_window(&help_window);
  if (hidden_group_window)
    show_group_hiding_window(&hidden_group_window);
  if (ImGui::BeginMenuBar())
  {
    if (ImGui::BeginMenu("Menu"))
    {
      if (ImGui::MenuItem("Pause", 0, false, !tracker->isPaused()))
        tracker->setPaused(true);
      if (ImGui::MenuItem("Continue", 0, false, tracker->isPaused()))
        tracker->setPaused(false);
      ImGui::MenuItem("Registered markers", NULL, &hidden_group_window);
      if (ImGui::MenuItem("Clear"))
        tracker->clear();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("D3D markers"))
    {
      if (!g_gpu_profiler_on && ImGui::Button("Enable GPU profiling"))
      {
        g_gpu_profiler_on = true;
      }
      ImGui::MenuItem("Track markers", NULL, &d3d_markers_window);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Filter"))
    {
      ImGui::PushItemWidth(100);
      static const char *filterName = FILTERS[filter];
      if (ImGui::BeginCombo("Type", filterName))
      {
        for (int i = 0; i < Filter::COUNT; ++i)
        {
          const char *name = FILTERS[i];
          if (ImGui::Selectable(name, filterName == name))
          {
            filterName = name;
            tracker->save();
            filter = static_cast<Filter>(i);
          }
        }
        ImGui::EndCombo();
      }
      if (ImGui::DragInt("Radius", &filter_size, 1, 0, 20))
        tracker->save();
      ImGui::PopItemWidth();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help"))
    {
      ImGui::MenuItem("Tutorial", NULL, &help_window);
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
  const Tracker::Map &map = tracker->getMap();
  for (auto &group : map)
  {
    auto itr = tracker->getVisibleMap().find(group.first);
    if (itr != tracker->getVisibleMap().end() && !itr->second)
      continue;
    if (ImGui::CollapsingHeader(group.first.c_str()))
    {
      ImPlot::SetNextPlotLimits(0, tracker->getHistorySize(), 0, 16);
      if (ImPlot::BeginPlot(group.first.c_str(), "frames", "msec", ImVec2(-1, 0), 0, ImPlotAxisFlags_Lock))
      {
        for (auto &itr : group.second)
        {
          Tracker::PlotData data = tracker->getValues(group.first, itr.first);
          if (data.size1 && data.values1)
          {
            int count = data.size1 + data.size2 - data.skipped - 1;
            ImPlot::PlotLineG(
              itr.first.c_str(),
              [](void *ptr, int idx) {
                Tracker::PlotData *dataPtr = reinterpret_cast<Tracker::PlotData *>(ptr);
                int count = dataPtr->size1 + dataPtr->size2 - dataPtr->skipped - 1;
                int x = tracker->getHistorySize() - (dataPtr->size1 + dataPtr->size2) + idx;
                if (filter == NONE || filter_size <= 0)
                  return ImPlotPoint(x, get_data_point(dataPtr, idx));
                int from = max(idx - filter_size, 0);
                int until = min(idx + filter_size + 1, count);
                float value = 0;
                switch (filter)
                {
                  case MIN: value = eastl::numeric_limits<float>::infinity(); break;
                  case MAX: value = -eastl::numeric_limits<float>::infinity(); break;
                  default: value = 0;
                }
                for (int i = from; i < until; ++i)
                {
                  float val = get_data_point(dataPtr, i);
                  switch (filter)
                  {
                    case AVG: value += val; break;
                    case MIN: value = min(value, val); break;
                    case MAX: value = max(value, val); break;
                    case NONE:
                    case COUNT:
                      // supres warnings for these options
                      break;
                  }
                }
                if (filter == AVG)
                  value /= until - from;
                return ImPlotPoint(x, value);
              },
              static_cast<void *>(&data), count);
          }
        }
        ImPlot::EndPlot();
      }
    }
  }
}

REGISTER_IMGUI_WINDOW_EX("Render", "Profile tracker", nullptr, 100, ImGuiWindowFlags_MenuBar, draw_window);
