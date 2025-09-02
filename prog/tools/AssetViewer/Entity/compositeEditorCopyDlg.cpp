// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorCopyDlg.h"
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>

CompositeEditorCopyDlg::CompositeEditorCopyDlg() : DialogWindow(nullptr, hdpi::_pxScaled(250), hdpi::_pxScaled(100), "Clone object")
{
  propertiesPanel->createEditInt(PID_CLONE_COUNT, "Count of clones:", 1);
}

int CompositeEditorCopyDlg::execute()
{
  if (showDialog() != PropPanel::DIALOG_ID_OK)
    return 0;

  return propertiesPanel->getInt(PID_CLONE_COUNT);
}

void CompositeEditorCopyDlg::show()
{
  autoSize();

  DialogWindow::show();
}
