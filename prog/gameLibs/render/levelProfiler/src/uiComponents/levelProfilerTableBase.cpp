// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math.h>
#include <string.h>
#include <EASTL/sort.h>
#include "levelProfilerTableBase.h"
#include "levelProfilerFilter.h"
#include "levelProfilerExporter.h"
#include <imgui/imgui_internal.h>

namespace levelprofiler
{

// Buffer sizes for string formatting
static constexpr int TABLE_ID_BUFFER_SIZE = 128;
static constexpr int COLUMN_NAME_BUFFER_SIZE = 64;
static constexpr int FILTER_ID_BUFFER_SIZE = 64;
static const ImVec4 NO_ROWS_TEXT_COLOR = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
static const ImVec4 WARNING_BUTTON_COLOR = ImVec4(0.95f, 0.7f, 0.1f, 1.0f);
static const ImVec4 WARNING_BUTTON_HOVERED_COLOR = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
static const ImVec4 WARNING_BUTTON_TEXT_COLOR = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
static const ImVec4 WARNING_BUTTON_SEPARATOR_COLOR = ImVec4(0.6f, 0.4f, 0.05f, 1.0f);
static constexpr float WARNING_BUTTON_ROUNDING = 8.0f;
static constexpr float CLOSE_CROSS_HALF_SIZE_PER_FRAME_HEIGHT = 0.2f;
static constexpr float CLOSE_CROSS_THICKNESS = 1.5f;
static constexpr int NOTICE_TEXT_SIZE = 48;
static constexpr int SUM_TEXT_SIZE = 32;
static constexpr float TOOLTIP_WIDTH_EM = 35.0f;
static const ImVec4 VIEW_BUTTON_DIM_COLOR = ImVec4(0.5f, 0.5f, 0.5f, 0.1f);
static const ImVec4 VIEW_BUTTON_DIM_HOVERED_COLOR = ImVec4(0.6f, 0.6f, 0.6f, 0.2f);
static const char *const VIEW_POPUP_ID = "##ColumnView";
static constexpr float SORT_MARKER_HALF_WIDTH_PER_HEIGHT = 0.3f;
// The default ImGui font has a one pixel stroke at its 13 px size.
static constexpr float SORT_MARKER_STROKE_PER_FONT_SIZE = 1.0f / 13.0f;
static constexpr float SORT_MARKER_GAP_PER_FONT_SIZE = 0.15f;
// The pill of a chip is this much higher and lower than its text, so the frame does not touch the letters: 1 px at the 13 px
// default font. It stays inside the vertical cell padding of the table, so the chips of two rows do not touch.
static constexpr float CELL_CHIP_PADDING_Y_PER_FONT_SIZE = 1.0f / 13.0f;

// Table freeze parameters
static constexpr int FREEZE_COLUMNS = 1; // The first column stays on screen when the table scrolls horizontally
static constexpr int FREEZE_ROWS = 2;    // Freeze header + filter row

// AutoFit parameters
static constexpr int AUTOFIT_QUEUE_VALUE = (1 << 3) - 1; // Fit for 3 frames
static constexpr float FORCE_AUTOFIT_WIDTH = -1.0f;      // Force auto-fit width
static constexpr float MAX_AUTOFIT_WIDTH = 320.0f;       // Maximum width for auto-fit (prevents overly wide columns)

LpProfilerTableColumn::LpProfilerTableColumn(const char *column_name, float width_value, ImGuiTableColumnFlags flags_value) :
  name(column_name ? column_name : "Column"), width(width_value), flags(flags_value)
{}

void LpProfilerTableColumn::setupColumn(ImGuiTableColumnFlags extra_flags) const
{
  ImGui::TableSetupColumn(getName(), flags | extra_flags, width);
}

namespace
{
// A column that shows one of its views; the cell, sort and header are the ones of the shown view, and so is the export unless
// the slot names the views to export.
class LpViewColumn final : public LpProfilerTableColumn
{
public:
  LpViewColumn(eastl::vector<LpColumnViewDesc> column_views, eastl::vector<int> export_views);

  const char *getName() const override { return shownColumn().getName(); }
  void drawCell(const void *item_data) const override { shownColumn().drawCell(item_data); }
  int compareItems(const void *item1, const void *item2) const override { return shownColumn().compareItems(item1, item2); }
  bool hasValue(const void *item_data) const override { return shownColumn().hasValue(item_data); }
  ProfilerString getCellText(const void *item_data, int column_index) const override
  {
    return shownColumn().getCellText(item_data, column_index);
  }
  ProfilerString getFullCellText(const void *item_data) const override { return shownColumn().getFullCellText(item_data); }
  int getExportPartCount() const override;
  ProfilerString getExportText(const void *item_data, int part) const override;
  ProfilerString getExportHeader(int part) const override;
  bool getNumber(const void *item_data, double &value) const override { return shownColumn().getNumber(item_data, value); }
  void drawCellTooltip(const void *item_data) const override { shownColumn().drawCellTooltip(item_data); }

  int getViewCount() const override { return static_cast<int>(views.size()); }
  int getView() const override { return shownView; }
  const char *getViewHeader(int view) const override { return views[view].column->getName(); }
  const char *getViewTitle(int view) const override { return views[view].title.c_str(); }
  const char *getViewShortName(int view) const override { return views[view].shortName.c_str(); }
  const char *getViewGlyph(int view) const override { return views[view].glyph.c_str(); }
  LpSortMarker getViewSortMarker(int view) const override { return views[view].column->getViewSortMarker(0); }
  const char *getViewDescription(int view) const override { return views[view].description.c_str(); }
  ProfilerString getViewPartHeader(int view, int part) const override { return views[view].column->getViewPartHeader(0, part); }

  const eastl::vector<int> &getViewFilterIndices(int view) const { return views[view].filterIndices; }
  // The first view that shows the value of the filter, or -1.
  int findViewWithFilter(int filter_index) const
  {
    for (int view = 0; view < getViewCount(); ++view)
      if (eastl::find(views[view].filterIndices.begin(), views[view].filterIndices.end(), filter_index) !=
          views[view].filterIndices.end())
        return view;
    return -1;
  }

private:
  void setView(int view) override;
  const LpProfilerTableColumn &shownColumn() const { return *views[shownView].column; }
  // The view and its part of an export part of the slot.
  eastl::pair<const LpProfilerTableColumn *, int> findExportPart(int part) const;

  eastl::vector<LpColumnViewDesc> views;
  eastl::vector<int> exportViews;
  int shownView = 0;
};

LpViewColumn::LpViewColumn(eastl::vector<LpColumnViewDesc> column_views, eastl::vector<int> export_views) :
  LpProfilerTableColumn(nullptr), views(eastl::move(column_views)), exportViews(eastl::move(export_views))
{
  G_ASSERT(!views.empty());
  for (int index = 0; index < static_cast<int>(views.size()); ++index)
  {
    const LpColumnViewDesc &view = views[index];
    G_ASSERT(!view.glyph.empty() && !view.title.empty() && view.column);
    const bool hasMarker = view.column->getViewSortMarker(0) != LpSortMarker::NONE;
    G_ASSERT(!view.shortName.empty() || hasMarker);
    G_ASSERT(!hasMarker || view.glyph.length() == 2);
    width = eastl::max(width, view.column->getWidth());
  }
  flags = views[0].column->getFlags();
}

void LpViewColumn::setView(int view) { shownView = view; }

int LpViewColumn::getExportPartCount() const
{
  if (exportViews.empty())
    return shownColumn().getExportPartCount();
  int count = 0;
  for (int view : exportViews)
    count += views[view].column->getExportPartCount();
  return count;
}

eastl::pair<const LpProfilerTableColumn *, int> LpViewColumn::findExportPart(int part) const
{
  if (exportViews.empty())
    return {&shownColumn(), part};
  for (int view : exportViews)
  {
    const LpProfilerTableColumn &column = *views[view].column;
    if (part < column.getExportPartCount())
      return {&column, part};
    part -= column.getExportPartCount();
  }
  G_ASSERT(false);
  return {&shownColumn(), 0};
}

ProfilerString LpViewColumn::getExportText(const void *item_data, int part) const
{
  const auto [column, columnPart] = findExportPart(part);
  return column->getExportText(item_data, columnPart);
}

ProfilerString LpViewColumn::getExportHeader(int part) const
{
  const auto [column, columnPart] = findExportPart(part);
  return column->getExportHeader(columnPart);
}

// The value filters of a view column; "shown" are the filters of the values that the shown view shows.
class LpViewFilter final : public ILpColumnFilter
{
public:
  LpViewFilter(const LpViewColumn &view_column, eastl::vector<eastl::unique_ptr<ILpColumnFilter>> value_filters) :
    column(view_column), filters(eastl::move(value_filters))
  {}

  bool isActive() const override
  {
    return eastl::any_of(filters.begin(), filters.end(), [](const auto &filter) { return filter->isActive(); });
  }
  bool isShownViewActive() const override
  {
    const eastl::vector<int> &shown = shownFilterIndices();
    return eastl::any_of(shown.begin(), shown.end(), [this](int filter) { return filters[filter]->isActive(); });
  }
  int getActiveOtherView() const override
  {
    for (int filter = 0; filter < static_cast<int>(filters.size()); ++filter)
      if (!isShown(filter) && filters[filter]->isActive())
        return column.findViewWithFilter(filter);
    return -1;
  }
  void reset() override
  {
    for (auto &filter : filters)
      filter->reset();
  }
  void resetShownView() override
  {
    for (int filter : shownFilterIndices())
      filters[filter]->reset();
  }
  void resetOtherViews() override
  {
    for (int filter = 0; filter < static_cast<int>(filters.size()); ++filter)
      if (!isShown(filter))
        filters[filter]->reset();
  }
  bool pass(const void *item) const override
  {
    return eastl::all_of(filters.begin(), filters.end(),
      [item](const auto &filter) { return !filter->isActive() || filter->pass(item); });
  }
  bool drawPopup() override
  {
    const eastl::vector<int> &shown = shownFilterIndices();
    if (shown.empty())
    {
      ImGui::TextDisabled("No filter for this view");
      return false;
    }
    bool changed = false;
    for (int part = 0; part < static_cast<int>(shown.size()); ++part)
    {
      if (part > 0)
        ImGui::Separator();
      ImGui::PushID(part);
      changed |= filters[shown[part]]->drawPopup();
      ImGui::PopID();
    }
    return changed;
  }

  // Each value filter gets the data bounds, so it has them when a view shows it.
  void beginRebind() override
  {
    for (auto &filter : filters)
      filter->beginRebind();
  }
  void rebindItem(const void *item) override
  {
    for (auto &filter : filters)
      filter->rebindItem(item);
  }
  void endRebind() override
  {
    for (auto &filter : filters)
      filter->endRebind();
  }

private:
  const eastl::vector<int> &shownFilterIndices() const { return column.getViewFilterIndices(column.getView()); }
  bool isShown(int filter) const
  {
    const eastl::vector<int> &shown = shownFilterIndices();
    return eastl::find(shown.begin(), shown.end(), filter) != shown.end();
  }

  const LpViewColumn &column;
  eastl::vector<eastl::unique_ptr<ILpColumnFilter>> filters;
};
} // namespace

void lp_set_cell_natural_max_x(float natural_max_x) { ImGui::GetCurrentWindow()->DC.CursorMaxPos.x = natural_max_x; }

void lp_draw_cell_chip(const char *text, const LpChipStyle &style)
{
  const float padding = ImGui::GetStyle().FramePadding.x;
  const ImVec2 textSize = ImGui::CalcTextSize(text);
  const ImVec2 cursor = ImGui::GetCursorScreenPos();
  // The text item sits lower than the cursor by the text base offset of the line; the pill is drawn first, under the text.
  const float textTop = cursor.y + ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset;
  const float paddingY = ImGui::GetFontSize() * CELL_CHIP_PADDING_Y_PER_FONT_SIZE;
  const ImVec2 min(cursor.x, textTop - paddingY);
  const ImVec2 max(cursor.x + textSize.x + padding * 2.0f, textTop + textSize.y + paddingY);
  const float rounding = (max.y - min.y) * 0.5f;
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  if (style.background.w > 0.0f)
    drawList->AddRectFilled(min, max, ImGui::GetColorU32(style.background), rounding);
  if (style.borderAlpha > 0.0f)
    drawList->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_Text, style.borderAlpha), rounding);
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding);
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_Text, style.textAlpha));
  ImGui::TextUnformatted(text);
  ImGui::PopStyleColor();
  lp_set_cell_natural_max_x(max.x);
}

void lp_append_name_count(ProfilerString &text, const char *name, int count, int min_shown_count)
{
  text += name;
  if (count >= min_shown_count)
    text.append_sprintf(" (%d)", count);
}

void lp_draw_tooltip_text(const char *text)
{
  ImGui::PushTextWrapPos(ImGui::GetFontSize() * TOOLTIP_WIDTH_EM);
  ImGui::TextUnformatted(text);
  ImGui::PopTextWrapPos();
}

int LpViewColumnBuilder::addValueFilter(eastl::unique_ptr<ILpColumnFilter> filter)
{
  G_ASSERT(filter);
  valueFilters.push_back(eastl::move(filter));
  return static_cast<int>(valueFilters.size()) - 1;
}

int LpViewColumnBuilder::addView(LpColumnViewDesc view)
{
  for (int filterIndex : view.filterIndices)
    G_ASSERT(filterIndex >= 0 && filterIndex < static_cast<int>(valueFilters.size()));
  views.push_back(eastl::move(view));
  return static_cast<int>(views.size()) - 1;
}

eastl::pair<int, int> LpViewColumnBuilder::addViews(eastl::pair<LpColumnViewDesc, LpColumnViewDesc> view_pair)
{
  const int first = addView(eastl::move(view_pair.first));
  const int second = addView(eastl::move(view_pair.second));
  return {first, second};
}

LpColumnSetup LpViewColumnBuilder::build()
{
  for (int view : exportViews)
    G_ASSERT(view >= 0 && view < static_cast<int>(views.size()));
  auto column = eastl::make_unique<LpViewColumn>(eastl::move(views), eastl::move(exportViews));
  for (int filter = 0; filter < static_cast<int>(valueFilters.size()); ++filter)
    G_ASSERT(column->findViewWithFilter(filter) >= 0);
  eastl::unique_ptr<ILpColumnFilter> filter;
  if (!valueFilters.empty())
    filter = eastl::make_unique<LpViewFilter>(*column, eastl::move(valueFilters));
  return {eastl::move(column), eastl::move(filter)};
}


LpProfilerTable::LpProfilerTable(const char *table_name, int column_count, LpTableFlags table_flags) :
  tableName(table_name ? table_name : "Table"), flags(table_flags), columnsCount(column_count)
{
  // Generate full table ID for ImGui
  char buffer[TABLE_ID_BUFFER_SIZE];
  snprintf(buffer, TABLE_ID_BUFFER_SIZE, "##%s_Table", tableName.c_str());
  tableId = buffer;

  columns.resize(columnsCount);
  deferredColumnToggle.resize(columnsCount, false);
  columnVisibility.resize(columnsCount, true);
  columnGroupIds.resize(columnsCount, -1);

  if (flags & LpTableFlags::REORDERABLE)
    initializeColumnOrderTracking();
}

LpProfilerTable::~LpProfilerTable()
{
  searchWidgets.clear();
  columns.clear();
}

void LpProfilerTable::addColumn(ColumnIndex index, LpColumnSetup setup)
{
  G_ASSERT_RETURN(index >= 0 && index < columnsCount && setup.column, );
  columns[index] = eastl::move(setup.column);
  if (setup.filter)
    setColumnFilter(index, eastl::move(setup.filter));
}

int LpProfilerTable::addColumnGroup(const char *name, ColumnIndex first_column, int column_count, int active_from)
{
  G_ASSERT_RETURN(first_column >= 0 && column_count > 0 && first_column + column_count <= columnsCount, -1);
  for (int i = first_column; i < first_column + column_count; ++i)
    G_ASSERT_RETURN(columnGroupIds[i] < 0, -1);
  const int group = static_cast<int>(columnGroups.size());
  columnGroups.push_back({name, first_column, column_count, active_from, active_from + column_count});
  for (int i = first_column; i < first_column + column_count; ++i)
    columnGroupIds[i] = group;
  return group;
}

void LpProfilerTable::setColumnGroupActiveCount(int group, int active_count)
{
  G_ASSERT_RETURN(group >= 0 && group < static_cast<int>(columnGroups.size()), );
  columnGroups[group].activeCount = active_count;
}

bool LpProfilerTable::isColumnAvailable(int index) const
{
  if (index < 0 || index >= columnsCount)
    return false;
  if (columnGroupIds[index] < 0)
    return true;
  const ColumnGroup &group = columnGroups[columnGroupIds[index]];
  return group.activeFrom + (index - group.firstColumn) < group.activeCount;
}

LpProfilerTableColumn *LpProfilerTable::getColumn(int index) const
{
  if (index < 0 || index >= columnsCount)
    return nullptr;

  return columns[index].get();
}

void LpProfilerTable::addPreset(LpColumnPreset preset)
{
  G_ASSERT_RETURN((flags & LpTableFlags::HIDEABLE) && !preset.name.empty(), );
  G_ASSERT_RETURN(!preset.allColumns || (preset.columns.empty() && preset.groups.empty()), );
  eastl::vector<bool> visibleColumns(columnsCount, false);
  for (int i = 0; i < columnsCount; ++i)
    visibleColumns[i] = preset.allColumns || (columns[i] && (columns[i]->getFlags() & ImGuiTableColumnFlags_NoHide));
  for (ColumnIndex column : preset.columns)
  {
    G_ASSERT_RETURN(column >= 0 && column < columnsCount, );
    visibleColumns[column] = true;
  }
  for (int group : preset.groups)
  {
    G_ASSERT_RETURN(group >= 0 && group < static_cast<int>(columnGroups.size()), );
    const ColumnGroup &columnGroup = columnGroups[group];
    for (int i = columnGroup.firstColumn; i < columnGroup.firstColumn + columnGroup.columnCount; ++i)
      visibleColumns[i] = true;
  }
  G_ASSERT_RETURN(preset.sortColumn >= 0 && preset.sortColumn < columnsCount && visibleColumns[preset.sortColumn], );
  eastl::vector<int> columnViews(columnsCount, 0);
  for (const LpColumnViewRef &ref : preset.views)
  {
    G_ASSERT_RETURN((ref.column >= 0) != (ref.group >= 0) && ref.view >= 0, );
    if (ref.column >= 0)
    {
      G_ASSERT_RETURN(ref.column < columnsCount && columnHasView(ref.column, ref.view), );
      columnViews[ref.column] = ref.view;
      continue;
    }
    G_ASSERT_RETURN(ref.group < static_cast<int>(columnGroups.size()), );
    const ColumnGroup &columnGroup = columnGroups[ref.group];
    bool hasColumn = false;
    for (int i = columnGroup.firstColumn; i < columnGroup.firstColumn + columnGroup.columnCount; ++i)
      if (columnHasView(i, ref.view))
      {
        columnViews[i] = ref.view;
        hasColumn = true;
      }
    G_ASSERT_RETURN(hasColumn, );
  }
  presets.push_back({eastl::move(preset), eastl::move(visibleColumns), eastl::move(columnViews)});
}

bool LpProfilerTable::presetMatchesColumns(int index) const
{
  const Preset &preset = presets[index];
  for (int i = 0; i < columnsCount; ++i)
    if (isColumnAvailable(i) &&
        (columnVisibility[i] != preset.visibleColumns[i] || (columns[i] && columns[i]->getView() != preset.columnViews[i])))
      return false;
  return true;
}

int LpProfilerTable::findAllColumnsPreset() const
{
  for (int index = 0; index < static_cast<int>(presets.size()); ++index)
    if (presets[index].desc.allColumns)
      return index;
  return -1;
}

void LpProfilerTable::applyPresetVisibility(int index)
{
  const eastl::vector<bool> &visibleColumns = presets[index].visibleColumns;
  for (int i = 0; i < columnsCount; ++i)
  {
    ImGui::TableSetColumnEnabled(i, visibleColumns[i]);
    columnVisibility[i] = visibleColumns[i];
  }
  activePreset = index;
}

void LpProfilerTable::applyPresetViews(int index)
{
  const eastl::vector<int> &columnViews = presets[index].columnViews;
  for (int i = 0; i < columnsCount; ++i)
    setColumnView(i, columnViews[i]);
}

bool LpProfilerTable::columnHasView(ColumnIndex column, int view) const
{
  return columns[column] && view < columns[column]->getViewCount();
}

void LpProfilerTable::setColumnView(ColumnIndex column, int view)
{
  LpProfilerTableColumn *tableColumn = getColumn(column);
  if (!tableColumn || tableColumn->getView() == view)
    return;
  G_ASSERT_RETURN(view >= 0 && view < tableColumn->getViewCount(), );
  tableColumn->setView(view);
  markRowsDirty();
}

void LpProfilerTable::setGroupView(int group, int view)
{
  const ColumnGroup &columnGroup = columnGroups[group];
  for (int i = columnGroup.firstColumn; i < columnGroup.firstColumn + columnGroup.columnCount; ++i)
    if (columnHasView(i, view))
      setColumnView(i, view);
}

// BeginMenu draws its arrow where MenuItem draws the check mark, so the check mark goes to the shortcut column.
static bool begin_checked_menu(const char *label, bool checked, bool enabled)
{
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  const float fontSize = ImGui::GetFontSize();
  const float checkWidth = IM_TRUNC(fontSize * 1.20f);
  const float menuWidth = window->DC.MenuColumns.DeclColumns(0.0f, 0.0f, checkWidth, 0.0f);
  const float stretchWidth = eastl::max(0.0f, ImGui::GetContentRegionAvail().x - menuWidth);
  const ImVec2 textPos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
  ImDrawList *drawList = window->DrawList;
  const bool isOpen = ImGui::BeginMenu(label, enabled);
  if (checked)
    ImGui::RenderCheckMark(drawList,
      ImVec2(textPos.x + window->DC.MenuColumns.OffsetShortcut + stretchWidth + fontSize * 0.40f, textPos.y + fontSize * 0.067f),
      ImGui::GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled), fontSize * 0.866f);
  return isOpen;
}

bool LpProfilerTable::groupViewDiffers(int group, int view) const
{
  const ColumnGroup &columnGroup = columnGroups[group];
  for (int i = columnGroup.firstColumn; i < columnGroup.firstColumn + columnGroup.columnCount; ++i)
    if (columnHasView(i, view) && columns[i]->getView() != view)
      return true;
  return false;
}

void LpProfilerTable::drawViewMenu()
{
  const LpProfilerTableColumn *column = getColumn(rightClickedColumnIndex);
  if (!column || column->getViewCount() < 2)
    return;
  const int columnIndex = rightClickedColumnIndex;
  const int group = columnGroupIds[columnIndex];
  if (ImGui::BeginMenu("Show as"))
  {
    for (int view = 0; view < column->getViewCount(); ++view)
    {
      ImGui::PushID(view);
      const bool isShown = view == column->getView();
      if (group < 0)
      {
        if (ImGui::MenuItem(column->getViewTitle(view), nullptr, isShown, !isShown))
          setColumnView(columnIndex, view);
      }
      else
      {
        const bool groupChanges = groupViewDiffers(group, view);
        if (begin_checked_menu(column->getViewTitle(view), isShown, !isShown || groupChanges))
        {
          if (ImGui::MenuItem("Apply to this column", nullptr, false, !isShown))
            setColumnView(columnIndex, view);
          ProfilerString groupLabel;
          groupLabel.sprintf("Apply to all %s", columnGroups[group].name.c_str());
          if (ImGui::MenuItem(groupLabel.c_str(), nullptr, false, groupChanges))
            setGroupView(group, view);
          ImGui::EndMenu();
        }
      }
      ImGui::PopID();
    }
    ImGui::EndMenu();
  }
  ImGui::Separator();
}

int LpProfilerTable::getPresetView(ColumnIndex column) const
{
  return activePreset >= 0 ? presets[activePreset].columnViews[column] : -1;
}

// The marker spans the ink of "#" and has its stroke in whole pixels, so it matches the letters at any font and scale.
struct SortMarkerShape
{
  float top = 0.0f; // From the top of the text line
  float bottom = 0.0f;
  float halfWidth = 0.0f; // Of an arrow, without the stroke; an arrow is as high as it is half wide
  float stroke = 1.0f;

  float width() const { return halfWidth * 2.0f + stroke; }
};

static SortMarkerShape sort_marker_shape()
{
  const float fontSize = ImGui::GetFontSize();
  ImFontBaked *baked = ImGui::GetFontBaked();
  const ImFontGlyph *ink = baked->FindGlyph('#');
  const float scale = fontSize / baked->Size;
  SortMarkerShape shape;
  shape.top = floorf(ink->Y0 * scale + 0.5f);
  shape.bottom = floorf(ink->Y1 * scale + 0.5f);
  shape.stroke = eastl::max(1.0f, floorf(fontSize * SORT_MARKER_STROKE_PER_FONT_SIZE + 0.5f));
  shape.halfWidth = eastl::max(2.0f, floorf((shape.bottom - shape.top) * SORT_MARKER_HALF_WIDTH_PER_HEIGHT + 0.5f));
  return shape;
}

static float sort_marker_gap() { return ImGui::GetFontSize() * SORT_MARKER_GAP_PER_FONT_SIZE; }

static ImVec2 calc_view_glyph_size(const char *glyph, LpSortMarker marker)
{
  ImVec2 size = ImGui::CalcTextSize(glyph);
  if (marker != LpSortMarker::NONE)
    size.x += sort_marker_shape().width() + sort_marker_gap();
  return size;
}

// A vertical line with an arrow at each end, as the vertical resize cursor; the default ImGui font has no such glyph.
static void draw_sort_marker(ImDrawList *draw_list, float x, float line_top, ImU32 color)
{
  const SortMarkerShape shape = sort_marker_shape();
  const float left = floorf(x);
  const float top = floorf(line_top) + shape.top;
  const float bottom = floorf(line_top) + shape.bottom;
  const float right = left + shape.width();
  const float centerX = left + shape.width() * 0.5f;
  draw_list->AddRectFilled(ImVec2(left + shape.halfWidth, top + shape.halfWidth),
    ImVec2(right - shape.halfWidth, bottom - shape.halfWidth), color);
  draw_list->AddTriangleFilled(ImVec2(centerX, top), ImVec2(right, top + shape.halfWidth), ImVec2(left, top + shape.halfWidth), color);
  draw_list->AddTriangleFilled(ImVec2(centerX, bottom), ImVec2(left, bottom - shape.halfWidth),
    ImVec2(right, bottom - shape.halfWidth), color);
}

// The marker and the letter of the sorted part in the text color, the other letter dim, as in a composite cell.
static void draw_view_glyph(ImVec2 pos, const char *glyph, LpSortMarker marker)
{
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
  if (marker == LpSortMarker::NONE)
  {
    drawList->AddText(pos, textColor, glyph);
    return;
  }
  const ImU32 dimColor = ImGui::GetColorU32(ImGuiCol_Text, LP_DIM_TEXT_ALPHA);
  const bool sortsLeft = marker == LpSortMarker::BEFORE;
  float x = pos.x;
  if (sortsLeft)
  {
    draw_sort_marker(drawList, x, pos.y, textColor);
    x += sort_marker_shape().width() + sort_marker_gap();
  }
  drawList->AddText(ImVec2(x, pos.y), sortsLeft ? textColor : dimColor, glyph, glyph + 1);
  x += ImGui::CalcTextSize(glyph, glyph + 1).x;
  drawList->AddText(ImVec2(x, pos.y), sortsLeft ? dimColor : textColor, glyph + 1, glyph + 2);
  if (!sortsLeft)
    draw_sort_marker(drawList, x + ImGui::CalcTextSize(glyph + 1, glyph + 2).x + sort_marker_gap(), pos.y, textColor);
}

// The suffix is drawn after the glyph in the text color; it can be null.
static bool view_glyph_button(const char *id, const char *glyph, LpSortMarker marker, const char *suffix)
{
  const ImVec2 glyphSize = calc_view_glyph_size(glyph, marker);
  const float suffixWidth = suffix ? ImGui::CalcTextSize(suffix).x : 0.0f;
  const ImVec2 padding = ImGui::GetStyle().FramePadding;
  const bool clicked = ImGui::Button(id, ImVec2(glyphSize.x + suffixWidth + padding.x * 2.0f, ImGui::GetFrameHeight()));
  const ImVec2 pos(ImGui::GetItemRectMin().x + padding.x, ImGui::GetItemRectMin().y + padding.y);
  draw_view_glyph(pos, glyph, marker);
  if (suffix)
    ImGui::GetWindowDrawList()->AddText(ImVec2(pos.x + glyphSize.x, pos.y), ImGui::GetColorU32(ImGuiCol_Text), suffix);
  return clicked;
}

// The bracket of a header names the right value of a composite cell: "(/X)" a share of X, "(vs X)" the value of X, "(+M)"
// another metric of the row.
static void append_header_notation(ProfilerString &text, const char *header)
{
  const char *open = strchr(header, '(');
  const char *close = open ? strchr(open, ')') : nullptr;
  if (!close)
    return;
  const char *body = open + 1;
  const int notationLength = static_cast<int>(close + 1 - open);
  if (*body == '/')
    text.append_sprintf("\n%.*s: the right value is a share of %.*s, in %%.", notationLength, open, static_cast<int>(close - body - 1),
      body + 1);
  else if (strncmp(body, "vs ", 3) == 0)
    text.append_sprintf("\n%.*s: the right value is the value of %.*s.", notationLength, open, static_cast<int>(close - body - 3),
      body + 3);
  else if (*body == '+')
    text.append_sprintf("\n%.*s: the right value is %.*s, another metric of the row.", notationLength, open,
      static_cast<int>(close - body - 1), body + 1);
}

void LpProfilerTable::drawHeaderTooltip(ColumnIndex column) const
{
  const LpProfilerTableColumn &tableColumn = *columns[column];
  const int view = tableColumn.getView();
  ProfilerString text(tableColumn.getViewTitle(view));
  if (const char *description = tableColumn.getViewDescription(view); *description)
    text.append_sprintf("\n%s", description);
  append_header_notation(text, tableColumn.getViewHeader(view));
  if (const LpSortMarker marker = tableColumn.getViewSortMarker(view); marker != LpSortMarker::NONE)
    text.append_sprintf("\nThe sort reads the bright value: %s.",
      tableColumn.getViewPartHeader(view, marker == LpSortMarker::BEFORE ? 0 : 1).c_str());
  if (!ImGui::BeginTooltip())
    return;
  lp_draw_tooltip_text(text.c_str());
  if (tableColumn.getViewCount() > 1)
    ImGui::TextDisabled("The view button under the header changes the view.");
  ImGui::EndTooltip();
}

void LpProfilerTable::drawViewButton(ColumnIndex column)
{
  const LpProfilerTableColumn &tableColumn = *columns[column];
  const int view = tableColumn.getView();
  const int presetView = getPresetView(column);
  const bool isDimmed = presetView < 0 || view == presetView;
  if (isDimmed)
  {
    ImGui::PushStyleColor(ImGuiCol_Button, VIEW_BUTTON_DIM_COLOR);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, VIEW_BUTTON_DIM_HOVERED_COLOR);
  }
  ImGui::PushID(column);
  const bool clicked = view_glyph_button("##view", tableColumn.getViewGlyph(view), tableColumn.getViewSortMarker(view), nullptr);
  ImGui::PopID();
  if (isDimmed)
    ImGui::PopStyleColor(2);
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Shown as: %s%s\nClick to change the view.", tableColumn.getViewTitle(view),
      isDimmed ? "" : " (not the view of the preset)");
  if (clicked)
  {
    viewPopupColumn = column;
    viewPopupAnchor = ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y);
    ImGui::PushID(tableId.c_str());
    ImGui::OpenPopup(VIEW_POPUP_ID);
    ImGui::PopID();
  }
}

static bool draw_segment(const char *label, bool is_active)
{
  if (is_active)
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
  const bool clicked = ImGui::Button(label);
  if (is_active)
    ImGui::PopStyleColor();
  return clicked;
}

void LpProfilerTable::drawViewPopup()
{
  if (ImGui::IsPopupOpen(VIEW_POPUP_ID))
    ImGui::SetNextWindowPos(viewPopupAnchor, ImGuiCond_Appearing);
  if (!ImGui::BeginPopup(VIEW_POPUP_ID))
    return;
  const LpProfilerTableColumn *column = getColumn(viewPopupColumn);
  if (!column)
  {
    ImGui::EndPopup();
    return;
  }

  const int group = columnGroupIds[viewPopupColumn];
  const bool toGroup = group >= 0 && viewApplyToGroup;
  if (group >= 0)
  {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Apply to:");
    ImGui::SameLine();
    if (draw_segment("This column", !viewApplyToGroup))
      viewApplyToGroup = false;
    ImGui::SameLine(0.0f, 0.0f);
    ProfilerString groupLabel;
    groupLabel.sprintf("All %s", columnGroups[group].name.c_str());
    if (draw_segment(groupLabel.c_str(), viewApplyToGroup))
      viewApplyToGroup = true;
  }

  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("Show as:");
  ImGui::SameLine();
  for (int view = 0; view < column->getViewCount(); ++view)
  {
    if (view > 0)
      ImGui::SameLine(0.0f, 0.0f);
    const bool isShown = view == column->getView();
    if (isShown)
      ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    ProfilerString id, suffix;
    id.sprintf("##view%d", view);
    if (const char *shortName = column->getViewShortName(view); *shortName)
      suffix.sprintf(" %s", shortName);
    const bool clicked = view_glyph_button(id.c_str(), column->getViewGlyph(view), column->getViewSortMarker(view),
      suffix.empty() ? nullptr : suffix.c_str());
    if (isShown)
      ImGui::PopStyleColor();
    if (clicked)
    {
      if (toGroup)
        setGroupView(group, view);
      else
        setColumnView(viewPopupColumn, view);
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", column->getViewTitle(view));
  }
  ImGui::TextDisabled("%s", column->getViewTitle(column->getView()));

  if (isFilterActive(viewPopupColumn))
    ImGui::TextDisabled("A filter belongs to its value: each view that shows the value uses it.");
  ImGui::EndPopup();
}

ProfilerString LpProfilerTable::getPresetLabel(int index) const
{
  ProfilerString label(presets[index].desc.name);
  if (isActivePresetModified(index))
    label += "*";
  return label;
}

ProfilerString LpProfilerTable::getPresetTooltip(int index) const
{
  const LpColumnPreset &preset = presets[index].desc;
  ProfilerString text;
  if (!preset.tooltip.empty())
    text.append_sprintf("%s\n\n", preset.tooltip.c_str());
  text += preset.allColumns ? "Columns: all" : "Columns:";
  const eastl::vector<int> &columnViews = presets[index].columnViews;
  for (int i = 0; i < columnsCount && !preset.allColumns; ++i)
    if (columns[i] && (columns[i]->getFlags() & ImGuiTableColumnFlags_NoHide))
      text.append_sprintf("\n  %s", columns[i]->getViewHeader(columnViews[i]));
  for (ColumnIndex column : preset.columns)
    if (columns[column])
      text.append_sprintf("\n  %s", columns[column]->getViewHeader(columnViews[column]));
  eastl::vector<int> groups(preset.groups.begin(), preset.groups.end());
  for (const LpColumnViewRef &ref : preset.views)
    if (ref.group >= 0 && eastl::find(groups.begin(), groups.end(), ref.group) == groups.end())
      groups.push_back(ref.group);
  for (int group : groups)
  {
    const ColumnGroup &columnGroup = columnGroups[group];
    text.append_sprintf("\n  %s", columnGroup.name.c_str());
    const char *separator = ": ";
    for (const LpColumnViewRef &ref : preset.views)
      if (ref.group == group)
        for (int i = columnGroup.firstColumn; i < columnGroup.firstColumn + columnGroup.columnCount; ++i)
          if (columnHasView(i, ref.view))
          {
            text.append_sprintf("%s%s", separator, columns[i]->getViewTitle(ref.view));
            separator = "; ";
            break;
          }
  }
  if (const LpProfilerTableColumn *sortColumn = getColumn(preset.sortColumn))
    text.append_sprintf("\n\nSort: %s, %s", sortColumn->getViewHeader(columnViews[preset.sortColumn]),
      preset.sortDirection == ImGuiSortDirection_Descending ? "descending" : "ascending");
  if (isActivePresetModified(index))
    text += "\n\n* The visible columns or their views differ from the preset. Click to apply the preset again.";
  return text;
}

void LpProfilerTable::drawPresetBar()
{
  ImGui::PushID(tableId.c_str());
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("Preset:");
  ImGui::SameLine();

  const ImGuiStyle &style = ImGui::GetStyle();
  float buttonsWidth = 0.0f;
  for (int index = 0; index < static_cast<int>(presets.size()); ++index)
    buttonsWidth += ImGui::CalcTextSize(getPresetLabel(index).c_str()).x + style.FramePadding.x * 2.0f + style.ItemSpacing.x;

  eastl::vector<int> hiddenFilterColumns;
  for (int column = 0; column < columnsCount; ++column)
    if (isColumnVisible(column) ? isFilterActive(column) && columnFilters[column]->isOtherViewActive() : isFilterActive(column))
      hiddenFilterColumns.push_back(column);
  const bool hasNotice = !hiddenFilterColumns.empty();
  char notice[NOTICE_TEXT_SIZE] = "";
  if (hasNotice)
    snprintf(notice, sizeof(notice), "Hidden filters: %d", static_cast<int>(hiddenFilterColumns.size()));
  const float noticeBodyWidth = hasNotice ? ImGui::CalcTextSize(notice).x + style.FramePadding.x * 2.0f : 0.0f;
  const float noticeButtonWidth = hasNotice ? noticeBodyWidth + ImGui::GetFrameHeight() : 0.0f;
  const float noticeWidth = hasNotice ? noticeButtonWidth + style.ItemSpacing.x : 0.0f;

  const float rowWidth = ImGui::GetContentRegionAvail().x;
  bool noticeInRow = true;
  if (buttonsWidth <= rowWidth)
  {
    noticeInRow = buttonsWidth + noticeButtonWidth <= rowWidth;
    for (int index = 0; index < static_cast<int>(presets.size()); ++index)
    {
      if (index > 0)
        ImGui::SameLine();
      const bool isActive = index == activePreset;
      if (isActive)
        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
      ImGui::PushID(index);
      if (ImGui::Button(getPresetLabel(index).c_str()))
        deferredPreset = index;
      ImGui::PopID();
      if (isActive)
        ImGui::PopStyleColor();
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", getPresetTooltip(index).c_str());
    }
  }
  else
  {
    const ProfilerString activeLabel = activePreset >= 0 ? getPresetLabel(activePreset) : ProfilerString();
    const float minComboWidth = ImGui::CalcTextSize(activeLabel.c_str()).x + style.FramePadding.x * 2.0f + ImGui::GetFrameHeight();
    noticeInRow = minComboWidth + noticeWidth <= rowWidth;
    ImGui::SetNextItemWidth(hasNotice && noticeInRow ? -noticeWidth : -FLT_MIN);
    if (ImGui::BeginCombo("##Preset", activeLabel.c_str()))
    {
      for (int index = 0; index < static_cast<int>(presets.size()); ++index)
      {
        ImGui::PushID(index);
        if (ImGui::Selectable(getPresetLabel(index).c_str(), index == activePreset))
          deferredPreset = index;
        ImGui::PopID();
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("%s", getPresetTooltip(index).c_str());
      }
      ImGui::EndCombo();
    }
  }

  if (hasNotice)
  {
    if (noticeInRow)
      ImGui::SameLine();
    const float offset = ImGui::GetContentRegionAvail().x - noticeButtonWidth;
    if (offset > 0.0f)
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
    drawHiddenFiltersButton(notice, noticeBodyWidth, hiddenFilterColumns);
  }
  ImGui::PopID();
}

void LpProfilerTable::drawHiddenFiltersButton(const char *label, float body_width, const eastl::vector<int> &hidden_filter_columns)
{
  const ImGuiStyle &style = ImGui::GetStyle();
  const float height = ImGui::GetFrameHeight();
  const ImVec2 bodySize(body_width, height);
  const ImVec2 bodyMin = ImGui::GetCursorScreenPos();
  const bool showClicked = ImGui::InvisibleButton("##ShowHiddenFilterColumns", bodySize);
  const bool bodyHovered = ImGui::IsItemHovered();

  ImGui::SameLine(0.0f, 0.0f);
  const ImVec2 closeMin = ImGui::GetCursorScreenPos();
  const bool resetClicked = ImGui::InvisibleButton("##ResetHiddenFilters", ImVec2(height, height));
  const bool closeHovered = ImGui::IsItemHovered();

  if (bodyHovered || closeHovered)
  {
    ProfilerString tooltip("The rows are filtered by these hidden columns and other views:");
    for (int column : hidden_filter_columns)
      tooltip.append_sprintf(!isColumnAvailable(column) ? "\n  %s (no data)"
                             : !isColumnVisible(column) ? "\n  %s"
                             : columnFilters[column]->isShownViewActive()
                               ? "\n  %s (another view; the click keeps the filtered shown view)"
                               : "\n  %s (another view)",
        columns[column]->getName());
    tooltip += "\n\nClick to show these columns with data in the filtered views, or click x to reset all these filters.";
    ImGui::SetTooltip("%s", tooltip.c_str());
  }

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  const ImVec2 bodyMax(bodyMin.x + bodySize.x, bodyMin.y + height);
  const ImVec2 closeMax(closeMin.x + height, closeMin.y + height);
  const ImU32 textColor = ImGui::GetColorU32(WARNING_BUTTON_TEXT_COLOR);
  drawList->AddRectFilled(bodyMin, bodyMax, ImGui::GetColorU32(bodyHovered ? WARNING_BUTTON_HOVERED_COLOR : WARNING_BUTTON_COLOR),
    WARNING_BUTTON_ROUNDING, ImDrawFlags_RoundCornersLeft);
  drawList->AddRectFilled(closeMin, closeMax, ImGui::GetColorU32(closeHovered ? WARNING_BUTTON_HOVERED_COLOR : WARNING_BUTTON_COLOR),
    WARNING_BUTTON_ROUNDING, ImDrawFlags_RoundCornersRight);
  drawList->AddLine(ImVec2(closeMin.x, closeMin.y + 1.0f), ImVec2(closeMin.x, closeMax.y - 1.0f),
    ImGui::GetColorU32(WARNING_BUTTON_SEPARATOR_COLOR));
  drawList->AddText(ImVec2(bodyMin.x + style.FramePadding.x, bodyMin.y + style.FramePadding.y), textColor, label);
  const ImVec2 center((closeMin.x + closeMax.x) * 0.5f, (closeMin.y + closeMax.y) * 0.5f);
  const float half = height * CLOSE_CROSS_HALF_SIZE_PER_FRAME_HEIGHT;
  drawList->AddLine(ImVec2(center.x - half, center.y - half), ImVec2(center.x + half, center.y + half), textColor,
    CLOSE_CROSS_THICKNESS);
  drawList->AddLine(ImVec2(center.x - half, center.y + half), ImVec2(center.x + half, center.y - half), textColor,
    CLOSE_CROSS_THICKNESS);

  if (showClicked)
    for (int column : hidden_filter_columns)
    {
      if (!isColumnAvailable(column))
        continue;
      if (!isColumnVisible(column))
        deferredColumnToggle[column] = true;
      // Switching away from a shown view with its own active filter only moves the notice to that filter.
      const int view = columnFilters[column]->getActiveOtherView();
      if (view >= 0 && !columnFilters[column]->isShownViewActive())
        setColumnView(column, view);
    }
  if (resetClicked)
    for (int column : hidden_filter_columns)
    {
      if (isColumnVisible(column))
      {
        columnFilters[column]->resetOtherViews();
        markRowsDirty();
      }
      else
        clearColumnFilter(column);
    }
}

void LpProfilerTable::drawPresetMenu()
{
  if (!ImGui::BeginMenu("Preset"))
    return;
  for (int index = 0; index < static_cast<int>(presets.size()); ++index)
  {
    ImGui::PushID(index);
    if (ImGui::MenuItem(getPresetLabel(index).c_str(), nullptr, index == activePreset))
      deferredPreset = index;
    ImGui::PopID();
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s", getPresetTooltip(index).c_str());
  }
  ImGui::EndMenu();
}

bool LpProfilerTable::draw()
{
  ImGuiTableFlags imguiFlags = 0;

  if (flags & LpTableFlags::SORTABLE)
    imguiFlags |= ImGuiTableFlags_Sortable;
  if (flags & LpTableFlags::RESIZABLE)
    imguiFlags |= ImGuiTableFlags_Resizable;
  if (flags & LpTableFlags::SCROLLABLE)
    imguiFlags |= ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
  if (flags & LpTableFlags::BORDERS)
    imguiFlags |= ImGuiTableFlags_Borders;
  if (flags & LpTableFlags::ROW_BG)
    imguiFlags |= ImGuiTableFlags_RowBg;
  if (flags & LpTableFlags::SORT_MULTI)
    imguiFlags |= ImGuiTableFlags_SortMulti;
  if (flags & LpTableFlags::SIZING_FIXED_FIT)
    imguiFlags |= ImGuiTableFlags_SizingFixedFit;
  if (flags & LpTableFlags::SIZING_STRETCH_PROP)
    imguiFlags |= ImGuiTableFlags_SizingStretchProp;
  if (flags & LpTableFlags::REORDERABLE)
    imguiFlags |= ImGuiTableFlags_Reorderable;
  if (flags & LpTableFlags::HIDEABLE)
    imguiFlags |= ImGuiTableFlags_Hideable;

  if (!presets.empty())
  {
    if (activePreset < 0 && deferredPreset < 0)
      deferredPreset = 0;
    drawPresetBar();
  }

  // The table is as wide as its columns, but not wider than the window: wider columns scroll. The width of the columns is
  // known from the previous frame.
  ImVec2 outerSize(0.0f, 0.0f);
  float innerWidth = 0.0f;
  if (imguiFlags & ImGuiTableFlags_ScrollX)
    if (const ImGuiTable *previousTable = ImGui::TableFindByID(ImGui::GetID(tableId.c_str())))
    {
      // ImGui gives the columns truncated widths and truncates the outer size; a fractional width would leave an empty band
      // after the last column or show a horizontal scrollbar.
      innerWidth = ImTrunc(previousTable->ColumnsGivenWidth);
      const float scrollbarWidth = previousTable->HasScrollbarYCurr ? ImGui::GetStyle().ScrollbarSize : 0.0f;
      outerSize.x = eastl::min(ImGui::GetContentRegionAvail().x, ImCeil(innerWidth + scrollbarWidth));
    }

  bool isTableVisible = ImGui::BeginTable(tableId.c_str(), columnsCount, imguiFlags, outerSize, innerWidth);
  if (!isTableVisible)
    return false;

  headerRightClickHandled = false;

  if (flags & LpTableFlags::FREEZE_HEADER)
    ImGui::TableSetupScrollFreeze(FREEZE_COLUMNS, FREEZE_ROWS);

  setupColumns();

  if ((flags & LpTableFlags::REORDERABLE) && isInitColumnOrderUpdate)
  {
    readColumnOrderFromImGui();

    if (isResettingColumnOrder)
    {
      if (ImGuiTable *table = ImGui::GetCurrentTable())
        table->IsResetDisplayOrderRequest = true;

      isResettingColumnOrder = false;
    }
    else
      columnOrderWasModified = compareColumnOrders();

    isInitColumnOrderUpdate = false;
  }

  if ((flags & LpTableFlags::REORDERABLE) && forceColumnOrderUpdate)
  {
    readColumnOrderFromImGui();
    columnOrderWasModified = compareColumnOrders();
    forceColumnOrderUpdate = false;
  }

  ensureAtLeastOneColumnVisible();
  handleDeferredColumnSizing();
  applyDeferredColumnWidthConstraints();
  drawHeaderRow();

  if (flags & LpTableFlags::REORDERABLE)
    updateCurrentColumnOrder();

  if (ImGui::BeginPopup("TableHeaderContextMenu"))
  {
    handleHeaderContextMenu();
    ImGui::EndPopup();
  }

  // Process any deferred column actions INSIDE table context
  processDeferredColumnActions();

  // Ensure column order is synchronized before drawing filter row
  if (flags & LpTableFlags::REORDERABLE)
    updateCurrentColumnOrder();

  // Second row: filter indicators (frozen with header)
  ImGui::TableNextRow();

  for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
  {
    ImGui::TableSetColumnIndex(columnIndex);
    ColumnIndex columnType = columnIndex;
    // The search field and the right-aligned indicator stretch to the column; its auto-fit reads only the buttons. The
    // search field takes a share of the column width, so it would make the auto-fit narrow the column at each step.
    float indicatorStartX = ImGui::GetCursorScreenPos().x;

    if (columnIndex < static_cast<int>(columnSearchWidgets.size()))
    {
      if (auto *sw = columnSearchWidgets[columnIndex]; sw)
      {
        sw->draw();
        ImGui::SameLine();
      }
    }

    const float cellStartX = ImGui::GetCursorPosX();
    if (columns[columnIndex] && columns[columnIndex]->getViewCount() > 1)
    {
      drawViewButton(columnIndex);
      ImGui::SameLine();
      indicatorStartX = ImGui::GetCursorScreenPos().x;
      ImGui::SetCursorPosX(cellStartX);
    }

    bool isActive = isShownViewFilterActive(columnType);
    char idBuffer[FILTER_ID_BUFFER_SIZE];
    snprintf(idBuffer, FILTER_ID_BUFFER_SIZE, "%s_filt%d", tableName.c_str(), columnIndex);
    LpFilterIndicator indicator(idBuffer);
    indicator.Draw(isActive);
    lp_set_cell_natural_max_x(indicatorStartX + ImGui::GetItemRectSize().x);
    if (indicator.IsClicked())
    {
      if (columnIndex < static_cast<int>(columnFilters.size()) && columnFilters[columnIndex])
      {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        ImVec2 anchor(min.x, max.y);
        ImGui::PushID(tableId.c_str());
        ImGui::OpenPopup(popupIds[columnIndex].c_str());
        ImGui::PopID();
        popupAnchorPositions[columnIndex] = anchor;
      }
    }
  }

  handleSorting();

  drawContent();

  ImGui::PushID(tableId.c_str());
  for (int col = 0; col < columnsCount; ++col)
  {
    if (!columnFilters[col])
      continue;
    const char *pid = popupIds[col].c_str();
    if (popupAnchorPositions[col].x != 0.f || popupAnchorPositions[col].y != 0.f)
    {
      if (ImGui::IsPopupOpen(pid))
        ImGui::SetNextWindowPos(popupAnchorPositions[col], ImGuiCond_Appearing, ImVec2(0.0f, 0.0f));
    }
    if (ImGui::BeginPopup(pid))
    {
      if (columnFilters[col]->drawPopup())
        markRowsDirty();
      ImGui::EndPopup();
    }
  }
  drawViewPopup();
  ImGui::PopID();

  for (int col = 0; col < columnsCount; ++col)
    popupAnchorPositions[col] = ImVec2(0.f, 0.f);

  ImGui::EndTable();

  // Suppress default table context menu if header right-click was handled
  if (headerRightClickHandled)
  {
    if (ImGui::BeginPopupContextItem("##TableContext", ImGuiPopupFlags_MouseButtonRight))
    {
      ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }
  }

  return true;
}

void LpProfilerTable::ensureFilterStorageSized()
{
  if (popupIds.size() == static_cast<size_t>(columnsCount))
    return;
  popupIds.resize(columnsCount);
  columnSearchWidgets.resize(columnsCount);
  popupAnchorPositions.resize(columnsCount);
  columnFilters.resize(columnsCount);
  searchIds.resize(columnsCount);
  for (int i = 0; i < columnsCount; ++i)
  {
    popupIds[i].clear();
    columnSearchWidgets[i] = nullptr;
    popupAnchorPositions[i] = ImVec2(0.f, 0.f);
  }
}

void LpProfilerTable::setColumnFilter(ColumnIndex column, eastl::unique_ptr<ILpColumnFilter> filter)
{
  ensureFilterStorageSized();
  popupIds[column].sprintf("##ColumnFilter%d", column);
  if (const char *hint = filter->getSearchHint())
  {
    searchIds[column].sprintf("##ColumnSearch%d", column);
    ILpColumnFilter *searchTarget = filter.get();
    columnSearchWidgets[column] = registerSearchWidget(searchIds[column].c_str(), hint, [this, searchTarget](const char *text) {
      searchTarget->setSearchText(text);
      markRowsDirty();
    });
  }
  columnFilters[column] = eastl::move(filter);
}

bool LpProfilerTable::isItemVisible(int item_index) const
{
  const void *item = getItem(item_index);
  for (const ILpColumnFilter *filter : activeColumnFilters)
    if (!filter->pass(item))
      return false;
  return true;
}

bool LpProfilerTable::isFilterActive(ColumnIndex column_index) const
{
  return column_index >= 0 && column_index < static_cast<ColumnIndex>(columnFilters.size()) && columnFilters[column_index] &&
         columnFilters[column_index]->isActive();
}

bool LpProfilerTable::isShownViewFilterActive(ColumnIndex column_index) const
{
  return isFilterActive(column_index) && columnFilters[column_index]->isShownViewActive();
}

void LpProfilerTable::clearAllFilters()
{
  for (int column = 0; column < static_cast<int>(columnFilters.size()); ++column)
    clearColumnFilter(column);
}

void LpProfilerTable::clearColumnFilter(ColumnIndex column)
{
  if (column < 0 || column >= static_cast<ColumnIndex>(columnFilters.size()) || !columnFilters[column])
    return;
  columnFilters[column]->reset();
  if (auto *searchWidget = getSearchWidget(column))
    searchWidget->clear();
  markRowsDirty();
}

void LpProfilerTable::rebindColumnFilters()
{
  const int itemCount = getItemCount();
  for (auto &filter : columnFilters)
    if (filter)
      filter->beginRebind();
  for (int i = 0; i < itemCount; ++i)
  {
    const void *item = getItem(i);
    for (auto &filter : columnFilters)
      if (filter)
        filter->rebindItem(item);
  }
  for (auto &filter : columnFilters)
    if (filter)
      filter->endRebind();
}

LpSearchWidget *LpProfilerTable::getSearchWidget(ColumnIndex column_index) const
{
  if (column_index < 0 || column_index >= static_cast<ColumnIndex>(columnSearchWidgets.size()))
    return nullptr;
  return columnSearchWidgets[column_index];
}

void LpProfilerTable::setupColumns()
{
  for (int i = 0; i < columnsCount; i++)
  {
    if (columns[i])
      columns[i]->setupColumn(isColumnAvailable(i) ? ImGuiTableColumnFlags_None : ImGuiTableColumnFlags_Disabled);
    else
    {
      char currentColumnName[COLUMN_NAME_BUFFER_SIZE];
      snprintf(currentColumnName, COLUMN_NAME_BUFFER_SIZE, "Column%d", i);
      ImGui::TableSetupColumn(currentColumnName, ImGuiTableColumnFlags_None);
    }
  }
}

void LpProfilerTable::handleSorting()
{
  if (!(flags & LpTableFlags::SORTABLE))
    return;

  ImGuiTableSortSpecs *specs = ImGui::TableGetSortSpecs();
  if (!specs || !specs->SpecsDirty)
    return;

  activeSortSpecs.assign(specs->Specs, specs->Specs + specs->SpecsCount);
  sortDirty = true;
  specs->SpecsDirty = false;
}

LpColumnSum LpProfilerTable::sumColumn(ColumnIndex column)
{
  LpColumnSum sum;
  G_ASSERT_RETURN(column >= 0 && column < columnsCount && columns[column], sum);
  refreshRows();
  const LpProfilerTableColumn &numbers = *columns[column];
  double value = 0.0;
  for (int itemIndex : displayOrder)
    if (numbers.getNumber(getItem(itemIndex), value))
      sum.visible += value;
  const int itemCount = getItemCount();
  for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex)
    if (numbers.getNumber(getItem(itemIndex), value))
      sum.total += value;
  return sum;
}

void lp_format_count_sum(char *text, int size, double value) { snprintf(text, size, "%.0f", value); }

void lp_format_memory_sum_mb(char *text, int size, double size_mb)
{
  if (size_mb < 1.0)
    snprintf(text, size, "%.2f KB", size_mb * 1024.0);
  else if (size_mb < 1024.0)
    snprintf(text, size, "%.2f MB", size_mb);
  else
    snprintf(text, size, "%.2f GB", size_mb / 1024.0);
}

void lp_draw_row_count(const char *label, int visible, int total) { ImGui::Text("%s: %d / %d", label, visible, total); }

void lp_draw_column_sum(const char *label, const LpColumnSum &sum, LpSumFormat format)
{
  char visibleText[SUM_TEXT_SIZE];
  char totalText[SUM_TEXT_SIZE];
  format(visibleText, sizeof(visibleText), sum.visible);
  format(totalText, sizeof(totalText), sum.total);
  ImGui::Text("%s: %s / %s", label, visibleText, totalText);
  if (sum.total > 0.0)
  {
    ImGui::SameLine();
    ImGui::TextDisabled("(%.2f%%)", 100.0 * sum.visible / sum.total);
  }
}

bool LpProfilerTable::isSortedBy(int column_index) const
{
  for (const ImGuiTableColumnSortSpecs &spec : activeSortSpecs)
    if (spec.ColumnIndex == column_index)
      return true;
  return false;
}

void LpProfilerTable::drawContent()
{
  refreshRows();
  drawRows();
}

void LpProfilerTable::refreshRows()
{
  if (pollItemsChanged())
  {
    rebuildItems();
    rebindColumnFilters();
    rowsDirty = true;
  }
  const bool rowsRebuilt = rowsDirty;
  if (rowsDirty)
  {
    activeColumnFilters.clear();
    for (const auto &filter : columnFilters)
      if (filter && filter->isActive())
        activeColumnFilters.push_back(filter.get());
    displayOrder.clear();
    for (int i = 0, n = getItemCount(); i < n; ++i)
      if (isItemVisible(i))
        displayOrder.push_back(i);
    rowsDirty = false;
  }
  if (rowsRebuilt || sortDirty)
  {
    sortRows(rowsRebuilt);
    sortDirty = false;
  }
}

void LpProfilerTable::sortRows(bool rows_rebuilt)
{
  if (activeSortSpecs.empty())
  {
    // A rebuild already gives the item order; only an order from an earlier sort needs it back.
    if (!rows_rebuilt)
      eastl::sort(displayOrder.begin(), displayOrder.end());
    return;
  }
  auto less = [this](int lhs, int rhs) {
    const void *a = getItem(lhs);
    const void *b = getItem(rhs);
    for (const ImGuiTableColumnSortSpecs &spec : activeSortSpecs)
    {
      const LpProfilerTableColumn *column = getColumn(spec.ColumnIndex);
      if (!column)
        continue;
      const bool hasA = column->hasValue(a);
      if (const bool hasB = column->hasValue(b); hasA != hasB)
        return hasA;
      if (!hasA)
        continue;
      if (int delta = column->compareItems(a, b); delta != 0)
        return spec.SortDirection == ImGuiSortDirection_Ascending ? delta < 0 : delta > 0;
    }
    if (int delta = compareItemsTieBreak(a, b); delta != 0)
      return delta < 0;
    return lhs < rhs;
  };
  eastl::sort(displayOrder.begin(), displayOrder.end(), less);
}

void LpProfilerTable::drawRows()
{
  if (displayOrder.empty())
  {
    if (const char *text = getNoRowsText())
    {
      ImGui::TableNextRow();
      if (ImGui::TableSetColumnIndex(0))
        ImGui::TextColored(NO_ROWS_TEXT_COLOR, "%s", text);
    }
    return;
  }

  ImGuiListClipper clipper;
  clipper.Begin(static_cast<int>(displayOrder.size()));
  while (clipper.Step())
  {
    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
    {
      void *item = getItemForDraw(displayOrder[row]);
      ImGui::TableNextRow();
      for (int col = 0; col < columnsCount; ++col)
      {
        if (!ImGui::TableSetColumnIndex(col))
          continue;
        LpProfilerTableColumn *column = getColumn(col);
        if (!column)
          continue;
        ImGui::PushID(row * columnsCount + col);
        column->drawCell(item);
        if (ImGui::TableGetHoveredColumn() == col && ImGui::TableGetHoveredRow() == ImGui::TableGetRowIndex())
          column->drawCellTooltip(item);
        if (col == 0 && ImGui::IsItemClicked())
          onItemClicked(item);
        drawRowContextMenu(row, col, item);
        ImGui::PopID();
      }
    }
  }
}

void LpProfilerTable::drawRowContextMenu(int row_index, int column_index, const void *item_data)
{
  if (!copyManager || !item_data)
    return;

  char contextMenuId[64];
  snprintf(contextMenuId, sizeof(contextMenuId), "table_context_%d_%d", row_index, column_index);

  if (ImGui::BeginPopupContextItem(contextMenuId))
  {
    // Create "Copy to clipboard" submenu
    if (ImGui::BeginMenu("Copy to clipboard"))
    {
      bool hasCopyOptions = false;

      drawCellCopyOptions(item_data, hasCopyOptions);
      drawRowCopyOption(item_data, hasCopyOptions);
      drawCustomCopyOptions(item_data);

      ImGui::EndMenu();
    }

    // Here can be added other non-copy related menu items in the future

    ImGui::EndPopup();
  }
}

ProfilerString LpProfilerTable::getRowText(const void *item_data) const
{
  if (!item_data)
    return ProfilerString{};

  ProfilerString result;

  for (int i = 0; i < columnsCount; ++i)
  {
    if (columns[i])
    {
      ProfilerString cellText = columns[i]->getFullCellText(item_data);
      if (!cellText.empty())
      {
        ProfilerString columnName = getColumnName(i);

        if (!result.empty())
          result += "\n";

        result += columnName + ": " + cellText;
      }
    }
  }

  return result;
}

ProfilerString LpProfilerTable::getColumnName(int column_index) const
{
  if (column_index < 0 || column_index >= columnsCount || !columns[column_index])
    return "Unknown";
  return ProfilerString(columns[column_index]->getName());
}

void LpProfilerTable::drawCellCopyMenuItem(const void *item_data, int col_idx, bool &hasCopyOptions)
{
  if (!columns[col_idx])
    return;

  if (ProfilerString cellText = columns[col_idx]->getCellText(item_data, col_idx); !cellText.empty())
  {
    ProfilerString columnName = getColumnName(col_idx);
    if (ImGui::MenuItem(columnName.c_str()))
    {
      CopyRequest request(CopyType::CONTEXT_CELL);
      request.customData = cellText;
      copyManager->executeContextCopy(request);
    }
    hasCopyOptions = true;
  }
}

void LpProfilerTable::drawCellCopyOptions(const void *item_data, bool &hasCopyOptions)
{
  if (copyMenuTopColumns.empty())
  {
    for (ColumnIndex column = 0; column < columnsCount; ++column)
      drawCellCopyMenuItem(item_data, column, hasCopyOptions);
    return;
  }

  for (ColumnIndex column : copyMenuTopColumns)
    drawCellCopyMenuItem(item_data, column, hasCopyOptions);
  if (ImGui::BeginMenu("Specific Cell"))
  {
    for (ColumnIndex column = 0; column < columnsCount; ++column)
      if (eastl::find(copyMenuTopColumns.begin(), copyMenuTopColumns.end(), column) == copyMenuTopColumns.end())
        drawCellCopyMenuItem(item_data, column, hasCopyOptions);
    ImGui::EndMenu();
  }
}

void LpProfilerTable::setCopyMenuTopColumns(eastl::vector<ColumnIndex> top_columns)
{
  for (ColumnIndex column : top_columns)
    G_ASSERT_RETURN(column >= 0 && column < columnsCount, );
  copyMenuTopColumns = eastl::move(top_columns);
}

void LpProfilerTable::drawRowCopyOption(const void *item_data, bool &hasCopyOptions)
{
  if (ProfilerString rowText = getRowText(item_data); !rowText.empty())
  {
    if (hasCopyOptions)
      ImGui::Separator();

    if (ImGui::MenuItem("Full Row"))
    {
      CopyRequest request(CopyType::CONTEXT_ROW);
      request.customData = rowText;
      copyManager->executeContextCopy(request);
    }
    hasCopyOptions = true;
  }
}

void LpProfilerTable::drawCustomCopyOptions(const void *item_data)
{
  if (copyMenuItems.empty())
    return;
  ImGui::Separator();
  for (const LpCopyMenuItem &item : copyMenuItems)
    if (ImGui::MenuItem(item.label.c_str()))
    {
      CopyRequest request(CopyType::CONTEXT_CUSTOM);
      request.customData = item.getText(item_data);
      copyManager->executeContextCopy(request);
    }
}


bool LpProfilerTable::hasAnyActiveFilters() const
{
  for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
  {
    if (columns[columnIndex])
    {
      if (ColumnIndex columnType = columnIndex; isFilterActive(columnType))
        return true;
    }
  }
  return false;
}

bool LpProfilerTable::areAllColumnsVisible() const
{
  for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
    if (isColumnAvailable(columnIndex) && !columnVisibility[columnIndex])
      return false;
  return true;
}

int LpProfilerTable::getVisibleColumnsCount() const
{
  int visibleCount = 0;
  for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
    if (isColumnVisible(columnIndex))
      visibleCount++;
  return visibleCount;
}

void LpProfilerTable::ensureAtLeastOneColumnVisible()
{
  ImGuiTable *table = ImGui::GetCurrentTable();
  bool hasVisibleColumn = false;
  for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
  {
    // The next frame state shows a change from TableSetColumnEnabled or the ImGui menu at once.
    columnVisibility[columnIndex] = table->Columns[columnIndex].IsUserEnabledNextFrame;
    if (isColumnVisible(columnIndex))
      hasVisibleColumn = true;
  }

  // Crash protection for ImGui - at least one column must always be visible
  if (!hasVisibleColumn && columnsCount > 0)
  {
    ImGui::TableSetColumnEnabled(0, true);
    columnVisibility[0] = true;
  }
}

bool LpProfilerTable::isColumnOrderModified() const
{
  if (!(flags & LpTableFlags::REORDERABLE) || !trackColumnOrder)
    return false;

  return columnOrderWasModified;
}

void LpProfilerTable::handleHeaderContextMenu()
{
  drawViewMenu();
  drawFilterOptionsMenu();
  drawColumnOptionsMenu();
  drawTableSizingOptions();
}

void LpProfilerTable::drawFilterOptionsMenu()
{
  if (!ImGui::BeginMenu("Filter Options"))
    return;

  bool hasFilters = hasAnyActiveFilters();
  if (ImGui::MenuItem("Clear All Filters", nullptr, false, hasFilters))
  {
    clearAllFilters();
  }

  if (hasFilters)
  {
    ImGui::Separator();

    for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
    {
      if (!columns[columnIndex])
        continue;

      if (ColumnIndex columnType = columnIndex; isShownViewFilterActive(columnType))
      {
        ProfilerString menuItemLabel;
        menuItemLabel.reserve(128);
        menuItemLabel += "Clear '";
        menuItemLabel += columns[columnIndex]->getName();
        menuItemLabel += "' Filter";

        if (ImGui::Selectable(menuItemLabel.c_str(), false, ImGuiSelectableFlags_NoAutoClosePopups))
        {
          // The item names the shown view; the "Hidden filters" button resets the filters of the other views.
          if (columnFilters[columnType]->isOtherViewActive())
          {
            columnFilters[columnType]->resetShownView();
            markRowsDirty();
          }
          else
            clearColumnFilter(columnType);
        }
      }
    }
  }

  ImGui::EndMenu();
}

void LpProfilerTable::drawColumnOptionsMenu()
{
  if (!ImGui::BeginMenu("Column Options"))
    return;

  if (!presets.empty())
  {
    drawPresetMenu();
    ImGui::Separator();
  }

  if (bool orderModified = isColumnOrderModified(); ImGui::MenuItem("Reset order", nullptr, false, orderModified))
    deferredResetColumnOrder = true;

  if (bool allVisible = areAllColumnsVisible(); ImGui::MenuItem("Unhide all columns", nullptr, false, !allVisible))
    deferredUnhideAllColumns = true;

  ImGui::Separator();

  drawBaseColumnsVisibility();
  drawGroupedColumnsVisibility();

  ImGui::EndMenu();
}

void LpProfilerTable::drawColumnVisibilityToggle(int column_index)
{
  const bool isVisible = columnVisibility[column_index];
  // Prevent hiding the last visible column (ImGui crash protection)
  const bool canToggle = !(isVisible && getVisibleColumnsCount() <= 1);

  char menuItemText[256];
  snprintf(menuItemText, sizeof(menuItemText), "%s %s", isVisible ? "Hide" : "Unhide", columns[column_index]->getName());
  ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_NoAutoClosePopups;
  if (!canToggle)
    selectableFlags |= ImGuiSelectableFlags_Disabled;
  if (ImGui::Selectable(menuItemText, false, selectableFlags) && canToggle)
    deferredColumnToggle[column_index] = true;
  if (!canToggle && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    ImGui::SetTooltip("Cannot hide the last visible column.\nAt least one column must remain visible.");
}

void LpProfilerTable::drawBaseColumnsVisibility()
{
  for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
    if (columnGroupIds[columnIndex] < 0 && columns[columnIndex] && !(columns[columnIndex]->getFlags() & ImGuiTableColumnFlags_NoHide))
      drawColumnVisibilityToggle(columnIndex);
}

void LpProfilerTable::drawGroupedColumnsVisibility()
{
  for (const ColumnGroup &group : columnGroups)
  {
    if (!isColumnAvailable(group.firstColumn) || !ImGui::BeginMenu(group.name.c_str()))
      continue;
    for (int columnIndex = group.firstColumn; columnIndex < group.firstColumn + group.columnCount; ++columnIndex)
      if (isColumnAvailable(columnIndex) && columns[columnIndex])
        drawColumnVisibilityToggle(columnIndex);
    ImGui::EndMenu();
  }
}

void LpProfilerTable::drawTableSizingOptions()
{
  ImGui::Separator();

  if (rightClickedColumnIndex >= 0 && columns[rightClickedColumnIndex])
  {
    ProfilerString menuLabel;
    menuLabel.reserve(128);
    menuLabel += "Size '";
    menuLabel += columns[rightClickedColumnIndex]->getName();
    menuLabel += "' to fit";

    if (ImGui::MenuItem(menuLabel.c_str()))
      deferredSizeColumnToFit = rightClickedColumnIndex;
  }

  if (ImGui::MenuItem("Size all columns to fit"))
    deferredSizeAllColumnsToFit = true;
}

void LpProfilerTable::processDeferredColumnActions()
{
  if (deferredPresetSort >= 0)
  {
    const LpColumnPreset &preset = presets[deferredPresetSort].desc;
    if (flags & LpTableFlags::SORTABLE)
      ImGui::TableSetColumnSortDirection(preset.sortColumn, preset.sortDirection, false);
    deferredPresetSort = -1;
  }

  if (deferredUnhideAllColumns)
  {
    if (int allColumnsPreset = findAllColumnsPreset(); allColumnsPreset >= 0)
      applyPresetVisibility(allColumnsPreset);
    else
      for (int i = 0; i < columnsCount; ++i)
      {
        ImGui::TableSetColumnEnabled(i, true);
        columnVisibility[i] = true;
      }
    deferredUnhideAllColumns = false;
  }

  if (deferredPreset >= 0)
  {
    applyPresetVisibility(deferredPreset);
    applyPresetViews(deferredPreset);
    deferredPresetSort = deferredPreset;
    deferredPreset = -1;
  }

  if (deferredResetColumnOrder)
  {
    resetColumnOrder();
    deferredResetColumnOrder = false;
  }

  const int columnsMax = eastl::min(columnsCount, (int)deferredColumnToggle.size());
  for (int columnIndex = 0; columnIndex < columnsMax; ++columnIndex)
  {
    if (deferredColumnToggle[columnIndex])
    {
      bool currentState = columnVisibility[columnIndex];
      ImGui::TableSetColumnEnabled(columnIndex, !currentState);
      columnVisibility[columnIndex] = !currentState;
      deferredColumnToggle[columnIndex] = false;
    }
  }
}

void LpProfilerTable::queueColumnAutoFit(ImGuiTable *table, int column_index)
{
  if (!table)
    return;

  if (column_index < 0 || column_index >= columnsCount)
    return;

  if (!columns[column_index])
    return;

  if (column_index >= table->ColumnsCount)
    return;

  ImGuiTableColumnFlags columnFlags = ImGui::TableGetColumnFlags(column_index);
  if (!(columnFlags & ImGuiTableColumnFlags_IsEnabled))
    return;

  table->Columns[column_index].AutoFitQueue = AUTOFIT_QUEUE_VALUE;
  table->Columns[column_index].CannotSkipItemsQueue = AUTOFIT_QUEUE_VALUE;

  if (table->Columns[column_index].Flags & ImGuiTableColumnFlags_WidthFixed)
    table->Columns[column_index].WidthRequest = FORCE_AUTOFIT_WIDTH;

  enqueueColumnWidthClamp(column_index);
}

void LpProfilerTable::enqueueColumnWidthClamp(int column_index)
{
  if (column_index < 0 || column_index >= columnsCount)
    return;

  auto alreadyQueued = eastl::find(pendingWidthClampColumns.begin(), pendingWidthClampColumns.end(), column_index);
  if (alreadyQueued != pendingWidthClampColumns.end())
    return;

  pendingWidthClampColumns.push_back(column_index);
}

void LpProfilerTable::applyDeferredColumnWidthConstraints()
{
  if (pendingWidthClampColumns.empty())
    return;

  ImGuiTable *table = ImGui::GetCurrentTable();
  if (!table)
    return;

  eastl::vector<int> retryColumns;
  retryColumns.reserve(pendingWidthClampColumns.size());

  for (int columnIndex : pendingWidthClampColumns)
  {
    if (columnIndex < 0 || columnIndex >= columnsCount)
      continue;
    if (!columns[columnIndex])
      continue;
    ImGuiTableColumnFlags columnFlags = ImGui::TableGetColumnFlags(columnIndex);
    if (!(columnFlags & ImGuiTableColumnFlags_IsEnabled))
      continue;
    float currentWidth = table->Columns[columnIndex].WidthGiven;
    if (currentWidth <= 0.0f)
    {
      retryColumns.push_back(columnIndex);
      continue;
    }

    int prevEnabled = table->Columns[columnIndex].PrevEnabledColumn;
    if (prevEnabled != -1 && table->Columns[prevEnabled].WidthGiven <= 0.0f)
    {
      retryColumns.push_back(columnIndex);
      continue;
    }

    int nextEnabled = table->Columns[columnIndex].NextEnabledColumn;
    if (nextEnabled != -1 && table->Columns[nextEnabled].WidthGiven <= 0.0f)
    {
      retryColumns.push_back(columnIndex);
      continue;
    }
    clampColumnWidth(columnIndex);
  }

  pendingWidthClampColumns = eastl::move(retryColumns);
}

void LpProfilerTable::clampColumnWidth(int column_index)
{
  ImGuiTable *table = ImGui::GetCurrentTable();
  if (!table)
    return;

  if (column_index < 0 || column_index >= table->ColumnsCount)
    return;

  ImGuiTableColumnFlags columnFlags = ImGui::TableGetColumnFlags(column_index);
  bool isStretch = (columnFlags & ImGuiTableColumnFlags_WidthStretch) != 0;

  float preferredWidth = 0.0f;
  if (columns[column_index])
    preferredWidth = columns[column_index]->getWidth();

  float minWidth = (preferredWidth > 0.0f) ? preferredWidth : 0.0f;

  float currentWidth = table->Columns[column_index].WidthGiven;

  if (currentWidth < minWidth)
  {
    table->Columns[column_index].WidthRequest = minWidth;
    return;
  }

  if (isStretch)
    return;

  float maxWidth = (preferredWidth > 0.0f) ? preferredWidth : MAX_AUTOFIT_WIDTH;

  if (currentWidth > maxWidth)
    table->Columns[column_index].WidthRequest = maxWidth;
}

void LpProfilerTable::handleDeferredColumnSizing()
{
  if (deferredSizeColumnToFit >= 0)
  {
    sizeColumnToFit(deferredSizeColumnToFit);
    deferredSizeColumnToFit = -1;
    rightClickedColumnIndex = -1;
  }

  if (deferredSizeAllColumnsToFit)
  {
    sizeAllColumnsToFit();
    deferredSizeAllColumnsToFit = false;
  }
}

void LpProfilerTable::initializeColumnOrderTracking()
{
  if (!(flags & LpTableFlags::REORDERABLE))
    return;

  trackColumnOrder = true;
  initialColumnOrder.clear();
  currentColumnOrder.clear();

  for (int i = 0; i < columnsCount; ++i)
  {
    initialColumnOrder.push_back(i);
    currentColumnOrder.push_back(i);
  }

  columnOrderWasModified = false;
}

void LpProfilerTable::updateCurrentColumnOrder()
{
  if (!trackColumnOrder)
    return;

  if (ImGuiTable *table = ImGui::GetCurrentTable(); !table)
    return;

  readColumnOrderFromImGui();
  columnOrderWasModified = compareColumnOrders();
}

void LpProfilerTable::readColumnOrderFromImGui()
{
  ImGuiTable *table = ImGui::GetCurrentTable();
  if (!table)
    return;

  for (int i = 0; i < columnsCount; ++i)
  {
    for (int originalIndex = 0; originalIndex < columnsCount; ++originalIndex)
    {
      if (table->Columns[originalIndex].DisplayOrder == i)
      {
        currentColumnOrder[i] = originalIndex;
        break;
      }
    }
  }
}

bool LpProfilerTable::compareColumnOrders() const
{
  if (initialColumnOrder.size() != currentColumnOrder.size())
    return true;

  for (size_t i = 0; i < initialColumnOrder.size(); ++i)
    if (initialColumnOrder[i] != currentColumnOrder[i])
      return true;

  return false;
}

void LpProfilerTable::resetColumnOrder()
{
  if (!(flags & LpTableFlags::REORDERABLE) || !trackColumnOrder)
    return;

  currentColumnOrder = initialColumnOrder;
  columnOrderWasModified = false;

  isResettingColumnOrder = true;
  forceColumnOrderUpdate = true;
  isInitColumnOrderUpdate = true;
}

void LpProfilerTable::drawHeaderRow()
{
  ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

  bool isDraggingDetected = false;

  if (bool rightClickIntercepted = drawHeaderColumns(isDraggingDetected); rightClickIntercepted)
    headerRightClickHandled = true;

  updateDragTracking(isDraggingDetected);
}

const char *LpProfilerTable::getColumnDisplayName(int column_index)
{
  if (columns[column_index])
    return columns[column_index]->getName();

  static char tempName[COLUMN_NAME_BUFFER_SIZE];
  snprintf(tempName, sizeof(tempName), "Column%d", column_index);
  return tempName;
}

bool LpProfilerTable::drawHeaderColumns(bool &is_dragging_detected)
{
  bool rightClickIntercepted = false;

  for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
  {
    ImGui::TableSetColumnIndex(columnIndex);

    const char *columnName = getColumnDisplayName(columnIndex);

    ImGui::TableHeader(columnName);
    if (const LpProfilerTableColumn *column = columns[columnIndex].get();
        column && (column->getViewCount() > 1 || *column->getViewDescription(column->getView())) && !ImGui::IsItemActive() &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
      drawHeaderTooltip(columnIndex);

    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
      rightClickedColumnIndex = columnIndex;
      rightClickIntercepted = true;
      ImGui::OpenPopup("TableHeaderContextMenu");
    }

    if (trackColumnOrder && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
      is_dragging_detected = true;
  }

  return rightClickIntercepted;
}

void LpProfilerTable::updateDragTracking(bool is_dragging_detected)
{
  if (trackColumnOrder)
  {
    if (wasDragging && !is_dragging_detected)
      columnOrderWasModified = true;
    wasDragging = is_dragging_detected;
  }
}

void LpProfilerTable::sizeColumnToFit(int column_index) { queueColumnAutoFit(ImGui::GetCurrentTable(), column_index); }

void LpProfilerTable::sizeAllColumnsToFit()
{
  ImGuiTable *table = ImGui::GetCurrentTable();
  if (!table)
    return;

  const int maxColumns = eastl::min(columnsCount, table->ColumnsCount);

  for (int i = 0; i < maxColumns; ++i)
    queueColumnAutoFit(table, i);
}

// --- LpProfilerTableExportSource ---

void LpProfilerTableExportSource::buildSnapshot(LpExportSnapshot &out)
{
  table.refreshRows();
  exportParts.clear();
  for (int i = 0, colCount = table.getColumnsCount(); i < colCount; ++i)
    if (table.isColumnVisible(i))
      if (auto *col = table.getColumn(i))
        for (int part = 0; part < col->getExportPartCount(); ++part)
        {
          LpExportColumn ec;
          ec.id = static_cast<int>(exportParts.size());
          ec.header = col->getExportHeader(part);
          out.columns.push_back(ec);
          exportParts.push_back({i, part});
        }
  const size_t rowCount = table.getDisplayOrder().size();
  out.rowIndices.reserve(rowCount);
  for (size_t i = 0; i < rowCount; ++i)
    out.rowIndices.push_back(i);
}

void LpProfilerTableExportSource::getCell(int column_id, size_t filtered_row_index, ProfilerString &out_value) const
{
  out_value.clear();
  if (column_id < 0 || column_id >= static_cast<int>(exportParts.size()))
    return;
  const ExportPart &exportPart = exportParts[column_id];
  const LpProfilerTableColumn *column = table.getColumn(exportPart.column);
  if (!column || filtered_row_index >= table.getDisplayOrder().size())
    return;
  out_value = column->getExportText(table.getRowItem(static_cast<int>(filtered_row_index)), exportPart.part);
}

} // namespace levelprofiler
