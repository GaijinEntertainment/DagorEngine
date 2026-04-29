// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/toastManager.h>
#include <propPanel/propPanel.h>

#include <dag/dag_vector.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>
#include <util/dag_string.h>

namespace PropPanel
{

class ToastManager
{
public:
  ToastManager();

  void loadIcons();

  void updateImgui();

  // Fire & forget api to queue and display toast messages.
  void setToastMessage(ToastMessage message);

  void updateImguiDebugPanel();

private:
  struct Icons
  {
    PropPanel::IconId getForType(ToastType type);

    PropPanel::IconId alert = PropPanel::IconId::Invalid;
    PropPanel::IconId success = PropPanel::IconId::Invalid;
  };

  dag::Vector<ToastMessage> messages;
  Icons icons;

  int debugOpaque = 3000;
  int debugFadeout = 500;
  String debugMessage;
  int debugType = 0;
  int debugPosType = 0;
  IPoint2 debugPos;
  bool debugCenterOnPos = false;
  bool debugFadeoutOnMouseLeave = false;
  bool debugDelayed = false;
  bool debugCreateDelayed = false;
  uint64_t debugDelayTimeMs = 0;
};

void imgui_toast_debug_panel();

} // namespace PropPanel
