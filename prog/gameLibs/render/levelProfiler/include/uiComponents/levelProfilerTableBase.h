// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <imgui/imgui.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unique_ptr.h>
#include "levelProfilerInterface.h"
#include "levelProfilerFilter.h"
#include "levelProfilerWidgets.h"

namespace levelprofiler
{

class GlobalCopyManager;
struct CopyRequest;

using SearchTextChangedCallback = eastl::function<void(const char *)>;
using SearchEnterCallback = eastl::function<void()>;

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

  ProfilerString getCellTextByType(const void *item_data, TableColumn column_type) const
  {
    return getCellText(item_data, static_cast<int>(column_type));
  }

protected:
  eastl::string name;
  float width = 0.0f; // Width in pixels; 0 for auto-sizing
  ImGuiTableColumnFlags flags = 0;
};

using LpDrawHeaderContextMenuCallback = void (*)(LpProfilerTableColumn *column, void *user_data);
using LpItemSelectedCallback = void (*)(int index, void *item_data, void *user_data);
using LpSortItemsCallback = int (*)(const ImGuiTableSortSpecs *sort_specs, const void *a, const void *b, void *user_data);

class LpProfilerTable
{
public:
  LpProfilerTable(const char *table_name, int column_count, LpTableFlags table_flags = LpTableFlags::DEFAULT);
  virtual ~LpProfilerTable();
  void setColumn(int index, LpProfilerTableColumn *column); // Takes ownership
  LpProfilerTableColumn *getColumn(int index) const;        // Returns non-owning pointer

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
  bool isSorted() const { return hasSorted; }
  int getHoveredColumn() const { return hoveredColumnIndex; }
  int getHoveredRow() const { return hoveredRowIndex; }
  const char *getTableID() const { return tableId.c_str(); }
  int getCurrentRowIndex() const { return currentRowIndex; } // Only valid inside drawContent()

  void beginRow();
  bool setColumn(int column_index);
  void endRow();

  void registerSearchBox(TableColumn table_column, LpSearchBox *search_box);

  virtual void openFilterPopup(TableColumn table_column) { filterPopupManager.open(table_column); }
  virtual void drawFilterPopups() { filterPopupManager.drawAll(); }

  LpSearchWidget *registerSearchWidget(TableColumn table_column, const char *name, const char *hint,
    SearchTextChangedCallback on_text_changed_callback, SearchEnterCallback on_enter_callback)
  {
    auto widget = eastl::make_unique<LpSearchWidget>(this, table_column, name, hint, on_text_changed_callback, on_enter_callback);
    LpSearchWidget *widgetPtr = widget.get();
    searchWidgets.push_back(eastl::move(widget));
    return widgetPtr;
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

private:
  eastl::unordered_map<TableColumn, LpSearchBox *> searchBoxes;

  void drawCellCopyMenuItem(const void *item_data, int col_idx, bool &hasCopyOptions);
  void drawCellCopyOptions(const void *item_data, bool &hasCopyOptions);
  void drawRowCopyOption(const void *item_data, bool &hasCopyOptions);
  void drawCustomCopyOptions(const void *item_data);

protected:
  virtual bool isFilterActive(TableColumn /*table_column*/) { return false; }

  GlobalCopyManager *copyManager = nullptr;

  eastl::string tableName;
  eastl::string tableId;
  LpTableFlags flags = LpTableFlags::DEFAULT;
  int columnsCount = 0;
  eastl::vector<eastl::unique_ptr<LpProfilerTableColumn>> columns; // Column definitions (owned)
  const ImGuiTableSortSpecs *sortSpecs = nullptr;                  // Current sort specifications
  bool hasSorted = false;
  bool hasResized = false;
  int hoveredColumnIndex = -1; // Currently hovered column (-1 if none)
  int hoveredRowIndex = -1;    // Currently hovered row (-1 if none)
  int currentRowIndex = -1;    // Row being drawn (in drawContent)

  LpDrawHeaderContextMenuCallback headerContextMenuCallback = nullptr;
  void *headerContextMenuCallbackUserData = nullptr;
  LpItemSelectedCallback itemSelectedCallback = nullptr;
  void *itemSelectedCallbackUserData = nullptr;
  LpSortItemsCallback sortItemsCallback = nullptr;
  void *sortItemsCallbackUserData = nullptr;

  virtual void setupColumns();
  virtual bool handleSorting();
  virtual void handleSelection() {}

  LpFilterPopupManager filterPopupManager;

  eastl::vector<eastl::unique_ptr<LpSearchWidget>> searchWidgets;
};

} // namespace levelprofiler