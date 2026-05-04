// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "cursorState.h"
#include <drv/hid/dag_hiDecl.h>


namespace darg
{

class MousePointer : public CursorPosState
{
public:
  void initMousePos();
  void onMouseMoveEvent(const Point2 &p);
  void setMousePos(const Point2 &p, bool reset_target); // immediate sync (device switch, external API)

  // Per-frame smooth animation step toward targetPos (internal only, no OS sync).
  bool moveToTarget(float dt);

  // Push current pos to OS driver. Call once per frame after all internal updates.
  void commitToDriver(bool force = false);

  // Mark that pos was changed internally and needs OS sync.
  void markNeedsDriverSync() { needsDriverSync = true; }

  static HumanInput::IGenPointing *get_mouse();

  void onShutdown();
  void syncMouseCursorLocation();
  void onActivateSceneInput();
  void onAppActivate();

  bool isProgrammaticMove() const { return isSettingMousePos; }

  bool mouseWasRelativeOnLastUpdate = false;

private:
  bool isSettingMousePos = false;
  bool needsDriverSync = false;
};

} // namespace darg
