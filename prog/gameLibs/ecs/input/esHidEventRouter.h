// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ecs/input/hidEventRouter.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiPointingData.h>
#include <drv/hid/dag_hiGlobals.h>
#include <generic/dag_tab.h>

using namespace HumanInput;

class ESHidEventRouter : public HumanInput::IGenPointingClient,
                         public HumanInput::IGenKeyboardClient,
                         public HumanInput::IGenJoystickClient
{
public:
#define DISPATCH_CLI_CALL(CLI, FUNC, ...) \
  if (CLI)                                \
  CLI->FUNC(__VA_ARGS__)

  void attached(IGenKeyboard *kbd) override
  {
    if (kbd)
    {
      kbd->enableLayoutChangeTracking(true);
      kbd->enableLocksChangeTracking(true);
    }
    DISPATCH_CLI_CALL(secKbd, attached, kbd);
  }

  void detached(IGenKeyboard *kbd) override { DISPATCH_CLI_CALL(secKbd, detached, kbd); }
  void attached(IGenPointing *pnt) override { DISPATCH_CLI_CALL(secPnt, attached, pnt); }
  void detached(IGenPointing *pnt) override { DISPATCH_CLI_CALL(secPnt, detached, pnt); }
  void attached(IGenJoystick *joy) override { DISPATCH_CLI_CALL(secJoy, attached, joy); }
  void detached(IGenJoystick *joy) override { DISPATCH_CLI_CALL(secJoy, detached, joy); }
  void gkcMagicCtrlAltEnd() override {}
  void gkcMagicCtrlAltF() override {}

#define PREPARE_CTX_K()                                    \
  ecs::IGenHidEventHandler::Context ctx;                   \
  const PointingRawState::Mouse &rs = raw_state_pnt.mouse; \
  ctx.keyModif = k->getRawState().shifts;                  \
  ctx.msX = rs.x;                                          \
  ctx.msY = rs.y;                                          \
  ctx.msButtons = rs.buttons;                              \
  ctx.msOverWnd = false;

#define PREPARE_CTX_M()                                         \
  ecs::IGenHidEventHandler::Context ctx;                        \
  const PointingRawState::Mouse &rs = pnt->getRawState().mouse; \
  ctx.keyModif = raw_state_kbd.shifts;                          \
  ctx.msX = rs.x;                                               \
  ctx.msY = rs.y;                                               \
  ctx.msButtons = rs.buttons;                                   \
  ctx.msOverWnd = pnt->isPointerOverWindow();

#define HANDLE_EVENT(EV_CALL)              \
  for (int i = 0; i < ehStack.size(); i++) \
    if (!ehStack[i]->ehIsActive())         \
      continue;                            \
    else if (ehStack[i]->EV_CALL)          \
      return;
  void gkcButtonDown(IGenKeyboard *k, int btn_idx, bool repeat, wchar_t wc) override
  {
    PREPARE_CTX_K();
    HANDLE_EVENT(gkehButtonDown(ctx, btn_idx, repeat, wc));
    DISPATCH_CLI_CALL(secKbd, gkcButtonDown, k, btn_idx, repeat, wc);
  }
  void gkcButtonUp(IGenKeyboard *k, int btn_idx) override
  {
    PREPARE_CTX_K();
    HANDLE_EVENT(gkehButtonUp(ctx, btn_idx));
    DISPATCH_CLI_CALL(secKbd, gkcButtonUp, k, btn_idx);
  }

  void gkcLayoutChanged(IGenKeyboard *k, const char *layout) override
  {
    PREPARE_CTX_K();
    HANDLE_EVENT(gkcLayoutChanged(ctx, layout));
    DISPATCH_CLI_CALL(secKbd, gkcLayoutChanged, k, layout);
  }

  void gkcLocksChanged(IGenKeyboard *k, unsigned locks) override
  {
    PREPARE_CTX_K();
    HANDLE_EVENT(gkcLocksChanged(ctx, locks));
    DISPATCH_CLI_CALL(secKbd, gkcLocksChanged, k, locks);
  }

  void gmcMouseMove(IGenPointing *pnt, float dx, float dy) override
  {
    PREPARE_CTX_M();
    HANDLE_EVENT(gmehMove(ctx, dx, dy));
    DISPATCH_CLI_CALL(secPnt, gmcMouseMove, pnt, dx, dy);
  }
  void gmcMouseButtonDown(IGenPointing *pnt, int btn_idx) override
  {
    PREPARE_CTX_M();
    HANDLE_EVENT(gmehButtonDown(ctx, btn_idx));
    DISPATCH_CLI_CALL(secPnt, gmcMouseButtonDown, pnt, btn_idx);
  }
  void gmcMouseButtonUp(IGenPointing *pnt, int btn_idx) override
  {
    PREPARE_CTX_M();
    HANDLE_EVENT(gmehButtonUp(ctx, btn_idx));
    DISPATCH_CLI_CALL(secPnt, gmcMouseButtonUp, pnt, btn_idx);
  }
  void gmcMouseWheel(IGenPointing *pnt, int scroll) override
  {
    PREPARE_CTX_M();
    HANDLE_EVENT(gmehWheel(ctx, scroll));
    DISPATCH_CLI_CALL(secPnt, gmcMouseWheel, pnt, scroll);
  }

  void gmcTouchBegan(IGenPointing *pnt, int touch_idx, const PointingRawState::Touch &touch) override
  {
    PREPARE_CTX_M();
    HANDLE_EVENT(gtehTouchBegan(ctx, pnt, touch_idx, touch));
    DISPATCH_CLI_CALL(secPnt, gmcTouchBegan, pnt, touch_idx, touch);
  }
  void gmcTouchMoved(IGenPointing *pnt, int touch_idx, const PointingRawState::Touch &touch) override
  {
    PREPARE_CTX_M();
    HANDLE_EVENT(gtehTouchMoved(ctx, pnt, touch_idx, touch));
    DISPATCH_CLI_CALL(secPnt, gmcTouchMoved, pnt, touch_idx, touch);
  }
  void gmcTouchEnded(IGenPointing *pnt, int touch_idx, const PointingRawState::Touch &touch) override
  {
    PREPARE_CTX_M();
    HANDLE_EVENT(gtehTouchEnded(ctx, pnt, touch_idx, touch));
    DISPATCH_CLI_CALL(secPnt, gmcTouchEnded, pnt, touch_idx, touch);
  }

  void gmcGesture(IGenPointing *pnt, Gesture::State state, const Gesture &gesture) override
  {
    PREPARE_CTX_M();
    HANDLE_EVENT(gpehGesture(ctx, pnt, state, gesture));
    DISPATCH_CLI_CALL(secPnt, gmcGesture, pnt, state, gesture);
  }

  void stateChanged(IGenJoystick *joy, int joy_ord_id) override
  {
    ecs::IGenHidEventHandler::Context ctx;
    const PointingRawState::Mouse &rs = raw_state_pnt.mouse;
    ctx.keyModif = raw_state_kbd.shifts;
    ctx.msX = rs.x;
    ctx.msY = rs.y;
    ctx.msButtons = rs.buttons;
    ctx.msOverWnd = false;

    HANDLE_EVENT(gjehStateChanged(ctx, joy, joy_ord_id));
    DISPATCH_CLI_CALL(secJoy, stateChanged, joy, joy_ord_id);
  }

#undef PREPARE_CTX_K
#undef PREPARE_CTX_M
#undef HANDLE_EVENT
#undef DISPATCH_CLI_CALL

public:
  static Tab<ecs::IGenHidEventHandler *> ehStack;
  static Tab<unsigned> ehStackPrio; // largest-to-smallest sorted array (parallel to ehStack)
  static ESHidEventRouter hidEvRouter;
  static HumanInput::IGenKeyboardClient *secKbd;
  static HumanInput::IGenPointingClient *secPnt;
  static HumanInput::IGenJoystickClient *secJoy;

  static void hid_event_handlers_clear();
};
