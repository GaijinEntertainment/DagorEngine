// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_imgui.h>
#include <imgui.h>
#include <implot.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/sort.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/numeric.h>
#include <EASTL/algorithm.h>
#include <generic/dag_span.h>
#include <drv/3d/dag_resourceTag.h>
#include <gpuMemoryInspector/gpuMemoryInspector.h>
#include <ioSys/dag_dataBlock.h>

// borrowed from DX12 driver
inline uint32_t size_to_unit_table(uint64_t sz)
{
  uint32_t unitIndex = 0;
  unitIndex += sz >= (1024 * 1024 * 1024);
  unitIndex += sz >= (1024 * 1024);
  unitIndex += sz >= (1024);
  return unitIndex;
}

inline const char *get_unit_name(uint32_t index)
{
  static const char *unitTable[] = {"Bytes", "KiBytes", "MiBytes", "GiBytes"};
  return unitTable[index];
}

inline float compute_unit_type_size(uint64_t sz, uint32_t unit_index) { return static_cast<float>(sz) / (powf(1024, unit_index)); }

class ByteUnits
{
  uint64_t size = 0;

public:
  ByteUnits() = default;

  ByteUnits(const ByteUnits &) = default;
  ByteUnits &operator=(const ByteUnits &) = default;


  ByteUnits(uint64_t v) : size{v} {}
  ByteUnits &operator=(uint64_t v)
  {
    size = v;
    return *this;
  }

  ByteUnits &operator+=(ByteUnits o)
  {
    size += o.size;
    return *this;
  }
  ByteUnits &operator-=(ByteUnits o)
  {
    size -= o.size;
    return *this;
  }

  friend ByteUnits operator+(ByteUnits l, ByteUnits r) { return {l.size + r.size}; }
  friend ByteUnits operator-(ByteUnits l, ByteUnits r) { return {l.size - r.size}; }

  uint64_t value() const { return size; }
  float units() const { return compute_unit_type_size(size, size_to_unit_table(size)); }
  const char *name() const { return get_unit_name(size_to_unit_table(size)); }
};

struct CompareHelper
{
  enum class State
  {
    Less,
    Equal,
    Greater
  };

  State state;

  template <typename T>
  CompareHelper onEqual(const T &l, const T &r) const
  {
    auto newState = state;
    if (State::Equal == state)
    {
      if (l < r)
      {
        newState = State::Less;
      }
      else if (l > r)
      {
        newState = State::Greater;
      }
    }
    return {.state = newState};
  }

  CompareHelper onEqual(const char *l, const char *r) const
  {
    auto newState = state;
    if (State::Equal == state)
    {
      if (!l && r)
      {
        newState = State::Less;
      }
      else if (l && !r)
      {
        newState = State::Greater;
      }
      else if (l && r)
      {
        auto c = strcmp(l, r);
        if (c < 0)
        {
          newState = State::Less;
        }
        else if (c > 0)
        {
          newState = State::Greater;
        }
      }
    }

    return {.state = newState};
  }

  bool isLess() const { return State::Less == state; }
};

template <typename T>
CompareHelper compare(const T &l, const T &r)
{
  return CompareHelper{.state = CompareHelper::State::Equal}.onEqual(l, r);
}

CompareHelper compare(const char *l, const char *r) { return CompareHelper{.state = CompareHelper::State::Equal}.onEqual(l, r); }

const char *as_string(TaggedResourceType type)
{
  switch (type)
  {
    case TaggedResourceType::DeviceBuffer: return "Device Buffer";
    case TaggedResourceType::HostBuffer: return "Host Buffer";
    case TaggedResourceType::Texture: return "Texture";
    case TaggedResourceType::BottomLevelAccelerationStructure: return "Blas";
    case TaggedResourceType::TopLevelAccelerationStructure: return "Tlas";
    case TaggedResourceType::OtherAccelerationStructure: return "Oas";
    case TaggedResourceType::AccelerationStructurePool: return "AS Pool";
    case TaggedResourceType::ResourceHeap: return "Resource Heap";
    case TaggedResourceType::UnspecifiedMemory: return "Unspecified";
    case TaggedResourceType::Undefined: return "Undefined";
    default: return "<ERROR>";
  }
}

using ExternalCallback = dag::FunctionRef<void(const ResourceVisitor &visitor) const>;

namespace
{
static const char *no_tag = "-- No Tag Select --";

struct PieModeInfo
{
  const char *name = nullptr;
  const char *formatString = nullptr;
  double (*transformer)(uint64_t value, uint64_t total) = nullptr;
};

constexpr PieModeInfo pie_modes[] = {
  {"%", "%f%%", [](uint64_t value, uint64_t total) { return 100.0 * double(value) / total; }},
  {"Bytes", "%.3f Bytes", [](uint64_t value, uint64_t) { return double(value); }},
  {"KiBytes", "%.3f KiBytes", [](uint64_t value, uint64_t) { return double(value) / (1024.0); }},
  {"MiBytes", "%.3f MiBytes", [](uint64_t value, uint64_t) { return double(value) / (1024.0 * 1024.0); }},
  {"GiBytes", "%.3f GiBytes", [](uint64_t value, uint64_t) { return double(value) / (1024.0 * 1024.0 * 1024.0); }},
};

enum class GroupSorting
{
  ByTag,
  BySize,
  ByPercentage,
};

enum class SortingOrder
{
  Ascending,
  Descending,
};

struct TagInfo
{
  const TaggedResourceInfo *firstResource = nullptr;
  uint32_t resourceCount = 0;
  uint64_t totalSize = 0;
  bool isAproximation = false;
};

class TagInfoTable
{
  eastl::vector<TagInfo> infoTable;
  eastl::vector<TaggedResourceInfo> dataStore;
  uint64_t totalSize = 0;
  bool isAproximation = false;

  GroupSorting lastTagInfoTableSorting = GroupSorting::BySize;
  SortingOrder lastTagInfoTableSortingOrder = SortingOrder::Descending;

  void sortDataStore()
  {
    // orders by tag first, then by size, then by alignment and finally by name
    // this makes use with tag handling simpler
    eastl::sort(dataStore.begin(), dataStore.end(), [](auto &l, auto &r) {
      // tag compare is pointer only, so cast it to void to avoid lexical ordering
      return compare((void *)l.tag, (void *)r.tag)
        // reversed on purpose we want bigger first
        .onEqual(r.sizeInBytes, l.sizeInBytes)
        // same as with size
        .onEqual(r.addressAlignment, l.addressAlignment)
        .onEqual(l.name, r.name)
        .isLess();
    });
  }

  void discoverTags()
  {
    // this assumes that sortDataStore was issued to sort data data
    // each time we encounter a entry that has a different tag, we found the beginning of a new region with the same tags
    infoTable.push_back({.firstResource = &dataStore.front()});
    for (auto &tri : dataStore)
    {
      if (tri.tag != infoTable.back().firstResource->tag)
      {
        infoTable.push_back({.firstResource = &tri});
      }
      ++infoTable.back().resourceCount;
      infoTable.back().totalSize += tri.sizeInBytes;
      infoTable.back().isAproximation = infoTable.back().isAproximation || tri.isAproximation;
      totalSize += tri.sizeInBytes;
      isAproximation = isAproximation || tri.isAproximation;
    }

    // sort the first time with size descending, this is the first use
    eastl::sort(infoTable.begin(), infoTable.end(),
      [](auto &l, auto &r) { return compare(r.totalSize, l.totalSize).onEqual(l.firstResource->tag, r.firstResource->tag).isLess(); });

    lastTagInfoTableSorting = GroupSorting::BySize;
    lastTagInfoTableSortingOrder = SortingOrder::Descending;
  }

  void sortInfoTable(GroupSorting sorting, SortingOrder order)
  {
    if (GroupSorting::ByPercentage == sorting)
    {
      // sorting by size or by percentage result in the same ordering
      sorting = GroupSorting::BySize;
    }
    if (lastTagInfoTableSorting == sorting && lastTagInfoTableSortingOrder == order)
    {
      return;
    }

    if (GroupSorting::ByTag == sorting)
    {
      if (SortingOrder::Ascending == order)
      {
        eastl::sort(infoTable.begin(), infoTable.end(), [](auto &l, auto &r) {
          return compare(l.firstResource->tag, r.firstResource->tag).onEqual(l.totalSize, r.totalSize).isLess();
        });
      }
      else
      {
        eastl::sort(infoTable.begin(), infoTable.end(), [](auto &l, auto &r) {
          return compare(r.firstResource->tag, l.firstResource->tag).onEqual(l.totalSize, r.totalSize).isLess();
        });
      }
    }
    else
    {
      if (SortingOrder::Ascending == order)
      {
        eastl::sort(infoTable.begin(), infoTable.end(), [](auto &l, auto &r) {
          return compare(l.totalSize, r.totalSize).onEqual(l.firstResource->tag, r.firstResource->tag).isLess();
        });
      }
      else
      {
        eastl::sort(infoTable.begin(), infoTable.end(), [](auto &l, auto &r) {
          return compare(r.totalSize, l.totalSize).onEqual(l.firstResource->tag, r.firstResource->tag).isLess();
        });
      }
    }

    lastTagInfoTableSorting = sorting;
    lastTagInfoTableSortingOrder = order;
  }

public:
  void beginBuild()
  {
    infoTable.clear();
    dataStore.clear();
    totalSize = 0;
    isAproximation = false;
  }
  void addTaggedResourceInfo(TaggedResourceInfo info)
  {
    if (info.name == nullptr)
    {
      info.name = "-";
    }
    if (info.tag == nullptr)
    {
      info.tag = "NULL";
    }
    dataStore.push_back(info);
  }
  void endBuild()
  {
    if (dataStore.empty())
    {
      return;
    }
    sortDataStore();
    discoverTags();
  }

  struct VisitTagInfoSettings
  {
    bool sorted = true;
    GroupSorting sorting = GroupSorting::BySize;
    SortingOrder order = SortingOrder::Descending;
    uint32_t offset = 0;
    uint32_t count = ~uint32_t(0);
  };

  template <typename V>
  void visitTags(const VisitTagInfoSettings &settings, V &&v)
  {
    if (settings.sorted)
    {
      sortInfoTable(settings.sorting, settings.order);
    }
    // have to handle count on its own first, otherwise offset + count may wrap around when count is set to ~0
    const size_t count = eastl::min<size_t>(settings.count, infoTable.size());
    const size_t lim = eastl::min<size_t>(settings.offset + count, infoTable.size());
    for (size_t i = settings.offset; i < lim; ++i)
    {
      v(const_cast<const TagInfo &>(infoTable[i]));
    }
  }

  uint64_t getTotalMemorySizeInBytes() const { return totalSize; }
  bool getIsAproximation() const { return isAproximation; }
  size_t getTagCount() const { return infoTable.size(); }
  const TagInfo *getInfoForTag(const char *tag) const
  {
    auto ref = eastl::find_if(infoTable.begin(), infoTable.end(), [tag](const auto &info) { return tag == info.firstResource->tag; });
    return ref != infoTable.end() ? &*ref : nullptr;
  }
};

class TagTableDataSet
{
  eastl::vector<const TagInfo *> dataSet;

public:
  void beginBuild() { dataSet.clear(); }
  void beginBuild(size_t re)
  {
    dataSet.clear();
    dataSet.reserve(re);
  }
  void add(const TagInfo &info) { dataSet.push_back(&info); }
  void endBuild(GroupSorting sorting, SortingOrder order)
  {
    if (GroupSorting::ByTag == sorting)
    {
      if (SortingOrder::Ascending == order)
      {
        eastl::sort(dataSet.begin(), dataSet.end(), [](auto l, auto r) {
          return compare(l->firstResource->tag, r->firstResource->tag).onEqual(l->totalSize, r->totalSize).isLess();
        });
      }
      else
      {
        eastl::sort(dataSet.begin(), dataSet.end(), [](auto l, auto r) {
          return compare(r->firstResource->tag, l->firstResource->tag).onEqual(l->totalSize, r->totalSize).isLess();
        });
      }
    }
    else /*if ((GroupSorting::BySize == sorting) || (GroupSorting::ByPercentage == sorting))*/
    {
      if (SortingOrder::Ascending == order)
      {
        eastl::sort(dataSet.begin(), dataSet.end(), [](auto l, auto r) {
          return compare(l->totalSize, r->totalSize).onEqual(l->firstResource->tag, r->firstResource->tag).isLess();
        });
      }
      else
      {
        eastl::sort(dataSet.begin(), dataSet.end(), [](auto l, auto r) {
          return compare(r->totalSize, l->totalSize).onEqual(l->firstResource->tag, r->firstResource->tag).isLess();
        });
      }
    }
  }
  template <typename V>
  void visit(V &&v)
  {
    for (auto info : dataSet)
    {
      v(*info);
    }
  }
};

struct State
{
  ResourceTypeFilter filter = {
    .includeDeviceMemoryBuffers = true,
    .includeHostMemoryBuffers = true,
    .includeTextures = true,
    .includeRayTraceBottomStructures = true,
    .includeRayTraceTopStructures = true,
    .includeRayTraceOtherStructures = true,
    .includeRayTraceStructurePools = true,
    .includeResourceHeaps = true,
    .includeUnspecifiedMemory = true,
  };
  // start with top 10
  int topCount = 10;
  const PieModeInfo *selectedPieMode = &pie_modes[0];
  ResourceTagType selectedTag = no_tag;
  GroupSorting groupSorting = GroupSorting::ByTag;
  SortingOrder groupSortingOrder = SortingOrder::Ascending;

  eastl::vector<eastl::unique_ptr<ExternalCallback>> callbacks;
  eastl::vector<const char *> pieChartNameSet;
  eastl::vector<double> pieChartDataSet;

  TagTableDataSet tagTableInfoSet;
  TagInfoTable infoTable;
};

State state;

void read_tagged_data()
{
  state.infoTable.beginBuild();

  d3d::visit_tagged_resources(state.filter, [](const TaggedResourceInfo &info) { state.infoTable.addTaggedResourceInfo(info); });
  for (auto &c : state.callbacks)
  {
    (*c)([](const TaggedResourceInfo &info) { state.infoTable.addTaggedResourceInfo(info); });
  }

  state.infoTable.endBuild();
}

void paint_filter_select()
{
  constexpr uint32_t max_selectors_per_row = 4;
  if (ImGui::TreeNodeEx("Resource Type Filters", ImGuiTreeNodeFlags_Framed))
  {
    ImGui::SetItemTooltip("%s",
      "Filters lets you filter which kind of resources will be shown, note that some resources are never "
      "shown, like resources placed in user heaps (the heaps themselves can be shown) and aliased textures.");

    if (ImGui::BeginTable("GPUMemoryInspectorFilterTable", max_selectors_per_row, ImGuiTableFlags_NoClip))
    {
      auto paintElement = [](auto name, auto selected, auto info) {
        ImGui::TableNextColumn();
        ImGui::Checkbox(name, &selected);
        ImGui::SetItemTooltip("%s", info);
        return selected;
      };
      state.filter.includeDeviceMemoryBuffers =
        paintElement("Device Memory Buffers", state.filter.includeDeviceMemoryBuffers, "Buffers backed by device memory (VMEM)");
      state.filter.includeHostMemoryBuffers =
        paintElement("Host Memory Buffers", state.filter.includeHostMemoryBuffers, "Buffers backed by system memory (RAM)");
      state.filter.includeTextures = paintElement("Textures", state.filter.includeTextures, "Textures of any kind");
      state.filter.includeRayTraceBottomStructures =
        paintElement("Bottom Level Acceleration Structures", state.filter.includeRayTraceBottomStructures,
          "Bottom Level Acceleration Structure (BLAS) usually backed by device memory (VMEM)");
      state.filter.includeRayTraceTopStructures = paintElement("Top Level Acceleration Structures",
        state.filter.includeRayTraceTopStructures, "Top Level Acceleration Structure (TLAS) usually backed by device memory (VMEM)");
      state.filter.includeRayTraceOtherStructures =
        paintElement("Other Acceleration Structures", state.filter.includeRayTraceOtherStructures,
          "A Ray Tracing Acceleration Structure type that is neither a TLAS or BLAS, usually Opacity Micro Maps and similar");
      state.filter.includeRayTraceStructurePools = paintElement("Acceleration Structure Pools",
        state.filter.includeRayTraceStructurePools, "Pools of various types of Ray Tracing Acceleration Structures stored within");
      state.filter.includeResourceHeaps = paintElement("Resource Heaps", state.filter.includeResourceHeaps,
        "User-created resource heaps (d3d::create_resource_heap), resources placed in these heaps are not shown individually");
      state.filter.includeUnspecifiedMemory = paintElement("Show unspecified memory", state.filter.includeUnspecifiedMemory,
        "Memory with unspecified resource association, usually reported by third party memory usage reports, like DLSS, XESS and "
        "such");
    }
    ImGui::EndTable();
    ImGui::TreePop();
  }
}

void paint_tag_chart()
{
  if (!ImGui::TreeNodeEx("Pie Chart", ImGuiTreeNodeFlags_Framed))
  {
    return;
  }
  ImGui::SetItemTooltip("%s", "Note that the pie chart only allows for moving the chart and hiding of Tag Groups in the chart. The "
                              "table below allows for more control, like clicking on the Tag group to make it the active one");

  ImGui::DragInt("Show Top N only", &state.topCount, 1.f, state.infoTable.getTagCount() > 0 ? 1 : 0, state.infoTable.getTagCount());
  ImGui::SetItemTooltip("Shows only the top %i Tag groups by size, can be manually edited with CTRL + Mouse Click", state.topCount);
  if (state.topCount > state.infoTable.getTagCount())
  {
    state.topCount = state.infoTable.getTagCount();
  }
  else if (state.topCount < 1)
  {
    if (!state.infoTable.getTagCount())
    {
      state.topCount = 1;
    }
  }

  // +1 is to avoid the case when there are for example 11 tags and top 10 is selected, putting one tag as others makes no sense
  // when there are 12 tags and top 10 is selected, this makes sense again
  const bool hasOther = (state.topCount + 1) < state.infoTable.getTagCount();
  const uint32_t countToVisit = hasOther ? static_cast<uint32_t>(state.topCount) : state.infoTable.getTagCount();

  // for automatic scaling of sizes, we count how many times we see >= Gi, >= Mi, >= Ki, and bytes
  // then we pick the one that we saw the most
  // could be tweaked a little biasing towards the bigger scaling
  uint32_t scaleBucketCounts[4] = {};

  state.pieChartNameSet.clear();
  state.pieChartNameSet.reserve(state.infoTable.getTagCount());
  state.pieChartDataSet.clear();
  state.pieChartDataSet.reserve(state.infoTable.getTagCount());
  state.tagTableInfoSet.beginBuild(state.infoTable.getTagCount());
  state.infoTable.visitTags(
    {
      .sorted = true,
      .sorting = GroupSorting::BySize,
      .order = SortingOrder::Descending,
      .offset = 0,
      .count = countToVisit,
    },
    [&scaleBucketCounts](auto &info) {
      state.tagTableInfoSet.add(info);
      state.pieChartNameSet.push_back(info.firstResource->tag);
      state.pieChartDataSet.push_back(state.selectedPieMode->transformer(info.totalSize, state.infoTable.getTotalMemorySizeInBytes()));
      ++scaleBucketCounts[size_to_unit_table(info.totalSize)];
    });

  uint64_t otherSize = 0;
  bool otherIsAproximation = false;
  if (hasOther)
  {
    state.infoTable.visitTags(
      {
        // keep the current sorting
        .sorted = false,
        .offset = countToVisit,
      },
      [&otherSize, &otherIsAproximation](auto &info) {
        otherIsAproximation = otherIsAproximation || info.isAproximation;
        otherSize += info.totalSize;
      });

    state.pieChartNameSet.push_back("Other");
    state.pieChartDataSet.push_back(state.selectedPieMode->transformer(otherSize, state.infoTable.getTotalMemorySizeInBytes()));
    ++scaleBucketCounts[size_to_unit_table(otherSize)];
  }

  uint32_t selectedScaleBucket = 0;
  for (uint32_t i = 1; i < 4; ++i)
  {
    if (scaleBucketCounts[i] <= scaleBucketCounts[selectedScaleBucket])
    {
      continue;
    }
    selectedScaleBucket = i;
  }

  if (ImPlot::BeginPlot("##GPUMemoryInspectorChartPlot", NULL, NULL, ImVec2(-1, 0), ImPlotFlags_Equal | ImPlotFlags_NoMouseText,
        ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations))
  {
    ImPlot::PlotPieChart(state.pieChartNameSet.data(), state.pieChartDataSet.data(), state.pieChartDataSet.size(), 0.6, 0.4, 0.4,
      state.selectedPieMode->formatString);
    ImPlot::EndPlot();
  }

  constexpr int table_height = 10;
  int child_height = (table_height + 3) * ImGui::GetTextLineHeightWithSpacing();

  if (
    ImGui::BeginTable("GPUMemoryInspectorTagSummaryTable", 3,
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY,
      ImVec2(0, child_height)))
  {
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort, 0.f, static_cast<ImGuiID>(GroupSorting::ByTag));
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_None, 0.f, static_cast<ImGuiID>(GroupSorting::BySize));
    ImGui::TableSetupColumn("%", ImGuiTableColumnFlags_None, 0.f, static_cast<ImGuiID>(GroupSorting::ByPercentage));
    ImGui::TableHeadersRow();

    auto sortingSpec = ImGui::TableGetSortSpecs();
    if (!sortingSpec)
    {
      state.groupSorting = GroupSorting::ByTag;
      state.groupSortingOrder = SortingOrder::Ascending;
      state.selectedPieMode = &pie_modes[0];
    }
    else
    {
      if (sortingSpec->SpecsDirty && sortingSpec->SpecsCount > 0)
      {
        state.groupSorting = static_cast<GroupSorting>(sortingSpec->Specs[0].ColumnUserID);
        state.groupSortingOrder =
          sortingSpec->Specs[0].SortDirection == ImGuiSortDirection_Descending ? SortingOrder::Descending : SortingOrder::Ascending;
      }
      sortingSpec->SpecsDirty = false;
    }

    // have to update pie mode each time as when group sizes change the mode may change too
    if (GroupSorting::BySize == state.groupSorting)
    {
      // selects the formatting of the sizes on the majority of sizes in the chart, eg when most are in the MiByste range, all will be
      // displayed in MiByte units
      state.selectedPieMode = &pie_modes[1 + selectedScaleBucket];
    }
    else if (GroupSorting::ByPercentage == state.groupSorting)
    {
      state.selectedPieMode = &pie_modes[0];
    }
    // we are not updating the mode when the sorting is the tag, so the formatting lingers until a sorting is selected that changes the
    // formatting

    state.tagTableInfoSet.endBuild(state.groupSorting, state.groupSortingOrder);

    state.tagTableInfoSet.visit([](const auto &info) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      bool selected = ImGui::Selectable(info.firstResource->tag, info.firstResource->tag == state.selectedTag,
        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
      if (selected)
      {
        state.selectedTag = info.firstResource->tag;
      }

      ByteUnits memoryUnits{info.totalSize};
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%s%.3f %s", info.isAproximation ? "~" : "", memoryUnits.units(), memoryUnits.name());

      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%f%%", 100.0 * double(info.totalSize) / state.infoTable.getTotalMemorySizeInBytes());
    });

    if (otherSize > 0)
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      bool open = ImGui::TreeNodeEx("Other", ImGuiTreeNodeFlags_SpanFullWidth);

      ByteUnits memoryUnits{otherSize};
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%s%.3f %s", otherIsAproximation ? "~" : "", memoryUnits.units(), memoryUnits.name());

      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%f%%", 100.0 * double(otherSize) / state.infoTable.getTotalMemorySizeInBytes());
      if (open)
      {
        state.tagTableInfoSet.beginBuild();
        state.infoTable.visitTags(
          {
            // keep the current sorting
            .sorted = false,
            .offset = countToVisit,
          },
          [](auto &info) { state.tagTableInfoSet.add(info); });

        state.tagTableInfoSet.endBuild(state.groupSorting, state.groupSortingOrder);

        state.tagTableInfoSet.visit([](const auto &info) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          bool selected = ImGui::Selectable(info.firstResource->tag, info.firstResource->tag == state.selectedTag,
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
          if (selected)
          {
            state.selectedTag = info.firstResource->tag;
          }

          ByteUnits memoryUnits{info.totalSize};
          ImGui::TableSetColumnIndex(1);
          ImGui::Text("%s%.3f %s", info.isAproximation ? "~" : "", memoryUnits.units(), memoryUnits.name());

          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%f%%", 100.0 * double(info.totalSize) / state.infoTable.getTotalMemorySizeInBytes());
        });
        ImGui::TreePop();
      }
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    bool selected =
      ImGui::Selectable("Total", no_tag == state.selectedTag, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
    if (selected)
    {
      state.selectedTag = no_tag;
    }

    ByteUnits memoryUnits{state.infoTable.getTotalMemorySizeInBytes()};
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%s%.3f %s", state.infoTable.getIsAproximation() ? "~" : "", memoryUnits.units(), memoryUnits.name());

    ImGui::TableSetColumnIndex(2);
    ImGui::TextUnformatted("-");

    ImGui::EndTable();
  }
  ImGui::TreePop();
}

void paint_tagged_table()
{
  if (!ImGui::TreeNodeEx("Resource Table", ImGuiTreeNodeFlags_Framed))
  {
    return;
  }

  auto selectedTagInfo = state.infoTable.getInfoForTag(state.selectedTag);

  if (!selectedTagInfo)
  {
    state.selectedTag = no_tag;
  }

  if (ImGui::BeginCombo("Tag##GPUMemoryInspectorTagSelector", state.selectedTag, 0))
  {
    bool noTagIsSel = state.selectedTag == no_tag;
    ImGui::Selectable(no_tag, &noTagIsSel);
    if (noTagIsSel)
    {
      state.selectedTag = no_tag;
      selectedTagInfo = nullptr;
    }

    state.infoTable.visitTags({.sorted = false}, [&selectedTagInfo](auto &info) {
      bool isSel = state.selectedTag == info.firstResource->tag;
      ImGui::Selectable(info.firstResource->tag, &isSel);
      if (isSel)
      {
        selectedTagInfo = &info;
        state.selectedTag = info.firstResource->tag;
      }
    });

    ImGui::EndCombo();
  }

  if (!selectedTagInfo)
  {
    ImGui::TextUnformatted("No Tag Selected");
    ImGui::TreePop();
    return;
  }

  if (ImGui::BeginTable("GPUMemoryInspectorTaggedResourceSummaryTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
  {
    ImGui::TableSetupColumn("Resource Type");
    ImGui::TableSetupColumn("Count");
    ImGui::TableSetupColumn("Size");
    ImGui::TableSetupColumn("%");
    ImGui::TableHeadersRow();

    uint32_t counts[static_cast<uint32_t>(TaggedResourceType::UnspecifiedMemory) + 1]{};
    uint64_t sizes[static_cast<uint32_t>(TaggedResourceType::UnspecifiedMemory) + 1]{};
    bool isAproximation[static_cast<uint32_t>(TaggedResourceType::UnspecifiedMemory) + 1]{};
    for (auto &res : make_span_const(selectedTagInfo->firstResource, selectedTagInfo->resourceCount))
    {
      ++counts[static_cast<uint32_t>(res.type)];
      sizes[static_cast<uint32_t>(res.type)] += res.sizeInBytes;
      isAproximation[static_cast<uint32_t>(res.type)] = isAproximation[static_cast<uint32_t>(res.type)] || res.isAproximation;
    }
    for (uint32_t i = 0; i < static_cast<uint32_t>(TaggedResourceType::UnspecifiedMemory) + 1; ++i)
    {
      if (!counts[i])
      {
        continue;
      }
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(as_string(static_cast<TaggedResourceType>(i)));

      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%u", counts[i]);

      ByteUnits memoryUnits{sizes[i]};
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%s%.3f %s", isAproximation[i] ? "~" : "", memoryUnits.units(), memoryUnits.name());

      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%f%%", 100.0 * double(sizes[i]) / selectedTagInfo->totalSize);
    }

    ImGui::EndTable();
  }

  constexpr int table_height = 25;
  int child_height = (table_height + 3) * ImGui::GetTextLineHeightWithSpacing();

  if (
    ImGui::BeginTable("GPUMemoryInspectorTaggedResourceTable", 5,
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, child_height)))
  {
    ImGui::TableSetupColumn("Type");
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Size");
    ImGui::TableSetupColumn("Alignment");
    ImGui::TableSetupColumn("Is Suballocated");
    ImGui::TableHeadersRow();

    for (auto &res : make_span_const(selectedTagInfo->firstResource, selectedTagInfo->resourceCount))
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(as_string(res.type));

      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted(res.name);

      ByteUnits memoryUnits{res.sizeInBytes};
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%s%.3f %s", res.isAproximation ? "~" : "", memoryUnits.units(), memoryUnits.name());

      memoryUnits = res.addressAlignment;
      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%.3f %s", memoryUnits.units(), memoryUnits.name());

      ImGui::TableSetColumnIndex(4);
      ImGui::TextUnformatted(res.isSubAllocated ? "Yes" : "No");
    }

    ImGui::EndTable();
  }
  ImGui::TreePop();
}
} // namespace


void inspector_window()
{
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  paint_filter_select();
  read_tagged_data();
  paint_tag_chart();
  paint_tagged_table();
}

GpuMemoryInspectorCallbackToken gpu_memory_inspect_register_external_callback(
  dag::FunctionRef<void(const ResourceVisitor &visitor) const> callback)
{
  state.callbacks.push_back(eastl::make_unique<ExternalCallback>(callback));
  return state.callbacks.back().get();
}

void gpu_memory_inspect_unregister_external_callback(GpuMemoryInspectorCallbackToken token)
{
  auto ref = eastl::find_if(state.callbacks.begin(), state.callbacks.end(), [token](auto &info) { return token == info.get(); });
  if (ref != state.callbacks.end())
  {
    state.callbacks.erase(ref);
  }
}

void gpu_memory_inspector_load_settings(const DataBlock *block)
{
  if (!block)
  {
    return;
  }
  auto config = block->getBlockByNameEx("gpuMemoryInspector");
  if (!config)
  {
    return;
  }
  state.filter.includeDeviceMemoryBuffers = config->getBool("includeDeviceMemoryBuffers", state.filter.includeDeviceMemoryBuffers);
  state.filter.includeHostMemoryBuffers = config->getBool("includeHostMemoryBuffers", state.filter.includeHostMemoryBuffers);
  state.filter.includeTextures = config->getBool("includeTextures", state.filter.includeTextures);
  state.filter.includeRayTraceBottomStructures =
    config->getBool("includeRayTraceBottomStructures", state.filter.includeRayTraceBottomStructures);
  state.filter.includeRayTraceTopStructures =
    config->getBool("includeRayTraceTopStructures", state.filter.includeRayTraceTopStructures);
  state.filter.includeRayTraceOtherStructures =
    config->getBool("includeRayTraceOtherStructures", state.filter.includeRayTraceOtherStructures);
  state.filter.includeRayTraceStructurePools =
    config->getBool("includeRayTraceStructurePools", state.filter.includeRayTraceStructurePools);
  state.filter.includeResourceHeaps = config->getBool("includeResourceHeaps", state.filter.includeResourceHeaps);
  state.filter.includeUnspecifiedMemory = config->getBool("includeUnspecifiedMemory", state.filter.includeUnspecifiedMemory);
}

REGISTER_IMGUI_WINDOW("Render", "GPU Memory", inspector_window);
