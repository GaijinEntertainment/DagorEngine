// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <supp/dag_math.h>
#include <workCycle/dag_workCycle.h>
#include "emu_hooks.h"
#include <string.h>
#include <stdlib.h>
#include <debug/dag_debug.h>
#include <math/integer/dag_IPoint2.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>

#if _TARGET_XBOX
#include <osApiWrappers/gdk/gameinput.h>
#endif


#define LOW_THRES   2000
#define MOUSE_SPEED 0.01

using namespace HumanInput;

class MouseEmuDriver final : public IGenPointingClassDrv, public IGenPointing
{
public:
  bool emuDriverEnabled = false;
  bool emuCursorEnabled = true;
  bool emuButtonsEnabled = true;
  bool hwMouseEnabled = false;


  MouseEmuDriver()
  {
    client = NULL;
    clip.l = 0;
    clip.t = 0;
    clip.r = 1024;
    clip.b = 1024;
    relMode = true;
    incX = 0.0;
    incY = 0.0;
    lastButtons = 0;
    lastHwButtons = 0;
  }
  ~MouseEmuDriver() { destroy(); }

  // generic hid class driver interface
  virtual void enable(bool en)
  {
    emuDriverEnabled = en;
    stg_pnt.mouseEnabled = (emuDriverEnabled && (emuCursorEnabled || emuButtonsEnabled)) || hwMouseEnabled;
  }
  virtual void acquireDevices() {}
  virtual void unacquireDevices() {}
  virtual void destroy()
  {
    enable(false);
    setClient(NULL);
  }
  virtual void refreshDeviceList() {}

  virtual int getDeviceCount() const { return 1; }
  virtual void updateDevices()
  {
    if (!is_main_thread()) // skip calls from gamepad updateDevice() in polling thread
      return;
    if (emuDriverEnabled && (emuCursorEnabled || emuButtonsEnabled))
    {
      uint64_t bw0 = raw_state_joy.buttons.getDWord0();

#if _TARGET_XBOX
      // Use second joystick from composite joystick as xinput-compatible device.
      const uint64_t lbMask = JOY_XINPUT_REAL_MASK_R_TRIGGER | (uint64_t)JOY_XINPUT_REAL_MASK_R_TRIGGER << JOY_XINPUT_REAL_BTN_COUNT;
      const int dx = raw_state_joy.x;
      const int dy = raw_state_joy.y;
#else
      const uint64_t lbMask = JOY_XINPUT_REAL_MASK_R_THUMB;
      const int dx = raw_state_joy.rx;
      const int dy = raw_state_joy.ry;
#endif

      if (emuButtonsEnabled)
      {
        if (!::stg_pnt.allowEmulatedLMB)
        {
          if (lastButtons & lbMask)
            onButtonUp(0);
          lastButtons = 0;
        }
        else
        {
          // emulate LMB with right thumb click
          if ((bw0 & lbMask) && !(lastButtons & lbMask))
            onButtonDown(0);
          else if (!(bw0 & lbMask) && (lastButtons & lbMask))
            onButtonUp(0);

          lastButtons = bw0;
        }
      }

      if (emuCursorEnabled)
      {
        int vx = abs(dx), vy = abs(dy);
        if (vx < LOW_THRES)
          vx = 0;
        else
          vx = (dx > 0) ? dx - LOW_THRES : dx + LOW_THRES;

        if (vy < LOW_THRES)
          vy = 0;
        else
          vy = (dy > 0) ? dy - LOW_THRES : dy + LOW_THRES;

        if (vx || vy)
        {
          incX += vx * ::dagor_game_act_time * MOUSE_SPEED;
          incY -= vy * ::dagor_game_act_time * MOUSE_SPEED;

          if (fabsf(incX) > 1.0f || fabsf(incY) > 1.0f)
          {
            vx = incX > 0 ? floorf(incX) : ceilf(incX);
            vy = incY > 0 ? floorf(incY) : ceilf(incY);
            incX -= vx;
            incY -= vy;

            onMouseMove(vx, vy);
          }
        }
      }
    }

    if (hwMouseEnabled)
      updateHWMouse();
  }

  // generic keyboard class driver interface
  IGenPointing *getDevice(int idx) const override { return (MouseEmuDriver *)this; }
  void useDefClient(IGenPointingClient *cli) override { setClient(cli); }

  // IGenPointing interface implementation
  virtual const char *getName() const { return "Mouse @ Gamepad360"; }

  const PointingRawState &getRawState() const override { return raw_state_pnt; }
  void setClient(IGenPointingClient *cli) override
  {
    if (cli == client)
      return;

    if (client)
      client->detached(this);
    client = cli;
    if (client)
      client->attached(this);
  }
  IGenPointingClient *getClient() const override { return client; }

  int getBtnCount() const { return 1; }
  const char *MouseEmuDriver::getBtnName(int idx) const
  {
    static const char *btn_names[7] = {"LMB", "RMB", "MMB", "M4B", "M5B", "MWUp", "MWDown"};
    return idx >= 0 && idx < countof(btn_names) ? btn_names[idx] : nullptr;
  }

  virtual void setClipRect(int l, int t, int r, int b)
  {
    clip.l = l;
    clip.t = t;
    clip.r = r;
    clip.b = b;
    // DEBUG_CTX("set clip rect: (%d,%d)-(%d,%d)", l, t, r, b);
    if (clampStateCoord())
      setPosition(raw_state_pnt.mouse.x, raw_state_pnt.mouse.y);
  }


  virtual void setMouseCapture(void * /*handle*/) {}
  virtual void releaseMouseCapture() {}

  virtual void setRelativeMovementMode(bool enable) { relMode = enable; }
  virtual bool getRelativeMovementMode() { return relMode; }

  virtual void setPosition(int x, int y)
  {
    raw_state_pnt.mouse.x = x;
    raw_state_pnt.mouse.y = y;
    clampStateCoord();

    if (client)
      client->gmcMouseMove(this, 0, 0);
  }
  virtual bool isPointerOverWindow() { return true; }

protected:
  IGenPointingClient *client;
  struct
  {
    int l, t, r, b;
  } clip;
  float incX, incY;
  bool relMode;
  uint64_t lastButtons;
  uint64_t lastHwButtons;
  IPoint2 currentHwPos = IPoint2(0, 0);
  int currentHwWheel = 0;

  void onMouseMove(int _dx, int _dy)
  {
    float dx = _dx * stg_pnt.xSens, dy = _dy * stg_pnt.ySens;

    raw_state_pnt.mouse.deltaX += dx;
    raw_state_pnt.mouse.deltaY += dy;

    raw_state_pnt.mouse.x += dx;
    raw_state_pnt.mouse.y += dy;
    clampStateCoord();

    if (client && (dx || dy))
      client->gmcMouseMove(this, dx, dy);
  }

  void onButtonDown(int btn_id)
  {
    raw_state_pnt.mouse.buttons |= 1 << btn_id;
    if (client)
      client->gmcMouseButtonDown(this, btn_id);
  }

  void onButtonUp(int btn_id)
  {
    raw_state_pnt.mouse.buttons &= ~(1 << btn_id);
    if (client)
      client->gmcMouseButtonUp(this, btn_id);
  }

  void onMouseWheel(int delta)
  {
    raw_state_pnt.mouse.deltaZ += delta * stg_pnt.zSens;
    if (client && delta)
      client->gmcMouseWheel(this, delta);
  }

  bool clampStateCoord()
  {
    bool clipped = false;

    if (raw_state_pnt.mouse.x < clip.l)
    {
      raw_state_pnt.mouse.x = clip.l;
      clipped = true;
    }
    if (raw_state_pnt.mouse.y < clip.t)
    {
      raw_state_pnt.mouse.y = clip.t;
      clipped = true;
    }
    if (raw_state_pnt.mouse.x > clip.r)
    {
      raw_state_pnt.mouse.x = clip.r;
      clipped = true;
    }
    if (raw_state_pnt.mouse.y > clip.b)
    {
      raw_state_pnt.mouse.y = clip.b;
      clipped = true;
    }

    return clipped;
  }

#if _TARGET_XBOX

  struct MouseState
  {
    IPoint2 currentHwPos = {0, 0};
    uint64_t lastHwButtons = 0;
    int currentHwWheel = 0;
  };

  MouseState mouse_states[gdk::gameinput::MAX_DEVICES_PER_TYPE] = {};

  void updateMouseState(MouseState &state, IGameInputDevice *device)
  {
    if (!device)
      return;

    gdk::gameinput::Reading reading = gdk::gameinput::get_current_reading(GameInputKindMouse, device);
    if (reading)
    {
      GameInputMouseState mouseState;
      if (reading->GetMouseState(&mouseState))
      {
        static uint32_t mouseButtons[] = {
          GameInputMouseButtons::GameInputMouseLeftButton,
          GameInputMouseButtons::GameInputMouseRightButton,
          GameInputMouseButtons::GameInputMouseMiddleButton,
          GameInputMouseButtons::GameInputMouseButton4,
          GameInputMouseButtons::GameInputMouseButton5,
        };

        for (int buttonNo = 0; buttonNo < countof(mouseButtons); buttonNo++)
        {
          uint64_t buttonBit = 1ull << buttonNo;
          if (mouseState.buttons & mouseButtons[buttonNo])
          {
            if (!(state.lastHwButtons & buttonBit))
              onButtonDown(buttonNo);
            state.lastHwButtons |= buttonBit;
          }
          else
          {
            if (state.lastHwButtons & buttonBit)
              onButtonUp(buttonNo);
            state.lastHwButtons &= ~buttonBit;
          }
        }

        IPoint2 newHwPos(mouseState.positionX, mouseState.positionY);
        IPoint2 delta = newHwPos - state.currentHwPos;
        delta *= max(1, clip.b / 1080);
        state.currentHwPos = newHwPos;
        if (delta.x != 0 || delta.y != 0)
          onMouseMove(delta.x, delta.y);

        int wheelInput = mouseState.wheelY;
        int wheelDelta = wheelInput - state.currentHwWheel;
        state.currentHwWheel = wheelInput;
        if (wheelDelta != 0)
          onMouseWheel(wheelDelta);
      }
    }
  }

  void updateHWMouse()
  {
    gdk::gameinput::DevicesList mouses;
    gdk::gameinput::get_devices(GameInputKindMouse, mouses);

    G_ASSERT(mouses.size() <= gdk::gameinput::MAX_DEVICES_PER_TYPE);

    for (size_t mouseNo = 0; mouseNo < mouses.size(); ++mouseNo)
      updateMouseState(mouse_states[mouseNo], mouses[mouseNo]);
  }
#else
  void updateHWMouse() {}
#endif
};


static MouseEmuDriver drv;

IGenPointingClassDrv *HumanInput::createMouseEmuClassDriver()
{
  memset(&raw_state_pnt, 0, sizeof(raw_state_pnt));
  humaninputxbox360::mouse_emu = &drv;
  return &drv;
}


void enable_xbox_hw_mouse(bool en)
{
  drv.hwMouseEnabled = en;
  stg_pnt.mouseEnabled = (drv.emuDriverEnabled && (drv.emuCursorEnabled || drv.emuButtonsEnabled)) || drv.hwMouseEnabled;
}


bool is_xbox_hw_mouse_enabled() { return drv.hwMouseEnabled; }


void enable_xbox_emulated_mouse(bool cursor, bool buttons)
{
  drv.emuCursorEnabled = cursor;
  drv.emuButtonsEnabled = buttons;
  stg_pnt.mouseEnabled = (drv.emuDriverEnabled && (drv.emuCursorEnabled || drv.emuButtonsEnabled)) || drv.hwMouseEnabled;
}
