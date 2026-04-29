// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_preferencesDialog.h"

#include "de_appwnd.h"

#include <EditorCore/ec_modelessDialogWindowController.h>
#include <oldEditor/de_cm.h>
#include <propPanel/control/container.h>

enum
{
  PID_TOOLBAR_SCALE = 1,
};

// The "Toolbar preferences" name is temporary, this will be the general preferences window.
PreferencesDialog::PreferencesDialog() : DialogWindow(nullptr, hdpi::_pxScaled(300), hdpi::_pxScaled(150), "Toolbar preferences")
{
  setDialogButtonText(PropPanel::DIALOG_ID_OK, "Close");
  removeDialogButton(PropPanel::DIALOG_ID_CANCEL);

  PropPanel::ContainerPropertyControl *panel = getPanel();
  G_ASSERT(panel);

  panel->createEditInt(PID_TOOLBAR_SCALE, "Toolbar scale (%)", get_app().getToolbarScalePercent());
  panel->setMinMaxStep(PID_TOOLBAR_SCALE, 50.0f, 400.0f, 1.0f);
}

void PreferencesDialog::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == PID_TOOLBAR_SCALE)
    get_app().setToolbarScalePercent(panel->getInt(pcb_id));
}

class PreferencesDialogController : public ModelessDialogWindowController<PreferencesDialog>
{
public:
  const char *getWindowId() const override { return WindowIds::MAIN_SETTINGS_EDIT_PREFERENCES; }

  void createDialog() override { dialog.reset(new PreferencesDialog()); }
};

static PreferencesDialogController preferences_dialog_controller;

IModelessWindowController *get_preferences_dialog_controller() { return &preferences_dialog_controller; }
