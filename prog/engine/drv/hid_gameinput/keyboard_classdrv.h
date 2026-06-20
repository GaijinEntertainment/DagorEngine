// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiKeyboard.h>

namespace HumanInput
{
class GameInputKeyboardDevice;


class GameInputKeyboardClassDriver final : public IGenKeyboardClassDrv
{
public:
  GameInputKeyboardClassDriver() = default;
  ~GameInputKeyboardClassDriver() { destroyDevices(); }

  bool init();
  void destroyDevices();

  // generic hid class driver interface
  void enable(bool en) override;
  void acquireDevices() override {}
  void unacquireDevices() override {}
  void destroy() override;

  void refreshDeviceList() override;

  int getDeviceCount() const override { return device ? 1 : 0; }
  void updateDevices() override;

  // generic keyboard class driver interface
  IGenKeyboard *getDevice(int idx) const override;
  bool isDeviceConfigChanged() const override { return false; }
  void useDefClient(IGenKeyboardClient *cli) override;

protected:
  GameInputKeyboardDevice *device = nullptr;
  IGenKeyboardClient *defClient = nullptr;
  bool enabled = false;
};
} // namespace HumanInput
