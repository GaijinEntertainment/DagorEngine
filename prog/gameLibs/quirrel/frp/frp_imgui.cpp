// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ctype.h>
#include <generic/dag_sort.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <quirrel/frp/dag_frp.h>

using namespace sqfrp;

namespace sqfrp
{

int pull_frp_imgui = 0;

typedef uint16_t VisualNodeIndex;
static constexpr VisualNodeIndex NULL_INDEX = ~VisualNodeIndex(0);

struct VisualNode
{
  const BaseObservable *observable;
  dag::Vector<VisualNodeIndex> sources, watchers;
  SimpleString name, sourceFile, filterText;
  uint16_t sourceLine;
  uint16_t numSubscribers;

  union
  {
    int16_t numUncollectedSources;
    int16_t visIndex;
    VisualNodeIndex newIndex;
  };

  union
  {
    struct
    {
      bool visible : 1;
      bool isUsed : 1;
      bool needImmediate : 1;
      bool collected : 1;
      bool selected : 1;
      bool linkHovered : 1;
    };
    uint8_t flags;
  };
  uint8_t selectionHue;
  uint16_t busRank;

  bool isHidden() const { return !visible && !selectionHue; }
};

// connected sub-graph
struct GraphComponent
{
  VisualNodeIndex startIndex, endIndex;
  VisualNodeIndex numSelected;

  size_t size() const { return endIndex - startIndex; }
  bool contains(VisualNodeIndex i) const { return startIndex <= i && i < endIndex; }
  dag::Vector<VisualNode>::iterator begin() const;
  dag::Vector<VisualNode>::iterator end() const;
};

struct VisualLink
{
  VisualNodeIndex from = NULL_INDEX;
  int16_t linkedNum = 0;
  int endIndex = 0;
  ImVec2 startPos;
};

struct ViewerSelection
{
  const BaseObservable *observable = nullptr;
  int hue = 0;
  ViewerSelection() = default;
  ViewerSelection(const BaseObservable *o, int h) : observable(o), hue(h) {}
};

} // namespace sqfrp

template <>
struct eastl::hash<ViewerSelection>
{
  ska::hash_size_t operator()(const ViewerSelection &self) { return eastl::hash<const void *>()(self.observable); }
};

template <>
struct eastl::equal_to<ViewerSelection>
{
  bool operator()(const ViewerSelection &a, const ViewerSelection &b) { return a.observable == b.observable; }
};


static ObservablesGraph *cur_graph = nullptr;
static dag::Vector<VisualNode> all_nodes;
static dag::Vector<GraphComponent> components;
static ska::flat_hash_map<const BaseObservable *, VisualNodeIndex> node_map;
static int num_computed = 0, num_used = 0, num_used_computed = 0, num_subscribers = 0, num_components;
static bool need_sorting = false, pending_sort = false;
static dag::Vector<VisualNodeIndex> sorted_nodes;
static ska::flat_hash_set<ViewerSelection> selected_nodes;
static ImGuiTextFilter name_filter;
static char name_select_buffer[256];
static VisualNodeIndex last_hovered = NULL_INDEX;
static VisualNodeIndex last_hovered_link = NULL_INDEX;
static VisualNodeIndex hovered_node = NULL_INDEX;
static VisualNodeIndex hovered_link = NULL_INDEX;

static void reset_graph(ObservablesGraph *g)
{
  cur_graph = g;
  all_nodes.clear();
  components.clear();
  node_map.clear();
  selected_nodes.clear();
  sorted_nodes.clear();
  last_hovered = NULL_INDEX;
  last_hovered_link = NULL_INDEX;
  hovered_node = NULL_INDEX;
  hovered_link = NULL_INDEX;
}

dag::Vector<VisualNode>::iterator GraphComponent::begin() const { return all_nodes.begin() + startIndex; }

dag::Vector<VisualNode>::iterator GraphComponent::end() const { return all_nodes.begin() + endIndex; }

static void process_multi_selection(ImGuiMultiSelectIO *ms, bool ignore = false)
{
  if (!ms || ignore)
    return;
  for (auto &r : ms->Requests)
    switch (r.Type)
    {
      case ImGuiSelectionRequestType_SetAll:
        // not supporting "select all" on purpose
        if (!selected_nodes.empty())
        {
          pending_sort = true;
          name_select_buffer[0] = 0;
        }
        selected_nodes.clear();
        for (auto &[_, ni] : node_map)
          all_nodes[ni].selected = false;
        break;

      case ImGuiSelectionRequestType_SetRange:
      {
        G_ASSERTF(r.RangeFirstItem == r.RangeLastItem, "range selection not supported");
        auto item = reinterpret_cast<const BaseObservable *>(r.RangeFirstItem);
        if (auto it = node_map.find(item); it != node_map.end())
          if (all_nodes[it->second].selected != r.Selected)
          {
            all_nodes[it->second].selected = r.Selected;
            pending_sort = true;
            name_select_buffer[0] = 0;
          }
        if (r.Selected)
        {
          auto si = eastl::max_element(selected_nodes.begin(), selected_nodes.end(),
            [](const ViewerSelection &a, const ViewerSelection &b) { return a.hue < b.hue; });
          selected_nodes.insert(ViewerSelection(item, si == selected_nodes.end() ? 1 : si->hue + 1));
        }
        else
          selected_nodes.erase(ViewerSelection(item, 0));
        break;
      }

      default: G_ASSERTF(false, "unexpected ImGuiSelectionRequestType %d", r.Type);
    }
}

static void compute_depth(VisualNode &n, int &max_depth, int depth = 0)
{
  if (n.watchers.empty())
  {
    if (depth > max_depth)
      max_depth = depth;
  }
  else
    for (auto wi : n.watchers)
      compute_depth(all_nodes[wi], max_depth, depth + 1);
}

static void collect_sources(VisualNodeIndex ni, int sel)
{
  auto &n = all_nodes[ni];
  if (n.collected)
    return;
  n.collected = true;
  if (!n.selected && sel > n.selectionHue)
    n.selectionHue = sel;
  for (auto s : n.sources)
    collect_sources(s, sel);
  sorted_nodes.push_back(ni);
}

static void collect_sources(GraphComponent &c, VisualNodeIndex ni, int sel = 0)
{
  if (!c.contains(ni))
    return;
  auto &n = all_nodes[ni];
  if (n.selected)
  {
    G_ASSERT(sel > 0);
    n.selectionHue = sel;
  }
  collect_sources(ni, sel);
}

static void collect_watchers(VisualNodeIndex ni, int sel)
{
  auto &n = all_nodes[ni];
  if (!n.collected)
    collect_sources(ni, 0);
  if (!n.selected && sel > n.selectionHue)
    n.selectionHue = sel;
  for (auto wi : n.watchers)
    if (!all_nodes[wi].collected)
      collect_watchers(wi, sel);
}

static void collect_watchers(GraphComponent &c, VisualNodeIndex ni, int sel = 0)
{
  if (!c.contains(ni))
    return;
  collect_watchers(ni, sel);
}

static ImVec2 last_line_pos, clip_min, clip_max;
static ImU32 line_color;
static ImDrawList *draw_list = nullptr;

static void start_line_drawing()
{
  draw_list = ImGui::GetWindowDrawList();
  clip_min = ImGui::GetWindowPos();
  clip_max = clip_min + ImGui::GetWindowSize();
  draw_list->PushClipRect(clip_min, clip_max);
}

static void start_line(const ImVec2 &pos, ImU32 color)
{
  line_color = color;
  last_line_pos = pos;
}

static void set_line_color(ImU32 color) { line_color = color; }

static void draw_line(const ImVec2 &pos)
{
  ImVec2 p0(min(pos.x, last_line_pos.x), min(pos.y, last_line_pos.y));
  ImVec2 p1(max(pos.x, last_line_pos.x), max(pos.y, last_line_pos.y));
  if (p1.y >= clip_min.y && p0.y <= clip_max.y)
    draw_list->AddRectFilled(p0, p1 + ImVec2(1, 1), line_color);
  last_line_pos = pos;
}

static void end_line_drawing() { draw_list->PopClipRect(); }

static ImU32 link_color(VisualNodeIndex from, bool hover, int sel)
{
  if (last_hovered_link == from || (hover && last_hovered_link == NULL_INDEX))
    return IM_COL32(255, 255, 255, 255);
  else if (sel)
  {
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(sel * (2 - 1.618033988749f), 0.9f, 1, r, g, b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1));
  }
  else
    return IM_COL32(130, 130, 130, 255);
}

void sqfrp::graph_viewer()
{
  // validate current graph pointer
  auto allGraphs = get_all_graphs();
  if (auto it = eastl::find(allGraphs.begin(), allGraphs.end(), cur_graph); it == allGraphs.end())
    reset_graph(nullptr);

  // select graph
  stlsort::sort(allGraphs.begin(), allGraphs.end(),
    [](const ObservablesGraph *a, const ObservablesGraph *b) { return a->graphName < b->graphName; });
  if (!cur_graph && !allGraphs.empty())
    reset_graph(allGraphs.front());

  if (ImGui::BeginCombo("##select-graph", cur_graph ? cur_graph->graphName.c_str() : "(select graph to view)",
        ImGuiComboFlags_WidthFitPreview))
  {
    for (auto g : allGraphs)
      if (ImGui::Selectable(g->graphName.c_str(), g == cur_graph))
        reset_graph(g);
    ImGui::EndCombo();
  }

  ImGui::SameLine();
  if (ImGui::Button("Reload"))
  {
    decltype(selected_nodes) selection;
    selection.swap(selected_nodes);
    reset_graph(cur_graph);
    selection.swap(selected_nodes);
  }

  // get nodes
  bool needReset = false;
  {
    OSSpinlockScopedLock guard(cur_graph->allObservablesLock);
    if (node_map.size() != cur_graph->allObservables.size())
    {
      // grab a copy and unlock, we're going to get busy
      needReset = true;
      node_map.clear();
      node_map.reserve(cur_graph->allObservables.size());
      all_nodes.clear();
      all_nodes.resize(cur_graph->allObservables.size());
      num_used = 0;
      num_used_computed = 0;
      num_subscribers = 0;

      VisualNodeIndex ni = 0;
      for (auto o : cur_graph->allObservables)
      {
        auto &n = all_nodes[ni];
        n.flags = 0;
        n.observable = o;
        n.numSubscribers = o->scriptSubscribers.size();
        num_subscribers += n.numSubscribers;
        node_map[o] = ni;

        if (auto info = o->getInitInfo())
        {
          if (info->initFuncName == "__main__")
            n.name = String(0, "(%s:%u)", info->initSourceFileName.c_str(), info->initSourceLine);
          else
            n.name = info->initFuncName;
          n.sourceFile = info->initSourceFileName;
          n.sourceLine = info->initSourceLine;
          n.filterText = String(0, "%s %s:%u", n.name.c_str(), n.sourceFile.c_str(), n.sourceLine).toLower();
        }
        else
        {
          String s;
          o->fillInfo(s);
          n.name = s;
          n.sourceLine = 0;
          n.filterText = n.name;
        }

        if (o->getComputed())
        {
          if (o->isUsed)
            num_used_computed++;
          n.isUsed = o->isUsed;
          n.needImmediate = o->needImmediate;
        }
        else
        {
          n.isUsed = !o->scriptSubscribers.empty();
          if (!n.isUsed)
            for (auto w : o->watchers)
              if (auto c = w->getComputed(); !c || c->isUsed)
              {
                n.isUsed = true;
                break;
              }
        }

        if (n.isUsed)
          num_used++;

        if (selected_nodes.count(ViewerSelection(o, 0)) > 0)
          n.selected = true;

        ni++;
      }

      num_computed = 0;
      for (auto &n : all_nodes)
        if (auto c = n.observable->getComputed())
        {
          num_computed++;
          n.sources.reserve(c->sources.size());
          for (auto &s : c->sources)
          {
            auto it = node_map.find(s.observable);
            if (it == node_map.end())
              continue;
            n.sources.push_back(it->second);
            all_nodes[it->second].watchers.push_back(&n - &all_nodes[0]);
          }
        }

      need_sorting = true;
    }
  }

  if (needReset)
  {
    // find components
    components.clear();
    unsigned nextIndex = 0;

    // note: collected flag is reset above when adding nodes
    eastl::function<void(VisualNode &)> collect = [&collect, &nextIndex](VisualNode &n) {
      if (n.collected)
        return;
      n.collected = true;
      n.newIndex = nextIndex++;
      for (auto s : n.sources)
        collect(all_nodes[s]);
      for (auto w : n.watchers)
        collect(all_nodes[w]);
    };

    for (auto &n : all_nodes)
      if (!n.collected)
      {
        auto &c = components.push_back();
        c.startIndex = nextIndex;
        collect(n);
        c.endIndex = nextIndex;
      }

    // remap indices
    for (auto &n : all_nodes)
    {
      node_map[n.observable] = n.newIndex;
      for (auto &s : n.sources)
        s = all_nodes[s].newIndex;
      for (auto &w : n.watchers)
        w = all_nodes[w].newIndex;
    }

    // permute nodes in the array
    dag::Vector<VisualNode> nodes;
    nodes.resize(all_nodes.size());
    for (auto &n : all_nodes)
    {
      using eastl::swap;
      swap(n, nodes[n.newIndex]);
    }
    all_nodes.swap(nodes);

    // compute stats
    num_components = 0;
    for (auto &c : components)
      if (c.size() > 1)
        num_components++;
  }

  if (pending_sort && !ImGui::GetIO().KeyCtrl)
  {
    pending_sort = false;
    need_sorting = true;
  }

  // select by name
  bool needScroll = false;
  float buttonSize = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2;

  ImGui::SameLine();
  ImGui::SetNextItemWidth(300);
  bool textChanged = ImGui::InputTextWithHint("##name-select", "Select by name", name_select_buffer, IM_ARRAYSIZE(name_select_buffer));

  ImGui::SameLine(0, 0);
  if (ImGui::Button("x##clear-name-select", ImVec2(buttonSize, buttonSize)) && name_select_buffer[0])
  {
    name_select_buffer[0] = 0;
    textChanged = true;
  }

  if (textChanged)
  {
    for (char *p = name_select_buffer; *p; p++)
      *p = tolower(*p);
    need_sorting = true;
    needScroll = true;
    selected_nodes.clear();
    int hue = 0;
    for (auto &n : all_nodes)
    {
      bool sel = name_select_buffer[0] && selected_nodes.size() < 1 && strstr(n.filterText, name_select_buffer);
      n.selected = sel;
      if (sel)
        selected_nodes.insert(ViewerSelection(n.observable, ++hue));
    }
  }

  // filter by name
  ImGui::SameLine();
  ImGui::SetNextItemWidth(300);
  textChanged =
    ImGui::InputTextWithHint("##name-filter", "Filter (-exc,inc)", name_filter.InputBuf, IM_ARRAYSIZE(name_filter.InputBuf));

  ImGui::SameLine(0, 0);
  if (ImGui::Button("x##clear-name-filter", ImVec2(buttonSize, buttonSize)))
  {
    name_filter.Clear();
    textChanged = true;
  }

  if (textChanged || need_sorting)
  {
    name_filter.Build();
    needScroll = true;
    for (auto &n : all_nodes)
      n.visible = !name_filter.IsActive() || name_filter.PassFilter(n.filterText);
  }

  static String saveMessage;
  if (cur_graph)
  {
    ImGui::SameLine();
    if (ImGui::Button("Export DOT"))
    {
      DagorDateTime time;
      ::get_local_time(&time);
      String fname(256, "%s %04d.%02d.%02d %02d.%02d.%02d.dot", "frp", time.year, time.month, time.day, time.hour, time.minute,
        time.second);

      if (auto file = df_open(fname.c_str(), DF_CREATE | DF_WRITE | DF_REALFILE_ONLY))
      {
        df_cprintf(file, "digraph \"%s\" {\n", cur_graph->graphName.c_str());
        for (auto &n : all_nodes)
        {
          df_cprintf(file, "_%p[label=\"%s\"", n.observable, n.name.c_str());
          if (n.sources.empty())
            df_cprintf(file, ",shape=box");
          df_cprintf(file, "];\n");
          for (auto wi : n.watchers)
            df_cprintf(file, "_%p->_%p;\n", n.observable, all_nodes[wi].observable);
        }
        df_cprintf(file, "}\n");
        df_close(file);
        saveMessage.setStrCat("Saved ", fname.c_str());
      }
      else
        saveMessage.setStrCat("Failed to save ", fname.c_str());

      ImGui::OpenPopup("SaveMessage");
    }
  }

  if (ImGui::BeginPopup("SaveMessage"))
  {
    ImGui::TextUnformatted(saveMessage.begin(), saveMessage.end());
    ImGui::EndPopup();
  }

  // title text
  auto drawList = ImGui::GetWindowDrawList();
  drawList->PushClipRectFullScreen();
  drawList->AddText(ImGui::GetWindowPos() + ImGui::GetStyle().FramePadding +
                      ImVec2(ImGui::CalcTextSize("Graph ViewerX").x + ImGui::GetFontSize() + ImGui::GetStyle().ItemInnerSpacing.x, 0),
    ImGui::GetColorU32(ImGuiCol_Text),
    String(0, "%s: %d/%d used Computed, %d/%d used Watched, %d subscribers, %d clusters",
      all_nodes.size() >= 3000 ? "aka Spaghetti Browser " : "", num_used_computed, num_computed, num_used - num_used_computed,
      all_nodes.size() - num_computed, num_subscribers, num_components));
  drawList->PopClipRect();

  // main display thing
  ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 120));
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0, 0, 255, 255));
  ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(0, 0, 0, 255));
  ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(0, 180, 255, 255));
  if (ImGui::BeginChild("display", ImVec2(0, 0), ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_HorizontalScrollbar))
  {
    float startY = ImGui::GetCursorScreenPos().y;

    process_multi_selection(ImGui::BeginMultiSelect(ImGuiMultiSelectFlags_NoRangeSelect | ImGuiMultiSelectFlags_NoSelectAll,
      selected_nodes.size(), sorted_nodes.size()));

    if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
    {
      auto delta = ImGui::GetIO().MouseDelta;
      ImGui::SetScrollX(ImGui::GetScrollX() - delta.x);
      ImGui::SetScrollY(ImGui::GetScrollY() - delta.y);
    }

    if (need_sorting)
    {
      need_sorting = false;
      pending_sort = false;
      needScroll = true;
      sorted_nodes.clear();
      sorted_nodes.reserve(all_nodes.size());

      // reset collected flags
      VisualNodeIndex defaultStart = NULL_INDEX;
      for (auto &comp : components)
      {
        comp.numSelected = 0;
        for (auto &n : comp)
        {
          if (n.selected)
            comp.numSelected++;
          n.collected = false;
          n.selectionHue = 0;
          if (n.sources.empty() && (defaultStart == NULL_INDEX || n.watchers.size() > all_nodes[defaultStart].watchers.size()))
            defaultStart = &n - &all_nodes[0];
        }
      }

      // sort components
      stlsort::sort(components.begin(), components.end(), [](const GraphComponent &a, const GraphComponent &b) {
        return a.numSelected == b.numSelected ? a.size() > b.size() : a.numSelected > b.numSelected;
      });

      // process each component separately
      dag::Vector<VisualNodeIndex> computeds;

      for (auto &comp : components)
      {
        int sortedStart = sorted_nodes.size();

        // collect selected nodes with their sources
        int hue = 0;
        for (auto selIter = selected_nodes.begin(); selIter != selected_nodes.end();)
          if (auto it = node_map.find(selIter->observable); it != node_map.end())
          {
            collect_sources(comp, it->second, selIter->hue);
            selIter++;
          }
          else
            selIter = selected_nodes.erase(selIter);

        if (selected_nodes.empty())
          collect_sources(comp, defaultStart);

        // collect related watchers
        hue = 0;
        for (auto vs : selected_nodes)
          collect_watchers(comp, node_map.find(vs.observable)->second, vs.hue);

        if (selected_nodes.empty())
          collect_watchers(comp, defaultStart);

        // gather remaining Computeds
        computeds.reserve(comp.size() - (sorted_nodes.size() - sortedStart));

        for (auto &n : comp)
          if (!n.collected && n.isUsed && !n.sources.empty())
          {
            n.numUncollectedSources = 0;
            for (auto s : n.sources)
              if (!all_nodes[s].collected)
                n.numUncollectedSources++;
            computeds.push_back(&n - &all_nodes[0]);
          }

        stlsort::sort(computeds.begin(), computeds.end(), [](const VisualNodeIndex a, const VisualNodeIndex b) {
          return all_nodes[a].numUncollectedSources < all_nodes[b].numUncollectedSources;
        });

        // collect Computeds
        for (auto ni : computeds)
          collect_sources(comp, ni);

        // if anything is left, it's in a loop, or not used
        for (auto &n : comp)
          if (!n.collected)
            collect_sources(comp, &n - &all_nodes[0]);
      }

      // set indices
      int i = 0;
      for (auto ni : sorted_nodes)
        all_nodes[ni].visIndex = i++;

      // sort sources
      auto indexCmp = [](const VisualNodeIndex a, const VisualNodeIndex b) { return all_nodes[a].visIndex < all_nodes[b].visIndex; };
      for (auto &n : all_nodes)
      {
        stlsort::sort(n.sources.begin(), n.sources.end(), indexCmp);
        stlsort::sort(n.watchers.begin(), n.watchers.end(), indexCmp);
      }
    }

    if (last_hovered != hovered_node)
    {
      last_hovered = hovered_node;

      // clear flags
      int hoverIndex = -1;
      for (int i = 0; i < sorted_nodes.size(); i++)
      {
        all_nodes[sorted_nodes[i]].linkHovered = false;
        if (sorted_nodes[i] == last_hovered)
          hoverIndex = i;
      }

      if (hoverIndex != -1)
      {
        all_nodes[last_hovered].linkHovered = true;
        auto hoverVisIndex = all_nodes[last_hovered].visIndex;

        // propagate linkHovered flag down
        for (int i = hoverIndex + 1; i < sorted_nodes.size(); i++)
          for (auto s : all_nodes[sorted_nodes[i]].sources)
            if (all_nodes[s].linkHovered)
            {
              all_nodes[sorted_nodes[i]].linkHovered = true;
              break;
            }

        // propagate linkHovered flag up
        for (int i = hoverIndex - 1; i >= 0; i--)
          for (auto wi : all_nodes[sorted_nodes[i]].watchers)
            if (auto &w = all_nodes[wi]; w.linkHovered && w.visIndex <= hoverVisIndex)
            {
              all_nodes[sorted_nodes[i]].linkHovered = true;
              break;
            }
      }
    }

    if (last_hovered_link != hovered_link)
    {
      if (last_hovered != NULL_INDEX)
        last_hovered_link = NULL_INDEX;
      else
      {
        last_hovered_link = hovered_link;

        // clear flags
        for (auto &n : all_nodes)
          n.linkHovered = false;

        if (last_hovered_link != NULL_INDEX)
        {
          auto &fromNode = all_nodes[last_hovered_link];
          fromNode.linkHovered = true;
          for (auto wi : fromNode.watchers)
            all_nodes[wi].linkHovered = true;
        }
      }
    }

    hovered_node = NULL_INDEX;
    hovered_link = NULL_INDEX;

    // set visIndex so we can compute link length
    int visIndex = 0;
    for (auto ni : sorted_nodes)
    {
      auto &n = all_nodes[ni];
      if (n.isHidden())
        continue;
      n.visIndex = visIndex++;
      n.busRank = 1;
    }

    dag::Vector<VisualLink> links;
    dag::Vector<uint16_t> sortedLinks;

    if (!components.empty())
      links.reserve(eastl::max_element(components.begin(), components.end(), [](const GraphComponent &a, const GraphComponent &b) {
        return a.size() < b.size();
      })->size());
    sortedLinks.reserve(links.capacity());

    // compute bus width and link ranks
    int busWidth = 0;
    for (auto ni : sorted_nodes)
    {
      auto &n = all_nodes[ni];
      if (n.isHidden())
        continue;

      for (auto s : n.sources)
      {
        if (all_nodes[s].isHidden())
          continue;
        auto it = eastl::find_if(sortedLinks.begin(), sortedLinks.end(), [&links, s](unsigned li) { return links[li].from == s; });
        if (it == sortedLinks.end())
          continue;

        auto &l = links[*it];
        if (--l.linkedNum <= 0)
        {
          // close link
          busWidth = max<int>(busWidth, all_nodes[l.from].busRank);
          l.from = NULL_INDEX;
          sortedLinks.erase(it);
        }
      }

      int numWatchers = 0;
      int endIndex = 0;
      VisualNodeIndex adjacent = NULL_INDEX;
      for (auto wi : n.watchers)
      {
        auto &w = all_nodes[wi];
        if (w.isHidden())
          continue;
        if (w.visIndex == n.visIndex + 1)
          adjacent = wi;
        numWatchers++;
        endIndex = w.visIndex;
      }

      if (numWatchers > 0 && (adjacent == NULL_INDEX || numWatchers > 1))
      {
        // new link
        auto it = eastl::find_if(links.begin(), links.end(), [](const VisualLink &l) { return l.from == NULL_INDEX; });
        if (it == links.end())
          it = links.append_default(1);
        it->from = ni;
        it->linkedNum = numWatchers;
        it->endIndex = endIndex;

        // insert sorted by remaining length
        int insertPos = eastl::upper_bound(sortedLinks.begin(), sortedLinks.end(), endIndex, [&links](int end, unsigned li) {
          return end > links[li].endIndex;
        }) - sortedLinks.begin();
        sortedLinks.insert(sortedLinks.begin() + insertPos, it - links.begin());

        // compute new link rank
        n.busRank = sortedLinks.size() - insertPos;

        // update rank of links up to and including new one
        for (int i = min<int>(insertPos, sortedLinks.size() - 2); i >= 0; i--)
        {
          auto &linkNode = all_nodes[links[sortedLinks[i + 0]].from];
          auto &nextNode = all_nodes[links[sortedLinks[i + 1]].from];
          linkNode.busRank = max<unsigned>(linkNode.busRank, nextNode.busRank + 1);
        }
      }
    }

    G_ASSERT(sortedLinks.empty());
    clear_and_shrink(sortedLinks);
    links.resize(busWidth);

    // display nodes and links
    const float linkStep = 5;
    float linkUpY = -1, linkDownY = -1;
    bool ignoreMultiselect = false;
    start_line_drawing();

    if (needScroll)
      ImGui::SetScrollFromPosX(ImGui::GetCursorScreenPos().x + busWidth * linkStep);

    int outLinkIndex = -1;
    int hoverAt = -1;
    for (auto ni : sorted_nodes)
    {
      auto &n = all_nodes[ni];
      if (n.isHidden())
        continue;

      // link hover
      auto orgPos = ImGui::GetCursorScreenPos();
      auto rmin = orgPos + ImVec2(0, -2 * linkStep);
      auto rmax = orgPos + ImVec2((n.sources.size() + 1 + busWidth + 1) * linkStep,
                             (n.sources.size() + 1) * linkStep + ImGui::GetTextLineHeight());
      if (ImGui::IsMouseHoveringRect(rmin, rmax))
      {
        auto mp = ImGui::GetMousePos() - rmin;
        int bi = roundf(mp.x / linkStep);
        int si = roundf(mp.y / linkStep);
        if (bi < busWidth && links[bi].from != NULL_INDEX)
        {
          hovered_link = links[bi].from;
        }
        else if (si <= 2)
        {
          if (outLinkIndex >= 0 && mp.x >= (outLinkIndex - 0.5f) * linkStep)
            hovered_link = links[outLinkIndex].from;
        }
        else
        {
          si = n.sources.size() + 2 - si;
          if (si >= 0)
          {
            auto s = n.sources[si];
            auto it = eastl::find_if(links.begin(), links.end(), [s](const VisualLink &l) { return l.from == s; });
            if (it != links.end() && mp.x >= ((it - links.begin()) - 0.5f) * linkStep)
              hovered_link = s;
          }
        }

        if (hovered_link != NULL_INDEX)
          hoverAt = n.visIndex;
      }

      // source links
      int si = -1;
      for (auto s : n.sources)
      {
        si++;
        auto endPos = orgPos + ImVec2((si + 1 + busWidth + 1) * linkStep, (n.sources.size() - si) * linkStep);

        auto &sn = all_nodes[s];
        if (sn.isHidden())
        {
          // stub to hidden source
          start_line(endPos, IM_COL32(100, 100, 100, 255));
          draw_line(ImVec2(endPos.x, orgPos.y + (n.sources.size() + 1) * linkStep));
          continue;
        }

        if (last_hovered_link == s)
        {
          if (hovered_link == NULL_INDEX)
            linkUpY = ImGui::GetCursorScreenPos().y - startY;
          else if (linkDownY < 0)
            linkDownY = ImGui::GetCursorScreenPos().y - startY + ImGui::GetTextLineHeight() + (n.sources.size() + 2) * linkStep;
        }

        auto it = eastl::find_if(links.rbegin(), links.rend(), [s](const VisualLink &l) { return l.from == s; });
        if (it == links.rend())
          continue;

        if (sn.visIndex + 1 != n.visIndex)
        {
          // bus segment
          bool hover = false;
          int sel = 0;
          for (auto wi : sn.watchers)
          {
            auto &w = all_nodes[wi];
            if (w.isHidden() || w.visIndex < n.visIndex)
              continue;
            hover = hover || w.linkHovered;
            sel = max<int>(sel, w.selectionHue);
          }
          sel = min<int>(sn.selectionHue, sel);

          start_line(it->startPos, link_color(s, sn.linkHovered && hover, sel));
          draw_line(ImVec2(it->startPos.x, endPos.y));

          // source connection
          set_line_color(link_color(s, sn.linkHovered && n.linkHovered, min<int>(sn.selectionHue, n.selectionHue)));
          draw_line(endPos);
          draw_line(ImVec2(endPos.x, orgPos.y + (n.sources.size() + 1) * linkStep));

          it->startPos.y = endPos.y;
        }

        if (--it->linkedNum <= 0)
          it->from = NULL_INDEX;
      }

      auto endPos = orgPos + ImVec2((busWidth + 2) * linkStep, ImGui::GetTextLineHeight() + (n.sources.size() + 1) * linkStep);
      int numWatchers = 0;
      VisualNodeIndex adjacent = NULL_INDEX;
      bool hover = false;
      int sel = 0;
      for (auto wi : n.watchers)
      {
        auto &w = all_nodes[wi];
        if (w.isHidden())
          continue;
        if (w.visIndex == n.visIndex + 1)
          adjacent = wi;
        numWatchers++;
        hover = hover || w.linkHovered;
        sel = max<int>(sel, w.selectionHue);
      }
      sel = min<int>(n.selectionHue, sel);

      outLinkIndex = -1;
      if (numWatchers > 0)
      {
        ImU32 color = link_color(ni, n.linkHovered && hover, sel);

        if (adjacent != NULL_INDEX)
        {
          // direct line to next node in list
          auto &w = all_nodes[adjacent];
          int si = eastl::find(w.sources.begin(), w.sources.end(), ni) - w.sources.begin();
          endPos = orgPos + ImVec2((si + 1 + busWidth + 1) * linkStep,
                              ImGui::GetTextLineHeight() + (n.sources.size() + 3 + w.sources.size()) * linkStep);
          start_line(endPos, color);
          draw_line(ImVec2(endPos.x, orgPos.y + ImGui::GetTextLineHeight() + (n.sources.size() + 1) * linkStep));

          if (numWatchers > 1)
          {
            hover = false;
            sel = 0;
            for (auto wi : n.watchers)
            {
              auto &w = all_nodes[wi];
              if (w.isHidden() || wi == adjacent)
                continue;
              hover = hover || w.linkHovered;
              sel = max<int>(sel, w.selectionHue);
            }
            sel = min<int>(n.selectionHue, sel);
            color = link_color(ni, n.linkHovered && hover, sel);
          }
        }

        if (adjacent == NULL_INDEX || numWatchers > 1)
        {
          // new link in the bus
          G_ASSERT(n.busRank <= links.size());
          auto it = links.end() - n.busRank;
          G_ASSERT(it->from == NULL_INDEX);
          outLinkIndex = it - links.begin();
          it->from = ni;
          it->linkedNum = numWatchers;
          it->startPos =
            orgPos + ImVec2((it - links.begin()) * linkStep, ImGui::GetTextLineHeight() + (n.sources.size() + 2) * linkStep);
          start_line(it->startPos, color);
          draw_line(ImVec2(endPos.x, it->startPos.y));
          if (adjacent == NULL_INDEX)
            draw_line(endPos);

          if (last_hovered_link == ni)
            linkUpY = ImGui::GetCursorScreenPos().y - startY;
        }
      }
      else if (!n.watchers.empty())
      {
        // stub to hidden watchers
        start_line(endPos, IM_COL32(100, 100, 100, 255));
        draw_line(ImVec2(endPos.x, endPos.y + linkStep));
      }

      ImGui::PushID(n.observable);
      ImGui::SetCursorScreenPos(orgPos + ImVec2((busWidth + 1) * linkStep, (n.sources.size() + 1) * linkStep));

      ImU32 textColor;
      if (ni == last_hovered || n.linkHovered)
        textColor = IM_COL32(255, 255, 255, 255);
      else if (n.selectionHue)
      {
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(n.selectionHue * (2 - 1.618033988749f), 0.9f, 1, r, g, b);
        textColor = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1));
      }
      else if (!n.isUsed || !n.collected)
        textColor = IM_COL32(180, 50, 50, 255);
      else if (n.needImmediate)
        textColor = IM_COL32(180, 50, 180, 255);
      else
        textColor = IM_COL32(180, 180, 180, 255);
      ImGui::PushStyleColor(ImGuiCol_Text, textColor);

      ImGui::SetNextItemSelectionUserData(reinterpret_cast<ImGuiSelectionUserData>(n.observable));
      if (ImGui::Selectable(n.name.c_str(), n.selected, 0, ImVec2(ImGui::CalcTextSize(n.name.c_str()).x, 0)))
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyAlt)
        {
          ignoreMultiselect = true;
          if (!n.sourceFile.empty())
          {
#if _TARGET_PC_WIN
            // Open source file via VSCode
            // https://code.visualstudio.com/docs/editor/command-line
            String cmd(0, "code -g %s:%u", n.sourceFile.c_str(), n.sourceLine);
            system(cmd.c_str());
#endif
          }
        }

      if (ImGui::IsItemHovered())
      {
        hovered_node = ni;
        hovered_link = NULL_INDEX;
      }

      if (needScroll && n.selected)
      {
        needScroll = false;
        ImGui::SetScrollHereX();
        ImGui::SetScrollHereY();
      }

      ImGui::PopStyleColor();
      ImGui::PopID();

      if (ImGui::BeginItemTooltip())
      {
        ImGui::TextUnformatted(n.name);
        ImGui::TextDisabled("%p", n.observable);
        if (!n.sourceFile.empty())
          ImGui::Text("%s:%u", n.sourceFile.c_str(), n.sourceLine);
        if (!n.isUsed)
          ImGui::TextColored(ImVec4(1, 0.1f, 0.1f, 1), "NOT USED");
        if (n.needImmediate)
          ImGui::TextColored(ImVec4(1, 0.1f, 1, 1), "IMMEDIATE");
        if (n.numSubscribers > 0)
          ImGui::TextColored(ImVec4(1, 0.9f, 0, 1), "%u subscribers", n.numSubscribers);

        ImGui::SeparatorText("Cluster stats");
        if (
          auto compIt = eastl::find_if(components.begin(), components.end(), [ni](const GraphComponent &c) { return c.contains(ni); });
          compIt != components.end())
        {
          int numEdges = 0, numSources = 0, numSinks = 0, numSubscribers = 0, maxDepth = 0;
          for (auto &cn : *compIt)
          {
            numSubscribers += cn.numSubscribers;
            if (cn.sources.empty())
            {
              numSources++;
              compute_depth(cn, maxDepth);
            }
            else
            {
              numEdges += cn.sources.size();
              if (cn.watchers.empty())
                numSinks++;
            }
          }

          if (ImGui::BeginTable("stats", 2))
          {
#define STAT(label, fmt, value) \
  ImGui::TableNextColumn();     \
  ImGui::Text(label);           \
  ImGui::TableNextColumn();     \
  ImGui::Text(fmt, value);
            STAT("Size", "%d", int(compIt->size()))
            STAT("Watched", "%d", numSources)
            STAT("Edges", "%d", numEdges)
            STAT("Sinks", "%d", numSinks)
            STAT("Depth", "%d", maxDepth)
            STAT("Subscribers", "%d", numSubscribers)
#undef STAT
            ImGui::EndTable();
          }
        }

        ImGui::Separator();
        ImGui::TextDisabled("Drag with middle mouse button to scroll");
        ImGui::TextDisabled("Hold Ctrl for multi-select");
        ImGui::TextDisabled("Alt-left-click to open source");
        ImGui::EndTooltip();
      }
    }

    if (hovered_link != NULL_INDEX)
    {
      if (ImGui::BeginTooltip())
      {
        auto &ln = all_nodes[hovered_link];
        ImGui::TextUnformatted(ln.name);
        ImGui::Indent();
        for (auto wi : ln.watchers)
        {
          auto &w = all_nodes[wi];
          if (w.isHidden())
            continue;
          if (hoverAt >= 0 && w.visIndex >= hoverAt)
          {
            ImGui::Separator();
            hoverAt = -1;
          }
          ImGui::TextUnformatted(w.name);
        }
        ImGui::Unindent();

        ImGui::Separator();
        ImGui::TextDisabled("Left-click to scroll up");
        ImGui::TextDisabled("Right-click to scroll down");

        ImGui::EndTooltip();
      }

      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && linkUpY >= 0)
        ImGui::SetScrollFromPosY(linkUpY + ImGui::GetCursorStartPos().y);
      else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && linkDownY >= 0)
        ImGui::SetScrollFromPosY(linkDownY + ImGui::GetCursorStartPos().y);
    }

    end_line_drawing();

    process_multi_selection(ImGui::EndMultiSelect(), ignoreMultiselect);
  }
  else
  {
    last_hovered = NULL_INDEX;
    hovered_node = NULL_INDEX;
    hovered_link = NULL_INDEX;
  }
  ImGui::EndChild();
  ImGui::PopStyleColor(4);
}

REGISTER_IMGUI_WINDOW("FRP", "Graph Viewer###FRP-graph-viewer", graph_viewer);
