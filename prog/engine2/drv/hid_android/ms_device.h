// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include <humanInput/dag_hiPointing.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <math/dag_mathBase.h>

namespace HumanInput
{
class TapMouseDevice : public IGenPointing, public IWndProcComponent
{
public:
  TapMouseDevice();
  ~TapMouseDevice();

  // IGenPointing interface implementation
  const PointingRawState &getRawState() const override { return state; }
  void setClient(IGenPointingClient *cli) override;
  IGenPointingClient *getClient() const override { return client; }

  virtual int getBtnCount() const { return 1; }
  virtual const char *getBtnName(int idx) const { return idx == 0 ? "TAP" : NULL; }

  virtual void setClipRect(int l, int t, int r, int b)
  {
    if (clipX0 == l && clipY0 == t && clipX1 == r && clipY1 == b)
      return;
    clipX0 = l, clipY0 = t, clipX1 = r, clipY1 = b;
    state.mouse.x = (l + r) / 2;
    state.mouse.y = (t + b) / 2;
  }

  virtual void setMouseCapture(void *handle) {}
  virtual void releaseMouseCapture() {}

  virtual const char *getName() const { return "Mouse@TapEmu-android"; }

  virtual void setRelativeMovementMode(bool enable) {}
  virtual bool getRelativeMovementMode() { return false; }

  virtual void setPosition(int x, int y)
  {
    state.mouse.x = clamp(x, clipX0, clipX1);
    state.mouse.y = clamp(y, clipY0, clipY1);
  }
  virtual bool isPointerOverWindow() { return true; }

  // IWndProcComponent interface implementation
  virtual RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result);

  void update();

protected:
  IGenPointingClient *client;
  PointingRawState state;
  int lbmPressedT0, lbmReleasedT0, lbmMotion, lbmMotionDx, lbmMotionDy;
  int clipX0 = 0, clipY0 = 0, clipX1 = 128, clipY1 = 128;
};
} // namespace HumanInput
