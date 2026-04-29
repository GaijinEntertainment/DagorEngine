// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_info.h>

#include <ioSys/dag_dataBlock.h>

#include <render/autoGraphics.h>
#include <render/cpuBenchmark.h>

#include "presetDatabase/sampler.h"

namespace auto_graphics::auto_quality_preset
{

using namespace detail;

AutoPreset get_auto_preset()
{
  PresetDatabaseSampler::initUserInfo();

  if (!PresetDatabaseSampler::hasUserInfo())
  {
    logwarn("user info is not initialized, auto graphical settings will not be applied");
    return eastl::nullopt;
  }

  return PresetDatabaseSampler{}.getAutoPreset();
}

bool is_valid() { return PresetDatabaseSampler::hasUserInfo(); }

dag::Vector<PresetData> get_preset_for_each_available_target_fps()
{
  return PresetDatabaseSampler().getAutoPresetForEachAvailableTargetFps();
}

} // namespace auto_graphics::auto_quality_preset