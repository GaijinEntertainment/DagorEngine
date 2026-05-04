// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sampler.h"
#include "gpuDatabase/sampler.h"

#include <render/cpuBenchmark.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_renderTarget.h>

#include <startup/dag_globalSettings.h>

#include <generic/dag_reverseView.h>
#include <EASTL/sort.h>

#include <fast_float/fast_float.h>

namespace auto_graphics::auto_quality_preset::detail
{

#define AUTO_PRESET_DATABASE_COLLECTION_BLK "auto_preset_database_collection.blk"

eastl::optional<PresetDatabaseRecord::Key> PresetDatabaseSampler::userKey = eastl::nullopt;

PresetDatabaseSampler::PresetDatabaseSampler() :
  autoPresetDatabaseCollection{},
  presetDict{nullptr},
  driverName{d3d::get_driver_code().map(d3d::dx11, "dx11").map(d3d::dx12, "dx12").map(d3d::vulkan, "vulkan").map(d3d::any, "")},
  fallbackDriverName{
    ::dgs_get_settings()->getBlockByName("graphics")->getBlockByNameEx("autoPreset")->getStr("fallbackDriverForDatabase", nullptr)}
{
  if (!autoPresetDatabaseCollection.load("config/" AUTO_PRESET_DATABASE_COLLECTION_BLK))
    logerr("can't load auto_preset_database.blk, auto graphical settings will not be applied");

  if (autoPresetDatabaseCollection.paramCount() > 0)
    logwarn(AUTO_PRESET_DATABASE_COLLECTION_BLK " should not contain any params, "
                                                "only database blocks for drivers");

  presetDict = autoPresetDatabaseCollection.getBlockByNameEx("presetDict");
}

void PresetDatabaseSampler::initUserInfo()
{
  G_ASSERTF_RETURN(!PresetDatabaseSampler::hasUserInfo(), , "it must be called once");

  int cpuScore = cpu_benchmark::run_benchmark();
  if (cpuScore < 1)
  {
    logwarn("failed to get CPU score, will use a placeholder value");
    cpuScore = 1;
  }

  auto maybeGpuInfo = get_gpu_info();
  if (!maybeGpuInfo.has_value())
  {
    logwarn("failed to get GPU score (see reason above), will use a placeholder value");
    maybeGpuInfo = eastl::make_optional<GpuInfo>(1, 1); // gpuScore=1, vramGb=1
  }

  const auto &[gpuScore, vramGb] = *maybeGpuInfo;
  const auto vramMb = vramGb << 10;

  userKey = PresetDatabaseRecord::Key{
    .cpuScore = cpuScore,
    .gpuScore = gpuScore,
    .vramMb = vramMb,
  };

  debug("auto_quality_preset: Trying to find auto preset for {cpuScore=%d, gpuScore=%d, vram=%d}", cpuScore, gpuScore, vramMb);
}

bool PresetDatabaseSampler::hasUserInfo() { return userKey.has_value(); }

AutoPreset PresetDatabaseSampler::getAutoPreset() const
{
  if (autoPresetDatabaseCollection.isEmpty())
    return eastl::nullopt;

  auto autoPreset = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBlockByNameEx("autoPreset");
  G_ASSERTF(!autoPreset->getBool("forceDisable", false), "Don't call getAutoPreset() if forceDisable is set");

  enum TargetFps
  {
    USE_REFRESH_RATE = -1,      // on first lauch or deleted config
    DISABLED_AUTO_SELECTOR = 0, // just ignore auto preset
    // any target fps (we will search closest available)
    // ...
  };

  // We can't set autoPreset/targetFps to USE_REFRESH_RATE by default because it would break existing users' settings.
  // Use laucnhCnt==0 to enable auto preset on first launch instead
  const bool firstLaunch = ::dgs_get_settings()->getInt("launchCnt", 0) == 0;
  int targetFps = firstLaunch ? USE_REFRESH_RATE : autoPreset->getInt("targetFps", DISABLED_AUTO_SELECTOR);

  switch (targetFps)
  {
    case (DISABLED_AUTO_SELECTOR): debug("skipping auto graphical settings"); return eastl::nullopt;
    case (USE_REFRESH_RATE):
    {
      double rate;
      d3d::driver_command(Drv3dCommand::GET_VSYNC_REFRESH_RATE, &rate);
      constexpr int DEFAULT_REFRESH_RATE = 60;
      targetFps = rate > 0.f ? static_cast<int>(round(rate)) : DEFAULT_REFRESH_RATE;
    }
    break;
  }

  AutoPreset result = getAutoPresetFromDriverDatabase(driverName, targetFps);
  if (!result.has_value() && fallbackDriverName)
  {
    debug("trying to find auto preset using fallback database for '%s' driver", fallbackDriverName);
    result = getAutoPresetFromDriverDatabase(fallbackDriverName, targetFps);
  }

  if (!result.has_value())
  {
    debug("failed to find auto preset for the hardware, will use minimum possible settings as fallback");
    auto getMinimalPresetForDriverAndTargetFps = [this](const char *driver_name, const char *target_name) -> AutoPreset {
      if (!driver_name || !target_name)
        return eastl::nullopt;

      auto driverDatabase = autoPresetDatabaseCollection.getBlockByNameEx(driver_name);
      if (driverDatabase->isEmpty())
        return eastl::nullopt;

      auto target_blk = driverDatabase->getBlockByNameEx(target_name);
      if (target_blk->isEmpty())
        return eastl::nullopt;

      return findMinimalRecordForTarget(*target_blk);
    };

    for (const char *driver : {driverName, fallbackDriverName})
    {
      result = getMinimalPresetForDriverAndTargetFps(driver, "target60");
      if (result.has_value())
        break;
    }

    if (!result.has_value())
    {
      // final fallback
      result = transform(
        PresetDatabaseRecord::Value{
          .presetId = 0,
          .resolution = IPoint2(1280, 720),
        },
        0);
    }
  }

  return result;
}

dag::Vector<PresetData> PresetDatabaseSampler::getAutoPresetForEachAvailableTargetFps() const
{
  using result_t = decltype(getAutoPresetForEachAvailableTargetFps());

  if (!userKey)
    return {};

  auto forDriver = [this](const char *driver_name) -> result_t {
    auto [driverDatabase, availableTargetFps] = getDriverDatabase(driver_name);
    if (driverDatabase->isEmpty() || availableTargetFps.empty())
    {
      logwarn("can't find database for driver '%s' in " AUTO_PRESET_DATABASE_COLLECTION_BLK, driver_name ? driver_name : "UNKNOWN");
      return {};
    }

    result_t result;
    result.reserve(availableTargetFps.size());

    for (auto &targetFps : availableTargetFps)
    {
      if (AutoPreset preset = iterateRecordsForSingleTarget(*driverDatabase, targetFps); preset.has_value())
        result.push_back(*preset);
    }

    return result;
  };

  auto result = forDriver(driverName);
  if (result.empty() && fallbackDriverName)
  {
    debug("trying to find auto preset using fallback database for '%s' driver", fallbackDriverName);
    result = forDriver(fallbackDriverName);
  }

  return result;
}

eastl::pair<const DataBlock *, dag::Vector<int>> PresetDatabaseSampler::getDriverDatabase(const char *driver_name) const
{
  auto driverDatabase = autoPresetDatabaseCollection.getBlockByNameEx(driver_name);
  if (driverDatabase->isEmpty())
    return eastl::make_pair(driverDatabase, dag::Vector<int>{});

  if (driverDatabase->paramCount() > 0)
    logwarn("%s driver database should not contain any params, only 'target<>' blocks", driver_name);

  dag::Vector<int> availableTargetFps;
  availableTargetFps.reserve(driverDatabase->blockCount());

  dblk::iterate_child_blocks(*driverDatabase, [&availableTargetFps, driver_name](const DataBlock &target_blk) {
    constexpr const char TARGET_PREFIX[] = "target";
    constexpr auto TARGET_PREFIX_LEN = sizeof(TARGET_PREFIX) - 1;

    // driver database contains only 'target<>' blocks by design
    const char *target = target_blk.getBlockName();
    if (strncmp(target, TARGET_PREFIX, TARGET_PREFIX_LEN) != 0)
    {
      logerr("unexpected block '%s' in <%s> driver database, expected 'target<fps>' blocks only, skipping it", target, driver_name);
      return;
    }

    const size_t targetLen = strlen(target);
    uint32_t targetFps = 0;
    auto res = fast_float::from_chars(target + TARGET_PREFIX_LEN, target + targetLen, targetFps);
    if (res.ec != std::errc())
    {
      logerr("failed to parse target fps for '%s' in <%s> driver database, skipping it", target, driver_name);
      return;
    }

    availableTargetFps.push_back(targetFps);
  });

  // Don't rely on block order in database file
  eastl::sort(availableTargetFps.begin(), availableTargetFps.end());

  return eastl::make_pair(driverDatabase, eastl::move(availableTargetFps));
}

AutoPreset PresetDatabaseSampler::getAutoPresetFromDriverDatabase(const char *driver_name, int target_fps) const
{
  const auto [driverDatabase, availableTargetFps] = getDriverDatabase(driver_name);
  if (driverDatabase->isEmpty() || availableTargetFps.empty())
  {
    logwarn("can't find database for driver '%s' in " AUTO_PRESET_DATABASE_COLLECTION_BLK, driver_name ? driver_name : "UNKNOWN");
    return eastl::nullopt;
  }

  // { 30, 60, 120 } and target_fps = 60 -> search in { 30, 60 }
  const eastl::span<const int> observedTargets{
    availableTargetFps.begin(), eastl::upper_bound(availableTargetFps.begin(), availableTargetFps.end(), target_fps)};

  for (auto &targetFps : dag::ReverseView(observedTargets))
  {
    if (AutoPreset result = iterateRecordsForSingleTarget(*driverDatabase, targetFps); result.has_value())
      return result;
  }

  return eastl::nullopt;
}

PresetData PresetDatabaseSampler::transform(PresetDatabaseRecord::Value value, int fps) const
{
  const char *presetStr = "";
  {
    int i = 0;
    for (; i < presetDict->paramCount(); ++i)
      // you can't get INVALID_PRESET_ID here, because bestPreset is always valid
      if (presetDict->getIntByNameId(presetDict->getParamNameId(i), PresetDatabaseRecord::Value::INVALID_PRESET_ID) == value.presetId)
        break;
    presetStr = presetDict->getParamName(i);
  }

  return {value.resolution, String{presetStr}, fps};
}

AutoPreset PresetDatabaseSampler::findMinimalRecordForTarget(const DataBlock &target_blk) const
{
  if (target_blk.isEmpty())
  {
    logwarn("%s block is empty, skipping", target_blk.getBlockName());
    return eastl::nullopt;
  }

  eastl::optional<PresetDatabaseRecord::Value> minimalPreset{};

  int start = 0;
  for (; start < target_blk.blockCount(); ++start)
  {
    const PresetDatabaseRecord record(*target_blk.getBlock(start), *presetDict);
    if (!record.valid())
    {
      logwarn("incorrect record in %s, all fields should be present in 'record[%d]', skipping", target_blk.getBlockName(), start);
      continue;
    }

    minimalPreset = record.value;
    break;
  }

  for (int i = start + 1; i < target_blk.blockCount(); ++i)
  {
    const PresetDatabaseRecord record(*target_blk.getBlock(i), *presetDict);
    if (!record.valid())
    {
      logwarn("incorrect record in %s, all fields should be present in 'record[%d]', skipping", target_blk.getBlockName(), i);
      continue;
    }

    // find record with minimal graphics preset and minimal resolution
    if (minimalPreset->isBetterByGraphics(record.value))
      minimalPreset = record.value;
  }

  if (minimalPreset.has_value())
    return transform(*minimalPreset, 0);

  return eastl::nullopt;
}

AutoPreset PresetDatabaseSampler::iterateRecordsForSingleTarget(const DataBlock &driver_database, int target_fps) const
{
  const DataBlock &targetBlk = *driver_database.getBlockByNameEx(String(16, "target%d", target_fps).c_str());
  if (targetBlk.isEmpty())
  {
    logwarn("%s block is empty, skipping", targetBlk.getBlockName());
    return eastl::nullopt;
  }

  IPoint2 screenSize;
  d3d::get_screen_size(screenSize.x, screenSize.y);

  const float displayScaling = max(d3d::get_display_scale(), 1.0f);
  if (displayScaling > 1.0f)
  {
    // scale screenSize and make even
    screenSize.x = static_cast<int>(roundf(screenSize.x / displayScaling)) & ~1;
    screenSize.y = static_cast<int>(roundf(screenSize.y / displayScaling)) & ~1;
  }

  constexpr int FULL_HD_AREA = 1920 * 1080; // milestone for auto preset logic
  const int renderArea = screenSize.x * screenSize.y;
  const bool useLogicForHighRes = renderArea >= FULL_HD_AREA;

  dag::Vector<PresetDatabaseRecord> hiResRecords;
  hiResRecords.reserve(targetBlk.blockCount());
  dag::Vector<PresetDatabaseRecord> lowResRecords;
  lowResRecords.reserve(targetBlk.blockCount());

  if (targetBlk.paramCount() > 0)
    logwarn("%s should not contain any params, only 'record' blocks", targetBlk.getBlockName());

  for (int i = 0; i < targetBlk.blockCount(); ++i)
  {
    const PresetDatabaseRecord record(*targetBlk.getBlock(i), *presetDict);
    if (!record.valid())
    {
      logwarn("incorrect record in %s, all fields should be present in 'record[%d]', skipping", targetBlk.getBlockName(), i);
      continue;
    }

    if (record.key < *userKey)
    {
      if (record.value.resolution.x * record.value.resolution.y >= FULL_HD_AREA)
        hiResRecords.push_back(record);
      else
        lowResRecords.push_back(record);
    }
  }

  if (hiResRecords.empty() && lowResRecords.empty())
  {
    logwarn("can't find suitable record in %s for cpuScore=%.2f, gpuScore=%d, vram=%dMb", targetBlk.getBlockName(), userKey->cpuScore,
      userKey->gpuScore, userKey->vramMb);
    return eastl::nullopt;
  }

  PresetDatabaseRecord::Value bestSettings{};
  if (useLogicForHighRes)
  {
    for (const auto &record : hiResRecords)
      if (record.value.resolution.x * record.value.resolution.y <= renderArea)
      {
        if (record.value.isBetterByGraphics(bestSettings))
          bestSettings = record.value;
      }
  }
  if (!bestSettings.valid())
  {
    for (const auto &record : lowResRecords)
      if (record.value.isBetterByResolution(bestSettings))
        bestSettings = record.value;
  }

  if (!bestSettings.valid())
  {
    logwarn("can't find suitable record in %s for cpuScore=%d, gpuScore=%d, vram=%dMb", targetBlk.getBlockName(), userKey->cpuScore,
      userKey->gpuScore, userKey->vramMb);
    return eastl::nullopt;
  }

  return transform(bestSettings, target_fps);
}

}; // namespace auto_graphics::auto_quality_preset::detail