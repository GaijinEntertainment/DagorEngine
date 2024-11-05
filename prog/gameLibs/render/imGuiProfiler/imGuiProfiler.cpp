// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/imGuiProfiler.h>
#include <util/dag_globDef.h>
#include <perfMon/dag_statDrv.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/algorithm.h>
#include <generic/dag_span.h>


struct Event
{
  int root;
  const char *name = nullptr;
  int parentIdx = -1;

  double tMin, tMax, tCurrent, tSpike;
  double unknownTimeMsec = 0.0; // Cpu Only

  bool isGpu;
  DrawStatSingle gpuDatas;

  ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_OpenOnArrow;
  char treeLabel[256] = {0};

  Event() = default;

  Event(int root, const char *name, const TimeIntervalInfo &ti, int parent) : name(name), root(root), parentIdx(parent)
  {
    if (ti.gpuValid)
    {
      isGpu = true;
      tMin = ti.tGpuMin;
      tMax = ti.tGpuMax;
      tCurrent = ti.tGpuCurrent;
      tSpike = ti.tGpuSpike;
      gpuDatas = ti.stat;
    }
    else
    {
      isGpu = false;
      tMin = ti.tMin;
      tMax = ti.tMax;
      tCurrent = ti.tCurrent;
      tSpike = ti.tSpike;
    }
  }

  void update(const TimeIntervalInfo &ti)
  {
    if (isGpu)
    {
      tMin = ti.tGpuMin < tMin ? ti.tGpuMin : tMin;
      tMax = ti.tGpuMax > tMax ? ti.tGpuMax : tMax;
      tCurrent = ti.tGpuCurrent;
      tSpike = ti.tGpuSpike;
      gpuDatas = ti.stat;
    }
    else
    {
      tMin = ti.tMin < tMin ? ti.tMin : tMin;
      tMax = ti.tMax > tMax ? ti.tMax : tMax;
      tCurrent = ti.tCurrent;
      tSpike = ti.tSpike;
    }
  }
};

struct SearchResult
{
  int pos;
  eastl::unique_ptr<eastl::string> name; // Need to be a constant size for sorting
  int id;
  SearchResult(int pos, eastl::string name, int id) : pos(pos), name(eastl::make_unique<eastl::string>(name)), id(id) {}
};

struct Context
{
  eastl::vector<Event> eventContainer;

  eastl::vector<int> roots;
  eastl::vector_map<eastl::string, eastl::vector_map<int, int>> nameLookUp; // Will be used for search filtering
  eastl::vector_map<uintptr_t, int> eventIdLookUp;
  eastl::vector<eastl::vector<int>> childrenMap;
  bool isConstructed = false;

  // ImGUI params
  bool enableAutoUpdate = false;
  int updateFreq = 100;


  eastl::vector<const char *> rootNames;

  const char *selectedItem = nullptr;
  int selectedRoot = -1;
  int selectedEventId = -1;

  int focusedEventId = -1;

  eastl::vector<int> jumpHistory;
  int historyPos = 0;

  eastl::vector<int> measuredEventIds;

  bool searchSelectedFlag = false;
  char searchText[100] = {0};
  eastl::vector<SearchResult> searchResults;
  bool measuredOpen = false;
} pCtx;


// Helper Functions

static void labelGenerator(char *buff, size_t size, const char *name, const Event &ev)
{
  snprintf(buff, size, "%s###%s,%s:%d:%d", name, name, ev.name, ev.root, ev.parentIdx);
}

static const char *printTime(double &time, int depth)
{
  if (floor(time) > 0)
  {
    switch (depth)
    {
      case 0: return "ms";
      case 1: return "us";
      default: return "ns";
    }
  }
  else if (depth < 2)
  {
    time *= 1000.0;
    return printTime(time, depth + 1);
  }
  return "ns";
}

static void goToEvent(int id)
{
  pCtx.focusedEventId = id;

  if (pCtx.historyPos < pCtx.jumpHistory.size())
    pCtx.jumpHistory.erase(pCtx.jumpHistory.begin() + pCtx.historyPos, pCtx.jumpHistory.end());

  pCtx.jumpHistory.push_back(pCtx.focusedEventId);
  pCtx.historyPos++;
}

static eastl::string generateSearchName(const Event &ev)
{
  if (ev.parentIdx == -1)
    return eastl::string(ev.name);
  else if (ev.parentIdx == ev.root)
    return eastl::string(eastl::string::CtorSprintf{}, "%s->%s", pCtx.eventContainer[ev.root].name, ev.name);
  else
  {
    const Event &pEv = pCtx.eventContainer[ev.parentIdx];
    if (ev.root == pEv.parentIdx)
      return eastl::string(eastl::string::CtorSprintf{}, "%s->%s->%s", pCtx.eventContainer[ev.root].name, pEv.name, ev.name);
    else
      return eastl::string(eastl::string::CtorSprintf{}, "%s->...->%s->%s", pCtx.eventContainer[ev.root].name, pEv.name, ev.name);
  }
}

static int cmpSearchResult(const void *p1, const void *p2)
{
  SearchResult *sR1 = (SearchResult *)p1, *sR2 = (SearchResult *)p2;
  return sR1->pos - sR2->pos;
}

static void createSearchResults()
{
  pCtx.searchResults.clear();

  if (strlen(pCtx.searchText) == 0)
    return;

  for (const auto &it : pCtx.nameLookUp)
  {
    int pos = it.first.find(pCtx.searchText);
    if (pos >= 0)
    {
      if (pCtx.searchSelectedFlag && pCtx.selectedRoot != -1)
      {
        if (auto iit = it.second.find(pCtx.selectedRoot); iit != it.second.end())
          pCtx.searchResults.emplace_back(pos, generateSearchName(pCtx.eventContainer[iit->second]), iit->second);
      }
      else
      {
        for (const auto &iit : it.second)
        {
          pCtx.searchResults.emplace_back(pos, generateSearchName(pCtx.eventContainer[iit.second]), iit.second);
        }
      }
    }
  }
  qsort(pCtx.searchResults.begin(), pCtx.searchResults.size(), sizeof(SearchResult), cmpSearchResult);
}

// Updater Logic
static void dumpExtractorFunction(void *ctx, uintptr_t cNode, uintptr_t pNode, const char *name, const TimeIntervalInfo &ti)
{
  Context &currCtx = *(Context *)ctx;
  if (eastl::vector_map<uintptr_t, int>::iterator it = currCtx.eventIdLookUp.find(cNode); it != currCtx.eventIdLookUp.end())
  {
    // Early exit if measured
    if (pCtx.measuredOpen && find_value_idx(pCtx.measuredEventIds, it->second) == -1)
      return;

    currCtx.eventContainer[it->second].update(ti);
    return;
  }
  if (pNode == 0)
  {
    currCtx.eventContainer.emplace_back(-1, name, ti, -1);
    currCtx.childrenMap.push_back({});
    int currEventIdx = currCtx.eventContainer.size() - 1;

    Event &currEvent = pCtx.eventContainer[currEventIdx];
    currEvent.root = currEventIdx;

    currCtx.roots.push_back(currEventIdx);
    currCtx.eventIdLookUp.insert(eastl::make_pair(cNode, currEventIdx));
    currCtx.nameLookUp[name].insert(eastl::make_pair(currEventIdx, currEventIdx));


    currCtx.rootNames.push_back(name);

    labelGenerator(currEvent.treeLabel, sizeof(char) * 256, currEvent.name, currEvent);
  }
  else
  {
    int parentEventId = currCtx.eventIdLookUp[pNode];
    int parentRootId = currCtx.eventContainer[parentEventId].root;


    currCtx.eventContainer.emplace_back(parentRootId, name, ti, parentEventId);
    currCtx.childrenMap.push_back({});
    int currEventIdx = currCtx.eventContainer.size() - 1;

    currCtx.nameLookUp[name].insert(eastl::make_pair(parentRootId, currEventIdx));

    currCtx.eventIdLookUp.insert(eastl::make_pair(cNode, currEventIdx));
    currCtx.childrenMap[parentEventId].push_back(currEventIdx);

    Event &currEvent = pCtx.eventContainer[currEventIdx];
    labelGenerator(currEvent.treeLabel, sizeof(char) * 256, currEvent.name, currEvent);
  }
}


// ImGui Window Parts
static void imDisplayEvent(const Event &displayEvent)
{
  ImGui::Separator();
  ImGui::Text("Name: %s", displayEvent.name);

  ImGui::Text("Thread: %s, Parent: %s", pCtx.eventContainer[displayEvent.root].name,
    (displayEvent.parentIdx == -1 ? "-" : pCtx.eventContainer[displayEvent.parentIdx].name));

  double time = displayEvent.tMin;
  const char *postfix = printTime(time, 0);
  ImGui::Text("tMin: %.3f%s", time, postfix);

  ImGui::SameLine();

  time = displayEvent.tCurrent;
  postfix = printTime(time, 0);
  ImGui::Text("tCurrent: %.3f%s", time, postfix);

  ImGui::SameLine();

  time = displayEvent.tMax;
  postfix = printTime(time, 0);
  ImGui::Text("tMax: %.3f%s", time, postfix);

  if (displayEvent.isGpu)
  {
    ImGui::Text("tris: %u, locks: %d, dip: %d, rt_change: %d", (uint32_t)displayEvent.gpuDatas.tri,
      displayEvent.gpuDatas.val[DRAWSTAT_LOCKVIB], displayEvent.gpuDatas.val[DRAWSTAT_DP], displayEvent.gpuDatas.val[DRAWSTAT_RT]);
    ImGui::Text("prog_changes: %d, instances: %d, render_passes: %d", displayEvent.gpuDatas.val[DRAWSTAT_PS],
      displayEvent.gpuDatas.val[DRAWSTAT_INS], displayEvent.gpuDatas.val[DRAWSTAT_RENDERPASS_LOGICAL]);
  }
}

static void imDisplayEventHover(const Event &displayEvent)
{
  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    imDisplayEvent(displayEvent);
    ImGui::End();
  }
}

static void imGuiMakeTree(Event &currEvent, const int idx)
{
  if (idx == pCtx.focusedEventId)
    currEvent.flag |= ImGuiTreeNodeFlags_DefaultOpen;
  else
    currEvent.flag &= ~ImGuiTreeNodeFlags_DefaultOpen;


  if (pCtx.childrenMap[idx].empty())
    currEvent.flag |= ImGuiTreeNodeFlags_Leaf;
  else
    currEvent.flag &= ~ImGuiTreeNodeFlags_Leaf;

  if (idx == pCtx.selectedEventId)
    currEvent.flag |= ImGuiTreeNodeFlags_Selected;
  else
    currEvent.flag &= ~ImGuiTreeNodeFlags_Selected;

  if (ImGui::TreeNodeEx(currEvent.treeLabel, currEvent.flag))
  {
    if (ImGui::IsItemClicked())
    {
      pCtx.selectedEventId = idx;
    }

    imDisplayEventHover(currEvent);

    for (const int &i : pCtx.childrenMap[idx])
      imGuiMakeTree(pCtx.eventContainer[i], i);

    ImGui::TreePop();
  }
  else
  {
    if (ImGui::IsItemClicked())
    {
      pCtx.selectedEventId = idx;
    }
    imDisplayEventHover(currEvent);
  }
}

static void setTooltip(const char *tooltip)
{
  if (!ImGui::IsItemHovered() || tooltip == nullptr)
    return;
  ImGui::BeginTooltip();
  ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
  ImGui::TextUnformatted(tooltip);
  ImGui::PopTextWrapPos();
  ImGui::EndTooltip();
}

static void toggleMeasuredEvent(int id)
{
  if (int idx = find_value_idx(pCtx.measuredEventIds, id); idx != -1)
    pCtx.measuredEventIds.erase(pCtx.measuredEventIds.begin() + idx);
  else
    pCtx.measuredEventIds.emplace_back(id);
}

static void searchPart()
{
  if (ImGui::Checkbox("Search only selected thread", &pCtx.searchSelectedFlag))
  {
    createSearchResults();
  }
  setTooltip("If checked, only the selected thread will be searched for the marker name");

  ImGui::Text("Search Text:");
  setTooltip("Start typing the marker name you would like to measure");
  if (ImGui::InputText(" ###Search text", pCtx.searchText, sizeof(pCtx.searchText)))
  {
    createSearchResults();
  }
  ImGui::SameLine();

  if (ImGui::Button("Clear"))
  {
    *pCtx.searchText = {0};
    pCtx.searchResults.clear();
  }

  if (!pCtx.searchResults.empty())
  {
    if (ImGui::TreeNodeEx("Search Results", ImGuiTreeNodeFlags_DefaultOpen))
    {
      if (ImGui::BeginListBox(" ###Search Results List", ImVec2(800, 200)))
      {
        for (const SearchResult &sr : pCtx.searchResults)
        {
          if (ImGui::Button(sr.name->c_str()))
          {
            pCtx.selectedEventId = sr.id;
            goToEvent(sr.id);
          }
          imDisplayEventHover(pCtx.eventContainer[sr.id]);

          ImGui::SameLine();

          if (ImGui::Button(
                eastl::string(eastl::string::CtorSprintf{}, "Toggle Measured###%s:MeasuredEvents", sr.name->c_str()).c_str()))
          {
            toggleMeasuredEvent(sr.id);
          }
          setTooltip("You can add/remove the marker from the measured events");
        }
        ImGui::EndListBox();
      }
      ImGui::TreePop();
    }
  }
}

static void selectorPart()
{
  ImGui::Separator();
  ImGui::Text("Event Selector");
  ImGui::SameLine();

  const Event &focusedEvent = pCtx.eventContainer[pCtx.focusedEventId];


  ImGui::BeginDisabled(pCtx.historyPos <= 1);
  if (ImGui::Button("<--"))
  {
    pCtx.focusedEventId = pCtx.jumpHistory[--pCtx.historyPos - 1];
  }
  ImGui::EndDisabled();

  ImGui::SameLine();

  ImGui::BeginDisabled(pCtx.historyPos >= pCtx.jumpHistory.size());
  if (ImGui::Button("-->"))
  {
    pCtx.focusedEventId = pCtx.jumpHistory[++pCtx.historyPos - 1];
  }
  ImGui::EndDisabled();

  ImGui::SameLine();

  ImGui::BeginDisabled(-1 == focusedEvent.parentIdx);
  if (ImGui::Button("^"))
  {
    goToEvent(focusedEvent.parentIdx);
  }
  ImGui::EndDisabled();

  ImGui::SameLine();

  ImGui::BeginDisabled(pCtx.selectedEventId < 0);
  if (ImGui::Button("Goto") && pCtx.eventContainer[pCtx.selectedEventId].root == pCtx.selectedRoot)
  {
    goToEvent(pCtx.selectedEventId);
  }
  setTooltip("Focuses on selected event");

  ImGui::SameLine();

  if (ImGui::Button("Toggle Measured"))
  {
    toggleMeasuredEvent(pCtx.selectedEventId);
  }
  setTooltip("You can add/remove the selected marker from the measured events");

  ImGui::EndDisabled();

  imGuiMakeTree(pCtx.eventContainer[pCtx.focusedEventId], pCtx.focusedEventId);
}


// ImGui Main Windows

static void imGuiSelectorWindow()
{
  if (ImGui::BeginCombo("Thread Selector", pCtx.selectedItem))
  {
    setTooltip("You can select which thread you want to browse through \n(you can only browse the main thread of the GPU)");
    for (int i = 0; i < pCtx.rootNames.size(); i++)
    {
      bool is_selected = (pCtx.selectedItem == pCtx.rootNames[i]);
      if (ImGui::Selectable(pCtx.rootNames[i], is_selected))
      {
        pCtx.selectedItem = pCtx.rootNames[i];
        pCtx.selectedRoot = pCtx.roots[i];
        pCtx.focusedEventId = pCtx.selectedRoot;
        pCtx.jumpHistory.push_back(pCtx.focusedEventId);
        pCtx.historyPos++;
      }
      if (is_selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  else
    setTooltip("You can select which thread you want to browse through \n(you can only browse the main thread of the GPU)");

  searchPart();

  if (pCtx.focusedEventId != -1)
  {
    selectorPart();
  }
}

static void imGuiMeasuredWindow()
{
  if (pCtx.measuredEventIds.empty())
  {
    ImGui::Text("No measured events yet, please add measured events from the display menu");
  }
  else
  {
    for (int i = 0; i < pCtx.measuredEventIds.size(); i++)
    {
      const Event &ev = pCtx.eventContainer[pCtx.measuredEventIds[i]];

      imDisplayEvent(ev);

      char label[128];
      labelGenerator(label, sizeof(char) * 128, "Remove", ev);
      if (ImGui::Button(label))
      {
        pCtx.measuredEventIds.erase(pCtx.measuredEventIds.begin() + i);
      }

      ImGui::SameLine();

      labelGenerator(label, sizeof(char) * 128, "^", ev);

      ImGui::BeginDisabled(i == 0);
      if (ImGui::Button(label))
      {
        eastl::swap(pCtx.measuredEventIds[i - 1], pCtx.measuredEventIds[i]);
      }
      ImGui::EndDisabled();

      ImGui::SameLine();

      labelGenerator(label, sizeof(char) * 128, "v", ev);

      ImGui::BeginDisabled(i == pCtx.measuredEventIds.size() - 1);
      if (ImGui::Button(label))
      {
        eastl::swap(pCtx.measuredEventIds[i], pCtx.measuredEventIds[i + 1]);
      }
      ImGui::EndDisabled();
    }
  }
}

static void imGuiWindow()
{
  ImGui::Checkbox("Enable Auto Update", &pCtx.enableAutoUpdate);
  setTooltip("Enables the profiler to auto update the event data based on a choosable frequency");

  if (pCtx.enableAutoUpdate)
  {
    ImGui::SliderInt("Update Frequency (Every X frame)", &pCtx.updateFreq, 1, 1000);
    setTooltip("The data will be collected from the internal profiler every X frame. \n -Note: Setting this low may impact your FPS, "
               "however it does not effect the accuracy of the measured data, as profiling is turned of whilst data is collected.");
  }
  else if (ImGui::Button("Capture Data"))
  {
    setTooltip("Captures all profiling markers.");
    da_profiler::dump_frames(dumpExtractorFunction, &pCtx);
  }

  ImGui::Separator();

  if (pCtx.eventContainer.empty())
  {
    ImGui::Text("You have to captura data at least once to use this tool");
    return;
  }


  if (ImGui::BeginTabBar("Tab Selector", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton))
  {
    if (ImGui::BeginTabItem("Event Selector"))
    {
      setTooltip("You can search and browse through all the markers in this tab.");
      imGuiSelectorWindow();
      pCtx.measuredOpen = false;
      ImGui::EndTabItem();
    }
    else
      setTooltip("You can search and browse through all the markers in this tab.");

    char label[40];
    snprintf(label, sizeof(label), "Measured Events(%d)###Measured Events", (int)pCtx.measuredEventIds.size());

    if (ImGui::BeginTabItem(label))
    {
      setTooltip("You can check a list of all the measured(toggled) events in this tab");
      imGuiMeasuredWindow();
      pCtx.measuredOpen = true;
      ImGui::EndTabItem();
    }
    else
      setTooltip("You can check a list of all the measured(toggled) events in this tab");
    ImGui::EndTabBar();
  }
}

#if TIME_PROFILER_ENABLED
REGISTER_IMGUI_WINDOW("render", "Profiler", imGuiWindow);
#endif


namespace render::imgui_profiler
{
void update_profiler(int frameNo)
{
  if (pCtx.enableAutoUpdate && frameNo % pCtx.updateFreq == 0)
  {
    da_profiler::dump_frames(dumpExtractorFunction, &pCtx);
  }
}
} // namespace render::imgui_profiler