// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiKeyboard.h>

namespace HumanInput
{
class WinKeyboardDevice;


class WinKeyboardClassDriver : public IGenKeyboardClassDrv
{
public:
  WinKeyboardClassDriver() : device(NULL), enabled(false), defClient(NULL) {}
  ~WinKeyboardClassDriver() { destroyDevices(); }

  bool init();
  void destroyDevices();

  // generic hid class driver interface
  virtual void enable(bool en);
  virtual void acquireDevices();
  virtual void unacquireDevices() {}
  virtual void destroy();

  virtual void refreshDeviceList();

  virtual int getDeviceCount() const { return device ? 1 : 0; }
  virtual void updateDevices() {}

  // generic keyboard class driver interface
  virtual IGenKeyboard *getDevice(int idx) const;
  virtual bool isDeviceConfigChanged() const { return false; }
  virtual void useDefClient(IGenKeyboardClient *cli);

protected:
  WinKeyboardDevice *device;
  IGenKeyboardClient *defClient;
  bool enabled;
};
} // namespace HumanInput
