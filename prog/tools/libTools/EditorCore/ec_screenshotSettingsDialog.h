// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_screenshot.h>

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <EASTL/functional.h>

class ScreenshotSettingsDialog : public PropPanel::DialogWindow
{
public:
  using Base = PropPanel::DialogWindow;
  using SettingsChangeHandler = eastl::function<void(const ScreenshotSettingsDialog &dialog)>;

  ScreenshotSettingsDialog(ScreenshotConfig screenshot_cfg, ScreenshotConfig cube_screenshot_cfg, ScreenshotDlgMode mode,
    SettingsChangeHandler settings_change_handler = nullptr);

  void fill();

  const ScreenshotConfig &getScreenshotCfg() const { return screenshotCfg; }
  const ScreenshotConfig &getCubeScreenshotCfg() const { return cubeScreenshotCfg; }

  void updateImguiDialog() override;

private:
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void applySettings();
  void fillScreenshotTab();
  void fillCubeScreenshotTab();
  void readCfgsFromControls();

  enum
  {
    PID_TAB_PANEL,
    PID_SCREENSHOT_TAB,
    PID_CUBE_SCREENSHOT_TAB,

    PID_SCRN_W,
    PID_SCRN_H,
    PID_FORMAT_RADIO,
    PID_JPEG_Q,

    PID_TRANSP_BACK,
    PID_DBG_GEOM,

    PID_NAME_ASSET,

    PID_CUBE_SIZE,
    PID_CUBE_FORMAT_RADIO,
    PID_CUBE_JPEG_Q,

    PID_CUBE_TRANSP_BACK,
    PID_CUBE_DBG_GEOM,

    PID_CUBE_NAME_ASSET,
  };

  ScreenshotConfig screenshotCfg;
  ScreenshotConfig cubeScreenshotCfg;
  const SettingsChangeHandler settingsChangeHandler;
  PropPanel::ContainerPropertyControl *screenshotTab;
  PropPanel::ContainerPropertyControl *cubeScreenshotTab;
  bool needsReloading;
  const ScreenshotDlgMode mode;
};
