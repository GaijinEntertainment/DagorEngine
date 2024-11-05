// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>

namespace PropPanel
{

class DialogWindow;

class DialogManager
{
public:
  void showDialog(DialogWindow &dialog);
  void hideDialog(DialogWindow &dialog);
  void updateImgui();

private:
  void showQueuedDialogs();
  void compactDialogStack();
  void renderModalDialog(DialogWindow &dialog, int stack_index);
  void renderModelessDialog(DialogWindow &dialog);
  void renderDialog(int stack_index);

  dag::Vector<DialogWindow *> dialogStack;
  dag::Vector<DialogWindow *> dialogsToShow;
  bool updating = false;
  bool needsCompaction = false;
};

extern DialogManager dialog_manager;

} // namespace PropPanel
