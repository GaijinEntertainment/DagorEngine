// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#import <GameController/GameController.h>

#include <EASTL/array.h>

namespace HumanInput
{
enum
{
  IOS_SENSOR_ADDITIONAL_AXIS_NUM = 8
};
} // namespace HumanInput

class iOSGamepadEx : public HumanInput::IGenJoystick
{
public:
  iOSGamepadEx(GCExtendedGamepad *gpad, const char *name);

  void clearGamepad();

  // general routines
  const char *getName() const override { return fullName; };
  const char *getDeviceID() const override { return "Gamepad"; }
  bool getDeviceDesc(DataBlock &out_desc) const override { return false; }
  unsigned short getUserId() const override { return 0; }

  const HumanInput::JoystickRawState &getRawState() const override { return rawState; }

  void setClient(HumanInput::IGenJoystickClient *cli) override;
  HumanInput::IGenJoystickClient *getClient() const override { return client; };

  // buttons and axes description
  int getButtonCount() const override { return BUTTONS_NUM; }
  const char *getButtonName(int idx) const override;
  int getAxisCount() const override { return AXES_NUM; }
  const char *getAxisName(int idx) const override;
  int getPovHatCount() const override { return 0; }
  const char *getPovHatName(int idx) const override { return ""; }

  // buttons and axes name resolver
  int getButtonId(const char *name) const override;
  int getAxisId(const char *name) const override;
  int getPovHatId(const char *name) const override { return -1; };

  // axis limits remap setup
  void setAxisLimits(int axis_id, float min_val, float max_val) override;

  // current state information
  bool getButtonState(int btn_id) const override;
  float getAxisPos(int axis_id) const override;
  int getAxisPosRaw(int axis_id) const override;

  int getPovHatAngle(int hat_id) const override { return -1; };

  // returns device connection status
  bool isConnected() override;

  // to call from driver
  void update();

  bool isStickDeadZoneSetupSupported() const override { return true; }
  float getStickDeadZoneScale(int stick_idx) const override
  {
    G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
    return stickDeadZoneScale[stick_idx];
  }
  void setStickDeadZoneScale(int stick_idx, float scale) override;
  float getStickDeadZoneAbs(int stick_idx) const override;

private:
  // read input from gamepad
  void processDPad();
  void processButtons();
  void processLeftStick();
  void processRightStick();
  void processGamepadInput();
  void updatePressTime();

  // gyroscope
  void updateGyroscopeParameters();

  void notifyClient();

  const char *fullName;
  GCExtendedGamepad *gamepad = nullptr;
  HumanInput::JoystickRawState rawState{};
  HumanInput::IGenJoystickClient *client = nullptr;

  static constexpr int BUTTONS_NUM = HumanInput::JOY_XINPUT_REAL_BTN_COUNT;
  static constexpr int AXES_NUM = HumanInput::JOY_XINPUT_REAL_AXIS_COUNT + HumanInput::IOS_SENSOR_ADDITIONAL_AXIS_NUM;
  struct iOSAxis
  {
    float val = 0;
    float min = -1.f;
    float max = 1.f;
  };
  eastl::array<iOSAxis, AXES_NUM> axis{};

  float stickDeadZoneScale[2] = {1.f, 1.f};
  float stickDeadZoneBase[2] = {0.2f, 0.2f};
};
