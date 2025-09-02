// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include "ec_screenshotSettingsDialog.h"

namespace
{
constexpr const char *TRANSP_BACK_CAPTION = "Show transparent background";
constexpr const char *DEBUG_GEOM_CAPTION = "Show debug geometry";
constexpr const char *NAME_ASSET_CAPTION = "Name as current asset";
constexpr const char *DEBUG_GEOM_TOOLTIP = "Splines, game objects, grid, etc.";
} // namespace

ScreenshotSettingsDialog::ScreenshotSettingsDialog(ScreenshotConfig screenshot_cfg, ScreenshotConfig cube_screenshot_cfg,
  ScreenshotDlgMode mode, SettingsChangeHandler settings_change_handler) :
  DialogWindow(0, hdpi::_pxScaled(305), hdpi::_pxScaled(490), "Screenshot settings"),
  screenshotCfg(std::move(screenshot_cfg)),
  cubeScreenshotCfg(std::move(cube_screenshot_cfg)),
  settingsChangeHandler(std::move(settings_change_handler)),
  screenshotTab(nullptr),
  cubeScreenshotTab(nullptr),
  needsReloading(false),
  mode(mode)
{}

void ScreenshotSettingsDialog::fill()
{
  if (!screenshotTab || !cubeScreenshotTab)
  {
    PropPanel::ContainerPropertyControl *panel = getPanel();
    G_ASSERT(panel);

    panel->clear();

    PropPanel::ContainerPropertyControl *screenTabPanel = panel->createTabPanel(PID_TAB_PANEL, "");
    screenshotTab = screenTabPanel->createTabPage(PID_SCREENSHOT_TAB, "Screenshot");
    cubeScreenshotTab = screenTabPanel->createTabPage(PID_CUBE_SCREENSHOT_TAB, "Cube Screenshot");
  }

  fillScreenshotTab();
  fillCubeScreenshotTab();

  setDialogButtonText(PropPanel::DIALOG_ID_OK, "Close");
  removeDialogButton(PropPanel::DIALOG_ID_CANCEL);
}

void ScreenshotSettingsDialog::updateImguiDialog()
{
  if (needsReloading)
  {
    fillScreenshotTab();
    fillCubeScreenshotTab();
    needsReloading = false;
  }

  Base::updateImguiDialog();
}

void ScreenshotSettingsDialog::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == PID_SCRN_W || pcb_id == PID_SCRN_H)
  {
    int val = panel->getInt(pcb_id);
    if (val < 1)
    {
      val = 1;
      panel->setInt(pcb_id, val);
    }
  }

  if (pcb_id == PID_FORMAT_RADIO || pcb_id == PID_CUBE_FORMAT_RADIO)
  {
    needsReloading = true;
  }

  readCfgsFromControls();
  if (settingsChangeHandler)
  {
    settingsChangeHandler(*this);
  }
}

void ScreenshotSettingsDialog::fillScreenshotTab()
{
  G_ASSERT(screenshotTab);
  if (!screenshotTab)
  {
    return;
  }

  screenshotTab->clear();

  screenshotTab->createSeparatorText(0, "Resolution");
  screenshotTab->createEditInt(PID_SCRN_W, "Width:", screenshotCfg.width);
  screenshotTab->createEditInt(PID_SCRN_H, "Height:", screenshotCfg.height, true, false);

  screenshotTab->createSeparatorText(0, "Format");
  PropPanel::ContainerPropertyControl &fmtRadio = *screenshotTab->createRadioGroup(PID_FORMAT_RADIO, "");
  fmtRadio.createRadio(static_cast<int>(ScreenshotFormat::JPEG), "JPEG");
  fmtRadio.createRadio(static_cast<int>(ScreenshotFormat::PNG), "PNG");
  fmtRadio.createRadio(static_cast<int>(ScreenshotFormat::TGA), "TGA");
  fmtRadio.createRadio(static_cast<int>(ScreenshotFormat::TIFF), "TIFF");
  screenshotTab->setInt(PID_FORMAT_RADIO, static_cast<int>(screenshotCfg.format));

  if (screenshotCfg.format == ScreenshotFormat::JPEG)
  {
    screenshotTab->createTrackInt(PID_JPEG_Q, "JPEG quality", screenshotCfg.jpegQuality, 1, 100, 1);
  }

  screenshotTab->createSeparatorText(0, "Display options");
  if (mode == ScreenshotDlgMode::ASSET_VIEWER)
  {
    screenshotTab->createCheckBox(PID_TRANSP_BACK, TRANSP_BACK_CAPTION, screenshotCfg.enableTransparentBackground);
  }
  screenshotTab->createCheckBox(PID_DBG_GEOM, DEBUG_GEOM_CAPTION, screenshotCfg.enableDebugGeometry);
  screenshotTab->setTooltipId(PID_DBG_GEOM, DEBUG_GEOM_TOOLTIP);

  if (mode == ScreenshotDlgMode::ASSET_VIEWER)
  {
    screenshotTab->createSeparatorText(0, "File naming");
    screenshotTab->createCheckBox(PID_NAME_ASSET, NAME_ASSET_CAPTION, screenshotCfg.nameAsCurrentAsset);
  }
}

void ScreenshotSettingsDialog::fillCubeScreenshotTab()
{
  G_ASSERT(cubeScreenshotTab);
  if (!cubeScreenshotTab)
  {
    return;
  }

  cubeScreenshotTab->clear();

  cubeScreenshotTab->createSeparatorText(0, "Resolution");
  {
    Tab<String> vals(tmpmem);
    int sel = -1;

    for (int i = ScreenshotConfig::MIN_CUBE_2_POWER; i <= ScreenshotConfig::MAX_CUBE_2_POWER; ++i)
    {
      unsigned sz = 1 << i;
      if (cubeScreenshotCfg.size == sz)
        sel = i - ScreenshotConfig::MIN_CUBE_2_POWER;

      String val(32, "%i x %i", sz, sz);
      vals.push_back(val);
    }

    cubeScreenshotTab->createCombo(PID_CUBE_SIZE, "", vals, sel);
  }

  cubeScreenshotTab->createSeparatorText(0, "Format");
  PropPanel::ContainerPropertyControl &fmtRadio = *cubeScreenshotTab->createRadioGroup(PID_CUBE_FORMAT_RADIO, "");
  fmtRadio.createRadio(static_cast<int>(ScreenshotFormat::DDSX), "DDSx");
  fmtRadio.createRadio(static_cast<int>(ScreenshotFormat::JPEG), "JPEG");
  fmtRadio.createRadio(static_cast<int>(ScreenshotFormat::PNG), "PNG");
  fmtRadio.createRadio(static_cast<int>(ScreenshotFormat::TGA), "TGA");
  fmtRadio.createRadio(static_cast<int>(ScreenshotFormat::TIFF), "TIFF");
  cubeScreenshotTab->setInt(PID_CUBE_FORMAT_RADIO, static_cast<int>(cubeScreenshotCfg.format));

  if (cubeScreenshotCfg.format == ScreenshotFormat::JPEG)
  {
    cubeScreenshotTab->createTrackInt(PID_CUBE_JPEG_Q, "JPEG quality", cubeScreenshotCfg.jpegQuality, 1, 100, 1);
  }

  cubeScreenshotTab->createSeparatorText(0, "Display options");
  if (mode == ScreenshotDlgMode::ASSET_VIEWER)
  {
    cubeScreenshotTab->createCheckBox(PID_CUBE_TRANSP_BACK, TRANSP_BACK_CAPTION, cubeScreenshotCfg.enableTransparentBackground);
  }
  cubeScreenshotTab->createCheckBox(PID_CUBE_DBG_GEOM, DEBUG_GEOM_CAPTION, cubeScreenshotCfg.enableDebugGeometry);
  cubeScreenshotTab->setTooltipId(PID_CUBE_DBG_GEOM, DEBUG_GEOM_TOOLTIP);

  if (mode == ScreenshotDlgMode::ASSET_VIEWER)
  {
    cubeScreenshotTab->createSeparatorText(0, "File naming");
    cubeScreenshotTab->createCheckBox(PID_CUBE_NAME_ASSET, NAME_ASSET_CAPTION, cubeScreenshotCfg.nameAsCurrentAsset);
  }
}

void ScreenshotSettingsDialog::readCfgsFromControls()
{
  G_ASSERT(screenshotTab);
  G_ASSERT(cubeScreenshotTab);

  if (screenshotTab)
  {
    // regular screenshot
    screenshotCfg.width = screenshotTab->getInt(PID_SCRN_W);
    screenshotCfg.height = screenshotTab->getInt(PID_SCRN_H);
    if (screenshotCfg.format == ScreenshotFormat::JPEG)
    {
      screenshotCfg.jpegQuality = screenshotTab->getInt(PID_JPEG_Q);
    }
    screenshotCfg.format = static_cast<ScreenshotFormat>(screenshotTab->getInt(PID_FORMAT_RADIO));
    screenshotCfg.enableDebugGeometry = screenshotTab->getBool(PID_DBG_GEOM);
    if (mode == ScreenshotDlgMode::ASSET_VIEWER)
    {
      screenshotCfg.enableTransparentBackground = screenshotTab->getBool(PID_TRANSP_BACK);
      screenshotCfg.nameAsCurrentAsset = screenshotTab->getBool(PID_NAME_ASSET);
    }
  }

  if (cubeScreenshotTab)
  {
    // cube screenshot
    const int pwr = cubeScreenshotTab->getInt(PID_CUBE_SIZE);
    cubeScreenshotCfg.size = pwr > -1 ? 1 << (pwr + ScreenshotConfig::MIN_CUBE_2_POWER) : 0;
    if (cubeScreenshotCfg.format == ScreenshotFormat::JPEG)
    {
      cubeScreenshotCfg.jpegQuality = cubeScreenshotTab->getInt(PID_CUBE_JPEG_Q);
    }
    cubeScreenshotCfg.format = static_cast<ScreenshotFormat>(cubeScreenshotTab->getInt(PID_CUBE_FORMAT_RADIO));
    cubeScreenshotCfg.enableDebugGeometry = cubeScreenshotTab->getBool(PID_CUBE_DBG_GEOM);
    if (mode == ScreenshotDlgMode::ASSET_VIEWER)
    {
      cubeScreenshotCfg.enableTransparentBackground = cubeScreenshotTab->getBool(PID_CUBE_TRANSP_BACK);
      cubeScreenshotCfg.nameAsCurrentAsset = cubeScreenshotTab->getBool(PID_CUBE_NAME_ASSET);
    }
  }
}
