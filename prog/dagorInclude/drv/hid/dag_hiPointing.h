//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/hid/dag_hiDecl.h>
#include <drv/hid/dag_hiHidClassDrv.h>
#include <drv/hid/dag_hiPointingData.h>

namespace HumanInput
{

// generic mouse interface
class IGenPointing
{
public:
  // general routines
  virtual const char *getName() const = 0;
  virtual const PointingRawState &getRawState() const = 0;
  virtual void setClient(IGenPointingClient *cli) = 0;
  virtual IGenPointingClient *getClient() const = 0;
  // query the last hardware state. That can be crucial for latency free actions
  virtual void fetchLastState() {}

  // buttons description
  virtual int getBtnCount() const = 0;
  virtual const char *getBtnName(int idx) const = 0;

  // mode of operation setup
  //   relative movement mode provides deltaX/deltaY update everytime
  //   mouse is moved (is there is hard clipping, e.g. winmouse, driver
  //   can implement mouse pointer wrapping to enable this mode)
  virtual void setRelativeMovementMode(bool enable) = 0;
  virtual bool getRelativeMovementMode() = 0;

  virtual void setClipRect(int l, int t, int r, int b) = 0;
  virtual void setPosition(int x, int y) = 0;

  virtual void setMouseCapture(void *handle) = 0;
  virtual void releaseMouseCapture() = 0;
  virtual bool isPointerOverWindow() = 0;
  virtual void hideMouseCursor(bool /*hide*/) {}
  virtual bool isMouseCursorHidden() { return true; }
  virtual bool isLeftHanded() const { return false; }
  virtual void setAlwaysClipToWindow(bool /*on*/) {}
};


// simple mouse client interface
class IGenPointingClient
{
public:
  virtual void attached(IGenPointing *mouse) = 0;
  virtual void detached(IGenPointing *mouse) = 0;

  virtual void gmcMouseMove(IGenPointing *mouse, float dx, float dy) = 0;
  virtual void gmcMouseButtonDown(IGenPointing *mouse, int btn_idx) = 0;
  virtual void gmcMouseButtonUp(IGenPointing *mouse, int btn_idx) = 0;
  virtual void gmcMouseWheel(IGenPointing *mouse, int scroll) = 0;

  virtual void gmcTouchBegan(IGenPointing * /*pnt*/, int /*touch_idx*/, const PointingRawState::Touch & /*touch*/) {}
  virtual void gmcTouchMoved(IGenPointing * /*pnt*/, int /*touch_idx*/, const PointingRawState::Touch & /*touch*/) {}
  virtual void gmcTouchEnded(IGenPointing * /*pnt*/, int /*touch_idx*/, const PointingRawState::Touch & /*touch*/) {}

  virtual void gmcGesture(IGenPointing * /*pnt*/, Gesture::State /*state*/, const Gesture & /*gesture*/) {}
};


// generic mouse class driver interface
class IGenPointingClassDrv : public IGenHidClassDrv
{
public:
  virtual IGenPointing *getDevice(int idx) const = 0;
  virtual void useDefClient(IGenPointingClient *cli) = 0;

  inline void hideMouseCursor(bool hide)
  {
    if (IGenPointing *m = getDeviceCount() ? getDevice(0) : NULL)
      m->hideMouseCursor(hide);
  }

  inline bool isMouseCursorHidden()
  {
    if (IGenPointing *m = getDeviceCount() ? getDevice(0) : NULL)
      return m->isMouseCursorHidden();
    return true;
  }
};
} // namespace HumanInput
