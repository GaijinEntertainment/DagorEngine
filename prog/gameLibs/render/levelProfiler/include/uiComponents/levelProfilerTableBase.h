// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>
#include <EASTL/unique_ptr.h>
#include "levelProfilerInterface.h"
#include "levelProfilerWidgets.h"
#include <imgui/imgui_internal.h>

namespace levelprofiler
{

template <typename T>
class span;

class GlobalCopyManager;
struct CopyRequest;

using SearchTextChangedCallback = eastl::function<void(const char *)>;
using SearchEnterCallback = eastl::function<void()>;

struct LpCallbackDelegate
{
  void (*fn)(void *) = nullptr;
  void *userData = nullptr;
  void call() const
  {
    if (fn)
      fn(userData);
  }
};

template <typename T, void (T::*Method)()>
inline LpCallbackDelegate makeDelegate(T *instance)
{
  struct Thunk
  {
    static void invoke(void *ud)
    {
      if (!ud)
        return;
      T *self = static_cast<T *>(ud);
      (self->*Method)();
    }
  };
  LpCallbackDelegate d;
  d.fn = &Thunk::invoke;
  d.userData = instance;
  return d;
}

using LpSearchChangeFn = void (*)(void *, const char *);
using LpSearchCommitFn = void (*)(void *);

struct LpSearchDelegate
{
  LpSearchChangeFn onChange = nullptr;
  LpSearchCommitFn onCommit = nullptr;
  void *userData = nullptr;
};

template <typename T, void (T::*OnChange)(const char *), void (T::*OnCommit)()>
inline LpSearchDelegate makeSearchDelegate(T *instance)
{
  struct Thunk
  {
    static void change(void *ud, const char *txt)
    {
      if (!ud)
        return;
      T *self = static_cast<T *>(ud);
      (self->*OnChange)(txt);
    }
    static void commit(void *ud)
    {
      if (!ud)
        return;
      T *self = static_cast<T *>(ud);
      (self->*OnCommit)();
    }
  };
  LpSearchDelegate d;
  d.onChange = &Thunk::change;
  d.onCommit = &Thunk::commit;
  d.userData = instance;
  return d;
}

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
  SELECTABLE = 1u << 7,
  SIZING_FIXED_FIT = 1u << 8,
  SIZING_STRETCH_PROP = 1u << 9,
  FREEZE_HEADER = 1u << 10,
  REORDERABLE = 1u << 11,
  HIDEABLE = 1u << 12,

  DEFAULT = SORTABLE | RESIZABLE | SCROLLABLE | BORDERS | ROW_BG | CONTEXT_MENU | SORT_MULTI | SELECTABLE | SIZING_FIXED_FIT |
            FREEZE_HEADER | REORDERABLE | HIDEABLE
};

class LpProfilerTableColumn
{
public:
  LpProfilerTableColumn(const char *name, float width_value = 0.0f, ImGuiTableColumnFlags flags_value = 0);
  virtual ~LpProfilerTableColumn() = default;

  const char *getName() const { return name.c_str(); }
  float getWidth() const { return width; }
  ImGuiTableColumnFlags getFlags() const { return flags; }

  virtual void setupColumn() const;

  virtual bool drawHeaderContextMenu() { return false; }

  virtual void drawCell(const void *item_data) const = 0;

  // Compare two items for sorting (0 if equal, <0 if item1<item2, >0 if item1>item2)
  virtual int compareItems(const void * /* item1 */, const void * /* item2 */) const { return 0; }

  virtual ProfilerString getCellText(const void * /* item_data */, int /* column_index */) const { return ProfilerString{}; }

  ProfilerString getCellTextByType(const void *item_data, ColumnIndex column_index) const
  {
    return getCellText(item_data, column_index);
  }

protected:
  eastl::string name;
  float width = 0.0f; // Width in pixels; 0 for auto-sizing
  ImGuiTableColumnFlags flags = 0;
};

using LpDrawHeaderContextMenuCallback = void (*)(LpProfilerTableColumn *column, void *user_data);
using LpItemSelectedCallback = void (*)(int index, void *item_data, void *user_data);
using LpSortItemsCallback = int (*)(const ImGuiTableSortSpecs *sort_specs, const void *a, const void *b, void *user_data);

struct LpColumnFilterSpec
{
  ColumnIndex column = -1;
  const char *popupId = nullptr;
  LpCallbackDelegate popupDraw;
  const char *searchWidgetId = nullptr;
  const char *searchPlaceholder = nullptr;
  LpSearchDelegate search;
};

class LpProfilerTable
{
public:
  LpProfilerTable(const char *table_name, int column_count, LpTableFlags table_flags = LpTableFlags::DEFAULT);
  virtual ~LpProfilerTable();
  void setColumn(int index, LpProfilerTableColumn *column); // Takes ownership
  LpProfilerTableColumn *getColumn(int index) const;        // Returns non-owning pointer
  bool isColumnVisible(int index) const
  {
    if (index < 0 || index >= columnsCount)
      return false;
    if (columnVisibility.empty())
      return true; // treat as all visible if system not initialized
    return columnVisibility[index];
  }

  // Template helper for creating columns with automatic ownership management
  template <typename T, typename... Args>
  void createColumn(int index, Args &&...args)
  {
    if (index < 0 || index >= columnsCount)
      return;
    columns[index] = eastl::make_unique<T>(eastl::forward<Args>(args)...);
  }

  void setHeaderContextMenuCallback(LpDrawHeaderContextMenuCallback callback, void *user_data = nullptr);
  void setItemSelectedCallback(LpItemSelectedCallback callback, void *user_data = nullptr);
  void setSortItemsCallback(LpSortItemsCallback callback, void *user_data = nullptr);

  void setCopyManager(GlobalCopyManager *copy_manager) { copyManager = copy_manager; }

  bool draw();
  virtual void drawContent() = 0;

  void highlightCurrentRow(ImColor highlight_color = ImColor(60, 140, 255, 64));

  const ImGuiTableSortSpecs *getSortSpecs() const { return sortSpecs; }
  bool isResized() const { return hasResized; }
  bool isSortActive() const { return (sortSpecs && sortSpecs->SpecsCount > 0); }
  int getHoveredColumn() const { return hoveredColumnIndex; }
  int getHoveredRow() const { return hoveredRowIndex; }
  const char *getTableID() const { return tableId.c_str(); }
  int getCurrentRowIndex() const { return currentRowIndex; } // Only valid inside drawContent()

  void beginRow();
  bool setColumn(int column_index);
  void endRow();

  LpSearchWidget *getSearchWidget(ColumnIndex column_index) const;

  void registerColumnFilters(span<const LpColumnFilterSpec> specs);

  LpSearchWidget *registerSearchWidget(ColumnIndex column_index, const char *name, const char *hint,
    SearchTextChangedCallback on_text_changed_callback, SearchEnterCallback on_enter_callback)
  {
    auto widget = eastl::make_unique<LpSearchWidget>(this, column_index, name, hint, on_text_changed_callback, on_enter_callback);
    LpSearchWidget *widgetPtr = widget.get();
    searchWidgets.push_back(eastl::move(widget));
    return widgetPtr;
  }

  // Template version for different column enum types
  template <typename ColumnEnum>
  LpSearchWidget *registerSearchWidget(ColumnEnum column_enum, const char *name, const char *hint,
    SearchTextChangedCallback on_text_changed_callback, SearchEnterCallback on_enter_callback)
  {
    return registerSearchWidget(static_cast<ColumnIndex>(static_cast<int>(column_enum)), name, hint, on_text_changed_callback,
      on_enter_callback);
  }

protected:
  virtual void drawRowContextMenu(int row_index, int column_index, const void *item_data);
  virtual ProfilerString getRowText(const void *item_data) const;
  virtual ProfilerString getColumnName(int column_index) const;
  virtual ProfilerString generateCustomCopyText(const void *item_data, const ProfilerString &menu_item) const;
  virtual eastl::vector<ProfilerString> getCustomCopyMenuItems(const void *item_data) const;

  virtual bool shouldShowCellCopyOptions() const { return true; }
  virtual bool shouldShowRowCopyOption() const { return true; }
  virtual eastl::vector<int> getEssentialColumnIndices() const { return {}; } // Override to show only specific columns
  virtual bool shouldGroupNonEssentialCells() const { return false; }         // Group non-essential cells in submenu
  // Column visibility grouping (override where needed, default: no groups)
  struct ColumnGroup
  {
    ProfilerString name;
    eastl::vector<int> columnIndices;
  };
  virtual eastl::vector<ColumnGroup> getColumnGroups() const { return {}; }
  virtual eastl::vector<int> getBaseColumnIndices() const
  {
    return {};
  } // Base (stable) column indices shown at top level before groups

private:
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
  const ImGuiTableSortSpecs *sortSpecs = nullptr;
  bool hasSorted = false;
  bool hasResized = false;
  int hoveredColumnIndex = -1;
  int hoveredRowIndex = -1;
  int currentRowIndex = -1; // Only valid inside drawContent()
  int rightClickedColumnIndex = -1;

  // Callback System
  LpDrawHeaderContextMenuCallback headerContextMenuCallback = nullptr;
  void *headerContextMenuCallbackUserData = nullptr;
  LpItemSelectedCallback itemSelectedCallback = nullptr;
  void *itemSelectedCallbackUserData = nullptr;
  LpSortItemsCallback sortItemsCallback = nullptr;
  void *sortItemsCallbackUserData = nullptr;

  // Core Virtual Functions
  virtual void setupColumns();
  virtual bool handleSorting();
  virtual void handleSelection() {};

  // Header and Context Menu Management
  void handleHeaderContextMenu();
  void drawFilterOptionsMenu();
  void drawColumnOptionsMenu();
  void drawColumnVisibilityToggles();
  void drawBaseColumnsVisibility();
  void drawGroupedColumnsVisibility();
  void drawTableSizingOptions();
  void drawHeaderRow();

  // Header Row Drawing Helpers
  const char *getColumnDisplayName(int column_index);
  bool drawHeaderColumns(bool &is_dragging_detected);
  void updateDragTracking(bool is_dragging_detected);

  // Filter Management
  virtual bool isFilterActive(ColumnIndex /*column_index*/) const { return false; }
  virtual void clearAllFilters() {}
  virtual void clearColumnFilter(ColumnIndex /* column */) {}

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
  // May override to ignore inactive columns; default checks all columns.
  virtual bool areAllColumnsEffectivelyVisible() const { return areAllColumnsVisible(); }
  int getVisibleColumnsCount() const;
  void ensureAtLeastOneColumnVisible();
  bool isColumnOrderModified() const;
  eastl::vector<bool> columnVisibility;

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

  // Column Order Access
  const eastl::vector<int> &getInitialColumnOrder() const { return initialColumnOrder; }
  eastl::vector<int> &getInitialColumnOrder() { return initialColumnOrder; }
  const eastl::vector<int> &getCurrentColumnOrder() const { return currentColumnOrder; }
  eastl::vector<int> &getCurrentColumnOrder() { return currentColumnOrder; }

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
  eastl::vector<LpCallbackDelegate> popupDraw;
  eastl::vector<uint8_t> hasPopup;
  eastl::vector<LpSearchWidget *> columnSearchWidgets;
  eastl::vector<ImVec2> popupAnchorPositions;

  void ensureFilterStorageSized();
};

} // namespace levelprofiler