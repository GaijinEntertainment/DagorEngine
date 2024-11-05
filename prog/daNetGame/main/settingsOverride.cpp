// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <main/settingsOverride.h>
#include <main/settingsOverrideUtil.h>
#include <main/settings.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlockUtils.h>

SettingsOverride::~SettingsOverride() { useDefaultConfigBlk(); }

void SettingsOverride::useCustomConfigBlk(DataBlock &&blk)
{
  G_ASSERTF_RETURN(!overrideEnabled, , "useCustomConfigBlk was called when override is already used!");

  overrideEnabled = true;
  FastNameMap diff = find_difference_in_config_blk(*dgs_get_settings(), blk);
  configBlk = eastl::move(blk);

  save_settings(nullptr);
  apply_settings_changes(diff);
}

static bool is_settings_reset_needed()
{
  // Do not change settings after the dx12 driver shut down already.
  int w, h;
  d3d::get_screen_size(w, h);
  return w > 0 && h > 0;
}

void SettingsOverride::useDefaultConfigBlk()
{
  if (overrideEnabled)
  {
    overrideEnabled = false;
    if (is_settings_reset_needed())
    {
      // Can't optimize graphics changes by comparing to current settings, because they are already applied.
      FastNameMap diff = find_difference_in_config_blk({}, configBlk);
      configBlk.reset();
      save_settings(nullptr);
      apply_settings_changes(diff);
    }
  }
}

const DataBlock *SettingsOverride::getSettings() const { return &configBlk; }

bool SettingsOverride::isSettingOverriden(const char *setting_path) const
{
  if (!overrideEnabled)
    return false;
  bool result = blk_path_exists(&configBlk, setting_path);
  return result;
}