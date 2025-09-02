// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>
#include <libTools/util/strUtil.h>
#include <EditorCore/ec_interface.h>

#include <EASTL/unique_ptr.h>

class ECSEditorRenameObjectMemberDlg
{
public:
  static bool show(String &member_name)
  {
    enum
    {
      PID_NAME = 11000,
    };

    eastl::unique_ptr<PropPanel::DialogWindow> dialogWindow(
      EDITORCORE->createDialog(hdpi::_pxScaled(250), hdpi::_pxScaled(100), "Rename object member"));
    dialogWindow->setInitialFocus(PropPanel::DIALOG_ID_NONE);

    PropPanel::ContainerPropertyControl &panel = *dialogWindow->getPanel();
    panel.createEditBox(PID_NAME, "Name", member_name);
    panel.setFocusById(PID_NAME);

    dialogWindow->autoSize();
    if (dialogWindow->showDialog() != PropPanel::DIALOG_ID_OK)
      return false;

    member_name = panel.getText(PID_NAME);
    trim(member_name);
    return !member_name.empty();
  }
};