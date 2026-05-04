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
static constexpr int FIRST_COL_INDEX = 0;
static constexpr int INVALID_INDEX = -1;

// Column default widths (in pixels)
static constexpr float COL_NAME_WIDTH = 180.0f;
static constexpr float COL_FORMAT_WIDTH = 60.0f;
static constexpr float COL_WIDTH_WIDTH = 60.0f;
static constexpr float COL_HEIGHT_WIDTH = 60.0f;
static constexpr float COL_MIPS_WIDTH = 60.0f;
static constexpr float COL_MEM_SIZE_WIDTH = 80.0f;
static constexpr float COL_TEX_USAGE_WIDTH = 180.0f;

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

namespace
{
static int safeCStrCompare(const char *a, const char *b)
{
  if (!a && !b)
    return UIConstants::EQUAL;
  if (!a)
    return UIConstants::LESS_THAN;
  if (!b)
    return UIConstants::GREATER_THAN;
  int r = strcmp(a, b);
  if (r < 0)
    return UIConstants::LESS_THAN;
  if (r > 0)
    return UIConstants::GREATER_THAN;
  return UIConstants::EQUAL;
}
} // anonymous namespace

// --- LpTextureTableColumn implementation ---

LpTextureTableColumn::LpTextureTableColumn(const char *name, TextureColumn column_type, float width, ImGuiTableColumnFlags flags) :
  LpProfilerTableColumn(name, width, flags), columnType(column_type)
{}

int LpTextureTableColumn::compareItems(const void *item1, const void *item2) const
{
  const TextureTableItem *a = static_cast<const TextureTableItem *>(item1);
  const TextureTableItem *b = static_cast<const TextureTableItem *>(item2);
  if (!a || !b)
    return 0;

  switch (columnType)
  {
    case TextureColumn::NAME: return a->name.compare(b->name);
    case TextureColumn::FORMAT:
      if (!a->data || !b->data)
        return UIConstants::EQUAL;
      {
        const char *fmtA = TextureModule::getFormatName(a->data->info.cflg);
        const char *fmtB = TextureModule::getFormatName(b->data->info.cflg);
        return safeCStrCompare(fmtA, fmtB);
      }
    case TextureColumn::WIDTH:
      if (!a->data || !b->data)
        return UIConstants::EQUAL;
      return a->data->info.w - b->data->info.w;
    case TextureColumn::HEIGHT:
      if (!a->data || !b->data)
        return UIConstants::EQUAL;
      return a->data->info.h - b->data->info.h;
    case TextureColumn::MIPS:
      if (!a->data || !b->data)
        return UIConstants::EQUAL;
      return a->data->info.mipLevels - b->data->info.mipLevels;
    case TextureColumn::MEM_SIZE:
      return (a->memorySizeMB < b->memorySizeMB)
               ? UIConstants::LESS_THAN
               : (a->memorySizeMB > b->memorySizeMB ? UIConstants::GREATER_THAN : UIConstants::EQUAL);
    case TextureColumn::USAGE: return a->usageUniqueCount - b->usageUniqueCount;
    default: return UIConstants::EQUAL;
  }
}

ProfilerString LpTextureTableColumn::getCellText(const void *item_data, int /* column_index */) const
{
  const TextureTableItem *currentItem = static_cast<const TextureTableItem *>(item_data);
  if (!currentItem || !currentItem->data)
    return ProfilerString{};

  return getCellTextForItem(currentItem);
}

void LpTextureTableColumn::drawCell(const void *item_data) const
{
  const TextureTableItem *currentItem = static_cast<const TextureTableItem *>(item_data);
  if (!currentItem || !currentItem->data)
  {
    ImGui::TextColored(UIConstants::ERROR_COLOR, "ERROR");
    return;
  }

  drawCellForItem(currentItem);
}

// --- LpTextureNameColumn implementation ---

LpTextureNameColumn::LpTextureNameColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Name", TextureColumn::NAME, width > 0 ? width : UIConstants::COL_NAME_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort)
{}

ProfilerString LpTextureNameColumn::getCellTextForItem(const TextureTableItem *item) const { return item->name; }

void LpTextureNameColumn::drawCellForItem(const TextureTableItem *item) const
{
  ImGui::Selectable(item->name.c_str(), item->isSelected, ImGuiSelectableFlags_SpanAllColumns);
}

// --- LpTextureFormatColumn implementation ---

LpTextureFormatColumn::LpTextureFormatColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Format", TextureColumn::FORMAT, width > 0 ? width : UIConstants::COL_FORMAT_WIDTH,
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
  LpTextureTableColumn("Width", TextureColumn::WIDTH, width > 0 ? width : UIConstants::COL_WIDTH_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpTextureWidthColumn::getCellTextForItem(const TextureTableItem *item) const
{
  return eastl::to_string(item->data->info.w);
}

void LpTextureWidthColumn::drawCellForItem(const TextureTableItem *item) const { ImGui::Text("%d", item->data->info.w); }

// --- LpTextureHeightColumn implementation ---

LpTextureHeightColumn::LpTextureHeightColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Height", TextureColumn::HEIGHT, width > 0 ? width : UIConstants::COL_HEIGHT_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpTextureHeightColumn::getCellTextForItem(const TextureTableItem *item) const
{
  return eastl::to_string(item->data->info.h);
}

void LpTextureHeightColumn::drawCellForItem(const TextureTableItem *item) const { ImGui::Text("%d", item->data->info.h); }

// --- LpTextureMipsColumn implementation ---

LpTextureMipsColumn::LpTextureMipsColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Mips", TextureColumn::MIPS, width > 0 ? width : UIConstants::COL_MIPS_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpTextureMipsColumn::getCellTextForItem(const TextureTableItem *item) const
{
  return eastl::to_string(item->data->info.mipLevels);
}

void LpTextureMipsColumn::drawCellForItem(const TextureTableItem *item) const { ImGui::Text("%d", item->data->info.mipLevels); }

// --- LpTextureSizeColumn implementation ---

LpTextureSizeColumn::LpTextureSizeColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Memory size", TextureColumn::MEM_SIZE, width > 0 ? width : UIConstants::COL_MEM_SIZE_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpTextureSizeColumn::getCellTextForItem(const TextureTableItem *item) const
{
  if (!item || !item->data)
    return ProfilerString();
  return eastl::to_string(item->memorySizeMB) + " MB";
}

void LpTextureSizeColumn::drawCellForItem(const TextureTableItem *item) const
{
  if (!item || !item->data)
  {
    ImGui::TextColored(UIConstants::ERROR_COLOR, "N/A");
    return;
  }
  ImGui::Text("%.2f MB", item->memorySizeMB);
}

// --- LpTextureUsageColumn implementation ---

LpTextureUsageColumn::LpTextureUsageColumn(float width, ImGuiTableColumnFlags flags) :
  LpTextureTableColumn("Usage", TextureColumn::USAGE, width > 0 ? width : UIConstants::COL_TEX_USAGE_WIDTH,
    flags | ImGuiTableColumnFlags_WidthFixed)
{}

ProfilerString LpTextureUsageColumn::getCellTextForItem(const TextureTableItem *item) const
{
  return item ? item->usageLabel : ProfilerString();
}

void LpTextureUsageColumn::drawCellForItem(const TextureTableItem *item) const
{
  if (!item)
  {
    ImGui::TextColored(UIConstants::ERROR_COLOR, "N/A");
    return;
  }
  const int count = item->usageUniqueCount;
  const bool uniq = count == 1;
  const bool shared = count > 1;
  const char *label = item->usageLabel.c_str();
  ImVec4 bg = UIConstants::TRANSPARENT_COLOR;
  ImVec4 hov = UIConstants::TRANSPARENT_COLOR;
  ImVec4 txt = UIConstants::GRAY_TINT_COLOR;
  if (uniq)
  {
    bg = UIConstants::ORANGE_BG;
    hov = UIConstants::ORANGE_HOVER;
    txt = UIConstants::WHITE_COLOR;
  }
  else if (shared)
  {
    bg = UIConstants::BLUE_BG;
    hov = UIConstants::BLUE_HOVER;
    txt = UIConstants::WHITE_COLOR;
  }
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, UIConstants::FRAME_ROUNDING);
  ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertFloat4ToU32(bg));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertFloat4ToU32(hov));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertFloat4ToU32(bg));
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertFloat4ToU32(txt));
  ImGui::Button(label, ImVec2(0, 0));
  ImGui::PopStyleColor(UIConstants::STYLE_COLOR_COUNT);
  ImGui::PopStyleVar();
}

// --- LpTextureTable implementation ---

LpTextureTable::LpTextureTable(TextureProfilerUI *profiler_ui_ptr) :
  LpProfilerTable("Textures", *TextureColumn::BASE_COUNT), profilerUI(profiler_ui_ptr)
{
  createColumn<LpTextureNameColumn>(*TextureColumn::NAME);
  createColumn<LpTextureFormatColumn>(*TextureColumn::FORMAT);
  createColumn<LpTextureWidthColumn>(*TextureColumn::WIDTH);
  createColumn<LpTextureHeightColumn>(*TextureColumn::HEIGHT);
  createColumn<LpTextureMipsColumn>(*TextureColumn::MIPS);
  createColumn<LpTextureSizeColumn>(*TextureColumn::MEM_SIZE);
  createColumn<LpTextureUsageColumn>(*TextureColumn::USAGE);

  eastl::array<LpColumnFilterSpec, 7> specs = {
    LpColumnFilterSpec{*TextureColumn::NAME, "NamePop", makeDelegate<LpTextureTable, &LpTextureTable::drawNameFilterContent>(this),
      "NameSearch", "Search...",
      makeSearchDelegate<LpTextureTable, &LpTextureTable::onNameSearchChange, &LpTextureTable::onNameSearchCommit>(this)},
    LpColumnFilterSpec{*TextureColumn::FORMAT, "FmtPop", makeDelegate<LpTextureTable, &LpTextureTable::drawFormatFilterContent>(this)},
    LpColumnFilterSpec{*TextureColumn::WIDTH, "WPop", makeDelegate<LpTextureTable, &LpTextureTable::drawWidthFilterContent>(this)},
    LpColumnFilterSpec{*TextureColumn::HEIGHT, "HPop", makeDelegate<LpTextureTable, &LpTextureTable::drawHeightFilterContent>(this)},
    LpColumnFilterSpec{*TextureColumn::MIPS, "MipPop", makeDelegate<LpTextureTable, &LpTextureTable::drawMipFilterContent>(this)},
    LpColumnFilterSpec{
      *TextureColumn::MEM_SIZE, "MemSzPop", makeDelegate<LpTextureTable, &LpTextureTable::drawMemorySizeFilterContent>(this)},
    LpColumnFilterSpec{*TextureColumn::USAGE, "TexUsagePop",
      makeDelegate<LpTextureTable, &LpTextureTable::drawTextureUsageFilterContent>(this), "UsageSearch", "Search...",
      makeSearchDelegate<LpTextureTable, &LpTextureTable::onUsageSearchChange, &LpTextureTable::onUsageSearchCommit>(this)}};

  registerColumnFilters(specs);

  setSortItemsCallback(&LpTextureTable::sort_table_items_callback, this);
  setItemSelectedCallback(&LpTextureTable::on_item_selected, this);

  if (flags & LpTableFlags::REORDERABLE && trackColumnOrder)
  {
    eastl::vector<int> initialOrder = {*TextureColumn::NAME, *TextureColumn::FORMAT, *TextureColumn::WIDTH, *TextureColumn::HEIGHT,
      *TextureColumn::MIPS, *TextureColumn::MEM_SIZE, *TextureColumn::USAGE};
    getInitialColumnOrder() = initialOrder;
    getCurrentColumnOrder() = initialOrder;
    columnOrderWasModified = false;
  }
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
  rebuildCache();
  if (isSortActive())
    sortFilteredTextures();
  ensureDisplayOrder();

  if (cachedItems.empty())
  {
    ImGui::TableNextRow();
    if (ImGui::TableSetColumnIndex(UIConstants::FIRST_COL_INDEX))
      ImGui::TextColored(UIConstants::WARNING_COLOR, "No textures match current filters");
    return;
  }

  for (int visualRow = 0; visualRow < static_cast<int>(displayOrder.size()); ++visualRow)
  {
    int idx = displayOrder[visualRow];
    if (idx < 0 || idx >= static_cast<int>(cachedItems.size()))
      continue;
    TextureTableItem &item = cachedItems[idx];
    item.isSelected = (item.name == selectedTextureName);
    beginRow();
    for (int col = 0; col < columnsCount; ++col)
    {
      if (!setColumn(col))
        continue;
      const auto &colPtr = columns[col];
      if (!colPtr)
        continue;
      ImGui::PushID(currentRowIndex * columnsCount + col);
      colPtr->drawCell(&item);
      if (col == UIConstants::FIRST_COL_INDEX && ImGui::IsItemClicked())
      {
        if (itemSelectedCallback)
          itemSelectedCallback(currentRowIndex, &item, itemSelectedCallbackUserData);
        selectedTextureName = item.name;
      }
      LpProfilerTable::drawRowContextMenu(currentRowIndex, col, &item);
      ImGui::PopID();
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

void LpTextureTable::sortFilteredTextures() const
{
  if (!profilerUI || !sortSpecs || sortSpecs->SpecsCount == 0)
    return;
  ensureDisplayOrder();
  auto comparator = [this](int lhs, int rhs) {
    const TextureTableItem &A = cachedItems[lhs];
    const TextureTableItem &B = cachedItems[rhs];
    int delta = compareTextureItems(&A, &B, sortSpecs);
    if (delta != 0)
      return delta < 0;
    return A.name < B.name;
  };
  eastl::stable_sort(displayOrder.begin(), displayOrder.end(), comparator);
}

void LpTextureTable::sortFilteredTextures() { static_cast<const LpTextureTable *>(this)->sortFilteredTextures(); }
void LpTextureTable::rebuildCache() const
{
  if (!profilerUI)
  {
    cachedItems.clear();
    displayOrder.clear();
    return;
  }
  TextureModule *texModule = profilerUI->getTextureModule();
  if (!texModule)
  {
    cachedItems.clear();
    displayOrder.clear();
    return;
  }
  RIModule *riModule = profilerUI->getRIModule();

  const auto &filteredNames = texModule->getFilteredTextures();
  const auto &allTextures = texModule->getTextures();
  static const eastl::unordered_map<ProfilerString, TextureUsage> EMPTY_USAGE_MAP;
  static const eastl::unordered_map<ProfilerString, eastl::vector<ProfilerString>> EMPTY_ASSETS_MAP;
  const auto &usageMap = riModule ? riModule->getTextureUsage() : EMPTY_USAGE_MAP;
  const auto &texToAssets = riModule ? riModule->getTextureToAssetsMap() : EMPTY_ASSETS_MAP;

  cachedItems.clear();
  nameToCacheIndex.clear();
  cachedItems.reserve(filteredNames.size());

  for (const auto &name : filteredNames)
  {
    auto it = allTextures.find(name);
    if (it == allTextures.end())
      continue;
    const TextureData &td = it->second;
    const char *fmt = TextureModule::getFormatName(td.info.cflg);
    if (!fmt)
      continue;
    const TextureUsage *usagePtr = nullptr;
    int uniqueCount = 0;
    if (riModule)
    {
      auto uit = usageMap.find(name);
      if (uit != usageMap.end())
      {
        usagePtr = &uit->second;
        uniqueCount = uit->second.unique;
      }
    }
    TextureTableItem item(it->first, td, usagePtr, false);
    item.memorySizeMB = texModule->getTextureMemorySize(td);
    item.usageUniqueCount = uniqueCount;
    if (uniqueCount == 0)
      item.usageLabel = "Non-RI";
    else if (uniqueCount == 1)
      item.usageLabel = "Unique";
    else
      item.usageLabel = "Shared(" + eastl::to_string(uniqueCount) + ")";
    if (riModule)
    {
      auto ait = texToAssets.find(name);
      if (ait != texToAssets.end() && !ait->second.empty())
      {
        for (size_t i = 0; i < ait->second.size(); ++i)
        {
          if (i)
            item.assetNamesList += ", ";
          item.assetNamesList += ait->second[i];
        }
      }
    }
    if (item.assetNamesList.empty())
      item.assetNamesList = "Non-RI";
    cachedItems.push_back(eastl::move(item));
    nameToCacheIndex[name] = static_cast<int>(cachedItems.size() - 1);
  }
  displayOrder.clear();
}

void LpTextureTable::ensureDisplayOrder() const
{
  if (displayOrder.size() != cachedItems.size())
  {
    displayOrder.resize(cachedItems.size());
    for (size_t i = 0; i < cachedItems.size(); ++i)
      displayOrder[i] = static_cast<int>(i);
  }
}

void LpTextureTable::sortDisplayOrder() const { sortFilteredTextures(); }

void LpTextureTable::buildDisplayOrderedNames(eastl::vector<ProfilerString> &out) const
{
  rebuildCache();
  if (isSortActive())
    sortFilteredTextures();
  ensureDisplayOrder();
  out.clear();
  out.reserve(displayOrder.size());
  for (int idx : displayOrder)
    if (idx >= 0 && idx < static_cast<int>(cachedItems.size()))
      out.push_back(cachedItems[idx].name);
}

void LpTextureTable::onNameSearchChange(const char *text)
{
  if (!profilerUI)
    return;
  profilerUI->getFilterManager().setNameSearch(text);
}
void LpTextureTable::onNameSearchCommit()
{
  if (!profilerUI)
    return;
  profilerUI->getFilterManager().applyFilters();
}
void LpTextureTable::onUsageSearchChange(const char *text)
{
  if (!profilerUI)
    return;
  profilerUI->getFilterManager().setTextureUsageSearch(text);
}
void LpTextureTable::onUsageSearchCommit()
{
  if (!profilerUI)
    return;
  profilerUI->getFilterManager().applyFilters();
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

  LpRangeFilterController<int> mipCtrl(&filter);
  if (drawRangeFilter<int>("PopupMip", "Levels", mipCtrl, "Min: %d", "Max: %d", UIConstants::MIPS_FILTER_SPEED))
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

  LpRangeFilterController<float> sizeCtrl(&filter);
  if (drawRangeFilter<float>("PopupSize", "Memory size", sizeCtrl, "Min: %.2f MB", "Max: %.2f MB", UIConstants::MEM_SIZE_FILTER_SPEED))
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
    filter.reset(UIConstants::SHARED_TEX_MIN_DEFAULT, static_cast<float>(maxTextureUsage));

    if (riModule)
    {
      auto &instanceFilter = fm.getAssetInstanceCountRangeFilter();
      int maxInstanceCount = riModule->getMaxAssetInstanceCount();
      if (maxInstanceCount > 0)
        instanceFilter.reset(0, maxInstanceCount);
    }

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

  ImGui::BeginDisabled(!shared); // Range filter is only relevant if "Shared" is active.
  LpRangeFilterController<int> usageCtrl(&filter);
  if (drawRangeFilter<int>("PopupTexUsageRange", "", usageCtrl, "Min: %d", "Max: %d", UIConstants::TEX_USAGE_FILTER_SPEED))
    fm.applyFilters();

  ImGui::EndDisabled();

  if (unique || shared)
  {
    ImGui::Separator();
    ImGui::Text("RI name filter:");
    auto &comp = fm.getTextureUsageFilterComponent();
    if (comp.drawInline())
      fm.applyFilters();

    ImGui::Separator();
    ImGui::Text("Asset instance count:");
    auto &instanceFilter = fm.getAssetInstanceCountRangeFilter();

    if (riModule)
    {
      int maxInstanceCount = riModule->getMaxAssetInstanceCount();
      if (maxInstanceCount > 0 && static_cast<int>(instanceFilter.getAbsoluteMax()) != maxInstanceCount)
        instanceFilter.setAbsoluteBounds(0, maxInstanceCount);
    }

    LpRangeFilterController<int> instanceCtrl(&instanceFilter);
    if (drawRangeFilter<int>("PopupAssetInstanceCount", "", instanceCtrl, "Min: %d", "Max: %d", UIConstants::TEX_USAGE_FILTER_SPEED))
      fm.applyFilters();
  }
}

bool LpTextureTable::isFilterActive(ColumnIndex column) const
{
  return profilerUI && profilerUI->getFilterManager().isColumnFilterActive(column);
}

int LpTextureTable::compareTextureItems(const void *item1, const void *item2, const ImGuiTableSortSpecs *sort_specs) const
{
  if (!sort_specs || !item1 || !item2 || sort_specs->SpecsCount == 0)
    return 0;

  const TextureTableItem *texItem1 = static_cast<const TextureTableItem *>(item1);
  const TextureTableItem *texItem2 = static_cast<const TextureTableItem *>(item2);

  if (!texItem1 || !texItem2)
    return 0;

  for (int i = 0; i < sort_specs->SpecsCount; i++)
  {
    const ImGuiTableColumnSortSpecs *spec = &sort_specs->Specs[i];
    if (!spec)
      continue;
    int columnIdx = spec->ColumnIndex;
    if (columnIdx < 0 || columnIdx >= columnsCount)
      continue;
    if (!columns[columnIdx])
      continue;
    const LpTextureTableColumn *column = static_cast<const LpTextureTableColumn *>(columns[columnIdx].get());
    if (!column)
      continue;
    if ((column->getColumnType() != TextureColumn::NAME) && (!texItem1->data || !texItem2->data))
      continue;
    int delta = column->compareItems(item1, item2);
    if (delta != 0)
      return spec->SortDirection == ImGuiSortDirection_Ascending ? delta : -delta;
  }
  return texItem1->name.compare(texItem2->name);
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

  table->setSelectedTexture(item->name);
  table->profilerUI->getTextureModule()->selectTexture(item->name.c_str());
}

eastl::vector<ProfilerString> LpTextureTable::getCustomCopyMenuItems(const void *item_data) const
{
  const TextureTableItem *item = static_cast<const TextureTableItem *>(item_data);
  if (!item)
    return {};

  eastl::vector<ProfilerString> items;
  items.push_back("Usage details");
  items.push_back("Full row with usage details");

  return items;
}

ProfilerString LpTextureTable::generateCustomCopyText(const void *item_data, const ProfilerString &menu_item) const
{
  const TextureTableItem *item = static_cast<const TextureTableItem *>(item_data);
  if (!item || !item->data)
    return ProfilerString{};

  if (menu_item == "Usage details")
    return generateUsageDetailsText(item);
  else if (menu_item == "Full row with usage details")
    return generateFullRowWithUsageText(item);

  return ProfilerString{};
}

ProfilerString LpTextureTable::generateUsageDetailsText(const TextureTableItem *item) const
{
  if (!item)
    return ProfilerString();
  return item->assetNamesList;
}

ProfilerString LpTextureTable::generateFullRowWithUsageText(const TextureTableItem *item) const
{
  if (!item || !item->data)
    return ProfilerString();
  ProfilerString info;
  info += "Texture: " + item->name + "\n";
  info += "Format: " + ProfilerString(TextureModule::getFormatName(item->data->info.cflg)) + "\n";
  info += "Dimensions: " + eastl::to_string(item->data->info.w) + "x" + eastl::to_string(item->data->info.h) + "\n";
  info += "Mip levels: " + eastl::to_string(item->data->info.mipLevels) + "\n";
  info += "Memory size: " + eastl::to_string(item->memorySizeMB) + " MB\n";
  info += "Usage: " + item->assetNamesList;

  return info;
}

void LpTextureTable::clearAllFilters()
{
  if (!profilerUI)
    return;

  auto &filterManager = profilerUI->getFilterManager();
  filterManager.resetAllFilters();
  if (auto *w = getSearchWidget(*TextureColumn::NAME))
    w->clear();
  if (auto *w = getSearchWidget(*TextureColumn::USAGE))
    w->clear();

  filterManager.applyFilters();
}

void LpTextureTable::clearColumnFilter(ColumnIndex column)
{
  if (!profilerUI)
    return;

  auto &filterManager = profilerUI->getFilterManager();

  switch (column)
  {
    case *TextureColumn::NAME:
      filterManager.getNameFilterComponent().reset();
      if (auto *w = getSearchWidget(column))
        w->clear();

      break;
    case *TextureColumn::FORMAT: filterManager.getFormatFilter().reset(); break;

    case *TextureColumn::WIDTH: filterManager.getWidthFilter().reset(); break;

    case *TextureColumn::HEIGHT: filterManager.getHeightFilter().reset(); break;

    case *TextureColumn::MIPS: filterManager.getMipRangeFilter().reset(); break;

    case *TextureColumn::MEM_SIZE: filterManager.getSizeRangeFilter().reset(); break;

    case *TextureColumn::USAGE:
      filterManager.getTextureUsageFilterComponent().reset();
      filterManager.getTextureUsageRangeFilter().reset();
      filterManager.getAssetInstanceCountRangeFilter().reset();
      filterManager.setTextureUsageFilters(true, true, true);
      if (auto *w = getSearchWidget(column))
        w->clear();
      return;
    default: break;
  }

  filterManager.applyFilters();
}

} // namespace levelprofiler