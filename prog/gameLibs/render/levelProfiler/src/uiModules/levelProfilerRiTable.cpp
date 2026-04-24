// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/sort.h>
#include "levelProfilerWidgets.h"
#include "riModule.h"
#include "levelProfilerRiTable.h"
#include "levelProfilerExporter.h"
#include <riGen/riGenExtra.h>
#include "generic/dag_enumerate.h"
#include <cfloat>

namespace levelprofiler
{
namespace
{
struct RiLayout
{
  int lodCount;
  static constexpr int PER_LOD_METRIC_GROUPS = 4;
  static constexpr int HEAVY_SHADERS_START_LOD = 2;
  int dipsBase() const { return static_cast<int>(RiColumn::BASE_COUNT); }
  int trisBase() const { return dipsBase() + lodCount; }
  int distBase() const { return trisBase() + lodCount; }
  int screenBase() const { return distBase() + lodCount; }
  int shadersBase() const { return screenBase() + lodCount; }
  int shadersCount() const { return lodCount > HEAVY_SHADERS_START_LOD ? (lodCount - HEAVY_SHADERS_START_LOD) : 0; }
  int totalColumns() const { return static_cast<int>(RiColumn::BASE_COUNT) + lodCount * PER_LOD_METRIC_GROUPS + shadersCount(); }
};

inline RiLayout makeRiLayout() { return RiLayout{rendinst::RiExtraPool::MAX_LODS}; }

} // anonymous namespace

namespace UIConstants
{

static constexpr float COL_RI_NAME_WIDTH = 180.0f;
static constexpr float COL_COUNT_WIDTH = 60.0f;
static constexpr float COL_RADIUS_WIDTH = 80.0f;
static constexpr float COL_DIPS_WIDTH = 60.0f;
static constexpr float COL_TRIS_WIDTH = 60.0f;
static constexpr float COL_DIST_WIDTH = 80.0f;
static constexpr float COL_SCREEN_WIDTH = 80.0f;
static constexpr float COL_SHADERS_WIDTH = 90.0f;

// Slider speeds for numeric filters (tuned per value scale)
static constexpr float COUNT_FILTER_SPEED = 1.0f;
static constexpr float RADIUS_FILTER_SPEED = 0.1f;
static constexpr float DIPS_FILTER_SPEED = 1.0f;
static constexpr float TRIS_FILTER_SPEED = 10.0f;
static constexpr float DIST_FILTER_SPEED = 1.0f;
static constexpr float SCREEN_FILTER_SPEED = 0.1f;
static constexpr int LESS_THAN = -1;
static constexpr int EQUAL = 0;
static constexpr int GREATER_THAN = 1;

} // namespace UIConstants

// --- LpRiTableColumn implementation ---

LpRiTableColumn::LpRiTableColumn(const char *name, RiColumn column_type, float width, ImGuiTableColumnFlags flags) :
  LpProfilerTableColumn(name, width, flags), columnType(column_type)
{}

int LpRiTableColumn::compareItems(const void *item1, const void *item2) const
{
  const RiTableItem *riItem1 = static_cast<const RiTableItem *>(item1);
  const RiTableItem *riItem2 = static_cast<const RiTableItem *>(item2);

  if (!riItem1 || !riItem1->data || !riItem2 || !riItem2->data)
    return UIConstants::EQUAL;

  const RiData *data1 = riItem1->data;
  const RiData *data2 = riItem2->data;

  switch (columnType)
  {
    case RiColumn::NAME: return data1->name.compare(data2->name);

    case RiColumn::COUNT_ON_MAP: return data1->countOnMap - data2->countOnMap;

    case RiColumn::BSPHERE_RAD:
    {
      float difference = data1->bSphereRadius - data2->bSphereRadius;
      return (difference < 0.0f) ? UIConstants::LESS_THAN : (difference > 0.0f ? UIConstants::GREATER_THAN : UIConstants::EQUAL);
    }

    case RiColumn::BBOX_RAD:
    {
      float difference = data1->bBoxRadius - data2->bBoxRadius;
      return (difference < 0.0f) ? UIConstants::LESS_THAN : (difference > 0.0f ? UIConstants::GREATER_THAN : UIConstants::EQUAL);
    }

    case RiColumn::PHYS_TRIS: return data1->collision.physTriangles - data2->collision.physTriangles;

    case RiColumn::TRACE_TRIS: return data1->collision.traceTriangles - data2->collision.traceTriangles;

    default: return UIConstants::EQUAL;
  }
}

ProfilerString LpRiTableColumn::getCellText(const void *item_data, int /* column_index */) const
{
  const RiTableItem *currentItem = static_cast<const RiTableItem *>(item_data);
  if (!currentItem || !currentItem->data)
    return ProfilerString();

  return getCellTextForItem(currentItem);
}

void LpRiTableColumn::drawCell(const void *item_data) const
{
  const RiTableItem *currentItem = static_cast<const RiTableItem *>(item_data);
  if (!currentItem || !currentItem->data)
  {
    ImGui::Text("N/A");
    return;
  }

  drawCellForItem(currentItem);
}

LpRiNameColumn::LpRiNameColumn(float width, ImGuiTableColumnFlags flags) :
  LpRiTableColumn("Name", RiColumn::NAME, width > 0 ? width : UIConstants::COL_RI_NAME_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort)
{}

ProfilerString LpRiNameColumn::getCellTextForItem(const RiTableItem *item) const { return item->data->name; }

void LpRiNameColumn::drawCellForItem(const RiTableItem *item) const
{
  ImGui::Selectable(item->data->name.c_str(), item->isSelected, ImGuiSelectableFlags_SpanAllColumns);
}

LpRiCountColumn::LpRiCountColumn(float width, ImGuiTableColumnFlags flags) :
  LpRiTableColumn("Count", RiColumn::COUNT_ON_MAP, width > 0 ? width : UIConstants::COL_COUNT_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpRiCountColumn::getCellTextForItem(const RiTableItem *item) const { return eastl::to_string(item->data->countOnMap); }

void LpRiCountColumn::drawCellForItem(const RiTableItem *item) const { ImGui::Text("%d", item->data->countOnMap); }

LpRiBSphereColumn::LpRiBSphereColumn(float width, ImGuiTableColumnFlags flags) :
  LpRiTableColumn("BSphere Rad", RiColumn::BSPHERE_RAD, width > 0 ? width : UIConstants::COL_RADIUS_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpRiBSphereColumn::getCellTextForItem(const RiTableItem *item) const
{
  return eastl::to_string(static_cast<int>(item->data->bSphereRadius * 10.0f)) + "." +
         eastl::to_string(static_cast<int>(item->data->bSphereRadius * 10.0f) % 10);
}

void LpRiBSphereColumn::drawCellForItem(const RiTableItem *item) const { ImGui::Text("%.1f", item->data->bSphereRadius); }

LpRiBBoxColumn::LpRiBBoxColumn(float width, ImGuiTableColumnFlags flags) :
  LpRiTableColumn("BBox Rad", RiColumn::BBOX_RAD, width > 0 ? width : UIConstants::COL_RADIUS_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpRiBBoxColumn::getCellTextForItem(const RiTableItem *item) const
{
  return eastl::to_string(static_cast<int>(item->data->bBoxRadius * 10.0f)) + "." +
         eastl::to_string(static_cast<int>(item->data->bBoxRadius * 10.0f) % 10);
}

void LpRiBBoxColumn::drawCellForItem(const RiTableItem *item) const { ImGui::Text("%.1f", item->data->bBoxRadius); }

LpRiLodDipsColumn::LpRiLodDipsColumn(int lod_index, float width, ImGuiTableColumnFlags flags) :
  LpRiTableColumn((ProfilerString("L") + eastl::to_string(lod_index) + " Dips").c_str(), RiColumn::NONE,
    width > 0 ? width : UIConstants::COL_DIPS_WIDTH, flags | ImGuiTableColumnFlags_WidthFixed),
  lodIndex(lod_index)
{}

ProfilerString LpRiLodDipsColumn::getCellTextForItem(const RiTableItem *item) const
{
  if (lodIndex >= static_cast<int>(item->data->lods.size()))
    return ProfilerString();
  return eastl::to_string(item->data->lods[lodIndex].drawCalls);
}

void LpRiLodDipsColumn::drawCellForItem(const RiTableItem *item) const
{
  if (lodIndex >= static_cast<int>(item->data->lods.size()))
    return;
  ImGui::Text("%d", item->data->lods[lodIndex].drawCalls);
}

int LpRiLodDipsColumn::compareItems(const void *item1, const void *item2) const
{
  const RiTableItem *riItem1 = static_cast<const RiTableItem *>(item1);
  const RiTableItem *riItem2 = static_cast<const RiTableItem *>(item2);
  if (!riItem1 || !riItem2 || !riItem1->data || !riItem2->data)
    return UIConstants::EQUAL;
  int dips1 = (lodIndex < static_cast<int>(riItem1->data->lods.size())) ? riItem1->data->lods[lodIndex].drawCalls : 0;
  int dips2 = (lodIndex < static_cast<int>(riItem2->data->lods.size())) ? riItem2->data->lods[lodIndex].drawCalls : 0;
  return dips1 - dips2;
}

LpRiLodTrisColumn::LpRiLodTrisColumn(int lod_index, float width, ImGuiTableColumnFlags flags) :
  LpRiTableColumn((ProfilerString("L") + eastl::to_string(lod_index) + " Tris").c_str(), RiColumn::NONE,
    width > 0 ? width : UIConstants::COL_TRIS_WIDTH, flags | ImGuiTableColumnFlags_WidthFixed),
  lodIndex(lod_index)
{}

ProfilerString LpRiLodTrisColumn::getCellTextForItem(const RiTableItem *item) const
{
  if (lodIndex >= static_cast<int>(item->data->lods.size()))
    return ProfilerString();
  return eastl::to_string(item->data->lods[lodIndex].totalFaces);
}

void LpRiLodTrisColumn::drawCellForItem(const RiTableItem *item) const
{
  if (lodIndex >= static_cast<int>(item->data->lods.size()))
    return;
  ImGui::Text("%d", item->data->lods[lodIndex].totalFaces);
}

int LpRiLodTrisColumn::compareItems(const void *item1, const void *item2) const
{
  const RiTableItem *riItem1 = static_cast<const RiTableItem *>(item1);
  const RiTableItem *riItem2 = static_cast<const RiTableItem *>(item2);
  if (!riItem1 || !riItem2 || !riItem1->data || !riItem2->data)
    return UIConstants::EQUAL;
  int tris1 = (lodIndex < static_cast<int>(riItem1->data->lods.size())) ? riItem1->data->lods[lodIndex].totalFaces : 0;
  int tris2 = (lodIndex < static_cast<int>(riItem2->data->lods.size())) ? riItem2->data->lods[lodIndex].totalFaces : 0;
  return tris1 - tris2;
}

LpRiPhysTrisColumn::LpRiPhysTrisColumn(float width, ImGuiTableColumnFlags flags) :
  LpRiTableColumn("Phys Tris", RiColumn::PHYS_TRIS, width > 0 ? width : UIConstants::COL_TRIS_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpRiPhysTrisColumn::getCellTextForItem(const RiTableItem *item) const
{
  return eastl::to_string(item->data->collision.physTriangles);
}

void LpRiPhysTrisColumn::drawCellForItem(const RiTableItem *item) const { ImGui::Text("%d", item->data->collision.physTriangles); }

LpRiTraceTrisColumn::LpRiTraceTrisColumn(float width, ImGuiTableColumnFlags flags) :
  LpRiTableColumn("Trace Tris", RiColumn::TRACE_TRIS, width > 0 ? width : UIConstants::COL_TRIS_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpRiTraceTrisColumn::getCellTextForItem(const RiTableItem *item) const
{
  return eastl::to_string(item->data->collision.traceTriangles);
}

void LpRiTraceTrisColumn::drawCellForItem(const RiTableItem *item) const { ImGui::Text("%d", item->data->collision.traceTriangles); }

LpRiLodDistColumn::LpRiLodDistColumn(int lod_index, float width, ImGuiTableColumnFlags flags) :
  LpRiTableColumn((ProfilerString("L") + eastl::to_string(lod_index) + " Dist").c_str(), RiColumn::NONE,
    width > 0 ? width : UIConstants::COL_DIST_WIDTH, flags | ImGuiTableColumnFlags_WidthFixed),
  lodIndex(lod_index)
{}

ProfilerString LpRiLodDistColumn::getCellTextForItem(const RiTableItem *item) const
{
  if (lodIndex >= static_cast<int>(item->data->lods.size()))
    return ProfilerString();
  return eastl::to_string(static_cast<int>(item->data->lods[lodIndex].lodDistance));
}

void LpRiLodDistColumn::drawCellForItem(const RiTableItem *item) const
{
  if (lodIndex >= static_cast<int>(item->data->lods.size()))
    return;
  ImGui::Text("%.0f", item->data->lods[lodIndex].lodDistance);
}

int LpRiLodDistColumn::compareItems(const void *item1, const void *item2) const
{
  const RiTableItem *riItem1 = static_cast<const RiTableItem *>(item1);
  const RiTableItem *riItem2 = static_cast<const RiTableItem *>(item2);
  if (!riItem1 || !riItem2 || !riItem1->data || !riItem2->data)
    return UIConstants::EQUAL;
  float dist1 = (lodIndex < static_cast<int>(riItem1->data->lods.size())) ? riItem1->data->lods[lodIndex].lodDistance : 0.0f;
  float dist2 = (lodIndex < static_cast<int>(riItem2->data->lods.size())) ? riItem2->data->lods[lodIndex].lodDistance : 0.0f;
  float diff = dist1 - dist2;
  return diff < 0.0f ? UIConstants::LESS_THAN : (diff > 0.0f ? UIConstants::GREATER_THAN : UIConstants::EQUAL);
}

LpRiLodScreenColumn::LpRiLodScreenColumn(int lod_index, float width, ImGuiTableColumnFlags flags) :
  LpRiTableColumn((ProfilerString("L") + eastl::to_string(lod_index) + " Screen%").c_str(), RiColumn::NONE,
    width > 0 ? width : UIConstants::COL_SCREEN_WIDTH, flags | ImGuiTableColumnFlags_WidthFixed),
  lodIndex(lod_index)
{}

ProfilerString LpRiLodScreenColumn::getCellTextForItem(const RiTableItem *item) const
{
  if (lodIndex >= static_cast<int>(item->data->lods.size()))
    return ProfilerString();
  return eastl::to_string(static_cast<int>(item->data->lods[lodIndex].screenPercent * 10.0f)) + "." +
         eastl::to_string(static_cast<int>(item->data->lods[lodIndex].screenPercent * 10.0f) % 10);
}

void LpRiLodScreenColumn::drawCellForItem(const RiTableItem *item) const
{
  if (lodIndex >= static_cast<int>(item->data->lods.size()))
    return;
  ImGui::Text("%.1f", item->data->lods[lodIndex].screenPercent);
}

int LpRiLodScreenColumn::compareItems(const void *item1, const void *item2) const
{
  const RiTableItem *riItem1 = static_cast<const RiTableItem *>(item1);
  const RiTableItem *riItem2 = static_cast<const RiTableItem *>(item2);
  if (!riItem1 || !riItem2 || !riItem1->data || !riItem2->data)
    return UIConstants::EQUAL;
  float scr1 = (lodIndex < static_cast<int>(riItem1->data->lods.size())) ? riItem1->data->lods[lodIndex].screenPercent : 0.0f;
  float scr2 = (lodIndex < static_cast<int>(riItem2->data->lods.size())) ? riItem2->data->lods[lodIndex].screenPercent : 0.0f;
  float diff = scr1 - scr2;
  return diff < 0.0f ? UIConstants::LESS_THAN : (diff > 0.0f ? UIConstants::GREATER_THAN : UIConstants::EQUAL);
}

LpRiHeavyShadersColumn::LpRiHeavyShadersColumn(int lod_index, float width, ImGuiTableColumnFlags flags) :
  LpRiTableColumn((ProfilerString("L") + eastl::to_string(lod_index) + " Shaders").c_str(), RiColumn::NONE,
    width > 0 ? width : UIConstants::COL_SHADERS_WIDTH, flags | ImGuiTableColumnFlags_WidthFixed),
  lodIndex(lod_index)
{}

ProfilerString LpRiHeavyShadersColumn::getCellTextForItem(const RiTableItem *item) const
{
  if (lodIndex >= static_cast<int>(item->data->lods.size()))
    return ProfilerString();

  const auto &heavyShaders = item->data->lods[lodIndex].heavyShaders;
  if (heavyShaders.empty())
    return ProfilerString();
  eastl::vector<ProfilerString> names;
  names.reserve(heavyShaders.size());
  for (auto &e : heavyShaders)
    names.push_back(e.name);
  eastl::sort(names.begin(), names.end());
  ProfilerString result;
  bool first = true;
  for (auto &n : names)
  {
    int cnt = 0;
    for (auto &e : heavyShaders)
      if (e.name == n)
      {
        cnt = e.count;
        break;
      }
    if (!first)
      result += ",\n";
    first = false;
    result += n;
    if (cnt > 1)
    {
      result += " ";
      result += eastl::to_string(cnt);
    }
  }
  return result;
}

void LpRiHeavyShadersColumn::drawCellForItem(const RiTableItem *item) const
{
  if (lodIndex >= static_cast<int>(item->data->lods.size()))
    return;

  const auto &heavyShaders = item->data->lods[lodIndex].heavyShaders;
  if (heavyShaders.empty())
    return;
  ProfilerString text = getCellTextForItem(item);
  ImGui::Text("%s", text.c_str());
}

int LpRiHeavyShadersColumn::compareItems(const void *item1, const void *item2) const
{
  const RiTableItem *riItem1 = static_cast<const RiTableItem *>(item1);
  const RiTableItem *riItem2 = static_cast<const RiTableItem *>(item2);
  if (!riItem1 || !riItem2 || !riItem1->data || !riItem2->data)
    return UIConstants::EQUAL;
  size_t count1 = (lodIndex < static_cast<int>(riItem1->data->lods.size())) ? riItem1->data->lods[lodIndex].heavyShaders.size() : 0;
  size_t count2 = (lodIndex < static_cast<int>(riItem2->data->lods.size())) ? riItem2->data->lods[lodIndex].heavyShaders.size() : 0;
  return static_cast<int>(count1) - static_cast<int>(count2);
}

// --- LpRiTable implementation ---

LpRiTable::LpRiTable(RIModule *ri_module_ptr) : LpProfilerTable("RI Assets", makeRiLayout().totalColumns()), riModule(ri_module_ptr)
{
  RiLayout layout = makeRiLayout();
  const int lodCount = layout.lodCount;

  createColumn<LpRiNameColumn>(*RiColumn::NAME);
  createColumn<LpRiCountColumn>(*RiColumn::COUNT_ON_MAP);
  createColumn<LpRiBSphereColumn>(*RiColumn::BSPHERE_RAD);
  createColumn<LpRiBBoxColumn>(*RiColumn::BBOX_RAD);
  createColumn<LpRiPhysTrisColumn>(*RiColumn::PHYS_TRIS);
  createColumn<LpRiTraceTrisColumn>(*RiColumn::TRACE_TRIS);

  for (int lod = 0; lod < lodCount; ++lod)
    createColumn<LpRiLodDipsColumn>(layout.dipsBase() + lod, lod);
  for (int lod = 0; lod < lodCount; ++lod)
    createColumn<LpRiLodTrisColumn>(layout.trisBase() + lod, lod);
  for (int lod = 0; lod < lodCount; ++lod)
    createColumn<LpRiLodDistColumn>(layout.distBase() + lod, lod);
  for (int lod = 0; lod < lodCount; ++lod)
    createColumn<LpRiLodScreenColumn>(layout.screenBase() + lod, lod);
  for (int lod = RiLayout::HEAVY_SHADERS_START_LOD; lod < lodCount; ++lod)
    createColumn<LpRiHeavyShadersColumn>(layout.shadersBase() + (lod - RiLayout::HEAVY_SHADERS_START_LOD), lod);

  setSortItemsCallback(sort_table_items_callback, this);
  setItemSelectedCallback(on_item_selected, this);

  eastl::vector<LpColumnFilterSpec> specs;
  specs.reserve(static_cast<size_t>(RiColumn::BASE_COUNT) + lodCount * 5);

  // Base columns
  specs.push_back(LpColumnFilterSpec{*RiColumn::NAME, "##RiNameFilter",
    makeDelegate<LpRiTable, &LpRiTable::drawNameFilterContent>(this), "##RiNameSearch", "Search RI names...",
    makeSearchDelegate<LpRiTable, &LpRiTable::onNameSearchChange, &LpRiTable::onNameSearchCommit>(this)});
  specs.push_back(
    LpColumnFilterSpec{*RiColumn::COUNT_ON_MAP, "##RiCountFilter", makeDelegate<LpRiTable, &LpRiTable::drawCountFilterContent>(this)});
  specs.push_back(LpColumnFilterSpec{
    *RiColumn::BSPHERE_RAD, "##RiBSphereFilter", makeDelegate<LpRiTable, &LpRiTable::drawBSphereFilterContent>(this)});
  specs.push_back(
    LpColumnFilterSpec{*RiColumn::BBOX_RAD, "##RiBBoxFilter", makeDelegate<LpRiTable, &LpRiTable::drawBBoxFilterContent>(this)});
  specs.push_back(LpColumnFilterSpec{
    *RiColumn::PHYS_TRIS, "##RiPhysTrisFilter", makeDelegate<LpRiTable, &LpRiTable::drawPhysTrisFilterContent>(this)});
  specs.push_back(LpColumnFilterSpec{
    *RiColumn::TRACE_TRIS, "##RiTraceTrisFilter", makeDelegate<LpRiTable, &LpRiTable::drawTraceTrisFilterContent>(this)});

  // LOD metric groups using persistent context objects (lodPopupContexts)
  lodPopupContexts.clear();
  lodPopupContexts.reserve(lodCount * 5);
  lodPopupIdStorage.clear();
  lodPopupIdStorage.reserve(lodCount * 4 + (lodCount - RiLayout::HEAVY_SHADERS_START_LOD));
  struct LodThunk
  {
    static void invoke(void *ud)
    {
      auto *ctx = static_cast<LpRiTable::LodPopupCtx *>(ud);
      if (!ctx || !ctx->table)
        return;
      switch (ctx->type)
      {
        case 0: ctx->table->drawLodDipsFilterContent(ctx->lod); break;
        case 1: ctx->table->drawLodTrisFilterContent(ctx->lod); break;
        case 2: ctx->table->drawLodDistFilterContent(ctx->lod); break;
        case 3: ctx->table->drawLodScreenFilterContent(ctx->lod); break;
        case 4: ctx->table->drawHeavyShadersFilterContent(ctx->lod); break;
        default: break;
      }
    }
  };
  for (int lod = 0; lod < lodCount; ++lod)
  {
    lodPopupContexts.push_back({this, lod, 0});
    int dipsCol = layout.dipsBase() + lod;
    lodPopupIdStorage.emplace_back();
    lodPopupIdStorage.back().sprintf("##RiLodDipsFilter_%d", lod);
    specs.push_back(
      LpColumnFilterSpec{dipsCol, lodPopupIdStorage.back().c_str(), LpCallbackDelegate{&LodThunk::invoke, &lodPopupContexts.back()}});

    lodPopupContexts.push_back({this, lod, 1});
    int trisCol = layout.trisBase() + lod;
    lodPopupIdStorage.emplace_back();
    lodPopupIdStorage.back().sprintf("##RiLodTrisFilter_%d", lod);
    specs.push_back(
      LpColumnFilterSpec{trisCol, lodPopupIdStorage.back().c_str(), LpCallbackDelegate{&LodThunk::invoke, &lodPopupContexts.back()}});

    lodPopupContexts.push_back({this, lod, 2});
    int distCol = layout.distBase() + lod;
    lodPopupIdStorage.emplace_back();
    lodPopupIdStorage.back().sprintf("##RiLodDistFilter_%d", lod);
    specs.push_back(
      LpColumnFilterSpec{distCol, lodPopupIdStorage.back().c_str(), LpCallbackDelegate{&LodThunk::invoke, &lodPopupContexts.back()}});

    lodPopupContexts.push_back({this, lod, 3});
    int screenCol = layout.screenBase() + lod;
    lodPopupIdStorage.emplace_back();
    lodPopupIdStorage.back().sprintf("##RiLodScreenFilter_%d", lod);
    specs.push_back(LpColumnFilterSpec{
      screenCol, lodPopupIdStorage.back().c_str(), LpCallbackDelegate{&LodThunk::invoke, &lodPopupContexts.back()}});
  }
  for (int lod = RiLayout::HEAVY_SHADERS_START_LOD; lod < lodCount; ++lod)
  {
    lodPopupContexts.push_back({this, lod, 4});
    int shadersCol = layout.shadersBase() + (lod - RiLayout::HEAVY_SHADERS_START_LOD);
    lodPopupIdStorage.emplace_back();
    lodPopupIdStorage.back().sprintf("##RiLodHeavyShadersFilter_%d", lod);
    specs.push_back(LpColumnFilterSpec{
      shadersCol, lodPopupIdStorage.back().c_str(), LpCallbackDelegate{&LodThunk::invoke, &lodPopupContexts.back()}});
  }

  registerColumnFilters(specs);
}

LpRiTable::~LpRiTable() {}

void LpRiTable::onNameSearchChange(const char *text) { nameFilter.setSearchText(text); }
void LpRiTable::onNameSearchCommit() {}

// Group columns for easier visibility management
eastl::vector<LpProfilerTable::ColumnGroup> LpRiTable::getColumnGroups() const
{
  eastl::vector<ColumnGroup> groups;
  RiLayout layout = makeRiLayout();
  int lodCountTotal = layout.lodCount;
  int activeLodCount = (currentMaxUsedLod > 0 && currentMaxUsedLod <= lodCountTotal) ? currentMaxUsedLod : lodCountTotal;


  auto pushLodGroup = [&](const char *name, int baseIndex) {
    ColumnGroup grp;
    grp.name = name;
    for (int lod = 0; lod < activeLodCount; ++lod)
      grp.columnIndices.push_back(baseIndex + lod);
    groups.push_back(eastl::move(grp));
  };

  pushLodGroup("LOD Dips", layout.dipsBase());
  pushLodGroup("LOD Tris", layout.trisBase());
  pushLodGroup("LOD Dist", layout.distBase());
  pushLodGroup("LOD Screen", layout.screenBase());

  // Heavy shaders group (if any)
  if (activeLodCount > RiLayout::HEAVY_SHADERS_START_LOD)
  {
    ColumnGroup shadersGroup;
    shadersGroup.name = "Heavy Shaders";
    for (int lod = RiLayout::HEAVY_SHADERS_START_LOD; lod < activeLodCount; ++lod)
      shadersGroup.columnIndices.push_back(layout.shadersBase() + (lod - RiLayout::HEAVY_SHADERS_START_LOD));
    groups.push_back(eastl::move(shadersGroup));
  }

  return groups;
}

void LpRiTable::setSelectedRi(const ProfilerString &ri_name) { selectedRiName = ri_name; }

void LpRiTable::drawContent()
{
  if (!riModule)
    return;

  if (isSortActive())
    sortFilteredRiItems();

  syncDynamicColumns(); // Ensure dynamic columns reflect current LOD usage

  const auto &riData = riModule->getRiData();

  if (!rangeFiltersInitialized)
    initRangeFilters();

  if (sortedRiOrder.size() != riData.size())
  {
    sortedRiOrder.clear();
    sortedRiOrder.reserve(riData.size());
    for (size_t i = 0; i < riData.size(); ++i)
      sortedRiOrder.push_back(static_cast<int>(i));
  }

  eastl::vector<RiTableItem> tableItems;
  tableItems.reserve(sortedRiOrder.size());
  for (int idx : sortedRiOrder)
  {
    if (idx < 0 || static_cast<size_t>(idx) >= riData.size())
      continue;
    const RiData &riRef = riData[static_cast<size_t>(idx)];
    if (!passesFilters(riRef))
      continue;
    bool isSelected = (selectedRiName == riRef.name);
    tableItems.emplace_back(riRef, isSelected);
  }

  filteredCount = tableItems.size();

  for (auto [rowIndex, item] : enumerate(tableItems))
  {
    beginRow();
    for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
    {
      if (!setColumn(columnIndex))
        continue;
      if (LpProfilerTableColumn *column = getColumn(columnIndex))
      {
        ImGui::PushID(currentRowIndex * columnsCount + columnIndex);
        column->drawCell(&item);
        if (columnIndex == 0 && ImGui::IsItemClicked())
        {
          setSelectedRi(item.data->name);
          if (itemSelectedCallback)
            itemSelectedCallback(currentRowIndex, &item, itemSelectedCallbackUserData);
        }
        drawRowContextMenu(currentRowIndex, columnIndex, &item);
        ImGui::PopID();
      }
    }
    endRow();
  }
}


bool LpRiTable::areAllColumnsEffectivelyVisible() const
{
  for (int i = 0; i < static_cast<int>(RiColumn::BASE_COUNT) && i < columnsCount; ++i)
    if (!columnVisibility[i])
      return false;

  RiLayout layout = makeRiLayout();
  int activeLodCount = currentMaxUsedLod > 0 ? currentMaxUsedLod : layout.lodCount;

  for (int lod = 0; lod < activeLodCount; ++lod)
    if (!columnVisibility[layout.dipsBase() + lod] || !columnVisibility[layout.trisBase() + lod] ||
        !columnVisibility[layout.distBase() + lod] || !columnVisibility[layout.screenBase() + lod])
      return false;

  if (activeLodCount > RiLayout::HEAVY_SHADERS_START_LOD)
    for (int lod = RiLayout::HEAVY_SHADERS_START_LOD; lod < activeLodCount; ++lod)
      if (!columnVisibility[layout.shadersBase() + (lod - RiLayout::HEAVY_SHADERS_START_LOD)])
        return false;

  return true;
}

void LpRiTable::handleSelection() {}

void LpRiTable::sortFilteredRiItems()
{
  if (!riModule)
    return;
  const auto &riData = riModule->getRiData();
  if (!sortSpecs || sortSpecs->SpecsCount == 0 || riData.empty())
    return;

  if (sortedRiOrder.size() != riData.size())
  {
    sortedRiOrder.clear();
    sortedRiOrder.reserve(riData.size());
    for (size_t i = 0; i < riData.size(); ++i)
      sortedRiOrder.push_back(static_cast<int>(i));
  }

  auto compareIndices = [this, &riData](int aIdx, int bIdx) -> bool {
    const RiData &a = riData[static_cast<size_t>(aIdx)];
    const RiData &b = riData[static_cast<size_t>(bIdx)];
    for (int i = 0; i < sortSpecs->SpecsCount; ++i)
    {
      const ImGuiTableColumnSortSpecs *spec = &sortSpecs->Specs[i];
      int columnIndex = spec->ColumnIndex;
      int delta = 0;
      if (columnIndex >= 0 && columnIndex < columnsCount)
      {
        if (auto *column = getColumn(columnIndex))
        {
          RiTableItem itemA(a);
          RiTableItem itemB(b);
          delta = column->compareItems(&itemA, &itemB);
        }
      }
      if (delta != 0)
        return spec->SortDirection == ImGuiSortDirection_Ascending ? (delta < 0) : (delta > 0);
    }
    return aIdx < bIdx;
  };

  eastl::stable_sort(sortedRiOrder.begin(), sortedRiOrder.end(), compareIndices);
}

void LpRiTable::buildFilteredIndexOrder(eastl::vector<int> &out) const
{
  out.clear();
  if (!riModule)
    return;
  const auto &riData = riModule->getRiData();
  if (riData.empty())
    return;
  if (sortedRiOrder.size() != riData.size())
  {
    for (auto [dataIndex, riRef] : enumerate(riData))
    {
      if (passesFilters(riRef))
        out.push_back(static_cast<int>(dataIndex));
    }
    return;
  }
  for (int idx : sortedRiOrder)
  {
    if (idx < 0 || static_cast<size_t>(idx) >= riData.size())
      continue;
    const RiData &riRef = riData[static_cast<size_t>(idx)];
    if (!passesFilters(riRef))
      continue;
    out.push_back(idx);
  }
}

bool LpRiTable::isFilterActive(ColumnIndex column) const
{
  RiLayout layout = makeRiLayout();
  int dipsBase = layout.dipsBase();
  int trisBase = layout.trisBase();
  int distBase = layout.distBase();
  int lodCount = layout.lodCount;
  switch (static_cast<int>(column))
  {
    case *RiColumn::NAME: return nameFilter.isActive();
    case *RiColumn::COUNT_ON_MAP: return countRangeFilter.isUsingMin() || countRangeFilter.isUsingMax();
    case *RiColumn::BSPHERE_RAD: return bSphereRadiusRangeFilter.isUsingMin() || bSphereRadiusRangeFilter.isUsingMax();
    case *RiColumn::BBOX_RAD: return bBoxRadiusRangeFilter.isUsingMin() || bBoxRadiusRangeFilter.isUsingMax();
    case *RiColumn::PHYS_TRIS: return physTrisRangeFilter.isUsingMin() || physTrisRangeFilter.isUsingMax();
    case *RiColumn::TRACE_TRIS: return traceTrisRangeFilter.isUsingMin() || traceTrisRangeFilter.isUsingMax();
    default:
      if (column >= dipsBase && column < dipsBase + lodCount)
      {
        int lod = static_cast<int>(column) - dipsBase;
        return lod >= 0 && lod < static_cast<int>(lodDipsRangeFilters.size()) &&
               (lodDipsRangeFilters[lod].isUsingMin() || lodDipsRangeFilters[lod].isUsingMax());
      }
      if (column >= trisBase && column < trisBase + lodCount)
      {
        int lod = static_cast<int>(column) - trisBase;
        return lod >= 0 && lod < static_cast<int>(lodTrisRangeFilters.size()) &&
               (lodTrisRangeFilters[lod].isUsingMin() || lodTrisRangeFilters[lod].isUsingMax());
      }
      if (column >= distBase && column < distBase + lodCount)
      {
        int lod = static_cast<int>(column) - distBase;
        return lod >= 0 && lod < static_cast<int>(lodDistRangeFilters.size()) &&
               (lodDistRangeFilters[lod].isUsingMin() || lodDistRangeFilters[lod].isUsingMax());
      }

      int screenBase = layout.screenBase();
      if (column >= screenBase && column < screenBase + lodCount)
      {
        int lod = static_cast<int>(column) - screenBase;
        return lod >= 0 && lod < static_cast<int>(lodScreenRangeFilters.size()) &&
               (lodScreenRangeFilters[lod].isUsingMin() || lodScreenRangeFilters[lod].isUsingMax());
      }

      int shadersBase = layout.shadersBase();
      int shadersCount = layout.shadersCount();
      if (shadersCount > 0 && column >= shadersBase && column < shadersBase + shadersCount)
      {
        int lod = (static_cast<int>(column) - shadersBase) + RiLayout::HEAVY_SHADERS_START_LOD;
        int filterIndex = lod - RiLayout::HEAVY_SHADERS_START_LOD;
        return filterIndex >= 0 && filterIndex < static_cast<int>(heavyShaderFilters.size()) &&
               !heavyShaderFilters[filterIndex].getOffList().empty();
      }
      return false;
  }
}

void LpRiTable::clearAllFilters()
{
  nameFilter.reset();
  if (auto *w = getSearchWidget(*RiColumn::NAME))
    w->clear();
  countRangeFilter.reset();
  bSphereRadiusRangeFilter.reset();
  bBoxRadiusRangeFilter.reset();
  physTrisRangeFilter.reset();
  traceTrisRangeFilter.reset();
  for (auto &f : lodDipsRangeFilters)
    f.reset();
  for (auto &f : lodTrisRangeFilters)
    f.reset();
  for (auto &f : lodDistRangeFilters)
    f.reset();
  for (auto &f : lodScreenRangeFilters)
    f.reset();
  for (auto &f : heavyShaderFilters)
    f.reset();
}

void LpRiTable::clearColumnFilter(ColumnIndex column)
{
  RiLayout layout = makeRiLayout();
  int dipsBase = layout.dipsBase();
  int trisBase = layout.trisBase();
  int distBase = layout.distBase();
  int lodCount = layout.lodCount;
  switch (static_cast<int>(column))
  {
    case *RiColumn::NAME:
      nameFilter.reset();
      if (auto *w = getSearchWidget(column))
        w->clear();
      break;
    case *RiColumn::COUNT_ON_MAP: countRangeFilter.reset(); break;
    case *RiColumn::BSPHERE_RAD: bSphereRadiusRangeFilter.reset(); break;
    case *RiColumn::BBOX_RAD: bBoxRadiusRangeFilter.reset(); break;
    case *RiColumn::PHYS_TRIS: physTrisRangeFilter.reset(); break;
    case *RiColumn::TRACE_TRIS: traceTrisRangeFilter.reset(); break;
    default:
      if (column >= dipsBase && column < dipsBase + lodCount)
      {
        int lod = static_cast<int>(column) - dipsBase;
        if (lod >= 0 && lod < static_cast<int>(lodDipsRangeFilters.size()))
          lodDipsRangeFilters[lod].reset();
      }
      else if (column >= trisBase && column < trisBase + lodCount)
      {
        int lod = static_cast<int>(column) - trisBase;
        if (lod >= 0 && lod < static_cast<int>(lodTrisRangeFilters.size()))
          lodTrisRangeFilters[lod].reset();
      }
      else if (column >= distBase && column < distBase + lodCount)
      {
        int lod = static_cast<int>(column) - distBase;
        if (lod >= 0 && lod < static_cast<int>(lodDistRangeFilters.size()))
          lodDistRangeFilters[lod].reset();
      }
      else
      {
        int screenBase = layout.screenBase();
        if (column >= screenBase && column < screenBase + lodCount)
        {
          int lod = static_cast<int>(column) - screenBase;
          if (lod >= 0 && lod < static_cast<int>(lodScreenRangeFilters.size()))
            lodScreenRangeFilters[lod].reset();
        }
        else
        {
          int shadersBase = layout.shadersBase();
          int shadersCount = layout.shadersCount();
          if (shadersCount > 0 && column >= shadersBase && column < shadersBase + shadersCount)
          {
            int lod = (static_cast<int>(column) - shadersBase) + RiLayout::HEAVY_SHADERS_START_LOD;
            int filterIndex = lod - RiLayout::HEAVY_SHADERS_START_LOD;
            if (filterIndex >= 0 && filterIndex < static_cast<int>(heavyShaderFilters.size()))
            {
              heavyShaderFilters[filterIndex].reset();
            }
          }
        }
      }
      break;
  }
}

void LpRiTable::drawNameFilterContent()
{
  if (nameFilter.drawInline()) {}
}

void LpRiTable::drawCountFilterContent()
{
  LpRangeFilterController<int> countCtrl(&countRangeFilter);
  drawRangeFilter<int>("CountRange", "Count", countCtrl, "Min: %d", "Max: %d", UIConstants::COUNT_FILTER_SPEED);
}

void LpRiTable::drawBSphereFilterContent()
{
  LpRangeFilterController<float> bsCtrl(&bSphereRadiusRangeFilter);
  drawRangeFilter<float>("BSphereRange", "BSphere Rad", bsCtrl, "Min: %.1f", "Max: %.1f", UIConstants::RADIUS_FILTER_SPEED);
}

void LpRiTable::drawBBoxFilterContent()
{
  LpRangeFilterController<float> bbCtrl(&bBoxRadiusRangeFilter);
  drawRangeFilter<float>("BBoxRange", "BBox Rad", bbCtrl, "Min: %.1f", "Max: %.1f", UIConstants::RADIUS_FILTER_SPEED);
}

void LpRiTable::drawLodDipsFilterContent(int lod_index)
{
  if (lod_index >= 0 && lod_index < static_cast<int>(lodDipsRangeFilters.size()))
  {
    char idBuf[32];
    char labelBuf[32];
    snprintf(idBuf, sizeof(idBuf), "LodDipsSingle%d", lod_index);
    snprintf(labelBuf, sizeof(labelBuf), "LOD %d Dips", lod_index);
    LpRangeFilterController<int> dipsCtrl(&lodDipsRangeFilters[lod_index]);
    drawRangeFilter<int>(idBuf, labelBuf, dipsCtrl, "Min: %d", "Max: %d", UIConstants::DIPS_FILTER_SPEED);
  }
}

void LpRiTable::drawLodTrisFilterContent(int lod_index)
{
  if (lod_index >= 0 && lod_index < static_cast<int>(lodTrisRangeFilters.size()))
  {
    char idBuf[32];
    char labelBuf[32];
    snprintf(idBuf, sizeof(idBuf), "LodTrisSingle%d", lod_index);
    snprintf(labelBuf, sizeof(labelBuf), "LOD %d Tris", lod_index);
    LpRangeFilterController<int> trisCtrl(&lodTrisRangeFilters[lod_index]);
    drawRangeFilter<int>(idBuf, labelBuf, trisCtrl, "Min: %d", "Max: %d", UIConstants::TRIS_FILTER_SPEED);
  }
}

void LpRiTable::drawPhysTrisFilterContent()
{
  LpRangeFilterController<int> physCtrl(&physTrisRangeFilter);
  drawRangeFilter<int>("PhysTrisRange", "Phys Tris", physCtrl, "Min: %d", "Max: %d", UIConstants::TRIS_FILTER_SPEED);
}

void LpRiTable::drawTraceTrisFilterContent()
{
  LpRangeFilterController<int> traceCtrl(&traceTrisRangeFilter);
  drawRangeFilter<int>("TraceTrisRange", "Trace Tris", traceCtrl, "Min: %d", "Max: %d", UIConstants::TRIS_FILTER_SPEED);
}

void LpRiTable::drawLodDistFilterContent(int lod_index)
{
  if (lod_index >= 0 && lod_index < static_cast<int>(lodDistRangeFilters.size()))
  {
    char idBuf[32];
    char labelBuf[32];
    snprintf(idBuf, sizeof(idBuf), "LodDistSingle%d", lod_index);
    snprintf(labelBuf, sizeof(labelBuf), "LOD %d Dist", lod_index);
    LpRangeFilterController<int> distCtrl(&lodDistRangeFilters[lod_index]);
    drawRangeFilter<int>(idBuf, labelBuf, distCtrl, "Min: %d", "Max: %d", UIConstants::DIST_FILTER_SPEED);
  }
}

void LpRiTable::drawLodScreenFilterContent(int lod_index)
{
  if (lod_index >= 0 && lod_index < static_cast<int>(lodScreenRangeFilters.size()))
  {
    char idBuf[32];
    char labelBuf[32];
    snprintf(idBuf, sizeof(idBuf), "LodScreenSingle%d", lod_index);
    snprintf(labelBuf, sizeof(labelBuf), "LOD %d Screen%%", lod_index);
    LpRangeFilterController<float> screenCtrl(&lodScreenRangeFilters[lod_index]);
    drawRangeFilter<float>(idBuf, labelBuf, screenCtrl, "Min: %.1f", "Max: %.1f", UIConstants::SCREEN_FILTER_SPEED);
  }
}

void LpRiTable::drawHeavyShadersFilterContent(int lod_index)
{
  if (!riModule)
    return;
  if (lod_index < RiLayout::HEAVY_SHADERS_START_LOD)
    return;
  int filterIndex = lod_index - RiLayout::HEAVY_SHADERS_START_LOD;
  if (filterIndex < 0 || filterIndex >= static_cast<int>(heavyShaderFilters.size()))
    return;

  const ProfilerString EMPTY_SENTINEL = "None";
  eastl::hash_set<ProfilerString> uniqueSet;
  const auto &riData = riModule->getRiData();
  bool hasEmpty = false;
  for (const auto &d : riData)
  {
    if (lod_index < static_cast<int>(d.lods.size()))
    {
      const auto &lod = d.lods[lod_index];
      if (lod.heavyShaders.empty())
        hasEmpty = true;
      for (const auto &entry : lod.heavyShaders)
        uniqueSet.insert(entry.name);
    }
    else
      hasEmpty = true; // missing LOD treated as empty
  }
  eastl::vector<ProfilerString> uniqueList;
  uniqueList.reserve(uniqueSet.size() + (hasEmpty ? 1 : 0));
  for (const auto &s : uniqueSet)
    uniqueList.push_back(s);
  eastl::sort(uniqueList.begin(), uniqueList.end());
  if (hasEmpty)
    uniqueList.push_back(EMPTY_SENTINEL);

  bool changed = false;
  LpMultiSelectWidget<ProfilerString> widget(MultiSelectLabelFuncs::ProfilerStringLabel);
  widget.Draw(uniqueList, heavyShaderFilters[filterIndex], changed);
}

void LpRiTable::initRangeFilters()
{
  if (!riModule)
    return;
  const auto &riData = riModule->getRiData();
  if (riData.empty())
  {
    rangeFiltersInitialized = true;
    return;
  }
  // Determine bounds
  int minCount = riData[0].countOnMap, maxCount = riData[0].countOnMap;
  int minPhys = riData[0].collision.physTriangles, maxPhys = riData[0].collision.physTriangles;
  int minTrace = riData[0].collision.traceTriangles, maxTrace = riData[0].collision.traceTriangles;
  // Lod bounds vectors sized per MAX_LODS
  RiLayout layout = makeRiLayout();
  int lodCount = layout.lodCount;
  eastl::vector<int> minDips(lodCount, INT_MAX), maxDips(lodCount, 0);
  eastl::vector<int> minTris(lodCount, INT_MAX), maxTris(lodCount, 0);
  eastl::vector<int> minDist(lodCount, INT_MAX), maxDist(lodCount, 0);
  eastl::vector<float> minScreen(lodCount, FLT_MAX), maxScreen(lodCount, 0.0f);
  for (const auto &d : riData)
  {
    if (d.countOnMap < minCount)
      minCount = d.countOnMap;
    if (d.countOnMap > maxCount)
      maxCount = d.countOnMap;
    if (d.collision.physTriangles < minPhys)
      minPhys = d.collision.physTriangles;
    if (d.collision.physTriangles > maxPhys)
      maxPhys = d.collision.physTriangles;
    if (d.collision.traceTriangles < minTrace)
      minTrace = d.collision.traceTriangles;
    if (d.collision.traceTriangles > maxTrace)
      maxTrace = d.collision.traceTriangles;

    size_t maxLodIndex = eastl::min(d.lods.size(), static_cast<size_t>(lodCount));
    for (size_t lodIndex = 0; lodIndex < maxLodIndex; ++lodIndex)
    {
      const auto &lod = d.lods[lodIndex];
      if (lod.drawCalls < minDips[lodIndex])
        minDips[lodIndex] = lod.drawCalls;
      if (lod.drawCalls > maxDips[lodIndex])
        maxDips[lodIndex] = lod.drawCalls;
      if (lod.totalFaces < minTris[lodIndex])
        minTris[lodIndex] = lod.totalFaces;
      if (lod.totalFaces > maxTris[lodIndex])
        maxTris[lodIndex] = lod.totalFaces;
      int lodDistInt = static_cast<int>(lod.lodDistance);
      if (lodDistInt < minDist[lodIndex])
        minDist[lodIndex] = lodDistInt;
      if (lodDistInt > maxDist[lodIndex])
        maxDist[lodIndex] = lodDistInt;
      if (lod.screenPercent < minScreen[lodIndex])
        minScreen[lodIndex] = lod.screenPercent;
      if (lod.screenPercent > maxScreen[lodIndex])
        maxScreen[lodIndex] = lod.screenPercent;
    }
  }
  countRangeFilter.reset(minCount, maxCount);
  physTrisRangeFilter.reset(minPhys, maxPhys);
  traceTrisRangeFilter.reset(minTrace, maxTrace);
  lodDipsRangeFilters.resize(lodCount);
  lodTrisRangeFilters.resize(lodCount);
  lodDistRangeFilters.resize(lodCount);
  lodScreenRangeFilters.resize(lodCount);
  int heavyCount = (lodCount > RiLayout::HEAVY_SHADERS_START_LOD) ? (lodCount - RiLayout::HEAVY_SHADERS_START_LOD) : 0;
  heavyShaderFilters.resize(heavyCount);

  float minBSphere = FLT_MAX, maxBSphere = 0.0f;
  float minBBox = FLT_MAX, maxBBox = 0.0f;
  for (const auto &d : riData)
  {
    if (d.bSphereRadius < minBSphere)
      minBSphere = d.bSphereRadius;
    if (d.bSphereRadius > maxBSphere)
      maxBSphere = d.bSphereRadius;
    if (d.bBoxRadius < minBBox)
      minBBox = d.bBoxRadius;
    if (d.bBoxRadius > maxBBox)
      maxBBox = d.bBoxRadius;
  }
  if (minBSphere == FLT_MAX)
    minBSphere = 0.0f;
  if (minBBox == FLT_MAX)
    minBBox = 0.0f;
  bSphereRadiusRangeFilter.reset(minBSphere, maxBSphere);
  bBoxRadiusRangeFilter.reset(minBBox, maxBBox);
  for (int i = 0; i < lodCount; ++i)
  {
    int dipsMin = (minDips[i] == INT_MAX) ? 0 : minDips[i];
    int dipsMax = maxDips[i];
    int trisMin = (minTris[i] == INT_MAX) ? 0 : minTris[i];
    int trisMax = maxTris[i];
    int distMin = (minDist[i] == INT_MAX) ? 0 : minDist[i];
    int distMax = maxDist[i];
    lodDipsRangeFilters[i].reset(dipsMin, dipsMax);
    lodTrisRangeFilters[i].reset(trisMin, trisMax);
    lodDistRangeFilters[i].reset(distMin, distMax);
    float scrMin = (minScreen[i] == FLT_MAX) ? 0.0f : minScreen[i];
    float scrMax = maxScreen[i];
    lodScreenRangeFilters[i].reset(scrMin, scrMax);
  }
  rangeFiltersInitialized = true;
}

bool LpRiTable::passesFilters(const RiData &data_ref) const
{
  if (!baseFiltersPass(data_ref))
    return false;
  return dynamicFiltersPass(data_ref);
}

bool LpRiTable::baseFiltersPass(const RiData &data_ref) const
{
  if (nameFilter.isActive() && !nameFilter.pass(data_ref.name))
    return false;
  if (!countRangeFilter.pass(data_ref.countOnMap))
    return false;
  if (!bSphereRadiusRangeFilter.pass(data_ref.bSphereRadius))
    return false;
  if (!bBoxRadiusRangeFilter.pass(data_ref.bBoxRadius))
    return false;
  if (!physTrisRangeFilter.pass(data_ref.collision.physTriangles))
    return false;
  if (!traceTrisRangeFilter.pass(data_ref.collision.traceTriangles))
    return false;
  return true;
}

bool LpRiTable::dynamicFiltersPass(const RiData &data_ref) const
{
  RiLayout layout = makeRiLayout();
  // Per LOD metrics and heavy shaders
  size_t lodsSize = data_ref.lods.size();
  size_t maxFilterIndex = eastl::min(lodsSize, lodDipsRangeFilters.size());
  for (size_t lodIndex = 0; lodIndex < maxFilterIndex; ++lodIndex)
  {
    const auto &lod = data_ref.lods[lodIndex];
    if (!lodDipsRangeFilters[lodIndex].pass(lod.drawCalls))
      return false;
    if (!lodTrisRangeFilters[lodIndex].pass(lod.totalFaces))
      return false;
    int lodDistInt = static_cast<int>(lod.lodDistance);
    if (lodIndex < lodDistRangeFilters.size() && !lodDistRangeFilters[lodIndex].pass(lodDistInt))
      return false;
    if (lodIndex < lodScreenRangeFilters.size() && !lodScreenRangeFilters[lodIndex].pass(lod.screenPercent))
      return false;

    if (static_cast<int>(lodIndex) >= RiLayout::HEAVY_SHADERS_START_LOD)
    {
      int filterIndex = static_cast<int>(lodIndex) - RiLayout::HEAVY_SHADERS_START_LOD;
      if (filterIndex >= 0 && filterIndex < static_cast<int>(heavyShaderFilters.size()))
      {
        if (!lod.heavyShaders.empty())
        {
          for (const auto &entry : lod.heavyShaders)
            if (!heavyShaderFilters[filterIndex].pass(entry.name))
              return false;
        }
        else
        {
          static const ProfilerString EMPTY_SENTINEL = "None";
          if (!heavyShaderFilters[filterIndex].pass(EMPTY_SENTINEL))
            return false;
        }
      }
    }
  }
  // Treat missing higher LODs as empty (for heavy shader filtering only)
  static const ProfilerString EMPTY_SENTINEL = "None";
  for (int lod = static_cast<int>(lodsSize); lod < layout.lodCount; ++lod)
  {
    if (lod < RiLayout::HEAVY_SHADERS_START_LOD)
      continue;
    int filterIndex = lod - RiLayout::HEAVY_SHADERS_START_LOD;
    if (filterIndex >= 0 && filterIndex < static_cast<int>(heavyShaderFilters.size()))
      if (!heavyShaderFilters[filterIndex].pass(EMPTY_SENTINEL))
        return false;
  }
  return true;
}

void LpRiTable::syncDynamicColumns()
{
  if (!riModule)
    return;
  RiLayout layout = makeRiLayout();
  const int lodCount = layout.lodCount;
  const auto &riDataRef = riModule->getRiData();
  int maxUsedLod = 0;
  for (const auto &d : riDataRef)
  {
    int lodSize = static_cast<int>(d.lods.size());
    if (lodSize > maxUsedLod)
      maxUsedLod = lodSize;
  }
  int prevMaxUsedLod = currentMaxUsedLod;
  if (maxUsedLod != prevMaxUsedLod)
  {
    auto colVisOk = [this](int colIdx) {
      return columnVisibility.size() == static_cast<size_t>(columnsCount) ? columnVisibility[colIdx] : true;
    };
    auto enableLodColumns = [&](int lod) {
      int dipsIdx = layout.dipsBase() + lod;
      if (colVisOk(dipsIdx))
        ImGui::TableSetColumnEnabled(dipsIdx, true);
      int trisIdx = layout.trisBase() + lod;
      if (colVisOk(trisIdx))
        ImGui::TableSetColumnEnabled(trisIdx, true);
      int distIdx = layout.distBase() + lod;
      if (colVisOk(distIdx))
        ImGui::TableSetColumnEnabled(distIdx, true);
      int screenIdx = layout.screenBase() + lod;
      if (colVisOk(screenIdx))
        ImGui::TableSetColumnEnabled(screenIdx, true);
      if (lod >= RiLayout::HEAVY_SHADERS_START_LOD)
      {
        int shaderIdx = layout.shadersBase() + (lod - RiLayout::HEAVY_SHADERS_START_LOD);
        if (colVisOk(shaderIdx))
          ImGui::TableSetColumnEnabled(shaderIdx, true);
      }
    };
    auto disableLodColumns = [&](int lod) {
      int dipsIdx = layout.dipsBase() + lod;
      ImGui::TableSetColumnEnabled(dipsIdx, false);
      int trisIdx = layout.trisBase() + lod;
      ImGui::TableSetColumnEnabled(trisIdx, false);
      int distIdx = layout.distBase() + lod;
      ImGui::TableSetColumnEnabled(distIdx, false);
      int screenIdx = layout.screenBase() + lod;
      ImGui::TableSetColumnEnabled(screenIdx, false);
      if (lod >= RiLayout::HEAVY_SHADERS_START_LOD)
      {
        int shaderIdx = layout.shadersBase() + (lod - RiLayout::HEAVY_SHADERS_START_LOD);
        ImGui::TableSetColumnEnabled(shaderIdx, false);
      }
    };
    if (maxUsedLod > prevMaxUsedLod)
      for (int lod = prevMaxUsedLod; lod < maxUsedLod; ++lod)
        enableLodColumns(lod);
    else if (maxUsedLod < prevMaxUsedLod)
      for (int lod = maxUsedLod; lod < prevMaxUsedLod; ++lod)
        disableLodColumns(lod);
    for (int lod = maxUsedLod; lod < lodCount; ++lod)
      disableLodColumns(lod);
    currentMaxUsedLod = maxUsedLod;
  }
  // Always ensure columns beyond current usage are disabled
  for (int lod = maxUsedLod; lod < lodCount; ++lod)
  {
    int dipsIdx = layout.dipsBase() + lod;
    ImGui::TableSetColumnEnabled(dipsIdx, false);
    int trisIdx = layout.trisBase() + lod;
    ImGui::TableSetColumnEnabled(trisIdx, false);
    int distIdx = layout.distBase() + lod;
    ImGui::TableSetColumnEnabled(distIdx, false);
    int screenIdx = layout.screenBase() + lod;
    ImGui::TableSetColumnEnabled(screenIdx, false);
    if (lod >= RiLayout::HEAVY_SHADERS_START_LOD)
    {
      int shaderIdx = layout.shadersBase() + (lod - RiLayout::HEAVY_SHADERS_START_LOD);
      ImGui::TableSetColumnEnabled(shaderIdx, false);
    }
  }
}

eastl::vector<ProfilerString> LpRiTable::getCustomCopyMenuItems(const void * /* item_data */) const { return {}; }

ProfilerString LpRiTable::generateCustomCopyText(const void * /* item_data */, const ProfilerString & /* menu_item */) const
{
  return ProfilerString();
}

ProfilerString LpRiTable::generateLodDetailsText(const RiTableItem * /* item */) const { return ProfilerString(); }

ProfilerString LpRiTable::generateFullRowText(const RiTableItem * /* item */) const { return ProfilerString(); }

int LpRiTable::compareRiItems(const void *item1, const void *item2, const ImGuiTableSortSpecs *sort_specs) const
{
  if (!item1 || !item2 || !sort_specs || sort_specs->SpecsCount == 0)
    return UIConstants::EQUAL;
  const RiTableItem *riItem1 = static_cast<const RiTableItem *>(item1);
  const RiTableItem *riItem2 = static_cast<const RiTableItem *>(item2);
  if (!riItem1->data || !riItem2->data)
    return UIConstants::EQUAL;
  for (int i = 0; i < sort_specs->SpecsCount; ++i)
  {
    const ImGuiTableColumnSortSpecs *spec = &sort_specs->Specs[i];
    int colIdx = spec->ColumnIndex;
    if (colIdx < 0 || colIdx >= columnsCount)
      continue;
    if (auto *column = getColumn(colIdx))
    {
      int delta = column->compareItems(item1, item2);
      if (delta != 0)
        return spec->SortDirection == ImGuiSortDirection_Ascending ? delta : -delta;
    }
  }
  if (riItem1->data && riItem2->data)
    return riItem1->data->name.compare(riItem2->data->name);
  return UIConstants::EQUAL;
}

int LpRiTable::sort_table_items_callback(const ImGuiTableSortSpecs *sort_specs, const void *a, const void *b, void *user_data)
{
  LpRiTable *table = static_cast<LpRiTable *>(user_data);
  if (!table)
    return UIConstants::EQUAL;
  return table->compareRiItems(a, b, sort_specs);
}

void LpRiTable::on_item_selected(int /* index */, void *item_data, void *user_data)
{
  RiTableItem *item = static_cast<RiTableItem *>(item_data);
  LpRiTable *table = static_cast<LpRiTable *>(user_data);

  if (item && item->data && table)
  {
    table->setSelectedRi(item->data->name);
  }
}

} // namespace levelprofiler
