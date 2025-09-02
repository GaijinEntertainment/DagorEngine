// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "joy_device.h"
#include <EASTL/unique_ptr.h>

namespace HumanInput
{

class HidJoystickDeviceXInput : public UDevJoystick
{
public:
  HidJoystickDeviceXInput(eastl::unique_ptr<HidJoystickDevice> device);
  virtual ~HidJoystickDeviceXInput() = default;

  static bool device_xinput_compatable(HidJoystickDevice *device);

  const char *getName() const override { return joydev->getName(); }
  const char *getDeviceID() const override { return joydev->getDeviceID(); }
  bool getDeviceDesc(DataBlock &) const override { return false; }
  unsigned short getUserId() const override { return 0; }
  const JoystickRawState &getRawState() const override { return state; }
  void setClient(IGenJoystickClient *cli) override;
  IGenJoystickClient *getClient() const override { return client; }

  int getButtonCount() const override;
  const char *getButtonName(int idx) const override;

  int getAxisCount() const override;
  const char *getAxisName(int idx) const override;

  int getPovHatCount() const override { return joydev->getPovHatCount(); }
  const char *getPovHatName(int idx) const override { return joydev->getPovHatName(idx); }

  int getButtonId(const char *name) const override;
  int getAxisId(const char *name) const override;
  int getPovHatId(const char *name) const override { return joydev->getPovHatId(name); }
  void setAxisLimits(int axis_id, float min_val, float max_val) override;
  bool getButtonState(int btn_id) const override;
  float getAxisPos(int axis_id) const override;
  int getAxisPosRaw(int axis_id) const override;
  int getPovHatAngle(int axis_id) const override;

  bool isConnected() override { return joydev->isConnected(); }

  void updateDevice(UDev::Device const &dev) override { joydev->updateDevice(dev); }
  const char *getModel() const override { return joydev->getModel(); }
  virtual const char *getNode() const { return joydev->getNode(); }

  bool updateState(int dt_msec, bool def) override;
  void setConnected(bool con) override { joydev->setConnected(con); }

private:
  bool updateSelfState();
  void applyCircularDeadZone(int axis_x, int axis_y, int deadzone);

  int getVirtualPOVAxis(int hat_id, int axis_id) const;

  eastl::unique_ptr<HidJoystickDevice> joydev;
  dag::Vector<AxisData> axes;
  dag::Vector<ButtonData> buttons;
  JoystickRawState state;
  IGenJoystickClient *client = nullptr;

  int lThumbDeadZone = 0;
  int rThumbDeadZone = 0;
};

} // namespace HumanInput
