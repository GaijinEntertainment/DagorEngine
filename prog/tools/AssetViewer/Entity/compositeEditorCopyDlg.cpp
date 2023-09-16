#include "compositeEditorCopyDlg.h"
#include <propPanel2/comWnd/panel_window.h>

#define DIALOG_WIDTH  290
#define DIALOG_HEIGHT 290

CompositeEditorCopyDlg::CompositeEditorCopyDlg() : CDialogWindow(nullptr, DIALOG_WIDTH, DIALOG_HEIGHT, "Clone object")
{
  mPropertiesPanel->createEditInt(PID_CLONE_COUNT, "Count of clones:", 1);
}

int CompositeEditorCopyDlg::execute()
{
  if (showDialog() != DIALOG_ID_OK)
    return 0;

  return mPropertiesPanel->getInt(PID_CLONE_COUNT);
}

void CompositeEditorCopyDlg::show()
{
  autoSize();

  __super::show();
}
