// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_ViewportWindowStatSettingsDialogController.h"

#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_ViewportWindowStatSettingsDialog.h>

#include <EASTL/unique_ptr.h>

// The statistics dialog contains both global settings (that apply to all viewports) and local settings (that apply
// only to the current viewport). To prevent inconsistency among the dialogs we only allow displaying a single dialog at
// a time. This makes the code a bit more complicated.
static eastl::unique_ptr<ViewportWindowStatSettingsDialog> stat_settings_dialog;
static ViewportWindowStatSettingsDialogController *dialog_owner = nullptr;

ViewportWindowStatSettingsDialogController::ViewportWindowStatSettingsDialogController(ViewportWindow &viewport_window,
  int viewport_index) :
  windowId(0, "%s%d", WindowIds::VIEWPORT_STAT_SETTINGS_PREFIX, viewport_index), viewportWindow(viewport_window)
{}

const char *ViewportWindowStatSettingsDialogController::getWindowId() const { return windowId; }

void ViewportWindowStatSettingsDialogController::releaseWindow()
{
  if (isOwningDialog())
  {
    dialog_owner = nullptr;
    stat_settings_dialog.reset();
  }
}

void ViewportWindowStatSettingsDialogController::showWindow(bool show)
{
  if (show)
  {
    if (!isOwningDialog())
    {
      dialog_owner = this;
      stat_settings_dialog.reset();
    }

    if (!stat_settings_dialog)
      stat_settings_dialog.reset(viewportWindow.createStatSettingsDialog());

    stat_settings_dialog->show();
  }
  else if (stat_settings_dialog)
  {
    stat_settings_dialog->hide();
  }
}

bool ViewportWindowStatSettingsDialogController::isWindowShown() const
{
  return isOwningDialog() && stat_settings_dialog && stat_settings_dialog->isVisible();
}

bool ViewportWindowStatSettingsDialogController::isOwningDialog() const { return dialog_owner == this; }
