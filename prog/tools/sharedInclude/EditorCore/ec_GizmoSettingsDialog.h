// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>

class IModelessWindowController;

class GizmoSettingsDialog : public PropPanel::DialogWindow
{
public:
  GizmoSettingsDialog();

private:
  void fillPanel();
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
};

IModelessWindowController *get_gizmo_settings_dialog_controller();