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
  saveSettings(false);
}