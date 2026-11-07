// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiJoyTriggerEffect.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <math/dag_Point3.h>
#include <math/dag_Quat.h>
#include "dsPadProtocol.h"
#include "../hid_commonCode/joystickAxisUtils.h"
#include "../hid_commonCode/joyMotionHistory.h"
#include "../hid_commonCode/touchpadFrameQueue.h"

namespace HumanInput
{

// One DualSense, DualSense Edge or DualShock 4. The driver IO thread feeds reports in and takes output
// reports out; the main thread reads the IGenJoystick state. Button and axis numbering equals the PS5 pad
// driver so the console presets and glyphs apply unchanged.
class DsPadDevice final : public IGenJoystick
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
    dspad::FirmwareInfo firmware;
    char mac[18] = {};
    dspad::Model model = dspad::Model::UNKNOWN;
    bool bluetooth = false;
  };

  DsPadDevice(const InitInfo &info, int ord_id);
  ~DsPadDevice() { setClient(nullptr); }

  // IO thread side; true when the pad presence changed: a DualShock 4 USB adapter answers the calibration request once paired
  bool onInputReport(const uint8_t *data, int len);
  bool takeOutputReport(uint8_t *out, int cap, int &len, bool quiet = false); // quiet: rumble off, triggers free
  void setCalibration(const dspad::SensorCalibration &calib, bool from_device);
  void setLost();

  // main thread side
  bool updateState(int dt_msec, bool def);
  dspad::Model getModel() const { return model; }
  const char *getMac() const { return mac; }

  // IGenJoystick
  const char *getName() const override { return dspad::model_name(model); }
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
  bool getTriggerEffectState(TriggerEffectStateInfo &out) const override;
  bool getMotionState(JoystickMotionState &out) const override;
  void enableMotionHistory(bool enable) override;
  int fetchMotionSamples(dag::Span<JoystickMotionSample> out, uint32_t &dropped) override;
  int fetchTouchpadFrames(dag::Span<JoystickTouchpadFrame> out) override;

  bool isConnected() override { return !interlocked_acquire_load(lost); }
  // false until the first report that shows a pad, a DualShock 4 USB adapter can be empty
  bool hasPad() const { return interlocked_acquire_load(padPresent) != 0; }
  bool isXinputCompatible() const override { return true; }
  bool isGamepadLayout() const override { return true; }
  void enableSensorsTiltCorrection(bool enable) override;
  void reportGyroSensorsAngDelta(bool report_ang_delta) override;
  void calibrateGyro() override;

private:
  // shared by the IO thread and the main thread under cs: input and motion flow in, output and sensor modes flow out
  struct Shared
  {
    dspad::InputSample sample;
    JoystickMotionState motion;
    Point3 delta = Point3(0, 0, 0); // rotation since the last updateState, rad, pad frame but yaw around gravity with tilt correction
    Quat orient = Quat(0, 0, 0, 1);
    uint32_t prevTicks = 0;
    int64_t prevHostTicks = 0;
    bool haveTicks = false;
    bool gyroEnabled = true;
    bool tiltCorrection = false;
    bool resetOrientation = false;
    dspad::OutputState out;
    bool outDirty = true;
  };

  void integrateSensors(const dspad::InputSample &s);
  void queueTouchpadFrame(const dspad::InputSample &s, bool present);
  void fillState(const Shared &snap, int dt_msec);

  mutable WinCritSec cs;
  Shared shared;
  JoystickMotionHistory motionHistory;
  hid::TouchpadFrameQueue touchpadFrames; // under cs: the IO thread pushes, the main thread fetches
  dspad::SensorCalibration calibration;
  bool calibrated = false;
  dspad::FirmwareInfo firmware;
  dspad::Model model;
  bool bluetooth;
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
