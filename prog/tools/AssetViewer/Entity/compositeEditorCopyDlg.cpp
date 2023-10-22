#include "compositeEditorCopyDlg.h"
#include <propPanel2/comWnd/panel_window.h>

CompositeEditorCopyDlg::CompositeEditorCopyDlg() : CDialogWindow(nullptr, hdpi::_pxScaled(290), hdpi::_pxScaled(290), "Clone object")
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
