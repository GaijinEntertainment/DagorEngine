// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiPointing.h>
#include "ms_device.h"

namespace HumanInput
{
class TapMouseDevice;

class TapMouseClassDriver : public IGenPointingClassDrv
{
public:
  TapMouseClassDriver() : device(NULL), enabled(false), defClient(NULL) {}
  ~TapMouseClassDriver() { destroyDevices(); }

  bool init();
  void destroyDevices();

  // generic hid class driver interface
  virtual void enable(bool en);
  virtual void acquireDevices() {}
  virtual void unacquireDevices() {}
  virtual void destroy();

  virtual void refreshDeviceList();

  virtual int getDeviceCount() const { return device ? 1 : 0; }
  virtual void updateDevices() { device->update(); }

  // generic keyboard class driver interface
  IGenPointing *getDevice(int idx) const override;
  void useDefClient(IGenPointingClient *cli) override;

protected:
  TapMouseDevice *device;
  IGenPointingClient *defClient;
  bool enabled;
};
} // namespace HumanInput
