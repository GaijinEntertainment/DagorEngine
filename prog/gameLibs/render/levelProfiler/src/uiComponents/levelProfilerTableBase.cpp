// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "levelProfilerTableBase.h"
#include "levelProfilerFilter.h"
#include "levelProfilerExporter.h"

namespace levelprofiler
{

// Buffer sizes for string formatting
static constexpr int TABLE_ID_BUFFER_SIZE = 128;
static constexpr int COLUMN_NAME_BUFFER_SIZE = 64;
static constexpr int FILTER_ID_BUFFER_SIZE = 64;

// Table freeze parameters
static constexpr int FREEZE_COLUMNS = 0; // No frozen columns
static constexpr int FREEZE_ROWS = 2;    // Freeze header + filter row

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
}

LpProfilerTable::~LpProfilerTable()
{
  // searchWidgets and columns automatically destroyed by eastl::unique_ptr
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

void LpProfilerTable::registerSearchBox(TableColumn table_column, LpSearchBox *search_box) { searchBoxes[table_column] = search_box; }

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

  if (flags & LpTableFlags::FREEZE_HEADER)
    ImGui::TableSetupScrollFreeze(FREEZE_COLUMNS, FREEZE_ROWS);

  setupColumns();

  ImGui::TableHeadersRow();

  // Second row: filter indicators (frozen with header)
  ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

  for (int columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
  {
    ImGui::TableSetColumnIndex(columnIndex);

    TableColumn columnType = static_cast<TableColumn>(columnIndex);
    auto iterator = searchBoxes.find(columnType);
    if (iterator != searchBoxes.end())
    {
      iterator->second->draw();
      ImGui::SameLine(); // Keep filter indicator on the same line if search box is present.
    }

    bool isActive = isFilterActive(columnType);
    char idBuffer[FILTER_ID_BUFFER_SIZE];
    snprintf(idBuffer, FILTER_ID_BUFFER_SIZE, "%s_filt%d", tableName.c_str(), columnIndex);
    LpFilterIndicator indicator(idBuffer);
    indicator.Draw(isActive);

    if (indicator.IsClicked())
      filterPopupManager.open(columnType);
  }

  handleSorting();

  drawContent();

  ImGui::EndTable();

  filterPopupManager.drawAll();

  if (flags & LpTableFlags::SELECTABLE)
    handleSelection();

  return true;
}

void LpProfilerTable::setupColumns()
{
  for (int i = 0; i < columnsCount; i++)
  {
    if (columns[i])
      columns[i]->setupColumn();

    else
    {
      // Default empty column if not defined
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
  if (ImGuiTableSortSpecs *mutableSortSpecs = const_cast<ImGuiTableSortSpecs *>(sortSpecs))
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

} // namespace levelprofiler