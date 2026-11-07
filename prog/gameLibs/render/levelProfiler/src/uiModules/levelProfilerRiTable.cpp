// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/sort.h>
#include <EASTL/algorithm.h>
#include "levelProfilerWidgets.h"
#include "riModule.h"
#include "levelProfilerRiTable.h"
#include "levelProfilerValueColumn.h"
#include "levelProfilerExporter.h"
#include <riGen/riGenExtra.h>
#include <math/dag_mathBase.h>

namespace levelprofiler
{
namespace
{
struct RiLayout
{
  int lodCount;
  static constexpr int PER_LOD_METRIC_GROUPS = 3;
  static constexpr int HEAVY_SHADERS_START_LOD = 2;
  int dipsBase() const { return static_cast<int>(RiColumn::BASE_COUNT); }
  int trisBase() const { return dipsBase() + lodCount; }
  int distBase() const { return trisBase() + lodCount; }
  int shadersBase() const { return distBase() + lodCount; }
  int shadersCount() const { return lodCount > HEAVY_SHADERS_START_LOD ? (lodCount - HEAVY_SHADERS_START_LOD) : 0; }
  int totalColumns() const { return static_cast<int>(RiColumn::BASE_COUNT) + lodCount * PER_LOD_METRIC_GROUPS + shadersCount(); }
};

inline RiLayout makeRiLayout() { return RiLayout{rendinst::RiExtraPool::MAX_LODS}; }

} // anonymous namespace

namespace UIConstants
{

static constexpr float COL_SHADERS_WIDTH = 90.0f;
static constexpr float COL_LOD_RATIO_WIDTH = 80.0f;
static constexpr float COL_COLLISION_RATIO_WIDTH = 100.0f;
static constexpr float COL_LOD_TRIS_SHARE_WIDTH = 120.0f;
static constexpr float COL_COLLISION_SHARE_WIDTH = 130.0f;
static constexpr float COL_DIST_SCREEN_WIDTH = 140.0f;

// Slider speeds for numeric filters (tuned per value scale)
static constexpr float COUNT_FILTER_SPEED = 1.0f;
static constexpr float LOD_COUNT_FILTER_SPEED = 0.1f;
static constexpr float RADIUS_FILTER_SPEED = 0.1f;
static constexpr float DIPS_FILTER_SPEED = 1.0f;
static constexpr float TRIS_FILTER_SPEED = 10.0f;
static constexpr float DIST_FILTER_SPEED = 1.0f;
static constexpr float SCREEN_FILTER_SPEED = 0.1f;
static constexpr float SCREEN_PIXELS_FILTER_SPEED = 0.5f;
static constexpr float RATIO_FILTER_SPEED = 0.5f;

} // namespace UIConstants

namespace
{
const ProfilerString HEAVY_SHADERS_NONE_VALUE = "None";
constexpr int RATIO_VIEW = 1;
constexpr int TRIS_BY_RATIO_VIEW = 3;
// Pixels go before the share: an artist judges a LOD switch by its size in pixels.
constexpr int DIST_PIXELS_BY_DIST_VIEW = 1;
constexpr int DIST_PIXELS_BY_PIXELS_VIEW = 2;
// The FOV mode of the game, not the table, decides whether the monitor aspect changes the screen size.
constexpr const char *SCREEN_PART_DESCRIPTION =
  "The screen size is for the table FOV on a 16:9 frame. With a Hor+ FOV, as in War Thunder and by default in DNG, the monitor "
  "aspect does not change it; War Thunder makes it 1.3 times larger on a triple-head screen (3:1 and wider).";
// The Screen parts of a Dist cell have a unit to tell them from the distance; the export header names the unit.
constexpr LpValueFormat<float> SCREEN_PART_FORMAT{.ui = lp_format_percent, .exportText = lp_format_float};
void format_pixels(char *text, int size, const float &value) { snprintf(text, size, "%.2f px", value); }
constexpr LpValueFormat<float> SCREEN_PIXELS_PART_FORMAT{.ui = format_pixels, .exportText = lp_format_float};

using TrisAndRatioDesc = LpCompositeColumnDesc<RiTableItem, int, float>;
using DistAndScreenColumn = LpCompositeColumn<RiTableItem, float, float>;
using DistAndScreenDesc = LpCompositeColumnDesc<RiTableItem, float, float>;

const ProfilerString &ri_name(const RiTableItem &item) { return item.data->name; }

const LodInfo *find_lod(const RiTableItem &item, int lod)
{
  return lod < static_cast<int>(item.data->lods.size()) ? &item.data->lods[lod] : nullptr;
}

ProfilerString lod_column_name(int lod_index, const char *metric)
{
  ProfilerString name;
  name.sprintf("L%d %s", lod_index, metric);
  return name;
}

ProfilerString screen_column_name(int lod_index, int fov)
{
  ProfilerString name;
  name.sprintf("L%d Screen%% @%d", lod_index, fov);
  return name;
}

ProfilerString screen_pixels_column_name(int lod_index, int fov, int height)
{
  ProfilerString name;
  name.sprintf("L%d Screen px @%d/%d", lod_index, fov, height);
  return name;
}

// Screen% is the share of the screen height that the longest bbox edge takes. With Hor+ the vertical extent of a 16:9 frame
// at the distance d is 2 * d * tan(hfov / 2) * 9 / 16 for any monitor aspect.
float screen_percent_scale(int fov) { return 100.0f / (2.0f * tanf(DegToRad(fov) * 0.5f) * 9.0f / 16.0f); }

template <typename T>
LpValueGetter<RiTableItem, T> ri_getter(T RiData::*field)
{
  return [field](const RiTableItem &item, T &value) {
    value = item.data->*field;
    return true;
  };
}

LpValueGetter<RiTableItem, int> collision_getter(int CollisionInfo::*field)
{
  return [field](const RiTableItem &item, int &value) {
    value = item.data->collision.*field;
    return true;
  };
}

template <typename T>
LpValueGetter<RiTableItem, T> lod_getter(int lod, T LodInfo::*field)
{
  return [lod, field](const RiTableItem &item, T &value) {
    const LodInfo *lodInfo = find_lod(item, lod);
    if (lodInfo)
      value = lodInfo->*field;
    return lodInfo != nullptr;
  };
}

float percent_of(int value, int reference) { return 100.0f * value / reference; }

using HeavyShadersFilter = LpMultiCheckColumnFilter<RiTableItem, ProfilerString>;

HeavyShadersFilter::ForEachKey heavy_shaders_of_lod(int lod)
{
  return [lod](const RiTableItem &item, const HeavyShadersFilter::KeyVisitor &visit) {
    if (const LodInfo *lodInfo = find_lod(item, lod))
      for (const LodInfo::HeavyShaderEntry &entry : lodInfo->heavyShaders)
        if (!visit(entry.name))
          return;
  };
}

LpValueGetter<RiTableItem, float> lod_tris_ratio_getter(int lod)
{
  return [lod](const RiTableItem &item, float &value) {
    const LodInfo *lodInfo = find_lod(item, lod);
    if (!lodInfo || item.data->lods[lod - 1].totalFaces <= 0)
      return false;
    value = percent_of(lodInfo->totalFaces, item.data->lods[lod - 1].totalFaces);
    return true;
  };
}

LpValueGetter<RiTableItem, float> collision_ratio_getter(int CollisionInfo::*field)
{
  return [field](const RiTableItem &item, float &value) {
    const LodInfo *lod0 = find_lod(item, 0);
    if (!lod0 || lod0->totalFaces <= 0)
      return false;
    value = percent_of(item.data->collision.*field, lod0->totalFaces);
    return true;
  };
}

void draw_no_lod_tooltip(const RiTableItem &item, int lod)
{
  const int lodCount = item.data->lodCount;
  ImGui::Text("No L%d: the asset has %d LOD%s", lod, lodCount, lodCount == 1 ? "" : "s");
}

void draw_lod_tris_tooltip(const RiTableItem &item, int lod)
{
  const LodInfo *lodInfo = find_lod(item, lod);
  if (!lodInfo)
  {
    draw_no_lod_tooltip(item, lod);
    return;
  }
  const LodInfo &previous = item.data->lods[lod - 1];
  ImGui::Text("L%d Tris: %d", lod, lodInfo->totalFaces);
  ImGui::Text("L%d Tris: %d", lod - 1, previous.totalFaces);
  if (previous.totalFaces > 0)
    ImGui::Text("L%d/L%d: %.2f%%", lod, lod - 1, percent_of(lodInfo->totalFaces, previous.totalFaces));
  if (lodInfo->totalFaces >= previous.totalFaces)
    ImGui::TextUnformatted("Not lighter than the previous LOD");
}

void draw_collision_tooltip(const RiTableItem &item, const char *kind, int CollisionInfo::*field)
{
  const int triangles = item.data->collision.*field;
  const bool isTrace = field == &CollisionInfo::traceTriangles;
  ImGui::Text("%s Tris: %d%s", kind, triangles, isTrace && triangles == 0 ? " (no trace collision)" : "");
  const LodInfo *lod0 = find_lod(item, 0);
  if (!lod0)
  {
    draw_no_lod_tooltip(item, 0);
    return;
  }
  ImGui::Text("L0 Tris: %d", lod0->totalFaces);
  if (lod0->totalFaces <= 0)
    return;
  ImGui::Text("%s/L0: %.2f%%", kind, percent_of(triangles, lod0->totalFaces));
  if (triangles > lod0->totalFaces)
    ImGui::TextUnformatted("Collision is heavier than the render mesh");
}

// The triangles and their share of a reference, as a number, a share, and both in one cell.
LpColumnSetup tris_and_ratio_column(const LpValueColumnDesc<RiTableItem, int> &tris_desc,
  const LpValueColumnDesc<RiTableItem, float> &ratio_desc, const TrisAndRatioDesc &both_desc, const char *ratio_short_name,
  const char *ratio_title, const char *both_title)
{
  LpViewColumnBuilder builder;
  const int trisFilterIndex = builder.addValueFilter(lp_range_filter(tris_desc, UIConstants::TRIS_FILTER_SPEED));
  const int ratioFilterIndex = builder.addValueFilter(lp_range_filter(ratio_desc, UIConstants::RATIO_FILTER_SPEED));
  builder.addView({"#", "Abs", "Triangles", lp_value_column(tris_desc), {trisFilterIndex}});
  builder.addView({"%", ratio_short_name, ratio_title, lp_value_column(ratio_desc), {ratioFilterIndex}});
  ProfilerString byTris, byRatio;
  byTris.sprintf("%s, sorted by the triangles", both_title);
  byRatio.sprintf("%s, sorted by the share", both_title);
  builder.addViews(lp_composite_views(both_desc, "#%", byTris.c_str(), byRatio.c_str(), trisFilterIndex, ratioFilterIndex));
  return builder.build();
}

LpColumnSetup collision_column(const char *kind, int CollisionInfo::*field)
{
  ProfilerString absName, ratioName, trisAndRatioName;
  absName.sprintf("%s Tris", kind);
  ratioName.sprintf("%s/L0 Tris", kind);
  trisAndRatioName.sprintf("%s Tris (/L0)", kind);
  LpCellTooltip<RiTableItem> tooltip = [kind, field](const RiTableItem &item) { draw_collision_tooltip(item, kind, field); };
  LpValueColumnDesc<RiTableItem, int> absDesc{
    absName.c_str(), LP_NARROW_COLUMN_WIDTH, collision_getter(field), LP_INT_FORMAT, tooltip};
  LpValueColumnDesc<RiTableItem, float> ratioDesc{
    ratioName.c_str(), UIConstants::COL_COLLISION_RATIO_WIDTH, collision_ratio_getter(field), LP_PERCENT_FORMAT, tooltip, tooltip};
  TrisAndRatioDesc trisAndRatioDesc{trisAndRatioName.c_str(), UIConstants::COL_COLLISION_SHARE_WIDTH,
    {absName.c_str(), absDesc.getter, LP_INT_FORMAT}, {ratioName.c_str(), ratioDesc.getter, LP_PERCENT_FORMAT}, tooltip, tooltip};
  return tris_and_ratio_column(absDesc, ratioDesc, trisAndRatioDesc, "L0", "Share of the L0 triangles",
    "Triangles and share of the L0 triangles");
}
} // namespace

LpRiNameColumn::LpRiNameColumn() :
  LpRiTableColumn("Name", LP_NAME_COLUMN_WIDTH,
    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_NoHide)
{}

ProfilerString LpRiNameColumn::getRowCellText(const RiTableItem &item) const { return item.data->name; }

int LpRiNameColumn::compareRows(const RiTableItem &item1, const RiTableItem &item2) const
{
  return item1.data->name.compare(item2.data->name);
}

void LpRiNameColumn::drawRowCell(const RiTableItem &item) const
{
  const float nameX = ImGui::GetCursorScreenPos().x;
  ImGui::Selectable(item.data->name.c_str(), item.isSelected, ImGuiSelectableFlags_SpanAllColumns);
  if (!item.isUpdating)
    return;

  // Row selection and the row context menu both query the last submitted item, so the marker is
  // painted rather than submitted: any widget here would take that role from the Selectable.
  const ImVec2 markerPos(nameX + ImGui::CalcTextSize(item.data->name.c_str()).x + 6.0f, ImGui::GetItemRectMin().y);
  ImGui::GetWindowDrawList()->AddText(markerPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), "(updating)");
}

LpRiHeavyShadersColumn::LpRiHeavyShadersColumn(int lod_index) :
  LpLongTextColumn(lod_column_name(lod_index, "Shaders").c_str(), UIConstants::COL_SHADERS_WIDTH, ImGuiTableColumnFlags_WidthFixed),
  lodIndex(lod_index)
{}

bool LpRiHeavyShadersColumn::rowHasValue(const RiTableItem &item) const
{
  const LodInfo *lodInfo = find_lod(item, lodIndex);
  return lodInfo && !lodInfo->heavyShaders.empty();
}

ProfilerString LpRiHeavyShadersColumn::getRowCellText(const RiTableItem &item) const
{
  if (!rowHasValue(item))
    return ProfilerString();

  const auto &heavyShaders = find_lod(item, lodIndex)->heavyShaders;
  ProfilerString result;
  result.reserve(heavyShaders.size() * LP_LIST_ENTRY_RESERVE_SIZE);
  for (const LodInfo::HeavyShaderEntry &entry : heavyShaders)
  {
    if (!result.empty())
      result += ", ";
    // One element is the usual case, so only a shader on several mesh elements shows the count.
    lp_append_name_count(result, entry.name.c_str(), entry.count, 2);
  }
  return result;
}

int LpRiHeavyShadersColumn::compareRows(const RiTableItem &item1, const RiTableItem &item2) const
{
  return lp_compare3(find_lod(item1, lodIndex)->heavyShaders.size(), find_lod(item2, lodIndex)->heavyShaders.size());
}

// The Screen% or Screen px range of one LOD, with the FOV and screen height fields of the table.
class LpRiScreenFilter final : public ILpColumnFilter
{
public:
  LpRiScreenFilter(LpRiTable &owner, LpValueGetter<RiTableItem, float> getter, const ProfilerString &label, float speed) :
    table(owner), range(eastl::move(getter), label, LP_FLOAT_FORMAT.rangeMinFormat, LP_FLOAT_FORMAT.rangeMaxFormat, speed)
  {}

  bool isActive() const override { return range.isActive(); }
  void reset() override { range.reset(); }
  bool pass(const void *item) const override { return range.pass(item); }

  bool drawPopup() override
  {
    const bool changed = range.drawPopup();
    ImGui::Separator();
    int fov = table.getScreenFov();
    if (ImGui::DragInt("FOV (hor, 16:9)", &fov, 1.0f, LpRiTable::MIN_SCREEN_FOV, LpRiTable::MAX_SCREEN_FOV, "%d",
          ImGuiSliderFlags_AlwaysClamp))
      table.setScreenFov(fov);
    int height = table.getScreenHeight();
    if (ImGui::DragInt("Height, px", &height, 1.0f, LpRiTable::MIN_SCREEN_HEIGHT, LpRiTable::MAX_SCREEN_HEIGHT, "%d",
          ImGuiSliderFlags_AlwaysClamp))
      table.setScreenHeight(height);
    return changed;
  }

  void beginRebind() override { range.beginRebind(); }
  void rebindItem(const void *item) override { range.rebindItem(item); }
  void endRebind() override { range.endRebind(); }

  void scale(float factor) { range.scale(factor); }

private:
  LpRiTable &table;
  LpRangeColumnFilter<RiTableItem, float> range;
};

// --- LpRiTable implementation ---

LpRiTable::LpRiTable(RIModule *ri_module_ptr) :
  LpTypedTable("RI Assets", makeRiLayout().totalColumns(), ri_name), riModule(ri_module_ptr)
{
  RiLayout layout = makeRiLayout();
  const int lodCount = layout.lodCount;
  screenPercentScale = screen_percent_scale(screenFov);

  using IntDesc = LpValueColumnDesc<RiTableItem, int>;
  using FloatDesc = LpValueColumnDesc<RiTableItem, float>;
  addColumn(*RiColumn::NAME, nameColumnSetup(eastl::make_unique<LpRiNameColumn>(), "Search RI names..."));
  addColumn(*RiColumn::COUNT_ON_MAP,
    lp_range_column(IntDesc{"Count", LP_NARROW_COLUMN_WIDTH, ri_getter(&RiData::countOnMap), LP_INT_FORMAT},
      UIConstants::COUNT_FILTER_SPEED));
  addColumn(*RiColumn::BSPHERE_RAD,
    lp_range_column(FloatDesc{"BSphere Rad", LP_MEDIUM_COLUMN_WIDTH, ri_getter(&RiData::bSphereRadius), LP_DISTANCE_FORMAT},
      UIConstants::RADIUS_FILTER_SPEED));
  addColumn(*RiColumn::BBOX_RAD,
    lp_range_column(FloatDesc{"BBox Rad", LP_MEDIUM_COLUMN_WIDTH, ri_getter(&RiData::bBoxRadius), LP_DISTANCE_FORMAT},
      UIConstants::RADIUS_FILTER_SPEED));
  addColumn(*RiColumn::LOD_COUNT, lp_range_column(IntDesc{"LODs", LP_NARROW_COLUMN_WIDTH, ri_getter(&RiData::lodCount), LP_INT_FORMAT},
                                    UIConstants::LOD_COUNT_FILTER_SPEED));
  addColumn(*RiColumn::PHYS_TRIS, collision_column("Phys", &CollisionInfo::physTriangles));
  addColumn(*RiColumn::TRACE_TRIS, collision_column("Trace", &CollisionInfo::traceTriangles));

  for (int lod = 0; lod < lodCount; ++lod)
  {
    const LpCellTooltip<RiTableItem> noLodTooltip = [lod](const RiTableItem &item) { draw_no_lod_tooltip(item, lod); };
    addColumn(layout.dipsBase() + lod, lp_range_column(IntDesc{lod_column_name(lod, "Dips").c_str(), LP_NARROW_COLUMN_WIDTH,
                                                         lod_getter(lod, &LodInfo::drawCalls), LP_INT_FORMAT, {}, noLodTooltip},
                                         UIConstants::DIPS_FILTER_SPEED));

    const ProfilerString trisName = lod_column_name(lod, "Tris");
    IntDesc trisDesc{trisName.c_str(), LP_NARROW_COLUMN_WIDTH, lod_getter(lod, &LodInfo::totalFaces), LP_INT_FORMAT, {}, noLodTooltip};
    if (lod == 0)
      addColumn(layout.trisBase(), lp_range_column(trisDesc, UIConstants::TRIS_FILTER_SPEED));
    else
    {
      const LpCellTooltip<RiTableItem> trisTooltip = [lod](const RiTableItem &item) { draw_lod_tris_tooltip(item, lod); };
      trisDesc.tooltip = trisDesc.emptyTooltip = trisTooltip;
      ProfilerString ratioName, trisAndRatioName;
      ratioName.sprintf("L%d/L%d Tris", lod, lod - 1);
      trisAndRatioName.sprintf("L%d Tris (/L%d)", lod, lod - 1);
      FloatDesc ratioDesc{
        ratioName.c_str(), UIConstants::COL_LOD_RATIO_WIDTH, lod_tris_ratio_getter(lod), LP_PERCENT_FORMAT, trisTooltip, trisTooltip};
      TrisAndRatioDesc trisAndRatioDesc{trisAndRatioName.c_str(), UIConstants::COL_LOD_TRIS_SHARE_WIDTH,
        {trisName.c_str(), trisDesc.getter, LP_INT_FORMAT}, {ratioName.c_str(), ratioDesc.getter, LP_PERCENT_FORMAT}, trisTooltip,
        trisTooltip};
      addColumn(layout.trisBase() + lod,
        tris_and_ratio_column(trisDesc, ratioDesc, trisAndRatioDesc, "Prev", "Share of the previous LOD triangles",
          "Triangles and share of the previous LOD triangles"));
    }

    auto screenGetter = [this, lod](float (LpRiTable::*metric)(const LodInfo &) const) -> LpValueGetter<RiTableItem, float> {
      return [this, lod, metric](const RiTableItem &item, float &value) {
        const LodInfo *lodInfo = find_lod(item, lod);
        if (lodInfo)
          value = (this->*metric)(*lodInfo);
        return lodInfo != nullptr;
      };
    };
    LpValueGetter<RiTableItem, float> percentGetter = screenGetter(&LpRiTable::getScreenPercent);
    LpValueGetter<RiTableItem, float> pixelGetter = screenGetter(&LpRiTable::getScreenPixels);

    LpCellTooltip<RiTableItem> distTooltip = [this, lod](const RiTableItem &item) {
      const LodInfo &lodInfo = *find_lod(item, lod);
      ImGui::Text("L%d Dist: %.2f m", lod, lodInfo.lodDistance);
      ImGui::Text("L%d Screen%% @%d: %.2f%%", lod, screenFov, getScreenPercent(lodInfo));
      ImGui::Text("L%d Screen px @%d/%d: %.2f", lod, screenFov, screenHeight, getScreenPixels(lodInfo));
      ImGui::Text("BBox Rad: %.2f m", item.data->bBoxRadius);
    };
    const ProfilerString distName = lod_column_name(lod, "Dist");
    const ProfilerString distAndScreenName = lod_column_name(lod, "Dist (+Screen%)");
    const ProfilerString distAndPixelsName = lod_column_name(lod, "Dist (+Screen px)");
    FloatDesc distDesc{
      distName.c_str(), LP_MEDIUM_COLUMN_WIDTH, lod_getter(lod, &LodInfo::lodDistance), LP_DISTANCE_FORMAT, distTooltip, noLodTooltip};
    const ProfilerString screenName = screen_column_name(lod, screenFov);
    const ProfilerString screenPixelsName = screen_pixels_column_name(lod, screenFov, screenHeight);
    DistAndScreenDesc distAndScreenDesc{distAndScreenName.c_str(), UIConstants::COL_DIST_SCREEN_WIDTH,
      {distName.c_str(), distDesc.getter, LP_DISTANCE_FORMAT}, {screenName.c_str(), percentGetter, SCREEN_PART_FORMAT}, distTooltip,
      noLodTooltip};
    LpViewColumnBuilder distBuilder;
    const int distFilterIndex = distBuilder.addValueFilter(lp_range_filter(distDesc, UIConstants::DIST_FILTER_SPEED));
    auto distScreenFilter =
      eastl::make_unique<LpRiScreenFilter>(*this, percentGetter, lod_column_name(lod, "Screen%"), UIConstants::SCREEN_FILTER_SPEED);
    ScreenLod screenLod{.distScreenFilter = distScreenFilter.get()};
    const int screenPercentFilterIndex = distBuilder.addValueFilter(eastl::move(distScreenFilter));
    auto distScreenViews =
      lp_composite_views(distAndScreenDesc, "DS", "Distance and share of the screen height, sorted by the distance",
        "Distance and share of the screen height, sorted by the share", distFilterIndex, screenPercentFilterIndex);
    screenLod.distScreenColumns[0] = static_cast<DistAndScreenColumn *>(distScreenViews.first.column.get());
    screenLod.distScreenColumns[1] = static_cast<DistAndScreenColumn *>(distScreenViews.second.column.get());

    DistAndScreenDesc distAndPixelsDesc{distAndPixelsName.c_str(), UIConstants::COL_DIST_SCREEN_WIDTH,
      {distName.c_str(), distDesc.getter, LP_DISTANCE_FORMAT}, {screenPixelsName.c_str(), pixelGetter, SCREEN_PIXELS_PART_FORMAT},
      distTooltip, noLodTooltip};
    auto distPixelsFilter = eastl::make_unique<LpRiScreenFilter>(*this, pixelGetter, lod_column_name(lod, "Screen px"),
      UIConstants::SCREEN_PIXELS_FILTER_SPEED);
    screenLod.distPixelsFilter = distPixelsFilter.get();
    const int screenPixelsFilterIndex = distBuilder.addValueFilter(eastl::move(distPixelsFilter));
    auto distPixelsViews =
      lp_composite_views(distAndPixelsDesc, "DP", "Distance and pixels of the screen height, sorted by the distance",
        "Distance and pixels of the screen height, sorted by the pixels", distFilterIndex, screenPixelsFilterIndex);
    for (LpColumnViewDesc *view : {&distScreenViews.first, &distScreenViews.second, &distPixelsViews.first, &distPixelsViews.second})
      view->description = SCREEN_PART_DESCRIPTION;
    screenLod.distPixelsColumns[0] = static_cast<DistAndScreenColumn *>(distPixelsViews.first.column.get());
    screenLod.distPixelsColumns[1] = static_cast<DistAndScreenColumn *>(distPixelsViews.second.column.get());
    screenLods.push_back(screenLod);

    distBuilder.addView({"#", "Dist", "Distance", lp_value_column(distDesc), {distFilterIndex}});
    distBuilder.addViews(eastl::move(distPixelsViews));
    distBuilder.addViews(eastl::move(distScreenViews));
    addColumn(layout.distBase() + lod, distBuilder.build());
  }
  for (int lod = RiLayout::HEAVY_SHADERS_START_LOD; lod < lodCount; ++lod)
    addColumn(layout.shadersBase() + (lod - RiLayout::HEAVY_SHADERS_START_LOD),
      {eastl::make_unique<LpRiHeavyShadersColumn>(lod), eastl::make_unique<HeavyShadersFilter>(heavy_shaders_of_lod(lod),
                                                          HEAVY_SHADERS_NONE_VALUE, MultiSelectLabelFuncs::ProfilerStringLabel)});

  const int dipsGroup = addColumnGroup("LOD Dips", layout.dipsBase(), lodCount);
  const int trisGroup = addColumnGroup("LOD Tris", layout.trisBase(), lodCount);
  const int distGroup = addColumnGroup("LOD Dist", layout.distBase(), lodCount);
  const int shadersGroup =
    addColumnGroup("Heavy Shaders", layout.shadersBase(), layout.shadersCount(), RiLayout::HEAVY_SHADERS_START_LOD);
  lodColumnGroupIds = {dipsGroup, trisGroup, distGroup, shadersGroup};
  for (int group : lodColumnGroupIds)
    setColumnGroupActiveCount(group, 0);

  addPreset({.name = "Geometry",
    .tooltip = "Which assets have heavy geometry? An asset without LODs, a LOD1 almost equal to LOD0?",
    .columns = {*RiColumn::COUNT_ON_MAP, *RiColumn::BBOX_RAD, *RiColumn::LOD_COUNT},
    .groups = {trisGroup},
    .views = {{.group = trisGroup, .view = TRIS_BY_RATIO_VIEW}},
    .sortColumn = layout.trisBase(),
    .sortDirection = ImGuiSortDirection_Descending});
  addPreset({.name = "LOD tuning",
    .tooltip = "Are the LOD distances correct? Does a LOD switch too late or too early?",
    .columns = {*RiColumn::COUNT_ON_MAP, *RiColumn::BBOX_RAD, *RiColumn::LOD_COUNT},
    .groups = {trisGroup, distGroup},
    .views = {{.group = trisGroup, .view = RATIO_VIEW}, {.group = distGroup, .view = DIST_PIXELS_BY_PIXELS_VIEW}},
    .sortColumn = layout.trisBase() + 1,
    .sortDirection = ImGuiSortDirection_Descending});
  addPreset({.name = "Materials",
    .tooltip = "Which assets have expensive materials: many dips on far LODs, layered shaders at a distance?",
    .columns = {*RiColumn::COUNT_ON_MAP},
    .groups = {dipsGroup, shadersGroup},
    .sortColumn = layout.dipsBase(),
    .sortDirection = ImGuiSortDirection_Descending});
  addPreset({.name = "Collision",
    .tooltip = "Is the collision too complex? Is there no trace collision?",
    .columns = {*RiColumn::COUNT_ON_MAP, *RiColumn::BSPHERE_RAD, *RiColumn::BBOX_RAD, *RiColumn::PHYS_TRIS, *RiColumn::TRACE_TRIS},
    .views = {{.column = *RiColumn::PHYS_TRIS, .view = TRIS_BY_RATIO_VIEW},
      {.column = *RiColumn::TRACE_TRIS, .view = TRIS_BY_RATIO_VIEW}},
    .sortColumn = *RiColumn::PHYS_TRIS,
    .sortDirection = ImGuiSortDirection_Descending});
  addPreset({.name = "All",
    .tooltip = "All columns, for example for a full export.",
    .allColumns = true,
    .views = {{.group = distGroup, .view = DIST_PIXELS_BY_DIST_VIEW}},
    .sortColumn = *RiColumn::NAME,
    .sortDirection = ImGuiSortDirection_Ascending});

  setCopyMenuTopColumns({*RiColumn::NAME});
}

void LpRiTable::drawContent()
{
  if (!riModule)
    return;

  LpProfilerTable::drawContent();
}

void LpRiTable::setScreenFov(int fov)
{
  fov = eastl::clamp(fov, MIN_SCREEN_FOV, MAX_SCREEN_FOV);
  if (fov == screenFov)
    return;
  const float oldScale = screenPercentScale;
  screenFov = fov;
  screenPercentScale = screen_percent_scale(fov);
  // Each Screen% and Screen px changes by the same factor, so the scaled ranges keep the filter of the user.
  const float factor = screenPercentScale / oldScale;
  for (const ScreenLod &screenLod : screenLods)
  {
    screenLod.distScreenFilter->scale(factor);
    screenLod.distPixelsFilter->scale(factor);
  }
  updateScreenColumnNames();
  markRowsDirty();
}

void LpRiTable::setScreenHeight(int height)
{
  height = eastl::clamp(height, MIN_SCREEN_HEIGHT, MAX_SCREEN_HEIGHT);
  if (height == screenHeight)
    return;
  const float factor = static_cast<float>(height) / screenHeight;
  screenHeight = height;
  for (const ScreenLod &screenLod : screenLods)
    screenLod.distPixelsFilter->scale(factor);
  updateScreenColumnNames();
  markRowsDirty();
}

void LpRiTable::updateScreenColumnNames()
{
  for (int lod = 0; lod < static_cast<int>(screenLods.size()); ++lod)
  {
    for (DistAndScreenColumn *column : screenLods[lod].distScreenColumns)
      column->setSecondName(screen_column_name(lod, screenFov).c_str());
    for (DistAndScreenColumn *column : screenLods[lod].distPixelsColumns)
      column->setSecondName(screen_pixels_column_name(lod, screenFov, screenHeight).c_str());
  }
}

bool LpRiTable::pollItemsChanged()
{
  if (!riModule)
    return false;

  // Only the count filter and the Count sort read countOnMap; the count cell reads it live.
  if (unsigned counts = riModule->getRiCountsGeneration(); counts != countsGeneration)
  {
    countsGeneration = counts;
    if (isFilterActive(*RiColumn::COUNT_ON_MAP) || isSortedBy(*RiColumn::COUNT_ON_MAP))
      markRowsDirty();
  }

  unsigned dataGeneration = riModule->getRiDataGeneration();
  if (dataGeneration == itemsGeneration)
    return false;
  itemsGeneration = dataGeneration;
  return true;
}

void LpRiTable::rebuildItems()
{
  const auto &riData = riModule->getRiData();
  items.clear();
  items.reserve(riData.size());
  int maxCollectedLodCount = 0;
  for (const RiData &riRef : riData)
  {
    items.emplace_back(riRef);
    maxCollectedLodCount = eastl::max(maxCollectedLodCount, static_cast<int>(riRef.lods.size()));
  }
  for (int group : lodColumnGroupIds)
    setColumnGroupActiveCount(group, maxCollectedLodCount);
}

void LpRiTable::prepareItemForDraw(RiTableItem &item) { item.isUpdating = riModule->isCollecting(); }

} // namespace levelprofiler
