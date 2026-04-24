// Copyright (C) Gaijin Games KFT.  All rights reserved.

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

// Table freeze parameters
static constexpr int FREEZE_COLUMNS = 0; // No frozen columns
static constexpr int FREEZE_ROWS = 2;    // Freeze header + filter row

// AutoFit parameters
static constexpr int AUTOFIT_QUEUE_VALUE = (1 << 3) - 1; // Fit for 3 frames
static constexpr float FORCE_AUTOFIT_WIDTH = -1.0f;      // Force auto-fit width
static constexpr float MAX_AUTOFIT_WIDTH = 320.0f;       // Maximum width for auto-fit (prevents overly wide columns)

LpProfilerTableColumn::LpProfilerTableColumn(const char *column_name, float width_value, ImGuiTableColumnFlags flags_value) :
  name(column_name ? column_name : "Column"), width(width_value), flags(flags_value)
{}

void LpProfilerTableColumn::setupColumn() const { ImGui::TableSetupColumn(name.c_str(), flags, width); }


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

  if (flags & LpTableFlags::REORDERABLE)
    initializeColumnOrderTracking();
}

LpProfilerTable::~LpProfilerTable()
{
  searchWidgets.clear();
  columns.clear();
}

void LpProfilerTable::setColumn(int index, LpProfilerTableColumn *column)
{
  if (index < 0 || index >= columnsCount || !column)
    return;

  columns[index] = eastl::unique_ptr<LpProfilerTableColumn>(column);
}

LpProfilerTableColumn *LpProfilerTable::getColumn(int index) const
{
  if (index < 0 || index >= columnsCount)
    return nullptr;

  return columns[index].get();
}

void LpProfilerTable::setHeaderContextMenuCallback(LpDrawHeaderContextMenuCallback callback, void *user_data)
{
  headerContextMenuCallback = callback;
  headerContextMenuCallbackUserData = user_data;
}

void LpProfilerTable::setItemSelectedCallback(LpItemSelectedCallback callback, void *user_data)
{
  itemSelectedCallback = callback;
  itemSelectedCallbackUserData = user_data;
}

void LpProfilerTable::setSortItemsCallback(LpSortItemsCallback callback, void *user_data)
{
  sortItemsCallback = callback;
  sortItemsCallbackUserData = user_data;
}

bool LpProfilerTable::draw()
{
  ImGuiTableFlags imguiFlags = 0;

  if (flags & LpTableFlags::SORTABLE)
    imguiFlags |= ImGuiTableFlags_Sortable;
  if (flags & LpTableFlags::RESIZABLE)
    imguiFlags |= ImGuiTableFlags_Resizable;
  if (flags & LpTableFlags::SCROLLABLE)
    imguiFlags |= ImGuiTableFlags_ScrollY;
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

  bool isTableVisible = ImGui::BeginTable(tableId.c_str(), columnsCount, imguiFlags);
  if (!isTableVisible)
    return false;

  hoveredColumnIndex = -1;
  hoveredRowIndex = -1;
  currentRowIndex = -1;
  hasResized = false;
  hasSorted = false;
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

    if (columnIndex < static_cast<int>(columnSearchWidgets.size()))
    {
      if (auto *sw = columnSearchWidgets[columnIndex]; sw)
      {
        sw->draw();
        ImGui::SameLine();
      }
    }

    bool isActive = isFilterActive(columnType);
    char idBuffer[FILTER_ID_BUFFER_SIZE];
    snprintf(idBuffer, FILTER_ID_BUFFER_SIZE, "%s_filt%d", tableName.c_str(), columnIndex);
    LpFilterIndicator indicator(idBuffer);
    indicator.Draw(isActive);
    if (indicator.IsClicked())
    {
      bool hasColumnPopup = (columnIndex < (int)popupIds.size() && hasPopup[columnIndex] && !popupIds[columnIndex].empty());
      if (hasColumnPopup)
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
    if (!hasPopup[col] || popupIds[col].empty())
      continue;
    const char *pid = popupIds[col].c_str();
    if (popupAnchorPositions[col].x != 0.f || popupAnchorPositions[col].y != 0.f)
    {
      if (ImGui::IsPopupOpen(pid))
        ImGui::SetNextWindowPos(popupAnchorPositions[col], ImGuiCond_Appearing, ImVec2(0.0f, 0.0f));
    }
    if (ImGui::BeginPopup(pid))
    {
      if (popupDraw[col].fn)
        popupDraw[col].call();
      ImGui::EndPopup();
    }
  }
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

  if (flags & LpTableFlags::SELECTABLE)
    handleSelection();

  return true;
}

void LpProfilerTable::ensureFilterStorageSized()
{
  if (popupIds.size() == static_cast<size_t>(columnsCount))
    return;
  popupIds.resize(columnsCount);
  popupDraw.resize(columnsCount);
  hasPopup.resize(columnsCount);
  columnSearchWidgets.resize(columnsCount);
  popupAnchorPositions.resize(columnsCount);
  for (int i = 0; i < columnsCount; ++i)
  {
    hasPopup[i] = 0;
    popupIds[i].clear();
    popupDraw[i] = LpCallbackDelegate{};
    columnSearchWidgets[i] = nullptr;
    popupAnchorPositions[i] = ImVec2(0.f, 0.f);
  }
}

void LpProfilerTable::registerColumnFilters(span<const LpColumnFilterSpec> specs)
{
  if (specs.empty())
    return;
  ensureFilterStorageSized();
  for (const auto &spec : specs)
  {
    if (spec.column < 0 || spec.column >= columnsCount)
      continue;
    if (spec.popupId && spec.popupDraw.fn)
    {
      popupIds[spec.column] = spec.popupId;
      popupDraw[spec.column] = spec.popupDraw;
      hasPopup[spec.column] = 1;
    }
    if (spec.searchWidgetId && spec.search.onChange && spec.search.onCommit)
    {
      auto changeLambda = [del = spec.search](const char *txt) { del.onChange(del.userData, txt); };
      auto commitLambda = [del = spec.search]() { del.onCommit(del.userData); };
      LpSearchWidget *widget = registerSearchWidget(spec.column, spec.searchWidgetId,
        spec.searchPlaceholder ? spec.searchPlaceholder : "Search...", changeLambda, commitLambda);
      columnSearchWidgets[spec.column] = widget;
    }
  }
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
      columns[i]->setupColumn();
    else
    {
      char currentColumnName[COLUMN_NAME_BUFFER_SIZE];
      snprintf(currentColumnName, COLUMN_NAME_BUFFER_SIZE, "Column%d", i);
      ImGui::TableSetupColumn(currentColumnName, ImGuiTableColumnFlags_None);
    }
  }
}

bool LpProfilerTable::handleSorting()
{
  if (!(flags & LpTableFlags::SORTABLE))
    return false;

  if (sortSpecs = ImGui::TableGetSortSpecs(); !sortSpecs || !sortSpecs->SpecsDirty)
    return false;

  if (sortItemsCallback)
  {
    hasSorted = true;
    sortItemsCallback(sortSpecs, nullptr, nullptr, sortItemsCallbackUserData);
  }

  // Reset SpecsDirty flag after handling the sort, as per ImGui recommendation.
  if (auto *mutableSortSpecs = const_cast<ImGuiTableSortSpecs *>(sortSpecs))
    mutableSortSpecs->SpecsDirty = false;

  return hasSorted;
}

void LpProfilerTable::beginRow()
{
  ImGui::TableNextRow();
  currentRowIndex++;

  if (ImVec2 minRect = ImGui::GetItemRectMin(), maxRect = ImGui::GetItemRectMax(); ImGui::IsMouseHoveringRect(minRect, maxRect))
    hoveredRowIndex = currentRowIndex;
}

bool LpProfilerTable::setColumn(int column_index) { return ImGui::TableSetColumnIndex(column_index); }

void LpProfilerTable::endRow()
{
  // provides logical pairing with beginRow()
}

void LpProfilerTable::highlightCurrentRow(ImColor highlight_color)
{
  ImVec2 min = ImGui::GetItemRectMin();
  ImVec2 max = ImGui::GetItemRectMax();

  // Ensure the highlight spans the full width of the table row.
  min.x = ImGui::GetWindowPos().x;
  max.x = min.x + ImGui::GetWindowWidth();

  ImGui::GetWindowDrawList()->AddRectFilled(min, max, highlight_color);
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
      ProfilerString cellText = columns[i]->getCellText(item_data, i);
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

ProfilerString LpProfilerTable::generateCustomCopyText(const void * /* item_data */, const ProfilerString & /* menu_item */) const
{
  return ProfilerString{};
}

eastl::vector<ProfilerString> LpProfilerTable::getCustomCopyMenuItems(const void * /* item_data */) const { return {}; }

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
  if (!shouldShowCellCopyOptions())
    return;

  auto essentialColumns = getEssentialColumnIndices();
  bool showOnlyEssential = !essentialColumns.empty();
  bool groupNonEssential = shouldGroupNonEssentialCells();

  // Show essential columns at top level
  if (showOnlyEssential)
  {
    for (int essential_idx : essentialColumns)
      if (essential_idx >= 0 && essential_idx < columnsCount)
        drawCellCopyMenuItem(item_data, essential_idx, hasCopyOptions);
  }

  // Show non-essential columns (grouped or at top level)
  if (groupNonEssential && showOnlyEssential)
  {
    if (ImGui::BeginMenu("Specific Cell"))
    {
      for (int col_idx = 0; col_idx < columnsCount; ++col_idx)
      {
        // Skip essential columns (already shown above)
        if (auto it = eastl::find(essentialColumns.begin(), essentialColumns.end(), col_idx); it == essentialColumns.end())
          drawCellCopyMenuItem(item_data, col_idx, hasCopyOptions);
      }
      ImGui::EndMenu();
    }
  }
  else if (!showOnlyEssential)
  {
    // Show all columns
    for (int col_idx = 0; col_idx < columnsCount; ++col_idx)
      drawCellCopyMenuItem(item_data, col_idx, hasCopyOptions);
  }
}

void LpProfilerTable::drawRowCopyOption(const void *item_data, bool &hasCopyOptions)
{
  if (!shouldShowRowCopyOption())
    return;

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
  if (auto customItems = getCustomCopyMenuItems(item_data); !customItems.empty())
  {
    ImGui::Separator();
    for (const auto &item : customItems)
    {
      if (ImGui::MenuItem(item.c_str()))
      {
        ProfilerString customText = generateCustomCopyText(item_data, item);
        CopyRequest request(CopyType::CONTEXT_CUSTOM);
        request.customData = customText;
        copyManager->executeContextCopy(request);
      }
    }
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
    if (!columnVisibility[columnIndex])
      return false;
  return true;
}

int LpProfilerTable::getVisibleColumnsCount() const
{
  int visibleCount = 0;
  for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
    if (columnVisibility[columnIndex])
      visibleCount++;
  return visibleCount;
}

void LpProfilerTable::ensureAtLeastOneColumnVisible()
{
  bool hasVisibleColumn = false;
  for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
  {
    const bool isEnabled = (ImGui::TableGetColumnFlags(columnIndex) & ImGuiTableColumnFlags_IsEnabled) != 0;
    columnVisibility[columnIndex] = isEnabled;
    if (isEnabled)
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
    clearAllFilters();

  if (hasFilters)
  {
    ImGui::Separator();

    for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
    {
      if (!columns[columnIndex])
        continue;

      if (ColumnIndex columnType = columnIndex; isFilterActive(columnType))
      {
        ProfilerString menuItemLabel;
        menuItemLabel.reserve(128);
        menuItemLabel += "Clear '";
        menuItemLabel += columns[columnIndex]->getName();
        menuItemLabel += "' Filter";

        if (ImGui::Selectable(menuItemLabel.c_str(), false, ImGuiSelectableFlags_NoAutoClosePopups))
          clearColumnFilter(columnType);
      }
    }
  }

  ImGui::EndMenu();
}

void LpProfilerTable::drawColumnOptionsMenu()
{
  if (!ImGui::BeginMenu("Column Options"))
    return;

  if (bool orderModified = isColumnOrderModified(); ImGui::MenuItem("Reset order", nullptr, false, orderModified))
    deferredResetColumnOrder = true;

  if (bool allVisible = areAllColumnsEffectivelyVisible(); ImGui::MenuItem("Unhide all columns", nullptr, false, !allVisible))
    deferredUnhideAllColumns = true;

  ImGui::Separator();

  auto groups = getColumnGroups();
  auto baseCols = getBaseColumnIndices();
  bool hasGroups = !groups.empty();
  bool hasBase = !baseCols.empty();

  if (hasBase)
    drawBaseColumnsVisibility();

  if (hasGroups)
    drawGroupedColumnsVisibility();

  if (!hasBase && !hasGroups)
    drawColumnVisibilityToggles();

  ImGui::EndMenu();
}

void LpProfilerTable::drawColumnVisibilityToggles()
{
  for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
  {
    if (!columns[columnIndex])
      continue;

    const char *columnName = columns[columnIndex]->getName();
    bool isVisible = columnVisibility[columnIndex];

    // Prevent hiding the last visible column (ImGui crash protection)
    bool canToggle = !(isVisible && getVisibleColumnsCount() <= 1);

    ProfilerString menuItemText;
    menuItemText.reserve(256);
    menuItemText += isVisible ? "Hide " : "Unhide ";
    menuItemText += columnName;

    ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_NoAutoClosePopups;
    if (!canToggle)
      selectableFlags |= ImGuiSelectableFlags_Disabled;

    if (ImGui::Selectable(menuItemText.c_str(), false, selectableFlags) && canToggle)
    {
      if (columnIndex >= (int)deferredColumnToggle.size())
        deferredColumnToggle.resize(columnIndex + 1, false);
      deferredColumnToggle[columnIndex] = true;
    }
    if (!canToggle && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
      ImGui::SetTooltip("Cannot hide the last visible column.\nAt least one column must remain visible.");
  }
}

void LpProfilerTable::drawBaseColumnsVisibility()
{
  auto baseCols = getBaseColumnIndices();
  for (int idx : baseCols)
  {
    if (idx < 0 || idx >= columnsCount || !columns[idx])
      continue;
    bool isVisible = columnVisibility[idx];
    const char *columnName = columns[idx]->getName();
    bool canToggle = !(isVisible && getVisibleColumnsCount() <= 1);
    char menuItemText[256];
    snprintf(menuItemText, sizeof(menuItemText), "%s %s", isVisible ? "Hide" : "Unhide", columnName);
    ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_NoAutoClosePopups;
    if (!canToggle)
      selectableFlags |= ImGuiSelectableFlags_Disabled;
    if (ImGui::Selectable(menuItemText, false, selectableFlags) && canToggle)
    {
      if (idx >= (int)deferredColumnToggle.size())
        deferredColumnToggle.resize(idx + 1, false);
      deferredColumnToggle[idx] = true;
    }
    if (!canToggle && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
      ImGui::SetTooltip("Cannot hide the last visible column.\nAt least one column must remain visible.");
  }
}

void LpProfilerTable::drawGroupedColumnsVisibility()
{
  auto groups = getColumnGroups();
  for (const auto &grp : groups)
  {
    if (!ImGui::BeginMenu(grp.name.c_str()))
      continue;
    for (int idx : grp.columnIndices)
    {
      if (idx < 0 || idx >= columnsCount)
        continue;
      if (!columns[idx])
        continue;
      bool isVisible = columnVisibility[idx];
      char label[192];
      snprintf(label, sizeof(label), "%s %s", isVisible ? "Hide" : "Unhide", columns[idx]->getName());
      ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_NoAutoClosePopups;
      // Prevent hiding the last visible column (same protection as elsewhere)
      bool canToggle = !(isVisible && getVisibleColumnsCount() <= 1);
      if (!canToggle)
        selectableFlags |= ImGuiSelectableFlags_Disabled;
      if (ImGui::Selectable(label, false, selectableFlags) && canToggle)
      {
        if (idx >= (int)deferredColumnToggle.size())
          deferredColumnToggle.resize(idx + 1, false);
        deferredColumnToggle[idx] = true;
      }
      if (!canToggle && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Cannot hide the last visible column. At least one must remain.");
    }
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
  if (deferredUnhideAllColumns)
  {
    for (int i = 0; i < columnsCount; ++i)
    {
      ImGui::TableSetColumnEnabled(i, true);
      columnVisibility[i] = true;
    }
    deferredUnhideAllColumns = false;
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
  hoveredColumnIndex = -1;

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

    if (ImGui::IsItemHovered())
    {
      hoveredColumnIndex = columnIndex;
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
      {
        rightClickedColumnIndex = columnIndex;
        rightClickIntercepted = true;
        ImGui::OpenPopup("TableHeaderContextMenu");
      }
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

} // namespace levelprofiler