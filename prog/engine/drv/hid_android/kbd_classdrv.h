// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiKeyboard.h>
#include "kbd_device.h"

namespace HumanInput
{
class ScreenKeyboardClassDriver : public IGenKeyboardClassDrv
{
public:
  ScreenKeyboardClassDriver() : device(NULL), enabled(false), defClient(NULL) {}
  ~ScreenKeyboardClassDriver() { destroyDevices(); }

  bool init();
  void destroyDevices();

  // generic hid class driver interface
  virtual void enable(bool en);
  virtual void acquireDevices() {}
  virtual void unacquireDevices() {}
  virtual void destroy();

  virtual void refreshDeviceList();

  virtual int getDeviceCount() const { return device ? 1 : 0; }
  virtual void updateDevices() {}

  // generic keyboard class driver interface
  virtual IGenKeyboard *getDevice(int idx) const { return (idx == 0) ? device : NULL; }
  virtual bool isDeviceConfigChanged() const { return false; }
  virtual void useDefClient(IGenKeyboardClient *cli);

protected:
  ScreenKeyboardDevice *device;
  IGenKeyboardClient *defClient;
  bool enabled;
};
} // namespace HumanInput
