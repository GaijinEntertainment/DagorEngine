// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "levelProfilerTableBase.h"
#include "levelProfilerFilter.h"
#include "textureModule.h"

namespace levelprofiler
{

class TextureProfilerUI;

struct TextureData;
struct TextureUsage;

// Represents a single item (row) in the texture table.
struct TextureTableItem
{
  const ProfilerString *name;
  const TextureData *data;
  const TextureUsage *textureUsage; // Texture usage information (can be null)
  bool isSelected;

  TextureTableItem(const ProfilerString &name_ref, const TextureData &data_ref, const TextureUsage *texture_usage_ptr = nullptr,
    bool is_selected = false) :
    name(&name_ref), data(&data_ref), textureUsage(texture_usage_ptr), isSelected(is_selected)
  {}
};


// Base class for columns in the texture table.
class LpTextureTableColumn : public LpProfilerTableColumn
{
public:
  LpTextureTableColumn(const char *name, TableColumn column_type, float width = 0.0f, ImGuiTableColumnFlags flags = 0);
  virtual ~LpTextureTableColumn() = default;
  TableColumn getColumnType() const { return columnType; }
  int compareItems(const void *item1, const void *item2) const override;

  ProfilerString getCellText(const void *item_data, int column_index) const override;

  void drawCell(const void *item_data) const override;

protected:
  virtual ProfilerString getCellTextForItem(const TextureTableItem *item) const = 0;
  virtual void drawCellForItem(const TextureTableItem *item) const = 0;

  TableColumn columnType = COLUMN_NONE;
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

  bool isFilterActive(TableColumn column) override;

  void openFilterPopup(TableColumn column) override;
  void drawFilterPopups() override;

private:
  TextureProfilerUI *profilerUI = nullptr;
  ProfilerString selectedTextureName;

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
    return {COL_NAME}; // Name shown at top level, others in submenu
  }
  bool shouldGroupNonEssentialCells() const override { return true; }

  int compareTextureItems(const void *item1, const void *item2, const ImGuiTableSortSpecs *sort_specs) const;
  static int sort_table_items_callback(const ImGuiTableSortSpecs *sort_specs, const void *a, const void *b, void *user_data);
  static void on_item_selected(int index, void *item_data, void *user_data);
};

} // namespace levelprofiler