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
  bool isSelected = false;
  bool isUpdating = false;

  explicit RiTableItem(const RiData &data_ref) : data(&data_ref) {}
};

using LpRiTableColumn = LpTypedTableColumn<RiTableItem>;

class LpRiNameColumn : public LpRiTableColumn
{
public:
  LpRiNameColumn();

protected:
  void drawRowCell(const RiTableItem &item) const override;
  int compareRows(const RiTableItem &item1, const RiTableItem &item2) const override;
  ProfilerString getRowCellText(const RiTableItem &item) const override;
};

class LpRiHeavyShadersColumn : public LpLongTextColumn<RiTableItem>
{
public:
  explicit LpRiHeavyShadersColumn(int lod_index);

protected:
  int compareRows(const RiTableItem &item1, const RiTableItem &item2) const override;
  bool rowHasValue(const RiTableItem &item) const override;
  ProfilerString getRowCellText(const RiTableItem &item) const override;

private:
  int lodIndex;
};

class LpRiScreenFilter;
template <typename Row, typename First, typename Second>
class LpCompositeColumn;

class LpRiTable : public LpTypedTable<RiTableItem>
{
public:
  LpRiTable(RIModule *ri_module_ptr);

  void drawContent() override;

  RIModule *getRIModule() const { return riModule; }
  ILpTableExportSource &getExportSource() { return exportSource; }

  // The horizontal FOV for 16:9 in degrees that Screen% uses; it is clamped to MIN_SCREEN_FOV..MAX_SCREEN_FOV.
  int getScreenFov() const { return screenFov; }
  void setScreenFov(int fov);
  float getScreenPercent(const LodInfo &lod) const { return lod.maxEdgePerDistance * screenPercentScale; }
  // The screen height in pixels that the Screen px view uses; it is clamped to MIN_SCREEN_HEIGHT..MAX_SCREEN_HEIGHT.
  int getScreenHeight() const { return screenHeight; }
  void setScreenHeight(int height);
  float getScreenPixels(const LodInfo &lod) const { return getScreenPercent(lod) * screenHeight * 0.01f; }

  static constexpr int DEFAULT_SCREEN_FOV = 90;
  static constexpr int MIN_SCREEN_FOV = 10;
  static constexpr int MAX_SCREEN_FOV = 170;
  static constexpr int DEFAULT_SCREEN_HEIGHT = 1080;
  static constexpr int MIN_SCREEN_HEIGHT = 240;
  static constexpr int MAX_SCREEN_HEIGHT = 4320;

protected:
  bool pollItemsChanged() override;
  void rebuildItems() override;
  void prepareItemForDraw(RiTableItem &item) override;

private:
  RIModule *riModule = nullptr;
  // The items point into riData, so they are rebuilt on every riDataGeneration change; RIModule::collect() bumps it in clear()
  // before it rebuilds riData.
  unsigned itemsGeneration = ~0u;
  unsigned countsGeneration = ~0u;
  LpProfilerTableExportSource exportSource{*this, "RenderInstances"};
  // The groups of the per-LOD columns; their active count is the most collected LODs of an asset.
  eastl::vector<int> lodColumnGroupIds;
  int screenFov = DEFAULT_SCREEN_FOV;
  int screenHeight = DEFAULT_SCREEN_HEIGHT;
  float screenPercentScale = 0.0f;
  // The parts of the columns of one LOD that follow the FOV and the screen height; the table base owns them.
  struct ScreenLod
  {
    LpRiScreenFilter *distScreenFilter = nullptr;
    LpRiScreenFilter *distPixelsFilter = nullptr;
    // The Dist and Screen% views and the Dist and Screen px views, one for the sort by each part.
    LpCompositeColumn<RiTableItem, float, float> *distScreenColumns[2] = {};
    LpCompositeColumn<RiTableItem, float, float> *distPixelsColumns[2] = {};
  };
  eastl::vector<ScreenLod> screenLods;

  void updateScreenColumnNames();
};

} // namespace levelprofiler
