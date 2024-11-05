// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ecs/input/hidEventRouter.h>

struct DearImGuiInputHandler : public ecs::IGenHidEventHandler
{
  DearImGuiInputHandler();
  bool ehIsActive() const override;
  bool gmehMove(const Context &ctx, float dx, float dy) override;
  bool gmehButtonDown(const Context &ctx, int btn_idx) override;
  bool gmehButtonUp(const Context &ctx, int btn_idx) override;
  bool gmehWheel(const Context &ctx, int dz) override;
  bool gkehButtonDown(const Context &ctx, int btn_idx, bool repeat, wchar_t wchar) override;
  bool gkehButtonUp(const Context &ctx, int btn_idx) override;
  bool gjehStateChanged(const Context &ctx, HumanInput::IGenJoystick *joy, int joy_ord_id) override;
  bool gtehTouchBegan(const Context &ctx, HumanInput::IGenPointing *, int touch_idx,
    const HumanInput::PointingRawState::Touch &touch) override;
  bool gtehTouchMoved(const Context &ctx, HumanInput::IGenPointing *, int touch_idx,
    const HumanInput::PointingRawState::Touch &touch) override;
  bool gtehTouchEnded(const Context &ctx, HumanInput::IGenPointing *, int touch_idx,
    const HumanInput::PointingRawState::Touch &touch) override;

  bool hybridInput = true;
  bool drawMouseCursor = true;
  int viewPortOffsetX = 0;
  int viewPortOffsetY = 0;

private:
  void getModifiers(const Context &ctx, bool &ctrl, bool &shift, bool &alt);
  void enableTouchMode();
  void enableMouseMode();
  int activeTouchIdx = -1;
};