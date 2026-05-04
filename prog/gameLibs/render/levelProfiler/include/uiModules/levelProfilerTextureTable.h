// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unordered_map.h>
#include "levelProfilerTableBase.h"
#include "levelProfilerFilter.h"
#include "textureModule.h"

namespace levelprofiler
{

class TextureProfilerUI;

struct TextureData;
struct TextureUsage;

struct TextureTableItem
{
  ProfilerString name;
  const TextureData *data = nullptr;
  const TextureUsage *textureUsage = nullptr;
  bool isSelected = false;

  float memorySizeMB = 0.0f;
  int usageUniqueCount = 0;
  ProfilerString usageLabel;
  ProfilerString assetNamesList;

  TextureTableItem() = default;
  TextureTableItem(const ProfilerString &name_ref, const TextureData &data_ref, const TextureUsage *usage_ptr, bool selected) :
    name(name_ref), data(&data_ref), textureUsage(usage_ptr), isSelected(selected)
  {}
};


// Base class for columns in the texture table.
class LpTextureTableColumn : public LpProfilerTableColumn
{
public:
  LpTextureTableColumn(const char *name, TextureColumn column_type, float width = 0.0f, ImGuiTableColumnFlags flags = 0);
  virtual ~LpTextureTableColumn() = default;
  TextureColumn getColumnType() const { return columnType; }
  int compareItems(const void *item1, const void *item2) const override;

  ProfilerString getCellText(const void *item_data, int column_index) const override;

  void drawCell(const void *item_data) const override;

protected:
  virtual ProfilerString getCellTextForItem(const TextureTableItem *item) const = 0;
  virtual void drawCellForItem(const TextureTableItem *item) const = 0;

  TextureColumn columnType = TextureColumn::NONE;
};


class LpTextureNameColumn : public LpTextureTableColumn
{
public:
  LpTextureNameColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const TextureTableItem *item) const override;
  void drawCellForItem(const TextureTableItem *item) const override;
};


class LpTextureFormatColumn : public LpTextureTableColumn
{
public:
  LpTextureFormatColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const TextureTableItem *item) const override;
  void drawCellForItem(const TextureTableItem *item) const override;
};


class LpTextureWidthColumn : public LpTextureTableColumn
{
public:
  LpTextureWidthColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const TextureTableItem *item) const override;
  void drawCellForItem(const TextureTableItem *item) const override;
};


class LpTextureHeightColumn : public LpTextureTableColumn
{
public:
  LpTextureHeightColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const TextureTableItem *item) const override;
  void drawCellForItem(const TextureTableItem *item) const override;
};


class LpTextureMipsColumn : public LpTextureTableColumn
{
public:
  LpTextureMipsColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const TextureTableItem *item) const override;
  void drawCellForItem(const TextureTableItem *item) const override;
};


class LpTextureSizeColumn : public LpTextureTableColumn
{
public:
  LpTextureSizeColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const TextureTableItem *item) const override;
  void drawCellForItem(const TextureTableItem *item) const override;
};


class LpTextureUsageColumn : public LpTextureTableColumn
{
public:
  LpTextureUsageColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

  void drawTextureUsageChip(const ProfilerString &texture_name) const;

protected:
  ProfilerString getCellTextForItem(const TextureTableItem *item) const override;
  void drawCellForItem(const TextureTableItem *item) const override;

private:
  mutable ProfilerString cachedSharedUsageChipLabel;
};


class LpTextureTable : public LpProfilerTable
{
public:
  LpTextureTable(TextureProfilerUI *profiler_ui_ptr);
  virtual ~LpTextureTable();

  void setProfilerUI(TextureProfilerUI *profiler_ui_ptr) { profilerUI = profiler_ui_ptr; }
  const ProfilerString &getSelectedTexture() const { return selectedTextureName; }
  void setSelectedTexture(const ProfilerString &texture_name);

  void drawContent() override;
  void handleSelection() override;

  void sortFilteredTextures();
  void sortFilteredTextures() const;
  void buildDisplayOrderedNames(eastl::vector<ProfilerString> &out) const;

  bool isFilterActive(ColumnIndex column) const override;

  int getColumnsCount() const { return columnsCount; }
  LpProfilerTableColumn *getColumn(int index) const { return LpProfilerTable::getColumn(index); }
  TextureProfilerUI *getProfilerUI() const { return profilerUI; }

protected:
  void clearAllFilters() override;
  void clearColumnFilter(ColumnIndex column) override;

private:
  TextureProfilerUI *profilerUI = nullptr;
  ProfilerString selectedTextureName;
  mutable eastl::vector<TextureTableItem> cachedItems;
  mutable eastl::vector<int> displayOrder;
  mutable eastl::unordered_map<ProfilerString, int> nameToCacheIndex;

  void rebuildCache() const;
  void ensureDisplayOrder() const;
  void sortDisplayOrder() const;

  void drawNameFilterContent();
  void drawFormatFilterContent();
  void drawWidthFilterContent();
  void drawHeightFilterContent();
  void drawMipFilterContent();
  void drawMemorySizeFilterContent();
  void drawTextureUsageFilterContent();

  eastl::vector<ProfilerString> getCustomCopyMenuItems(const void *item_data) const override;
  ProfilerString generateCustomCopyText(const void *item_data, const ProfilerString &menu_item) const override;
  ProfilerString generateUsageDetailsText(const TextureTableItem *item) const;
  ProfilerString generateFullRowWithUsageText(const TextureTableItem *item) const;
  ProfilerString getAssetNamesForTexture(const ProfilerString &texture_name) const;

  bool shouldShowCellCopyOptions() const override { return true; }
  bool shouldShowRowCopyOption() const override { return false; }
  eastl::vector<int> getEssentialColumnIndices() const override
  {
    return {*TextureColumn::NAME}; // Name shown at top level, others in submenu
  }
  bool shouldGroupNonEssentialCells() const override { return true; }
  eastl::vector<int> getBaseColumnIndices() const override
  {
    static_assert(static_cast<int>(TextureColumn::NAME) == 0, "Texture base columns must start at 0");
    static_assert(static_cast<int>(TextureColumn::BASE_COUNT) > static_cast<int>(TextureColumn::NAME),
      "Unexpected texture column enum ordering / sentinel position");
    eastl::vector<int> indices;
    indices.reserve(static_cast<int>(TextureColumn::BASE_COUNT));
    for (int i = 0; i < static_cast<int>(TextureColumn::BASE_COUNT); ++i)
      indices.push_back(i);
    return indices;
  }

  int compareTextureItems(const void *item1, const void *item2, const ImGuiTableSortSpecs *sort_specs) const;
  static int sort_table_items_callback(const ImGuiTableSortSpecs *sort_specs, const void *a, const void *b, void *user_data);
  static void on_item_selected(int index, void *item_data, void *user_data);

  void onNameSearchChange(const char *text);
  void onNameSearchCommit();
  void onUsageSearchChange(const char *text);
  void onUsageSearchCommit();
};

} // namespace levelprofiler