// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiPointing.h>
#include <math/integer/dag_IPoint2.h>
#include <osApiWrappers/gdk/gameinput.h>

namespace HumanInput
{
class MouseEmuDriver final : public IGenPointingClassDrv, public IGenPointing
{
public:
  bool emuDriverEnabled = false;
  bool emuCursorEnabled = true;
  bool emuButtonsEnabled = true;
  bool hwMouseEnabled = false;

  MouseEmuDriver() = default;
  ~MouseEmuDriver();

  // generic hid class driver interface
  void enable(bool en) override;
  void acquireDevices() override {}
  void unacquireDevices() override {}
  void destroy() override;
  void refreshDeviceList() override {}

  int getDeviceCount() const override { return 1; }
  void updateDevices() override;

  // generic keyboard class driver interface
  IGenPointing *getDevice(int idx) const override { return (MouseEmuDriver *)this; }
  void useDefClient(IGenPointingClient *cli) override { setClient(cli); }

  // IGenPointing interface implementation
  const char *getName() const override { return "Mouse @ Gamepad"; }

  const PointingRawState &getRawState() const override;
  void setClient(IGenPointingClient *cli) override;
  IGenPointingClient *getClient() const override { return client; }

  int getBtnCount() const override { return 1; }
  const char *getBtnName(int idx) const override;

  void setClipRect(int l, int t, int r, int b) override;

  void setMouseCapture(void * /*handle*/) override {}
  void releaseMouseCapture() override {}

  void setRelativeMovementMode(bool enable) override { relMode = enable; }
  bool getRelativeMovementMode() override { return relMode; }

  void setPosition(int x, int y) override;
  bool isPointerOverWindow() override { return true; }

protected:
  IGenPointingClient *client = nullptr;
  struct
  {
    int l, t, r, b;
  } clip = {0, 0, 1024, 1024};
  float incX = 0.f, incY = 0.f;
  bool relMode = true;
  uint64_t lastButtons = 0;

  void onMouseMove(int _dx, int _dy);
  void onButtonDown(int btn_id);
  void onButtonUp(int btn_id);
  void onMouseWheel(int delta);
  bool clampStateCoord();

  struct MouseState
  {
    IPoint2 currentHwPos = {0, 0};
    uint64_t lastHwButtons = 0;
    int currentHwWheel = 0;
  };

  MouseState mouse_states[gdk::gameinput::MAX_DEVICES_PER_TYPE] = {};

  void updateMouseState(MouseState &state, IGameInputDevice *device);
  void updateHWMouse();
};
} // namespace HumanInput
