// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/immediateFocusLossHandler.h>

namespace PropPanel
{

IImmediateFocusLossHandler *focused_immediate_focus_loss_handler = nullptr;

void send_immediate_focus_loss_notification()
{
  if (focused_immediate_focus_loss_handler)
  {
    focused_immediate_focus_loss_handler->onImmediateFocusLoss();
    focused_immediate_focus_loss_handler = nullptr;
  }
}

} // namespace PropPanel