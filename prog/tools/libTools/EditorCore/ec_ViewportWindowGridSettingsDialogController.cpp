// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ec_ViewportWindowGridSettingsDialogController.h"

#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/ec_interface.h>

#include <EASTL/unique_ptr.h>

// The grid edit dialog contains both global settings (that apply to all viewports) and local settings (that apply
// only to the current viewport). To prevent inconsistency among the dialogs we only allow displaying a single dialog at
// a time. This makes the code a bit more complicated.
static eastl::unique_ptr<GridEditDialog> grid_settings_dialog;
static ViewportWindowGridSettingsDialogController *dialog_owner = nullptr;

ViewportWindowGridSettingsDialogController::ViewportWindowGridSettingsDialogController(int viewport_index) :
  windowId(0, "%s%d", WindowIds::VIEWPORT_GRID_SETTINGS_PREFIX, viewport_index), viewportIndex(viewport_index)
{}

const char *ViewportWindowGridSettingsDialogController::getWindowId() const { return windowId; }

void ViewportWindowGridSettingsDialogController::releaseWindow()
{
  if (isOwningDialog())
  {
    dialog_owner = nullptr;
    grid_settings_dialog.reset();
  }
}

void ViewportWindowGridSettingsDialogController::showWindow(bool show)
{
  if (show)
  {
    if (!isOwningDialog())
    {
      dialog_owner = this;
      grid_settings_dialog.reset();
    }

    if (!grid_settings_dialog)
    {
      IGridSettingChangeEventHandler *changeEventHandler = EDITORCORE->queryEditorInterface<IGridSettingChangeEventHandler>();
      G_ASSERT(changeEventHandler);
      grid_settings_dialog.reset(new GridEditDialog(*changeEventHandler, EDITORCORE->getGrid(), "Viewport grid settings"));
    }

    grid_settings_dialog->showGridEditDialog(viewportIndex);
  }
  else if (grid_settings_dialog)
  {
    grid_settings_dialog->hide();
  }
}

bool ViewportWindowGridSettingsDialogController::isWindowShown() const
{
  return isOwningDialog() && grid_settings_dialog && grid_settings_dialog->isVisible();
}

bool ViewportWindowGridSettingsDialogController::isOwningDialog() const { return dialog_owner == this; }

void ViewportWindowGridSettingsDialogController::onGridVisibilityChanged(int viewport_index)
{
  if (grid_settings_dialog)
    grid_settings_dialog->onGridVisibilityChanged(viewport_index);
}

void ViewportWindowGridSettingsDialogController::onSnapSettingChanged()
{
  if (grid_settings_dialog)
    grid_settings_dialog->onSnapSettingChanged();
}
