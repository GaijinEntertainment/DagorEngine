// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <daECS/core/componentTypes.h>

class SettingsOverride
{
public:
  SettingsOverride() = default;
  SettingsOverride(SettingsOverride &&) = default;
  SettingsOverride &operator=(const SettingsOverride &) = delete;
  ~SettingsOverride();
  void useCustomConfigBlk(DataBlock &&blk);
  void useDefaultConfigBlk();
  const DataBlock *getSettings() const;
  bool isSettingOverriden(const char *setting_path) const;

private:
  DataBlock configBlk;
  bool overrideEnabled = false;
};

ECS_DECLARE_RELOCATABLE_TYPE(SettingsOverride);

void apply_settings_override_entity();
bool is_setting_overriden(const char *setting_path);
