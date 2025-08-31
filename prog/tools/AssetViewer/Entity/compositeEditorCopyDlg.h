// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>

class CompositeEditorCopyDlg : private PropPanel::DialogWindow
{
public:
  CompositeEditorCopyDlg();

  int execute();

private:
  void show() override;

  enum
  {
    PID_CLONE_COUNT = 1
  };
};
