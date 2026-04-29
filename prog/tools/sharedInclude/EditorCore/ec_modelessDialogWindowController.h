// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_modelessWindowController.h>
#include <propPanel/commonWindow/dialogWindow.h>

#include <EASTL/unique_ptr.h>

// A simple base class that can be used when implementing IModelessWindowController for a DialogWindow.
// In most of the cases it is enough to override only the createDialog() method.
template <typename DialogType>
class ModelessDialogWindowController : public IModelessWindowController
{
public:
  void releaseWindow() override { dialog.reset(); }

  void showWindow(bool show = true) override
  {
    if (show)
    {
      if (!dialog)
        createDialog();

      if (dialog)
        dialog->show();
    }
    else if (dialog)
    {
      dialog->hide();
    }
  }

  bool isWindowShown() const override { return dialog && dialog->isVisible(); }

protected:
  // The derived class must override this method and create the dialog window.
  // For example:
  // void createDialog() override { dialog.reset(new DialogType()); }
  virtual void createDialog() = 0;

  eastl::unique_ptr<DialogType> dialog;
};
