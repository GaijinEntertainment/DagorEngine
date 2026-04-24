// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_modelessDialogWindowController.h>
#include <EditorCore/ec_screenshot.h>

#include "ec_screenshotSettingsDialog.h"

class ScreenshotSettingsDialogController : public ModelessDialogWindowController<ScreenshotSettingsDialog>
{
public:
  void setConfig(ScreenshotDlgMode in_mode, ScreenshotConfig *screenshot_cfg, ScreenshotConfig *cube_screenshot_cfg)
  {
    mode = in_mode;
    screenshotCfg = screenshot_cfg;
    cubeScreenshotCfg = cube_screenshot_cfg;
  }

private:
  const char *getWindowId() const override { return WindowIds::MAIN_SETTINGS_SCREENSHOT; }

  void createDialog() override
  {
    G_ASSERT(screenshotCfg);
    G_ASSERT(cubeScreenshotCfg);
    if (!screenshotCfg || !cubeScreenshotCfg)
      return;

    dialog.reset(
      new ScreenshotSettingsDialog(*screenshotCfg, *cubeScreenshotCfg, mode, [this](const ScreenshotSettingsDialog &dialog) {
        *screenshotCfg = dialog.getScreenshotCfg();
        *cubeScreenshotCfg = dialog.getCubeScreenshotCfg();
      }));

    dialog->fill();
  }

  ScreenshotDlgMode mode = ScreenshotDlgMode::ASSET_VIEWER;
  ScreenshotConfig *screenshotCfg = nullptr;
  ScreenshotConfig *cubeScreenshotCfg = nullptr;
};
