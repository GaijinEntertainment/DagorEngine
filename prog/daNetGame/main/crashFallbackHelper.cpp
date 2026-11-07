// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "crashFallbackHelper.h"
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include "main.h"
#include "settings.h"

bool CrashFallbackHelper::previousStartupFailed()
{
  return ::dgs_get_settings()->getBlockByNameEx("dx12")->getBool("previous_run_failed", false);
}

bool CrashFallbackHelper::alreadySetToAuto()
{
  return ::dgs_get_settings()->getBlockByNameEx("dx12")->getBool("set_driver_to_auto_due_to_failed_run", false);
}

void CrashFallbackHelper::saveSettings(bool failed)
{
  DataBlock *dx12Block = get_settings_override_blk()->addBlock("dx12");
  dx12Block->setBool("previous_run_failed", failed);
  save_settings(nullptr, false);
}

void CrashFallbackHelper::beforeStartup() { saveSettings(true); }

void CrashFallbackHelper::successfullyLoaded() { saveSettings(false); }

void CrashFallbackHelper::setSettingToAuto()
{
  DataBlock *videoBlock = get_settings_override_blk()->addBlock("video");
  videoBlock->setStr("driver", "auto");
  get_settings_override_blk()->addBlock("dx12")->setBool("set_driver_to_auto_due_to_failed_run", true);
  saveSettings(false);
}
