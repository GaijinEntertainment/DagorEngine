// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiJoyTriggerEffect.h>
#include <drv/hid/dag_hiTouchpadGestures.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <math/dag_Point3.h>
#include <math/dag_Quat.h>
#include "dsPadProtocol.h"
#include "../hid_commonCode/joystickAxisUtils.h"

namespace HumanInput
{

// One DualSense, DualSense Edge or DualShock 4. The driver IO thread feeds reports in and takes output
// reports out; the main thread reads the IGenJoystick state. Button and axis numbering equals the PS5 pad
// driver so the console presets and glyphs apply unchanged.
class SonyPadDevice final : public IGenJoystick
{
public:
  enum
  {
    AXES_NUM = 10,
    BUTTONS_NUM = 35,
    BTN_CREATE_IDX = 28,
    BTN_PS_IDX = 29,
    BTN_MUTE_IDX = 30,
    BTN_FN_LEFT_IDX = 31,
    BTN_FN_RIGHT_IDX = 32,
    BTN_PADDLE_LEFT_IDX = 33,
    BTN_PADDLE_RIGHT_IDX = 34,
  };
  static constexpr int DEFAULT_STICK_DEADZONE = 13; // of 127, the usual PS5 pad value

  struct InitInfo
  {
    sonypad::Model model = sonypad::Model::UNKNOWN;
    bool bluetooth = false;
    bool rumbleV2 = true;
    sonypad::FirmwareInfo firmware;
    char mac[18] = {};
  };

  SonyPadDevice(const InitInfo &info, int ord_id);
  ~SonyPadDevice() { setClient(nullptr); }

  // IO thread side; true when the pad presence changed: a DualShock 4 USB adapter answers the calibration request once paired
  bool onInputReport(const uint8_t *data, int len);
  bool takeOutputReport(uint8_t *out, int cap, int &len, bool quiet = false); // quiet: rumble off, triggers free
  void setCalibration(const sonypad::SensorCalibration &calib, bool from_device);
  void setLost();

  // main thread side
  bool updateState(int dt_msec, bool def);
  sonypad::Model getModel() const { return model; }
  const char *getMac() const { return mac; }

  // IGenJoystick
  const char *getName() const override { return sonypad::model_name(model); }
  const char *getDeviceID() const override { return "ds4"; }
  bool getDeviceDesc(DataBlock &out_desc) const override;
  unsigned short getUserId() const override { return ordId; }
  const JoystickRawState &getRawState() const override { return state; }
  void setClient(IGenJoystickClient *cli) override;
  IGenJoystickClient *getClient() const override { return client; }

  int getButtonCount() const override { return devButtonCount; }
  const char *getButtonName(int idx) const override;
  int getAxisCount() const override { return devAxisCount; }
  const char *getAxisName(int idx) const override;
  int getPovHatCount() const override { return 0; }
  const char *getPovHatName(int) const override { return nullptr; }
  int getButtonId(const char *name) const override;
  int getAxisId(const char *name) const override;
  int getPovHatId(const char *) const override { return -1; }

  void setAxisLimits(int axis_id, float min_val, float max_val) override;
  void enableGyroscope(bool enable) override;

  bool getButtonState(int btn_id) const override;
  float getAxisPos(int axis_id) const override;
  int getAxisPosRaw(int axis_id) const override;
  int getPovHatAngle(int) const override { return -1; }

  bool isStickDeadZoneSetupSupported() const override { return true; }
  float getStickDeadZoneScale(int stick_idx) const override;
  void setStickDeadZoneScale(int stick_idx, float scale) override;
  float getStickDeadZoneAbs(int stick_idx) const override;

  int getPairedButtonForAxis(int axis_idx) const override;
  int getPairedAxisForButton(int btn_idx) const override;
  int getJointAxisForAxis(int axis_idx) const override;

  void doRumble(float lowFreq, float highFreq) override;
  void resetSensorsOrientation() override;
  void setLight(bool set, unsigned char r, unsigned char g, unsigned char b) override;
  void setPlayerLeds(unsigned mask) override;
  void setMicLed(JoystickMicLed mode) override;
  void setTriggerEffect(const TriggerEffectParam &param) override;
  bool getMotionState(JoystickMotionState &out) const override;
  bool getTouchpadState(JoystickTouchpadState &out) const override;
  bool fetchTouchpadGestures(GestureCb cb) override;

  bool isConnected() override { return !interlocked_acquire_load(lost); }
  // false until the first report that shows a pad, a DualShock 4 USB adapter can be empty
  bool hasPad() const { return interlocked_acquire_load(padPresent) != 0; }
  bool isXinputCompatible() const override { return true; }
  bool isGamepadLayout() const override { return true; }
  void enableSensorsTiltCorrection(bool enable) override;
  void reportGyroSensorsAngDelta(bool report_ang_delta) override;

private:
  // shared by the IO thread and the main thread under cs: input and motion flow in, output and sensor modes flow out
  struct Shared
  {
    sonypad::InputSample sample;
    JoystickMotionState motion;
    Point3 deltaLocal = Point3(0, 0, 0); // integrated rotation since the last updateState, pad frame, rad
    float deltaWorldYaw = 0;             // the same as the player space yaw of GyroWiki
    Quat orient = Quat(0, 0, 0, 1);
    Point3 gravity = Point3(0, 1, 0);
    uint32_t prevTicks = 0;
    int64_t prevHostTicks = 0;
    bool haveTicks = false;
    bool gyroEnabled = true;
    bool tiltCorrection = false;
    bool resetOrientation = false;
    sonypad::OutputState out;
    bool outDirty = true;
  };

  void integrateSensors(const sonypad::InputSample &s);
  void fillState(const Shared &snap, int dt_msec);

  mutable WinCritSec cs;
  Shared shared;
  // under cs: the IO thread feeds the recognizer per report, the main thread drains the queue
  TouchpadGestureRecognizer gestures;
  TouchpadGestureQueue gestureQueue;
  sonypad::SensorCalibration calibration;
  bool calibrated = false;
  sonypad::FirmwareInfo firmware;
  sonypad::Model model;
  bool bluetooth, rumbleV2;
  volatile bool lost = false;
  uint8_t btSeq = 0;
  char mac[18];
  volatile int padPresent = 0;

  IGenJoystickClient *client = nullptr;
  JoystickRawState state;
  int intSensors[3] = {0, 0, 0};
  LogicalAxis axis[AXES_NUM];
  int devButtonCount, devAxisCount;
  unsigned short ordId;
  float stickDeadZoneScale[2] = {1, 1};
  bool gyroSensorsAngDelta = false;
  int lastKeyPressedTime = 0;

  static const char *buttonName[BUTTONS_NUM];
  static const char *axisName[AXES_NUM];
};

} // namespace HumanInput
