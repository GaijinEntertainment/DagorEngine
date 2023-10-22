// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DRV_HID_MOUSE_MS_DEVICE_COMMON_H
#define _GAIJIN_DRV_HID_MOUSE_MS_DEVICE_COMMON_H
#pragma once

#include <humanInput/dag_hiPointing.h>

namespace HumanInput
{

class GenericMouseDevice : public IGenPointing
{
public:
  GenericMouseDevice();
  ~GenericMouseDevice();

  // IGenPointing interface implementation
  const PointingRawState &getRawState() const override { return state; }
  void setClient(IGenPointingClient *cli) override;
  IGenPointingClient *getClient() const override { return client; }

  virtual int getBtnCount() const;
  virtual const char *getBtnName(int idx) const;

  virtual void setClipRect(int l, int t, int r, int b);

  virtual void setMouseCapture(void *handle);
  virtual void releaseMouseCapture();

  virtual void hideMouseCursor(bool hide);
  virtual bool isMouseCursorHidden() { return hidden; }
  virtual bool isLeftHanded() const { return areButtonsSwapped; }

  struct ClipRect
  {
    int l, t, r, b;
  };

protected:
  bool hidden;
  bool areButtonsSwapped;
  IGenPointingClient *client;
  PointingRawState state;
  int accDz;
  ClipRect clip;

  bool clampStateCoord();
};
} // namespace HumanInput

#endif
