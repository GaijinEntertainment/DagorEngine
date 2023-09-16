// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include <humanInput/dag_hiPointing.h>
#include <osApiWrappers/dag_wndProcComponent.h>

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

  virtual int getBtnCount() const { return 21; }
  virtual const char *getBtnName(int idx) const
  {
    switch (idx)
    {
      case 0: return "TAP";
      case 1: return "TOUCH2";
      case 2: return "TOUCH3";
      case 3: return "TOUCH4";
      case 4: return "TOUCH5";
      case 19: return "LONG-TOUCH";
      case 20: return "TOUCH";
    }
    return NULL;
  }

  virtual void setClipRect(int l, int t, int r, int b);

  virtual void setMouseCapture(void *handle) {}
  virtual void releaseMouseCapture() {}

  virtual const char *getName() const { return "Mouse@TapEmu-iOS"; }

  virtual void setRelativeMovementMode(bool enable) {}
  virtual bool getRelativeMovementMode() { return false; }

  virtual void setPosition(int x, int y);
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
