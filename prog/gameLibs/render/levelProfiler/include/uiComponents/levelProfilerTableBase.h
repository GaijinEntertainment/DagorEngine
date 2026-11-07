// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>
#include <EASTL/unique_ptr.h>
#include "levelProfilerInterface.h"
#include "levelProfilerWidgets.h"
#include "levelProfilerColumnFilter.h"
#include "levelProfilerExporter.h"
#include <imgui/imgui_internal.h>

namespace levelprofiler
{

struct CopyRequest;

using SearchTextChangedCallback = eastl::function<void(const char *)>;

class LpProfilerTableColumn;

enum LpTableFlags : uint32_t
{
  TABLE_NONE = 0,
  SORTABLE = 1u << 0,
  RESIZABLE = 1u << 1,
  SCROLLABLE = 1u << 2,
  BORDERS = 1u << 3,
  ROW_BG = 1u << 4,
  CONTEXT_MENU = 1u << 5,
  SORT_MULTI = 1u << 6,
  SIZING_FIXED_FIT = 1u << 8,
  SIZING_STRETCH_PROP = 1u << 9,
  FREEZE_HEADER = 1u << 10,
  REORDERABLE = 1u << 11,
  HIDEABLE = 1u << 12,

  DEFAULT = SORTABLE | RESIZABLE | SCROLLABLE | BORDERS | ROW_BG | CONTEXT_MENU | SORT_MULTI | SIZING_FIXED_FIT | FREEZE_HEADER |
            REORDERABLE | HIDEABLE
};

// The alpha of the Text color for the part that does not sort. An opaque gray, as TextDisabled, is unreadable on the row
// selection and button hover colors.
inline constexpr float LP_DIM_TEXT_ALPHA = 0.55f;

// Where the view glyph of a composite view draws the sort marker: before its two letters when the view sorts by the left part,
// after them when it sorts by the right part.
enum class LpSortMarker
{
  NONE,
  BEFORE,
  AFTER
};

class LpProfilerTableColumn
{
public:
  LpProfilerTableColumn(const char *name, float width_value = 0.0f, ImGuiTableColumnFlags flags_value = 0);
  virtual ~LpProfilerTableColumn() = default;

  virtual const char *getName() const { return name.c_str(); }
  float getWidth() const { return width; }
  ImGuiTableColumnFlags getFlags() const { return flags; }

  virtual void setupColumn(ImGuiTableColumnFlags extra_flags) const;

  // The table draws its rows with ImGuiListClipper, so a cell must fit one line of text.
  virtual void drawCell(const void *item_data) const = 0;

  // Compare two items for sorting (0 if equal, <0 if item1<item2, >0 if item1>item2). The table compares only items that both
  // have a value.
  virtual int compareItems(const void * /* item1 */, const void * /* item2 */) const { return 0; }
  // False for an item without the value that the sort reads: the sort puts it after the items with a value in both directions.
  // Such a cell is empty or shows the missing text of the value, and a composite cell can still show its other value.
  virtual bool hasValue(const void * /* item_data */) const { return true; }

  virtual ProfilerString getCellText(const void * /* item_data */, int /* column_index */) const { return ProfilerString{}; }
  // The cell text for the Full Row copy: a column that shows a short label gives the full list here.
  virtual ProfilerString getFullCellText(const void *item_data) const { return getCellText(item_data, 0); }
  // The export has a column for each part of a column: a composite column exports each value. A part index is in
  // [0, getExportPartCount()). The exported cell text is a number without its unit, which getExportHeader names.
  virtual int getExportPartCount() const { return 1; }
  virtual ProfilerString getExportText(const void *item_data, int /* part */) const { return getFullCellText(item_data); }
  virtual ProfilerString getExportHeader(int /* part */) const { return name; }
  // The number of a column with numeric values. Returns false for an item without a value and for a column without numbers.
  virtual bool getNumber(const void * /* item_data */, double & /* value */) const { return false; }
  // The table calls it when the mouse is over the cell, also over an empty cell. Draws nothing without a tooltip.
  virtual void drawCellTooltip(const void * /* item_data */) const {}

  // A view is another value in the same column slot. A view index is in [0, getViewCount()).
  virtual int getViewCount() const { return 1; }
  virtual int getView() const { return 0; }
  // The header is the column name of the view, in the header and in the column lists; the title is the full view name,
  // in the menus and the view tooltips. The header tooltip explains the first bracket of the header when it is "(/X)", a
  // share of X, "(vs X)", the value of X, or "(+M)", another metric of the row; another bracket gets no explanation.
  virtual const char *getViewHeader(int /* view */) const { return getName(); }
  virtual const char *getViewTitle(int /* view */) const { return getName(); }
  virtual const char *getViewShortName(int /* view */) const { return ""; }
  virtual const char *getViewGlyph(int /* view */) const { return ""; }
  virtual LpSortMarker getViewSortMarker(int /* view */) const { return LpSortMarker::NONE; }
  // How the value of the view is got, in the header tooltip; empty when the title says enough. A non-empty description gives
  // the header tooltip also to a column with one view.
  virtual const char *getViewDescription(int /* view */) const { return ""; }
  // The export header of a part of the view: the header tooltip names the sorted part with it.
  virtual ProfilerString getViewPartHeader(int /* view */, int part) const { return getExportHeader(part); }

protected:
  friend class LpProfilerTable;
  // Only the table changes the view, as the rows are sorted again.
  virtual void setView(int /* view */) {}

  eastl::string name;
  float width = 0.0f; // Width in pixels; 0 for auto-sizing
  ImGuiTableColumnFlags flags = 0;
};

inline constexpr float LP_NAME_COLUMN_WIDTH = 180.0f;
inline constexpr float LP_NARROW_COLUMN_WIDTH = 60.0f;
inline constexpr float LP_MEDIUM_COLUMN_WIDTH = 80.0f;

// Reserve per entry of a list cell: entry name + " (count)" + ", ".
inline constexpr size_t LP_LIST_ENTRY_RESERVE_SIZE = 64;

// Appends "name (count)" to the text; the count is not shown when it is less than min_shown_count.
void lp_append_name_count(ProfilerString &text, const char *name, int count, int min_shown_count = 0);

// Draws the text in the open tooltip, wrapped at the tooltip width of the tables.
void lp_draw_tooltip_text(const char *text);

template <typename T>
inline int lp_compare3(const T &a, const T &b)
{
  return a < b ? -1 : (b < a ? 1 : 0);
}

// Row must be the item type of the table that owns the column: the casts from the table items are not checked.
template <typename Row>
class LpTypedTableColumn : public LpProfilerTableColumn
{
public:
  using LpProfilerTableColumn::LpProfilerTableColumn;

  void drawCell(const void *item_data) const final { drawRowCell(*static_cast<const Row *>(item_data)); }
  int compareItems(const void *item1, const void *item2) const final
  {
    return compareRows(*static_cast<const Row *>(item1), *static_cast<const Row *>(item2));
  }
  bool hasValue(const void *item_data) const final { return rowHasValue(*static_cast<const Row *>(item_data)); }
  ProfilerString getCellText(const void *item_data, int /* column_index */) const final
  {
    return getRowCellText(*static_cast<const Row *>(item_data));
  }
  ProfilerString getFullCellText(const void *item_data) const final { return getRowFullText(*static_cast<const Row *>(item_data)); }
  ProfilerString getExportText(const void *item_data, int part) const final
  {
    return getRowExportText(*static_cast<const Row *>(item_data), part);
  }
  void drawCellTooltip(const void *item_data) const final { drawRowTooltip(*static_cast<const Row *>(item_data)); }

protected:
  virtual void drawRowCell(const Row &row) const = 0;
  virtual int compareRows(const Row &row1, const Row &row2) const = 0;
  virtual bool rowHasValue(const Row & /* row */) const { return true; }
  virtual ProfilerString getRowCellText(const Row &row) const = 0;
  virtual ProfilerString getRowFullText(const Row &row) const { return getRowCellText(row); }
  virtual ProfilerString getRowExportText(const Row &row, int /* part */) const { return getRowFullText(row); }
  // Opens the tooltip window itself and draws the content in it; draws nothing for a row without a tooltip.
  virtual void drawRowTooltip(const Row & /* row */) const {}
};

// A text that can be wider than its column, for example a list: the cell shows it in one line, the tooltip shows the full text
// of the cell (getRowFullText) wrapped.
template <typename Row>
class LpLongTextColumn : public LpTypedTableColumn<Row>
{
public:
  using LpTypedTableColumn<Row>::LpTypedTableColumn;

protected:
  void drawRowCell(const Row &row) const override
  {
    if (const ProfilerString text = this->getRowCellText(row); !text.empty())
      ImGui::TextUnformatted(text.c_str());
  }
  void drawRowTooltip(const Row &row) const override
  {
    const ProfilerString text = this->getRowFullText(row);
    if (!text.empty() && ImGui::BeginTooltip())
    {
      lp_draw_tooltip_text(text.c_str());
      ImGui::EndTooltip();
    }
  }
};

// A column and its filter; the filter is null for a column without one.
struct LpColumnSetup
{
  eastl::unique_ptr<LpProfilerTableColumn> column;
  eastl::unique_ptr<ILpColumnFilter> filter;
};

struct LpColumnViewDesc
{
  // The text of the view button, for example "#" or "%"; a composite view has a single-byte (ASCII) letter for each part.
  ProfilerString glyph;
  ProfilerString shortName; // The short name in the view switch, for example "Prev"; can be empty for a view with a marker
  ProfilerString title;     // The full name in the menus and tooltips
  eastl::unique_ptr<LpProfilerTableColumn> column;
  // The value filters of the values that the view shows, as indices from LpViewColumnBuilder::addValueFilter, in the order of
  // its parts; the filter popup of the view draws them in this order. Empty for a view without a filter.
  eastl::vector<int> filterIndices;
  ProfilerString description; // How the value is got, in the header tooltip; can be empty
};

// Builds a column with views. A filter belongs to a value of the column, not to a view: each view that shows the value uses its
// filter, so the filter stays with the value when a view or a preset changes. A value filter gets its index when it is added,
// and a view lists the indices of its values. The column needs a view, each view a column, and each value filter a view. The
// slot takes the widest view width and the flags of view 0. Its filter passes a row when all value filters pass it, so a filter
// of a value that the shown view does not show stays active.
class LpViewColumnBuilder
{
public:
  int addValueFilter(eastl::unique_ptr<ILpColumnFilter> filter);
  // The index of the added view.
  int addView(LpColumnViewDesc view);
  eastl::pair<int, int> addViews(eastl::pair<LpColumnViewDesc, LpColumnViewDesc> view_pair);
  // The export of the slot gives the parts of these views, whichever view is shown, for an export of all values of the slot;
  // by default the export follows the shown view.
  void setExportViews(eastl::vector<int> view_indices) { exportViews = eastl::move(view_indices); }
  LpColumnSetup build();

private:
  eastl::vector<LpColumnViewDesc> views;
  eastl::vector<eastl::unique_ptr<ILpColumnFilter>> valueFilters;
  eastl::vector<int> exportViews;
};

// An item aligned to the right edge of a cell makes the cell content as wide as the column, so the column auto-fit cannot
// narrow it. Called after the last item of such a cell, this gives the auto-fit the natural end of the content in screen space.
void lp_set_cell_natural_max_x(float natural_max_x);

// The look of a chip, a short text in a pill in a table cell.
struct LpChipStyle
{
  ImVec4 background = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // No fill when transparent
  float borderAlpha = 0.0f;                           // The alpha of the frame in the text color; no frame at 0
  float textAlpha = 1.0f;
};
// A cell without a value that has a reason to show, for example "stub": dim text in a thin frame.
inline constexpr LpChipStyle LP_EMPTY_CHIP{.borderAlpha = LP_DIM_TEXT_ALPHA * 0.5f, .textAlpha = LP_DIM_TEXT_ALPHA};

// Draws a chip on the text line of the cell, where the values of the other cells are.
void lp_draw_cell_chip(const char *text, const LpChipStyle &style = LP_EMPTY_CHIP);

// The sum of one numeric column over the visible rows and over all rows of the table.
struct LpColumnSum
{
  double visible = 0.0;
  double total = 0.0;
};

using LpSumFormat = void (*)(char *text, int size, double value);
void lp_format_count_sum(char *text, int size, double value);
// KB below 1 MB, GB from 1024 MB.
void lp_format_memory_sum_mb(char *text, int size, double size_mb);

// The counters line under a table: "<label>: visible / total".
void lp_draw_row_count(const char *label, int visible, int total);
// "<label>: visible / total (share)", the share dim.
void lp_draw_column_sum(const char *label, const LpColumnSum &sum, LpSumFormat format);

// An item of the "Copy to clipboard" menu of a row, after the cells and the Full Row.
struct LpCopyMenuItem
{
  ProfilerString label;
  eastl::function<ProfilerString(const void *item)> getText;
};

// The view of one column, or of each column of a group that has this view.
struct LpColumnViewRef
{
  ColumnIndex column = -1;
  int group = -1; // Group id from addColumnGroup; set either column or group
  int view = 0;
};

// The visible columns, their views and the sort of a table for one question of the user. A column with
// ImGuiTableColumnFlags_NoHide is visible in each preset.
struct LpColumnPreset
{
  ProfilerString name;
  ProfilerString tooltip; // The question that the preset answers
  eastl::vector<ColumnIndex> columns;
  eastl::vector<int> groups;            // Group ids from addColumnGroup; the preset shows all their columns
  bool allColumns = false;              // Shows every column; "Unhide all columns" shows this preset and keeps the user sort
  eastl::vector<LpColumnViewRef> views; // The other columns show view 0
  ColumnIndex sortColumn = 0;
  ImGuiSortDirection sortDirection = ImGuiSortDirection_Ascending;
};

class LpProfilerTable
{
public:
  LpProfilerTable(const char *table_name, int column_count, LpTableFlags table_flags = LpTableFlags::DEFAULT);
  virtual ~LpProfilerTable();
  LpProfilerTableColumn *getColumn(int index) const; // Returns non-owning pointer
  int getColumnsCount() const { return columnsCount; }
  bool isColumnVisible(int index) const { return isColumnAvailable(index) && columnVisibility[index]; }
  bool isColumnAvailable(int index) const;

  void setCopyManager(GlobalCopyManager *copy_manager) { copyManager = copy_manager; }

  bool draw();
  virtual void drawContent();

  // Brings the row cache up to date with the data, the filters and the sort specs; draw() does it too.
  void refreshRows();
  // Item indices of the rows in display order, as of the last refreshRows() or draw().
  const eastl::vector<int> &getDisplayOrder() const { return displayOrder; }
  // The item of a display row; valid until the next refreshRows() or draw().
  const void *getRowItem(int row) const { return getItem(displayOrder[row]); }

  bool isSortedBy(int column_index) const;
  // Brings the rows up to date first, as refreshRows() does.
  LpColumnSum sumColumn(ColumnIndex column);

  LpSearchWidget *getSearchWidget(ColumnIndex column_index) const;

  LpSearchWidget *registerSearchWidget(const char *name, const char *hint, SearchTextChangedCallback on_text_changed_callback)
  {
    auto widget = eastl::make_unique<LpSearchWidget>(name, hint, eastl::move(on_text_changed_callback));
    LpSearchWidget *widgetPtr = widget.get();
    searchWidgets.push_back(eastl::move(widget));
    return widgetPtr;
  }

protected:
  // Rows are cached: items are rebuilt when pollItemsChanged() says so, the filtered rows when
  // markRowsDirty() is called, and the order when the sort specs change. pollItemsChanged() runs
  // first in each refresh and can call markRowsDirty().
  virtual bool pollItemsChanged() { return false; }
  virtual void rebuildItems() {}
  virtual int getItemCount() const = 0;
  virtual const void *getItem(int item_index) const = 0;
  // Called for the rows that are on screen only, so per-frame state like selection is set here.
  virtual void *getItemForDraw(int item_index) = 0;
  // A click on the first cell of the row of the item.
  virtual void onItemClicked(const void * /* item */) {}
  // An item is visible when every active column filter passes it.
  bool isItemVisible(int item_index) const;
  virtual int compareItemsTieBreak(const void * /* item1 */, const void * /* item2 */) const { return 0; }
  virtual const char *getNoRowsText() const { return nullptr; }

  void markRowsDirty() { rowsDirty = true; }

  virtual void drawRowContextMenu(int row_index, int column_index, const void *item_data);

  // The cells at the top level of the "Copy to clipboard" menu; the other cells go to the "Specific Cell" submenu. Without a
  // call, all cells are at the top level. Call it in the table constructor, as addCopyMenuItem.
  void setCopyMenuTopColumns(eastl::vector<ColumnIndex> top_columns);
  void addCopyMenuItem(LpCopyMenuItem item) { copyMenuItems.push_back(eastl::move(item)); }

  // Adds a submenu of the column menu for adjacent columns; call it in the table constructor. Returns the group id. Column i of the
  // group is available while active_from + i is less than the active count of the group (by default all columns are). An unavailable
  // column is hidden and is not in the menus; the user visibility of the column does not change.
  int addColumnGroup(const char *name, ColumnIndex first_column, int column_count, int active_from = 0);
  void setColumnGroupActiveCount(int group, int active_count);

  // Call it in the table constructor after addColumn and addColumnGroup; the table must be HIDEABLE, and SORTABLE to get the
  // sort of a preset. A preset is not added without a name, with a column or group out of range, with columns or groups beside
  // allColumns, with a sort column out of range or hidden by the preset, or with a view that its column does not have or that
  // no column of its group has; a view ref of a group sets the columns of the group that have the view. The sort is set in
  // the frame after the preset is applied, when its columns appear; if the sort column is not available then, ImGui sorts by
  // the first enabled column without NoSort. The table applies the first preset at its first draw, over the column state that
  // ImGui restores from its settings.
  void addPreset(LpColumnPreset preset);

private:
  void sortRows(bool rows_rebuilt);
  void drawRows();
  void setColumnFilter(ColumnIndex column, eastl::unique_ptr<ILpColumnFilter> filter);
  bool isShownViewFilterActive(ColumnIndex column_index) const;

  struct Preset
  {
    LpColumnPreset desc;
    eastl::vector<bool> visibleColumns;
    eastl::vector<int> columnViews;
  };
  eastl::vector<Preset> presets;
  int activePreset = -1;
  int deferredPreset = -1;
  // ImGui drops the sort of a hidden column, so the sort is set in the next frame, when the preset columns appear.
  int deferredPresetSort = -1;

  void drawPresetBar();
  void drawHiddenFiltersButton(const char *label, float body_width, const eastl::vector<int> &hidden_filter_columns);
  void drawPresetMenu();
  ProfilerString getPresetLabel(int index) const;
  ProfilerString getPresetTooltip(int index) const;
  bool presetMatchesColumns(int index) const;
  bool isActivePresetModified(int index) const { return index == activePreset && !presetMatchesColumns(index); }
  int findAllColumnsPreset() const;
  void applyPresetVisibility(int index);
  void applyPresetViews(int index);

  bool columnHasView(ColumnIndex column, int view) const;
  void setColumnView(ColumnIndex column, int view);
  // Sets the view of the group columns that have it.
  void setGroupView(int group, int view);
  // True when setGroupView(group, view) changes a column.
  bool groupViewDiffers(int group, int view) const;
  void drawViewMenu();
  // -1 without an active preset.
  int getPresetView(ColumnIndex column) const;
  void drawViewButton(ColumnIndex column);
  void drawHeaderTooltip(ColumnIndex column) const;
  void drawViewPopup();
  ColumnIndex viewPopupColumn = -1;
  ImVec2 viewPopupAnchor;
  // The "Apply to" choice of the view popup, kept for the session.
  bool viewApplyToGroup = false;

  eastl::vector<int> displayOrder;
  bool rowsDirty = true;
  bool sortDirty = true;
  // A copy, because ImGui keeps the specs in its table pool, which moves when it grows.
  eastl::vector<ImGuiTableColumnSortSpecs> activeSortSpecs;

  eastl::vector<ColumnIndex> copyMenuTopColumns;
  eastl::vector<LpCopyMenuItem> copyMenuItems;
  ProfilerString getRowText(const void *item_data) const;
  ProfilerString getColumnName(int column_index) const;
  void drawCellCopyMenuItem(const void *item_data, int col_idx, bool &hasCopyOptions);
  void drawCellCopyOptions(const void *item_data, bool &hasCopyOptions);
  void drawRowCopyOption(const void *item_data, bool &hasCopyOptions);
  void drawCustomCopyOptions(const void *item_data);

protected:
  GlobalCopyManager *copyManager = nullptr;

  // Core Table Configuration
  eastl::string tableName;
  eastl::string tableId;
  LpTableFlags flags = LpTableFlags::DEFAULT;
  int columnsCount = 0;
  eastl::vector<eastl::unique_ptr<LpProfilerTableColumn>> columns;

  // Table State and Interaction
  int rightClickedColumnIndex = -1;

  // Core Virtual Functions
  virtual void setupColumns();
  void handleSorting();

  // Header and Context Menu Management
  void handleHeaderContextMenu();
  void drawFilterOptionsMenu();
  void drawColumnOptionsMenu();
  void drawColumnVisibilityToggle(int column_index);
  void drawBaseColumnsVisibility();
  void drawGroupedColumnsVisibility();
  void drawTableSizingOptions();
  void drawHeaderRow();

  // Header Row Drawing Helpers
  const char *getColumnDisplayName(int column_index);
  bool drawHeaderColumns(bool &is_dragging_detected);
  void updateDragTracking(bool is_dragging_detected);

  // Sets the column and its filter, if any. Call it in the table constructor: the table binds the filter to the items and
  // applies it only at the next item rebuild.
  void addColumn(ColumnIndex index, LpColumnSetup setup);

  // Filter Management
  bool isFilterActive(ColumnIndex column_index) const;
  void clearAllFilters();
  void clearColumnFilter(ColumnIndex column);

  // Template versions for different column enum types
  template <typename ColumnEnum>
  bool isFilterActive(ColumnEnum column_enum) const
  {
    return isFilterActive(static_cast<ColumnIndex>(column_enum));
  }

  template <typename ColumnEnum>
  void clearColumnFilter(ColumnEnum column_enum)
  {
    clearColumnFilter(static_cast<ColumnIndex>(column_enum));
  }

  bool hasAnyActiveFilters() const;

  // Column Visibility and State Management
  bool areAllColumnsVisible() const;
  int getVisibleColumnsCount() const;
  void ensureAtLeastOneColumnVisible();
  bool isColumnOrderModified() const;
  // The user visibility layer; isColumnVisible() also checks the availability.
  eastl::vector<bool> columnVisibility;

  struct ColumnGroup
  {
    ProfilerString name;
    ColumnIndex firstColumn = 0;
    int columnCount = 0;
    int activeFrom = 0;
    int activeCount = 0;
  };
  eastl::vector<ColumnGroup> columnGroups;
  // The group id of each column, -1 for a column without a group.
  eastl::vector<int> columnGroupIds;

  // Column Sizing Functions
  void handleDeferredColumnSizing();
  void sizeColumnToFit(int column_index);
  void queueColumnAutoFit(ImGuiTable *table, int column_index);
  void sizeAllColumnsToFit();
  void enqueueColumnWidthClamp(int column_index);
  void applyDeferredColumnWidthConstraints();
  void clampColumnWidth(int column_index);

  // Column Order Management
  void initializeColumnOrderTracking();
  void readColumnOrderFromImGui();
  void updateCurrentColumnOrder();
  bool compareColumnOrders() const;
  void resetColumnOrder();

  eastl::vector<int> initialColumnOrder;
  eastl::vector<int> currentColumnOrder;
  bool trackColumnOrder = false;
  bool columnOrderWasModified = false;
  bool isInitColumnOrderUpdate = true;
  bool isResettingColumnOrder = false;
  bool forceColumnOrderUpdate = false;
  bool wasDragging = false;
  bool headerRightClickHandled = false;

  // Deferred Actions System
  void processDeferredColumnActions();

  bool deferredUnhideAllColumns = false;
  bool deferredResetColumnOrder = false;
  bool deferredSizeAllColumnsToFit = false;
  int deferredSizeColumnToFit = -1; // -1 if no column to size
  eastl::vector<bool> deferredColumnToggle;
  eastl::vector<int> pendingWidthClampColumns;

  // Search and Widget Management
  eastl::vector<eastl::unique_ptr<LpSearchWidget>> searchWidgets;

  // Parallel arrays for popup + search metadata (size == columnsCount)
  eastl::vector<ProfilerString> popupIds;
  eastl::vector<LpSearchWidget *> columnSearchWidgets;
  eastl::vector<ImVec2> popupAnchorPositions;
  eastl::vector<eastl::unique_ptr<ILpColumnFilter>> columnFilters;
  // The active filters of the current filter pass; refreshRows() fills it before it calls isItemVisible().
  eastl::vector<const ILpColumnFilter *> activeColumnFilters;
  // LpSearchBox keeps the id pointer, so the ids live here for the life of the table.
  eastl::vector<ProfilerString> searchIds;

  void ensureFilterStorageSized();
  void rebindColumnFilters();
};

// A table that owns its rows, all of one type. Row has a bool isSelected, which the table sets before it draws the row. The name
// of a row orders the rows that the sort finds equal and keeps the selection over an item rebuild.
template <typename Row>
class LpTypedTable : public LpProfilerTable
{
public:
  using NameGetter = eastl::function<const ProfilerString &(const Row &)>;
  using SelectedCallback = eastl::function<void(const Row &)>;

  LpTypedTable(const char *table_name, int column_count, NameGetter name_getter, LpTableFlags table_flags = LpTableFlags::DEFAULT) :
    LpProfilerTable(table_name, column_count, table_flags), getRowName(eastl::move(name_getter))
  {}

  const ProfilerString &getSelectedName() const { return selectedName; }
  void setSelectedName(const ProfilerString &name) { selectedName = name; }
  // Called on each click on the first cell of a row, also of the selected row, after the table set the selected name to the
  // name of the row. The row reference is valid only during the call.
  void setSelectedCallback(SelectedCallback callback) { onSelected = eastl::move(callback); }

  // As of the last refreshRows() or draw().
  int getRowCount() const { return static_cast<int>(items.size()); }
  int getVisibleRowCount() const { return static_cast<int>(getDisplayOrder().size()); }
  // The row at a display index in [0, getVisibleRowCount()); valid until the next refreshRows() or draw().
  const Row &getVisibleRow(int row) const { return items[getDisplayOrder()[row]]; }

protected:
  int getItemCount() const final { return static_cast<int>(items.size()); }
  const void *getItem(int item_index) const final { return &items[item_index]; }
  // Sets the per-frame state of a row that is on screen.
  virtual void prepareItemForDraw(Row & /* row */) {}

  // The Name column with a text filter on the row name, so the filter, the sort and the selection read the same name.
  LpColumnSetup nameColumnSetup(eastl::unique_ptr<LpProfilerTableColumn> column, const char *search_hint) const
  {
    return {eastl::move(column), eastl::make_unique<LpTextColumnFilter<Row>>(getRowName, search_hint)};
  }

  void addCopyMenuItem(const char *label, eastl::function<ProfilerString(const Row &)> get_text)
  {
    LpProfilerTable::addCopyMenuItem(
      {label, [getText = eastl::move(get_text)](const void *item) { return getText(*static_cast<const Row *>(item)); }});
  }

  // rebuildItems() fills it.
  eastl::vector<Row> items;

private:
  void *getItemForDraw(int item_index) final
  {
    Row &row = items[item_index];
    row.isSelected = getRowName(row) == selectedName;
    prepareItemForDraw(row);
    return &row;
  }
  int compareItemsTieBreak(const void *item1, const void *item2) const final
  {
    return getRowName(*static_cast<const Row *>(item1)).compare(getRowName(*static_cast<const Row *>(item2)));
  }
  void onItemClicked(const void *item) final
  {
    const Row &row = *static_cast<const Row *>(item);
    selectedName = getRowName(row);
    if (onSelected)
      onSelected(row);
  }

  const NameGetter getRowName;
  ProfilerString selectedName;
  SelectedCallback onSelected;
};

// Exports the visible columns and the rows of a table in display order; each column gives an export header and a cell text for
// each of its parts.
class LpProfilerTableExportSource final : public ILpTableExportSource
{
public:
  LpProfilerTableExportSource(LpProfilerTable &tbl, const char *export_table_name) : table(tbl), exportTableName(export_table_name) {}

  void buildSnapshot(LpExportSnapshot &out) override;
  void getCell(int column_id, size_t filtered_row_index, ProfilerString &out_value) const override;
  const char *getExportTableName() const override { return exportTableName; }

private:
  struct ExportPart
  {
    ColumnIndex column = -1;
    int part = 0;
  };

  LpProfilerTable &table;
  const char *exportTableName = nullptr;
  // The export column ids of the last buildSnapshot index this list.
  eastl::vector<ExportPart> exportParts;
};

} // namespace levelprofiler
