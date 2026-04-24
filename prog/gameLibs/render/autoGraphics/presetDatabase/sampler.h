// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "record.h"

#include <render/autoGraphics.h>

namespace auto_graphics::auto_quality_preset::detail
{
class PresetDatabaseSampler
{
public:
  explicit PresetDatabaseSampler();

  AutoPreset getAutoPreset() const;

  // ideally on game start
  static void initUserInfo();

  static bool hasUserInfo();

  dag::Vector<PresetData> getAutoPresetForEachAvailableTargetFps() const;

private:
  // cpuScore, gpuScore, vramMb for user system
  // It will be used for database sampling
  // It must be set once, before getAutoPreset()
  static eastl::optional<PresetDatabaseRecord::Key> userKey;

  DataBlock autoPresetDatabaseCollection;
  const DataBlock *presetDict; // a part of autoPresetDatabaseCollection

  const char *driverName = nullptr;
  const char *fallbackDriverName = nullptr;

  AutoPreset getAutoPresetFromDriverDatabase(const char *driver_name, int target_fps) const;

  AutoPreset iterateRecordsForSingleTarget(const DataBlock &driver_database, int target_fps) const;

  AutoPreset findMinimalRecordForTarget(const DataBlock &target_blk) const;

  PresetData transform(PresetDatabaseRecord::Value value, int fps) const;

  eastl::pair<const DataBlock *, dag::Vector<int>> getDriverDatabase(const char *driver_name) const;
};

}; // namespace auto_graphics::auto_quality_preset::detail