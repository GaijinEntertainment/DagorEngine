// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "levelProfilerTableBase.h"
#include "levelProfilerFilter.h"
#include "riModule.h"

namespace levelprofiler
{

struct RiTableItem
{
  const RiData *data;
  bool isSelected;

  RiTableItem(const RiData &data_ref, bool is_selected = false) : data(&data_ref), isSelected(is_selected) {}
};

class LpRiTableColumn : public LpProfilerTableColumn
{
public:
  LpRiTableColumn(const char *name, RiColumn column_type, float width = 0.0f, ImGuiTableColumnFlags flags = 0);
  virtual ~LpRiTableColumn() = default;

  RiColumn getColumnType() const { return columnType; }
  int compareItems(const void *item1, const void *item2) const override;
  ProfilerString getCellText(const void *item_data, int column_index) const override;
  void drawCell(const void *item_data) const override;

protected:
  virtual ProfilerString getCellTextForItem(const RiTableItem *item) const = 0;
  virtual void drawCellForItem(const RiTableItem *item) const = 0;

  RiColumn columnType = RiColumn::NONE;
};

class LpRiNameColumn : public LpRiTableColumn
{
public:
  LpRiNameColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const RiTableItem *item) const override;
  void drawCellForItem(const RiTableItem *item) const override;
};

class LpRiCountColumn : public LpRiTableColumn
{
public:
  LpRiCountColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const RiTableItem *item) const override;
  void drawCellForItem(const RiTableItem *item) const override;
};

class LpRiBSphereColumn : public LpRiTableColumn
{
public:
  LpRiBSphereColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const RiTableItem *item) const override;
  void drawCellForItem(const RiTableItem *item) const override;
};

class LpRiBBoxColumn : public LpRiTableColumn
{
public:
  LpRiBBoxColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const RiTableItem *item) const override;
  void drawCellForItem(const RiTableItem *item) const override;
};

class LpRiLodDipsColumn : public LpRiTableColumn
{
public:
  LpRiLodDipsColumn(int lod_index, float width = 0.0f, ImGuiTableColumnFlags flags = 0);
  int compareItems(const void *item1, const void *item2) const override;

protected:
  ProfilerString getCellTextForItem(const RiTableItem *item) const override;
  void drawCellForItem(const RiTableItem *item) const override;

private:
  int lodIndex;
};

class LpRiLodTrisColumn : public LpRiTableColumn
{
public:
  LpRiLodTrisColumn(int lod_index, float width = 0.0f, ImGuiTableColumnFlags flags = 0);
  int compareItems(const void *item1, const void *item2) const override;

protected:
  ProfilerString getCellTextForItem(const RiTableItem *item) const override;
  void drawCellForItem(const RiTableItem *item) const override;

private:
  int lodIndex;
};

class LpRiPhysTrisColumn : public LpRiTableColumn
{
public:
  LpRiPhysTrisColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const RiTableItem *item) const override;
  void drawCellForItem(const RiTableItem *item) const override;
};

class LpRiTraceTrisColumn : public LpRiTableColumn
{
public:
  LpRiTraceTrisColumn(float width = 0.0f, ImGuiTableColumnFlags flags = 0);

protected:
  ProfilerString getCellTextForItem(const RiTableItem *item) const override;
  void drawCellForItem(const RiTableItem *item) const override;
};

class LpRiLodDistColumn : public LpRiTableColumn
{
public:
  LpRiLodDistColumn(int lod_index, float width = 0.0f, ImGuiTableColumnFlags flags = 0);
  int compareItems(const void *item1, const void *item2) const override;

protected:
  ProfilerString getCellTextForItem(const RiTableItem *item) const override;
  void drawCellForItem(const RiTableItem *item) const override;

private:
  int lodIndex;
};

class LpRiLodScreenColumn : public LpRiTableColumn
{
public:
  LpRiLodScreenColumn(int lod_index, float width = 0.0f, ImGuiTableColumnFlags flags = 0);
  int compareItems(const void *item1, const void *item2) const override;

protected:
  ProfilerString getCellTextForItem(const RiTableItem *item) const override;
  void drawCellForItem(const RiTableItem *item) const override;

private:
  int lodIndex;
};

class LpRiHeavyShadersColumn : public LpRiTableColumn
{
public:
  LpRiHeavyShadersColumn(int lod_index, float width = 0.0f, ImGuiTableColumnFlags flags = 0);
  int compareItems(const void *item1, const void *item2) const override;

protected:
  ProfilerString getCellTextForItem(const RiTableItem *item) const override;
  void drawCellForItem(const RiTableItem *item) const override;

private:
  int lodIndex;
};

class LpRiTable : public LpProfilerTable
{
public:
  LpRiTable(RIModule *ri_module_ptr);
  virtual ~LpRiTable();

  void setRIModule(RIModule *ri_module_ptr) { riModule = ri_module_ptr; }
  const ProfilerString &getSelectedRi() const { return selectedRiName; }
  void setSelectedRi(const ProfilerString &ri_name);

  void drawContent() override;
  void handleSelection() override;
  void sortFilteredRiItems();

  size_t getFilteredRiCount() const { return filteredCount; }
  size_t getTotalRiCount() const { return riModule ? riModule->getRiData().size() : 0u; }
  void buildFilteredIndexOrder(eastl::vector<int> &out) const;
  int getColumnsCount() const { return columnsCount; }
  LpProfilerTableColumn *getColumn(int index) const { return LpProfilerTable::getColumn(index); }
  RIModule *getRIModule() const { return riModule; }

  bool isFilterActive(ColumnIndex column) const override;

protected:
  void clearAllFilters() override;
  void clearColumnFilter(ColumnIndex column) override;

private:
  RIModule *riModule = nullptr;
  ProfilerString selectedRiName;
  eastl::vector<int> sortedRiOrder;
  size_t filteredCount = 0;
  int currentMaxUsedLod = 0;
  bool areAllColumnsEffectivelyVisible() const override;
  LpIncludeExcludeFilter nameFilter;

  bool rangeFiltersInitialized = false;
  LpRangeFilter<int> countRangeFilter;
  LpRangeFilter<int> physTrisRangeFilter;
  LpRangeFilter<int> traceTrisRangeFilter;
  eastl::vector<LpRangeFilter<int>> lodDipsRangeFilters;
  eastl::vector<LpRangeFilter<int>> lodTrisRangeFilters;
  eastl::vector<LpRangeFilter<int>> lodDistRangeFilters;
  eastl::vector<CheckFilter<ProfilerString>> heavyShaderFilters;
  LpRangeFilter<float> bSphereRadiusRangeFilter;
  LpRangeFilter<float> bBoxRadiusRangeFilter;
  eastl::vector<LpRangeFilter<float>> lodScreenRangeFilters;
  eastl::vector<ProfilerString> lodPopupIdStorage;
  struct LodPopupCtx
  {
    LpRiTable *table;
    int lod;
    uint8_t type; // 0=dips 1=tris 2=dist 3=screen 4=heavy
  };
  eastl::vector<LodPopupCtx> lodPopupContexts;
  void initRangeFilters();
  // Filtering helpers
  bool passesFilters(const RiData &data_ref) const;      // wrapper combining base + dynamic
  bool baseFiltersPass(const RiData &data_ref) const;    // name, counts, BB/sphere size, phys/trace
  bool dynamicFiltersPass(const RiData &data_ref) const; // per-LOD + heavy shaders

  // Dynamic column visibility management extracted from drawContent
  void syncDynamicColumns();

  void drawNameFilterContent();
  void drawCountFilterContent();
  void drawBSphereFilterContent();
  void drawBBoxFilterContent();
  void drawLodDipsFilterContent(int lod_index);
  void drawLodTrisFilterContent(int lod_index);
  void drawLodDistFilterContent(int lod_index);
  void drawPhysTrisFilterContent();
  void drawTraceTrisFilterContent();
  void drawLodScreenFilterContent(int lod_index);
  void drawHeavyShadersFilterContent(int lod_index);

  eastl::vector<ProfilerString> getCustomCopyMenuItems(const void *item_data) const override;
  ProfilerString generateCustomCopyText(const void *item_data, const ProfilerString &menu_item) const override;
  ProfilerString generateLodDetailsText(const RiTableItem *item) const;
  ProfilerString generateFullRowText(const RiTableItem *item) const;

  bool shouldShowCellCopyOptions() const override { return true; }
  bool shouldShowRowCopyOption() const override { return true; }
  eastl::vector<int> getEssentialColumnIndices() const override
  {
    return {*RiColumn::NAME}; // Name shown at top level, others in submenu
  }
  bool shouldGroupNonEssentialCells() const override { return true; }
  eastl::vector<ColumnGroup> getColumnGroups() const override;
  eastl::vector<int> getBaseColumnIndices() const override
  {
    static_assert(static_cast<int>(RiColumn::NAME) == 0, "RI base columns must start at 0");
    static_assert(static_cast<int>(RiColumn::BASE_COUNT) > static_cast<int>(RiColumn::NAME),
      "Unexpected RI column enum ordering / sentinel position");
    static constexpr int RI_BASE_STABLE_COLUMN_COUNT = static_cast<int>(RiColumn::BASE_COUNT);
    eastl::vector<int> indices;
    indices.reserve(RI_BASE_STABLE_COLUMN_COUNT);
    for (int i = 0; i < RI_BASE_STABLE_COLUMN_COUNT; ++i)
      indices.push_back(i);
    return indices;
  }

  // Sorting and selection
  int compareRiItems(const void *item1, const void *item2, const ImGuiTableSortSpecs *sort_specs) const;
  static int sort_table_items_callback(const ImGuiTableSortSpecs *sort_specs, const void *a, const void *b, void *user_data);
  static void on_item_selected(int index, void *item_data, void *user_data);

  void onNameSearchChange(const char *text);
  void onNameSearchCommit();
};

} // namespace levelprofiler
