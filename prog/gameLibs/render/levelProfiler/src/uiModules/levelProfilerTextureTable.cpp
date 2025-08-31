// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/sort.h>
#include "levelProfilerUI.h"
#include "levelProfilerWidgets.h"
#include "riModule.h"
#include "levelProfilerTextureTable.h"

namespace levelprofiler
{
namespace UIConstants
{
// Table configuration
static constexpr int TABLE_COLUMN_COUNT = 7;
static constexpr int FIRST_COL_INDEX = 0;
static constexpr int INVALID_INDEX = -1;
static constexpr int FIRST_TAB_INDEX = 0;

// Column default widths (in pixels)
static constexpr float COL_FORMAT_WIDTH = 60.0f;
static constexpr float COL_WIDTH_WIDTH = 55.0f;
static constexpr float COL_HEIGHT_WIDTH = 55.0f;
static constexpr float COL_MIPS_WIDTH = 45.0f;
static constexpr float COL_MEM_SIZE_WIDTH = 100.0f;
static constexpr float COL_TEX_USAGE_WIDTH = 320.0f;

// UI styling
static constexpr float FRAME_ROUNDING = 8.0f;
static constexpr int STYLE_COLOR_COUNT = 4;

// Filter widget speeds
static constexpr float MEM_SIZE_FILTER_SPEED = 0.1f;
static constexpr float MIPS_FILTER_SPEED = 0.25f;
static constexpr float TEX_USAGE_FILTER_SPEED = 1.0f;

// Default values
static constexpr int DEFAULT_MAX_TEX_USAGE = 100; // Default to 100 if riModule is null for safety
static constexpr int SHARED_TEX_MIN_DEFAULT = 2;

// Color constants for UI
static const ImVec4 ERROR_COLOR = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
static const ImVec4 WARNING_COLOR = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
static const ImVec4 GRAY_TINT_COLOR = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
static const ImVec4 WHITE_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
static const ImVec4 TRANSPARENT_COLOR = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

// Usage chip colors
static const ImVec4 ORANGE_BG = ImVec4(1.0f, 0.4f, 0.0f, 0.25f);
static const ImVec4 ORANGE_HOVER = ImVec4(0.9f, 0.5f, 0.1f, 0.25f);
static const ImVec4 BLUE_BG = ImVec4(0.2f, 0.6f, 1.0f, 0.25f);
static const ImVec4 BLUE_HOVER = ImVec4(0.3f, 0.7f, 0.9f, 0.25f);

// Comparison result constants
static constexpr int LESS_THAN = -1;
static constexpr int EQUAL = 0;
static constexpr int GREATER_THAN = 1;
} // namespace UIConstants

// --- LpTextureTableColumn implementation ---

LpTextureTableColumn::LpTextureTableColumn(const char *name, TableColumn column_type, float width, ImGuiTableColumnFlags flags) :
  LpProfilerTableColumn(name, width, flags), columnType(column_type)
{}

int LpTextureTableColumn::compareItems(const void *item1, const void *item2) const
{
  const TextureTableItem *textureItem1 = static_cast<const TextureTableItem *>(item1);
  const TextureTableItem *textureItem2 = static_cast<const TextureTableItem *>(item2);

  if (!textureItem1 || !textureItem1->name || !textureItem1->data || !textureItem2 || !textureItem2->name || !textureItem2->data)
    return 0;

  switch (columnType)
  {
    case COL_NAME: return textureItem1->name->compare(*textureItem2->name);

    case COL_FORMAT:
      return strcmp(TextureModule::getFormatName(textureItem1->data->info.cflg),
        TextureModule::getFormatName(textureItem2->data->info.cflg));

    case COL_WIDTH: return textureItem1->data->info.w - textureItem2->data->info.w;

    case COL_HEIGHT: return textureItem1->data->info.h - textureItem2->data->info.h;

    case COL_MIPS: return textureItem1->data->info.mipLevels - textureItem2->data->info.mipLevels;

    case COL_MEM_SIZE:
    {
      // Texture size calculation requires access to TextureModule.
      auto profilerUserInterface =
        static_cast<TextureProfilerUI *>(ILevelProfiler::getInstance()->getTab(UIConstants::FIRST_TAB_INDEX)->module);
      auto texModule = profilerUserInterface->getTextureModule();

      float difference = texModule->getTextureMemorySize(*textureItem1->data) - texModule->getTextureMemorySize(*textureItem2->data);
      return (difference < 0) ? UIConstants::LESS_THAN : (difference > 0 ? UIConstants::GREATER_THAN : UIConstants::EQUAL);
    }

    case COL_TEX_USAGE:
    {
      // Compare texture usage counts (null texture usages come first)
      if (!textureItem1->textureUsage && !textureItem2->textureUsage)
        return UIConstants::EQUAL;
      if (!textureItem1->textureUsage)
        return UIConstants::LESS_THAN;
      if (!textureItem2->textureUsage)
        return UIConstants::GREATER_THAN;
      return textureItem1->textureUsage->unique - textureItem2->textureUsage->unique;
    }

    default: return UIConstants::EQUAL;
  }
}

ProfilerString LpTextureTableColumn::getCellText(const void *item_data, int /* column_index */) const
{
  const TextureTableItem *currentItem = static_cast<const TextureTableItem *>(item_data);
  if (!currentItem || !currentItem->name || !currentItem->data)
    return ProfilerString{};

  return getCellTextForItem(currentItem);
}

void LpTextureTableColumn::drawCell(const void *item_data) const
{
  const TextureTableItem *currentItem = static_cast<const TextureTableItem *>(item_data);
  if (!currentItem || !currentItem->name || !currentItem->data)
  {
    ImGui::TextColored(UIConstants::ERROR_COLOR, "ERROR");
    return;
  }

  drawCellForItem(currentItem);
}


// --- LpTextureNameColumn implementation ---

LpTextureNameColumn::LpTextureNameColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Name", COL_NAME, width, flags | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort)
{}

ProfilerString LpTextureNameColumn::getCellTextForItem(const TextureTableItem *item) const { return *item->name; }

void LpTextureNameColumn::drawCellForItem(const TextureTableItem *item) const
{
  ImGui::Selectable(item->name->c_str(), item->isSelected, ImGuiSelectableFlags_SpanAllColumns);
}


// --- LpTextureFormatColumn implementation ---

LpTextureFormatColumn::LpTextureFormatColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Format", COL_FORMAT, width > 0 ? width : UIConstants::COL_FORMAT_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpTextureFormatColumn::getCellTextForItem(const TextureTableItem *item) const
{
  return ProfilerString(TextureModule::getFormatName(item->data->info.cflg));
}

void LpTextureFormatColumn::drawCellForItem(const TextureTableItem *item) const
{
  ImGui::Text("%s", TextureModule::getFormatName(item->data->info.cflg));
}

// --- LpTextureWidthColumn implementation ---

LpTextureWidthColumn::LpTextureWidthColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Width", COL_WIDTH, width > 0 ? width : UIConstants::COL_WIDTH_WIDTH, flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpTextureWidthColumn::getCellTextForItem(const TextureTableItem *item) const
{
  return eastl::to_string(item->data->info.w);
}

void LpTextureWidthColumn::drawCellForItem(const TextureTableItem *item) const { ImGui::Text("%d", item->data->info.w); }

// --- LpTextureHeightColumn implementation ---

LpTextureHeightColumn::LpTextureHeightColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Height", COL_HEIGHT, width > 0 ? width : UIConstants::COL_HEIGHT_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpTextureHeightColumn::getCellTextForItem(const TextureTableItem *item) const
{
  return eastl::to_string(item->data->info.h);
}

void LpTextureHeightColumn::drawCellForItem(const TextureTableItem *item) const { ImGui::Text("%d", item->data->info.h); }

// --- LpTextureMipsColumn implementation ---

LpTextureMipsColumn::LpTextureMipsColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Mips", COL_MIPS, width > 0 ? width : UIConstants::COL_MIPS_WIDTH, flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpTextureMipsColumn::getCellTextForItem(const TextureTableItem *item) const
{
  return eastl::to_string(item->data->info.mipLevels);
}

void LpTextureMipsColumn::drawCellForItem(const TextureTableItem *item) const { ImGui::Text("%d", item->data->info.mipLevels); }

// --- LpTextureSizeColumn implementation ---

LpTextureSizeColumn::LpTextureSizeColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Memory size", COL_MEM_SIZE, width > 0 ? width : UIConstants::COL_MEM_SIZE_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpTextureSizeColumn::getCellTextForItem(const TextureTableItem *item) const
{
  auto profilerUI = static_cast<TextureProfilerUI *>(ILevelProfiler::getInstance()->getTab(UIConstants::FIRST_TAB_INDEX)->module);
  float sizeMB = profilerUI->getTextureModule()->getTextureMemorySize(*item->data);
  return eastl::to_string(sizeMB) + " MB";
}

void LpTextureSizeColumn::drawCellForItem(const TextureTableItem *item) const
{
  auto profilerUserInterface =
    static_cast<TextureProfilerUI *>(ILevelProfiler::getInstance()->getTab(UIConstants::FIRST_TAB_INDEX)->module);
  auto texModule = profilerUserInterface->getTextureModule();

  float sizeMB = texModule->getTextureMemorySize(*item->data);

  ImGui::Text("%.2f MB", sizeMB);
}

// --- LpTextureUsageColumn implementation ---

LpTextureUsageColumn::LpTextureUsageColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Usage", COL_TEX_USAGE, width > 0 ? width : UIConstants::COL_TEX_USAGE_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpTextureUsageColumn::getCellTextForItem(const TextureTableItem *item) const
{
  if (!item->textureUsage || item->textureUsage->unique == 0)
    return "Non-RI";
  else if (item->textureUsage->unique == 1)
    return "Unique";
  else
    return "Shared(" + eastl::to_string(item->textureUsage->unique) + ")";
}

void LpTextureUsageColumn::drawCellForItem(const TextureTableItem *item) const { drawTextureUsageChip(*item->name); }

void LpTextureUsageColumn::drawTextureUsageChip(const ProfilerString &texture_name) const
{
  auto profilerUserInterface =
    static_cast<TextureProfilerUI *>(ILevelProfiler::getInstance()->getTab(UIConstants::FIRST_TAB_INDEX)->module);
  auto renderInstanceModule = profilerUserInterface->getRIModule();

  const auto &textureUsageMap = renderInstanceModule->getTextureUsage();


  const char *labelText;
  ImVec4 backgroundColor, backgroundHoverColor, textColorValue;

  if (auto textureUsageIterator = textureUsageMap.find(texture_name);
      textureUsageIterator == textureUsageMap.end() || textureUsageIterator->second.unique == 0)
  {
    // Non-RI texture
    labelText = "Non-RI";
    backgroundColor = UIConstants::TRANSPARENT_COLOR;
    backgroundHoverColor = UIConstants::TRANSPARENT_COLOR;
    textColorValue = UIConstants::GRAY_TINT_COLOR; // Gray tint
  }
  else if (textureUsageIterator->second.unique == 1)
  {
    // Unique texture (used by only one asset)
    labelText = "Unique";
    backgroundColor = UIConstants::ORANGE_BG;
    backgroundHoverColor = UIConstants::ORANGE_HOVER;
    textColorValue = UIConstants::WHITE_COLOR;
  }
  else
  {
    // Shared texture (used by multiple assets)
    cachedSharedUsageChipLabel.sprintf("Shared(%d)", textureUsageIterator->second.unique);
    labelText = cachedSharedUsageChipLabel.c_str();
    backgroundColor = UIConstants::BLUE_BG;
    backgroundHoverColor = UIConstants::BLUE_HOVER;
    textColorValue = UIConstants::WHITE_COLOR;
  }

  // Draw chip with rounded corners
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, UIConstants::FRAME_ROUNDING);
  ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertFloat4ToU32(backgroundColor));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertFloat4ToU32(backgroundHoverColor));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertFloat4ToU32(backgroundColor));
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertFloat4ToU32(textColorValue));

  ImGui::Button(labelText, ImVec2(0, 0));

  ImGui::PopStyleColor(UIConstants::STYLE_COLOR_COUNT);
  ImGui::PopStyleVar();
}

// --- LpTextureTable implementation ---

LpTextureTable::LpTextureTable(TextureProfilerUI *profiler_ui_ptr) :
  LpProfilerTable("Textures", UIConstants::TABLE_COLUMN_COUNT), profilerUI(profiler_ui_ptr)
{
  createColumn<LpTextureNameColumn>(COL_NAME);
  createColumn<LpTextureFormatColumn>(COL_FORMAT);
  createColumn<LpTextureWidthColumn>(COL_WIDTH);
  createColumn<LpTextureHeightColumn>(COL_HEIGHT);
  createColumn<LpTextureMipsColumn>(COL_MIPS);
  createColumn<LpTextureSizeColumn>(COL_MEM_SIZE);
  createColumn<LpTextureUsageColumn>(COL_TEX_USAGE);

  // Register search boxes for text filtering
  auto &filterManagerInstance = profilerUI->getFilterManager();

  registerSearchWidget(
    COL_NAME, "NameSearch", "Search...", [&](const char *search_text) { filterManagerInstance.setNameSearch(search_text); },
    [&]() { filterManagerInstance.applyFilters(); });

  registerSearchWidget(
    COL_TEX_USAGE, "UsageSearch", "Search...",
    [&](const char *search_text) { filterManagerInstance.setTextureUsageSearch(search_text); },
    [&]() { filterManagerInstance.applyFilters(); });

  filterPopupManager.registerPopup(COL_NAME, "NamePop", [this]() { drawNameFilterContent(); });
  filterPopupManager.registerPopup(COL_FORMAT, "FmtPop", [this]() { drawFormatFilterContent(); });
  filterPopupManager.registerPopup(COL_WIDTH, "WPop", [this]() { drawWidthFilterContent(); });
  filterPopupManager.registerPopup(COL_HEIGHT, "HPop", [this]() { drawHeightFilterContent(); });
  filterPopupManager.registerPopup(COL_MIPS, "MipPop", [this]() { drawMipFilterContent(); });
  filterPopupManager.registerPopup(COL_MEM_SIZE, "MemSzPop", [this]() { drawMemorySizeFilterContent(); });
  filterPopupManager.registerPopup(COL_TEX_USAGE, "UsagePop", [this]() { drawTextureUsageFilterContent(); });

  setSortItemsCallback(&LpTextureTable::sort_table_items_callback, this);
  setItemSelectedCallback(&LpTextureTable::on_item_selected, this);
}

LpTextureTable::~LpTextureTable()
{
  // Base class handles cleanup of columns and widgets
}

void LpTextureTable::setSelectedTexture(const ProfilerString &texture_name) { selectedTextureName = texture_name; }

void LpTextureTable::drawContent()
{
  if (!profilerUI)
    return;

  auto texModule = profilerUI->getTextureModule();
  auto renderInstanceModule = profilerUI->getRIModule();

  const auto &currentFilteredTextures = texModule->getFilteredTextures();
  const auto &allTextures = texModule->getTextures();
  const auto &textureUsageMap = renderInstanceModule->getTextureUsage();

  if (currentFilteredTextures.empty())
  {
    ImGui::TableNextRow();
    if (ImGui::TableSetColumnIndex(UIConstants::FIRST_COL_INDEX))
      ImGui::TextColored(UIConstants::WARNING_COLOR, "No textures match current filters");

    return;
  }

  for (const auto &currentTextureName : currentFilteredTextures)
  {
    auto textureIterator = allTextures.find(currentTextureName);
    if (textureIterator == allTextures.end())
    {
      // Texture not found - should never happen but handle gracefully
      beginRow();
      if (setColumn(COL_NAME))
      {
        ImGui::TextColored(UIConstants::ERROR_COLOR, "ERROR: %s", currentTextureName.c_str());
      }
      endRow();
      continue;
    }

    const TextureData &currentTextureData = textureIterator->second;
    const TextureUsage *currentTextureUsage = nullptr;

    if (auto currentTextureUsageIterator = textureUsageMap.find(currentTextureName);
        currentTextureUsageIterator != textureUsageMap.end())
      currentTextureUsage = &currentTextureUsageIterator->second;

    TextureTableItem currentTableItem(currentTextureName, currentTextureData, currentTextureUsage,
      currentTextureName == selectedTextureName);

    beginRow();

    // Draw each column cell
    for (int columnIndex = 0, n = columnsCount; columnIndex < n; columnIndex++)
    {
      if (const auto &currentColumn = columns[columnIndex]; setColumn(columnIndex) && currentColumn)
      {
        ImGui::PushID(currentRowIndex * columnsCount + columnIndex);

        currentColumn->drawCell(&currentTableItem);

        if (columnIndex == UIConstants::FIRST_COL_INDEX && ImGui::IsItemClicked())
        {
          if (itemSelectedCallback)
            itemSelectedCallback(currentRowIndex, &currentTableItem, itemSelectedCallbackUserData);

          selectedTextureName = currentTextureName;
        }

        LpProfilerTable::drawRowContextMenu(currentRowIndex, columnIndex, &currentTableItem);

        ImGui::PopID();
      }
    }

    endRow();
  }
}

void LpTextureTable::handleSelection()
{
  if (!profilerUI || !ImGui::IsMouseClicked(ImGuiMouseButton_Left) || hoveredRowIndex <= UIConstants::INVALID_INDEX)
    return;

  auto texModule = profilerUI->getTextureModule();
  const auto &filtered = texModule->getFilteredTextures();

  if (hoveredRowIndex >= static_cast<int>(filtered.size()))
    return;

  const ProfilerString &name = filtered[hoveredRowIndex];
  const auto &allTextures = texModule->getTextures();
  auto it = allTextures.find(name);
  if (it == allTextures.end())
    return;

  const TextureUsage *textureUsagePtr = nullptr;
  if (profilerUI->getRIModule())
  {
    auto &textureUsageMap = profilerUI->getRIModule()->getTextureUsage();
    if (auto uit = textureUsageMap.find(name); uit != textureUsageMap.end())
      textureUsagePtr = &uit->second;
  }

  TextureTableItem item(name, it->second, textureUsagePtr, false);
  on_item_selected(hoveredRowIndex, &item, this);
}

void LpTextureTable::sortFilteredTextures()
{
  if (!profilerUI)
    return;

  if (!sortSpecs || sortSpecs->SpecsCount == 0)
    return;

  auto textureModule = profilerUI->getTextureModule();
  auto riModule = profilerUI->getRIModule();

  const auto &textures = textureModule->getTextures();
  const auto &texUsage = riModule->getTextureUsage();
  auto &filteredTextures = const_cast<eastl::vector<ProfilerString> &>(textureModule->getFilteredTextures());

  auto compareFunc = [this, &textures, &texUsage](const ProfilerString &a, const ProfilerString &b) -> bool {
    auto itA = textures.find(a);
    auto itB = textures.find(b);

    if (itA == textures.end() || itB == textures.end())
      return a < b; // Fallback to string comparison

    const TextureUsage *textureUsageA = nullptr;
    if (auto textureUsageItA = texUsage.find(a); textureUsageItA != texUsage.end())
      textureUsageA = &textureUsageItA->second;

    const TextureUsage *textureUsageB = nullptr;
    if (auto textureUsageItB = texUsage.find(b); textureUsageItB != texUsage.end())
      textureUsageB = &textureUsageItB->second;

    TextureTableItem itemA(a, itA->second, textureUsageA);
    TextureTableItem itemB(b, itB->second, textureUsageB);

    return compareTextureItems(&itemA, &itemB, sortSpecs) < 0;
  };

  eastl::sort(filteredTextures.begin(), filteredTextures.end(), compareFunc);
}

void LpTextureTable::openFilterPopup(TableColumn column)
{
  switch (column)
  {
    case COL_NAME: ImGui::OpenPopup("NamePop"); break;
    case COL_FORMAT: ImGui::OpenPopup("FmtPop"); break;
    case COL_WIDTH: ImGui::OpenPopup("WPop"); break;
    case COL_HEIGHT: ImGui::OpenPopup("HPop"); break;
    case COL_MIPS: ImGui::OpenPopup("MipPop"); break;
    case COL_MEM_SIZE: ImGui::OpenPopup("MemSzPop"); break;
    case COL_TEX_USAGE: ImGui::OpenPopup("TexUsagePop"); break;
    default: break; // Should not happen with valid TableColumn values.
  }
}

void LpTextureTable::drawFilterPopups()
{
  if (ImGui::BeginPopup("NamePop"))
  {
    drawNameFilterContent();
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup("FmtPop"))
  {
    drawFormatFilterContent();
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup("WPop"))
  {
    drawWidthFilterContent();
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup("HPop"))
  {
    drawHeightFilterContent();
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup("MipPop"))
  {
    drawMipFilterContent();
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup("MemSzPop"))
  {
    drawMemorySizeFilterContent();
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup("TexUsagePop"))
  {
    drawTextureUsageFilterContent();
    ImGui::EndPopup();
  }
}

void LpTextureTable::drawNameFilterContent()
{
  if (!profilerUI)
    return;

  auto &fm = profilerUI->getFilterManager();
  auto &filter = fm.getNameFilterComponent();

  // Draw include/exclude UI
  if (filter.drawInline())
    fm.applyFilters();
}

void LpTextureTable::drawFormatFilterContent()
{
  if (!profilerUI)
    return;

  auto &fm = profilerUI->getFilterManager();
  eastl::vector<ProfilerString> formats = fm.getUniqueFormats();
  bool changed = false;

  LpMultiSelectWidget<ProfilerString> widget(MultiSelectLabelFuncs::ProfilerStringLabel);
  widget.Draw(formats, fm.getFormatFilter(), changed);

  if (changed)
    fm.applyFilters();
}

void LpTextureTable::drawWidthFilterContent()
{
  if (!profilerUI)
    return;

  auto &fm = profilerUI->getFilterManager();
  eastl::vector<int> widths = fm.getUniqueWidths();
  bool changed = false;

  LpMultiSelectWidget<int> widget(MultiSelectLabelFuncs::IntLabelWithOther);
  widget.Draw(widths, fm.getWidthFilter(), changed);

  if (changed)
    fm.applyFilters();
}

void LpTextureTable::drawHeightFilterContent()
{
  if (!profilerUI)
    return;

  auto &fm = profilerUI->getFilterManager();
  eastl::vector<int> heights = fm.getUniqueHeights();
  bool changed = false;

  LpMultiSelectWidget<int> widget(MultiSelectLabelFuncs::IntLabelWithOther);
  widget.Draw(heights, fm.getHeightFilter(), changed);

  if (changed)
    fm.applyFilters();
}

void LpTextureTable::drawMipFilterContent()
{
  if (!profilerUI)
    return;

  auto &fm = profilerUI->getFilterManager();
  auto textureModule = profilerUI->getTextureModule();
  auto &filter = fm.getMipRangeFilter();

  int defaultMin = textureModule->getMipMinDefault();
  int defaultMax = textureModule->getMipMaxDefault();
  filter.setAbsoluteBounds(defaultMin, defaultMax);

  if (levelprofiler::LpRangeFilterWidget<int>::Draw("PopupMip", // ID
        "Levels",                                               // Label
        filter,                                                 // Filter object
        "Min: %d",                                              // Min format
        "Max: %d",                                              // Max format
        UIConstants::MIPS_FILTER_SPEED                          // Speed
        ))
    fm.applyFilters();
}

void LpTextureTable::drawMemorySizeFilterContent()
{
  if (!profilerUI)
    return;

  auto &fm = profilerUI->getFilterManager();
  auto textureModule = profilerUI->getTextureModule();
  auto &filter = fm.getSizeRangeFilter();

  float defaultMin = textureModule->getMemorySizeMinDefault();
  float defaultMax = textureModule->getMemorySizeMaxDefault();
  filter.setAbsoluteBounds(defaultMin, defaultMax);

  if (levelprofiler::LpRangeFilterWidget<float>::Draw("PopupSize", // ID
        "Memory size",                                             // Label
        filter,                                                    // Filter object
        "Min: %.2f MB",                                            // Min format
        "Max: %.2f MB",                                            // Max format
        UIConstants::MEM_SIZE_FILTER_SPEED                         // Speed
        ))
    fm.applyFilters();
}

void LpTextureTable::drawTextureUsageFilterContent()
{
  if (!profilerUI)
    return;

  auto &fm = profilerUI->getFilterManager();
  auto riModule = profilerUI->getRIModule();
  int maxTextureUsage = riModule ? riModule->getMaxUniqueTextureUsageCount() : UIConstants::DEFAULT_MAX_TEX_USAGE;
  auto &filter = fm.getTextureUsageRangeFilter();

  if (static_cast<int>(filter.getAbsoluteMax()) != maxTextureUsage)
    filter.setAbsoluteBounds(filter.getAbsoluteMin(), static_cast<float>(maxTextureUsage));

  if (ImGui::Button("Clear"))
  {
    fm.setTextureUsageFilters(true, true, true);
    filter.reset(UIConstants::SHARED_TEX_MIN_DEFAULT, static_cast<float>(maxTextureUsage)); // Reset range to sensible defaults,
                                                                                            // min 2 for shared.
    fm.applyFilters();
  }

  ImGui::Separator();

  bool nonRI = fm.getTextureUsageFilterNonRI();
  bool unique = fm.getTextureUsageFilterUnique();
  bool shared = fm.getTextureUsageFilterShared();

  if (ImGui::Checkbox("Non-RI", &nonRI))
    fm.setTextureUsageFilters(nonRI, unique, shared);
  if (ImGui::Checkbox("Unique", &unique))
    fm.setTextureUsageFilters(nonRI, unique, shared);
  if (ImGui::Checkbox("Shared", &shared))
    fm.setTextureUsageFilters(nonRI, unique, shared);

  ImGui::Separator();
  ImGui::Text("Shared range:");

  ImGui::BeginDisabled(!shared);                                          // Range filter is only relevant if "Shared" is active.
  if (levelprofiler::LpRangeFilterWidget<int>::Draw("PopupTexUsageRange", // ID
        "",                                                               // Label
        filter,                                                           // Filter object
        "Min: %d",                                                        // Min format
        "Max: %d",                                                        // Max format
        UIConstants::TEX_USAGE_FILTER_SPEED                               // Speed
        ))
    fm.applyFilters();

  ImGui::EndDisabled();

  if (unique || shared)
  {
    ImGui::Separator();
    ImGui::Text("RI name filter:");
    auto &comp = fm.getTextureUsageFilterComponent();
    if (comp.drawInline())
      fm.applyFilters();
  }
}

bool LpTextureTable::isFilterActive(TableColumn column)
{
  return profilerUI && profilerUI->getFilterManager().isColumnFilterActive(column);
}

int LpTextureTable::compareTextureItems(const void *item1, const void *item2, const ImGuiTableSortSpecs *sort_specs) const
{
  if (!sort_specs || !item1 || !item2 || sort_specs->SpecsCount == 0)
    return 0;

  for (int i = 0; i < sort_specs->SpecsCount; i++)
  {
    const ImGuiTableColumnSortSpecs *spec = &sort_specs->Specs[i];
    int columnIdx = spec->ColumnIndex;

    if (columnIdx < UIConstants::INVALID_INDEX || columnIdx >= columnsCount || !columns[columnIdx])
      continue;

    const LpTextureTableColumn *column = static_cast<const LpTextureTableColumn *>(columns[columnIdx].get());
    int delta = column->compareItems(item1, item2);

    if (delta != 0)
      return spec->SortDirection == ImGuiSortDirection_Ascending ? delta : -delta;
  }

  return 0;
}

int LpTextureTable::sort_table_items_callback(const ImGuiTableSortSpecs *sort_specs, const void *a, const void *b, void *user_data)
{
  LpTextureTable *table = static_cast<LpTextureTable *>(user_data);
  if (table)
    return table->compareTextureItems(a, b, sort_specs);
  return 0;
}

void LpTextureTable::on_item_selected(int /* index */, void *item_data, void *user_data)
{
  // Selection callback handler
  TextureTableItem *item = static_cast<TextureTableItem *>(item_data);
  LpTextureTable *table = static_cast<LpTextureTable *>(user_data);

  table->setSelectedTexture(*item->name);
  table->profilerUI->getTextureModule()->selectTexture(item->name->c_str());
}

eastl::vector<ProfilerString> LpTextureTable::getCustomCopyMenuItems(const void *item_data) const
{
  const TextureTableItem *item = static_cast<const TextureTableItem *>(item_data);
  if (!item || !item->name)
    return {};

  eastl::vector<ProfilerString> items;
  items.push_back("Usage details");
  items.push_back("Full row with usage details");

  return items;
}

ProfilerString LpTextureTable::generateCustomCopyText(const void *item_data, const ProfilerString &menu_item) const
{
  const TextureTableItem *item = static_cast<const TextureTableItem *>(item_data);
  if (!item || !item->name || !item->data)
    return ProfilerString{};

  if (menu_item == "Usage details")
    return generateUsageDetailsText(item);
  else if (menu_item == "Full row with usage details")
    return generateFullRowWithUsageText(item);

  return ProfilerString{};
}

ProfilerString LpTextureTable::generateUsageDetailsText(const TextureTableItem *item) const
{
  if (!item->textureUsage || item->textureUsage->unique == 0)
    return "Non-RI";

  return getAssetNamesForTexture(*item->name);
}

ProfilerString LpTextureTable::generateFullRowWithUsageText(const TextureTableItem *item) const
{
  ProfilerString info;
  info += "Texture: " + *item->name + "\n";
  info += "Format: " + ProfilerString(TextureModule::getFormatName(item->data->info.cflg)) + "\n";
  info += "Dimensions: " + eastl::to_string(item->data->info.w) + "x" + eastl::to_string(item->data->info.h) + "\n";
  info += "Mip levels: " + eastl::to_string(item->data->info.mipLevels) + "\n";

  auto textureProfilerUI =
    static_cast<TextureProfilerUI *>(ILevelProfiler::getInstance()->getTab(UIConstants::FIRST_TAB_INDEX)->module);
  float sizeMB = textureProfilerUI->getTextureModule()->getTextureMemorySize(*item->data);
  info += "Memory size: " + eastl::to_string(sizeMB) + " MB\n";

  if (!item->textureUsage || item->textureUsage->unique == 0)
    info += "Usage: Non-RI";
  else
    info += "Usage: " + getAssetNamesForTexture(*item->name);

  return info;
}

ProfilerString LpTextureTable::getAssetNamesForTexture(const ProfilerString &texture_name) const
{
  auto textureProfilerUI =
    static_cast<TextureProfilerUI *>(ILevelProfiler::getInstance()->getTab(UIConstants::FIRST_TAB_INDEX)->module);
  auto renderInstanceModule = textureProfilerUI->getRIModule();
  const auto &textureToAssetsMap = renderInstanceModule->getTextureToAssetsMap();

  if (auto assetsIt = textureToAssetsMap.find(texture_name); assetsIt != textureToAssetsMap.end() && !assetsIt->second.empty())
  {
    const auto &assetNames = assetsIt->second;
    ProfilerString result;

    for (size_t i = 0; i < assetNames.size(); ++i)
    {
      if (i > 0)
        result += ", ";
      result += assetNames[i];
    }
    return result;
  }

  return "Non-RI";
}

} // namespace levelprofiler