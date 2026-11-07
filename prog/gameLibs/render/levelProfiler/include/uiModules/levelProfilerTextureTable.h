// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unordered_map.h>
#include "levelProfilerTableBase.h"
#include "levelProfilerFilter.h"
#include "textureModule.h"

namespace levelprofiler
{

class TextureProfilerUI;
class RIModule;

struct TextureData;

struct TextureTableItem
{
  ProfilerString name;
  const TextureData *data = nullptr;
  // Points into the RI module map, so it is valid until the next RI collect.
  const eastl::vector<ProfilerString> *assetNames = nullptr;
  bool isSelected = false;
  ProfilerString formatName;
  ProfilerString qualityReason; // The filter key of the Quality column

  int usageUniqueCount = 0;
  ProfilerString usageLabel;

  TextureTableItem(const ProfilerString &name_ref, const TextureData &data_ref) : name(name_ref), data(&data_ref) {}
};

using LpTextureTableColumn = LpTypedTableColumn<TextureTableItem>;

// The memory of the counters line, in MB: the state that the Mem column shows, and for a pair view or a difference view the
// memory that the shown state lost against the other one.
struct LpTextureMemorySums
{
  ProfilerString label; // "Max Mem"
  LpColumnSum memory;
  ProfilerString differenceLabel; // "Not loaded (Max - Res)"; empty for a state view
  LpColumnSum difference;
};

class LpTextureNameColumn : public LpTextureTableColumn
{
public:
  LpTextureNameColumn();

protected:
  void drawRowCell(const TextureTableItem &item) const override;
  int compareRows(const TextureTableItem &item1, const TextureTableItem &item2) const override;
  ProfilerString getRowCellText(const TextureTableItem &item) const override;
};

// The cell shows the usage kind; the tooltip, the Full Row copy and the export give the assets that use the texture, with their
// instance counts.
class LpTextureUsageColumn : public LpLongTextColumn<TextureTableItem>
{
public:
  explicit LpTextureUsageColumn(const RIModule &ri_module);

protected:
  void drawRowCell(const TextureTableItem &item) const override;
  int compareRows(const TextureTableItem &item1, const TextureTableItem &item2) const override;
  ProfilerString getRowCellText(const TextureTableItem &item) const override;
  ProfilerString getRowFullText(const TextureTableItem &item) const override;
  void drawRowTooltip(const TextureTableItem &item) const override;

private:
  const RIModule &riModule;
};

class LpTextureTable : public LpTypedTable<TextureTableItem>
{
public:
  LpTextureTable(TextureProfilerUI *profiler_ui_ptr);

  ILpTableExportSource &getExportSource() { return exportSource; }

  // Brings the rows up to date first, as refreshRows() does.
  LpTextureMemorySums sumMemory();

protected:
  bool pollItemsChanged() override;
  void rebuildItems() override;
  const char *getNoRowsText() const override;

private:
  TextureProfilerUI *profilerUI = nullptr;
  LpProfilerTableExportSource exportSource{*this, "Textures"};
  unsigned cachedTextureDataGeneration = ~0u;
  unsigned cachedRiDataGeneration = ~0u;
  unsigned cachedRiCountsGeneration = ~0u;
};

} // namespace levelprofiler