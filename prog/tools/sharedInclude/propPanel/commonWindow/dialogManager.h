//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace PropPanel
{

// PropPanel dialogs use this interface to make it possible to easily do
// something before and after showing a modal dialog.
class IModalDialogEventHandler
{
public:
  // Called before a modal dialog or message box is shown.
  virtual void beforeModalDialogShown() = 0;

  // Called after a modal dialog or message box has been shown.
  virtual void afterModalDialogShown() = 0;
};

void render_dialogs();

void set_modal_dialog_events(IModalDialogEventHandler *event_handler);

} // namespace PropPanel