// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#import <GameController/GameController.h>

#include <EASTL/array.h>
#include <util/dag_simpleString.h>

namespace HumanInput
{
static int constexpr IOS_SENSOR_ADDITIONAL_AXIS_NUM = 8;
} // namespace HumanInput

class iOSGamepadEx final : public HumanInput::IGenJoystick
{
public:
  iOSGamepadEx(GCController *gc, const char *name);
  ~iOSGamepadEx();

  void clearGamepad();
  bool isController(GCController *gc) const { return controller == gc; }
  GCController *getController() const { return gamepad ? controller : nil; }

  bool setMotionActive(bool want);

  // general routines
  const char *getName() const override { return fullName.str(); }
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
  bool isXinputCompatible() const override { return true; }

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

  bool isDeviceGyro() const override { return gamepad == nullptr; }

private:
  // read input from gamepad
  void processButtons();
  void processLeftStick();
  void processRightStick();
  void processTriggers();
  void processGamepadInput();
  void updatePressTime();
  void setPressLatch(bool enable);
  void latchButton(GCControllerButtonInput *input, int btn, bool enable);

  // gyroscope
  void updateGyroscopeParameters();
  void readPadMotion(double &rate_x, double &rate_y, double &acc_x, double &acc_y, double &acc_z) const;

  void notifyClient();

  SimpleString fullName;
  GCController *controller = nil;
  GCExtendedGamepad *gamepad = nil;
  HumanInput::JoystickRawState rawState{};
  HumanInput::IGenJoystickClient *client = nullptr;

  static constexpr int BUTTONS_NUM = HumanInput::JOY_XINPUT_REAL_BTN_COUNT;
  static constexpr int AXES_NUM = HumanInput::JOY_XINPUT_REAL_AXIS_COUNT + HumanInput::IOS_SENSOR_ADDITIONAL_AXIS_NUM;
  struct iOSAxis
  {
    float val = 0;
    float kMul = 1.f;
    float kAdd = 0.f;
  };
  eastl::array<iOSAxis, AXES_NUM> axis{};

  float stickDeadZoneScale[2] = {1.f, 1.f};
  float stickDeadZoneBase[2] = {0.2f, 0.2f};

  int prevUpdateTimeMsec = 0;
  unsigned latchedPressMask = 0;
  bool motionActive = false;
};
