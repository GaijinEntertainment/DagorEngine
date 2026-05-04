// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>

class IModelessWindowController;

class PreferencesDialog : public PropPanel::DialogWindow
{
public:
  PreferencesDialog();

private:
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
};

IModelessWindowController *get_preferences_dialog_controller();