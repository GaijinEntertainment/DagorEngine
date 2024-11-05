// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>

class EditorWorkspace;


class ApplicationCreator : public PropPanel::DialogWindow
{
public:
  explicit ApplicationCreator(EditorWorkspace &wsp);

  // DialogWindow interface

  virtual bool onOk();
  virtual bool onCancel() { return true; };

  // ControlEventHandler methods from CDialogWindow

  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);

private:
  EditorWorkspace &wsp;

  void correctAppPath(PropPanel::ContainerPropertyControl &panel);
};
