#pragma once
#include <propPanel2/comWnd/dialog_window.h>

class CompositeEditorCopyDlg : CDialogWindow
{
public:
  CompositeEditorCopyDlg();

  int execute();

private:
  virtual void show() override;

  enum
  {
    PID_CLONE_COUNT = 1
  };
};
