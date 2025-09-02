// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>

class EditorWorkspace;


class ApplicationCreator : public PropPanel::DialogWindow
{
public:
  explicit ApplicationCreator(EditorWorkspace &wsp);

  // DialogWindow interface

  bool onOk() override;
  bool onCancel() override { return true; };

  // ControlEventHandler methods from CDialogWindow

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

private:
  EditorWorkspace &wsp;

  void correctAppPath(PropPanel::ContainerPropertyControl &panel);
};
