// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>
#include <oldEditor/de_interface.h>

#include <EASTL/unique_ptr.h>

class ECSEditorCopyDlg
{
public:
  static bool show(int &clone_count)
  {
    enum
    {
      PID_CLONE_COUNT = 11000,
    };

    eastl::unique_ptr<PropPanel::DialogWindow> dialogWindow(
      DAGORED2->createDialog(hdpi::_pxScaled(250), hdpi::_pxScaled(100), "Clone object"));

    PropPanel::ContainerPropertyControl &panel = *dialogWindow->getPanel();
    panel.createEditInt(PID_CLONE_COUNT, "Count of clones:", 1);

    dialogWindow->autoSize();
    if (dialogWindow->showDialog() != PropPanel::DIALOG_ID_OK)
      return false;

    clone_count = panel.getInt(PID_CLONE_COUNT);
    if (clone_count < 1)
      clone_count = 1;

    return true;
  }
};