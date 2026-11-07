// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <unittest/catch2_eastl_tostring.h>

#include <drv/hid/dag_hiComposite.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiJoyTriggerEffect.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <string.h>

using namespace HumanInput;
using Catch::Matchers::WithinAbs;

namespace
{
const char *const BUTTON_NAMES[] = {"A", "B", "X", "Y"};
const char *const AXIS_NAMES[] = {"L.Thumb.h", "L.Thumb.v", "R.Thumb.h", "R.Thumb.v", "L.Trigger", "R.Trigger", "Axis7", "Axis8"};

struct FakePad final : IGenJoystick
{
  explicit FakePad(const char *device_id, bool gamepad_layout = false) : id(device_id), gamepadLayout(gamepad_layout)
  {
    memset(&state, 0, sizeof(state));
  }

  const char *id;
  bool gamepadLayout;
  JoystickRawState state;
  float dzBase = 0, dzScale[2] = {1, 1};
  bool tiltCorrection = false, angDelta = false;
  int rumbles = 0;
  bool hasTriggerEffect = false;
  TriggerEffectStateInfo triggerEffect;
  bool touchpad = false;
  int touchpadFrames = 0;

  const char *getName() const override { return id; }
  const char *getDeviceID() const override { return id; }
  bool getDeviceDesc(DataBlock &) const override { return false; }
  unsigned short getUserId() const override { return 0; }
  const JoystickRawState &getRawState() const override { return state; }
  void setClient(IGenJoystickClient *) override {}
  IGenJoystickClient *getClient() const override { return nullptr; }
  int getButtonCount() const override { return eastl::size(BUTTON_NAMES); }
  const char *getButtonName(int idx) const override { return BUTTON_NAMES[idx]; }
  int getAxisCount() const override { return eastl::size(AXIS_NAMES); }
  const char *getAxisName(int idx) const override { return AXIS_NAMES[idx]; }
  int getPovHatCount() const override { return 1; }
  const char *getPovHatName(int) const override { return "D-pad"; }
  int getButtonId(const char *) const override { return -1; }
  int getAxisId(const char *) const override { return -1; }
  int getPovHatId(const char *) const override { return -1; }
  void setAxisLimits(int, float, float) override {}
  bool getButtonState(int idx) const override { return state.buttons.get(idx); }
  float getAxisPos(int) const override { return 0; }
  int getAxisPosRaw(int) const override { return 0; }
  int getPovHatAngle(int idx) const override { return state.povValues[idx]; }
  bool isConnected() override { return true; }
  bool isGamepadLayout() const override { return gamepadLayout; }
  void setStickDeadZoneScale(int stick_idx, float scale) override { dzScale[stick_idx] = scale; }
  float getStickDeadZoneAbs(int stick_idx) const override { return dzScale[stick_idx] * dzBase; }
  void enableSensorsTiltCorrection(bool enable) override { tiltCorrection = enable; }
  void reportGyroSensorsAngDelta(bool report_ang_delta) override { angDelta = report_ang_delta; }
  void doRumble(float, float) override { rumbles++; }
  bool getTriggerEffectState(TriggerEffectStateInfo &out) const override
  {
    if (hasTriggerEffect)
      out = triggerEffect;
    return hasTriggerEffect;
  }
  int fetchTouchpadFrames(dag::Span<JoystickTouchpadFrame> out) override
  {
    const int count = eastl::min<int>(touchpadFrames, out.size());
    touchpadFrames = 0;
    return touchpad ? count : -1;
  }
};

struct FakeDriver final : IGenJoystickClassDrv
{
  explicit FakeDriver(int vendor_id = GAMEPAD_VENDOR_UNKNOWN) : vendorId(vendor_id) {}

  eastl::vector<FakePad *> pads;
  int vendorId;

  void enable(bool) override {}
  void acquireDevices() override {}
  void unacquireDevices() override {}
  void destroy() override {}
  void refreshDeviceList() override {}
  int getDeviceCount() const override { return pads.size(); }
  void updateDevices() override {}
  IGenJoystick *getDevice(int idx) const override { return pads[idx]; }
  IGenJoystick *getDeviceByUserId(unsigned short) const override { return nullptr; }
  bool isDeviceConfigChanged() const override { return false; }
  void useDefClient(IGenJoystickClient *) override {}
  IGenJoystick *getDefaultJoystick() override { return nullptr; }
  void setDefaultJoystick(IGenJoystick *) override {}
  void enableAutoDefaultJoystick(bool) override {}
  IGenJoystickClassDrv *setSecondaryClassDrv(IGenJoystickClassDrv *) override { return nullptr; }
  bool isStickDeadZoneSetupSupported() const override { return true; }
  int getVendorId() const override { return vendorId; }
};

struct Composite
{
  Composite() { global_cls_composite_drv_joy = drv; }
  ~Composite()
  {
    drv->destroy();
    global_cls_composite_drv_joy = nullptr;
  }

  void add(FakeDriver &class_drv, bool is_xinput) { drv->addClassDrv(&class_drv, is_xinput); }
  void refresh() { drv->acquireDevices(); }
  void poll() { drv->updateDevices(); }
  IGenJoystick &joy() const { return *drv->getDevice(0); }
  const JoystickRawState &state() const { return joy().getRawState(); }

  CompositeJoystickClassDriver *drv = CompositeJoystickClassDriver::create();
};

struct DualSenseAndXbox
{
  DualSenseAndXbox()
  {
    sony.pads = {&dualSense};
    xinput.pads = {&xbox};
    c.drv->setGamepadLayoutGroup(true);
    c.add(sony, true);
    c.add(xinput, true);
    c.add(dinput, false);
  }

  FakeDriver sony{GAMEPAD_VENDOR_SONY}, xinput{GAMEPAD_VENDOR_MICROSOFT}, dinput;
  FakePad dualSense{"ds", true}, xbox{"x360"}, stick{"1234:5678"};
  Composite c;
};
} // namespace

TEST_CASE("pads of the gamepad layout share the first pad's controls, other devices do not")
{
  DualSenseAndXbox p;
  p.dinput.pads = {&p.stick};
  p.c.refresh();
  REQUIRE(p.c.drv->getRealDeviceCount(true) == 3);
  const auto first = p.c.drv->getRealDeviceDesc(0), second = p.c.drv->getRealDeviceDesc(1), stick = p.c.drv->getRealDeviceDesc(2);
  CHECK(second.buttons.ofs == first.buttons.ofs);
  CHECK(second.axes.ofs == first.axes.ofs);
  CHECK(second.axes.count == 6);
  CHECK(stick.buttons.ofs != first.buttons.ofs);
  CHECK(stick.axes.ofs != first.axes.ofs);
  CHECK(p.c.joy().getButtonCount() == 2 * eastl::size(BUTTON_NAMES));

  p.stick.state.x = 300;
  p.c.poll();
  CHECK(p.c.state().x == 300);

  p.c.drv->setGamepadLayoutGroup(false);
  const auto xbox = p.c.drv->getRealDeviceDesc(0), dualSense = p.c.drv->getRealDeviceDesc(1);
  REQUIRE(strcmp(xbox.deviceIdString, "x360") == 0);
  CHECK(dualSense.buttons.ofs != xbox.buttons.ofs);
  CHECK(dualSense.axes.ofs != xbox.axes.ofs);
}

TEST_CASE("the pad pressed last drives the shared controls alone")
{
  DualSenseAndXbox p;
  p.xbox.state.x = 1000;
  p.xbox.state.buttons.set(1);
  p.c.poll();
  CHECK(p.c.state().buttons.get(1));
  CHECK(p.c.state().x == 1000);

  p.dualSense.state.x = -500;
  p.c.poll();
  CHECK(p.c.state().x == 1000);

  p.dualSense.state.buttons.set(0);
  p.c.poll();
  CHECK(p.c.state().x == -500);
  CHECK(p.c.state().buttons.get(0));
  CHECK_FALSE(p.c.state().buttons.get(1));
}

TEST_CASE("the first XInput pad keeps its native names after a device of another driver, a second one is prefixed")
{
  FakeDriver xinput, dinput;
  FakePad stick("1234:5678"), xbox("x360"), second("1234:0002");
  Composite c;
  dinput.pads = {&stick};
  c.add(xinput, true);
  c.add(dinput, false);
  xinput.pads = {&xbox, &second};
  c.refresh();

  const auto first = c.drv->getRealDeviceDesc(1), other = c.drv->getRealDeviceDesc(2);
  REQUIRE(eastl::string(first.name) == "x360");
  REQUIRE(eastl::string(other.name) == "1234:0002");
  CHECK(eastl::string(c.joy().getButtonName(first.buttons.ofs)) == "A");
  CHECK(eastl::string(c.joy().getAxisName(first.axes.ofs)) == "L.Thumb.h");
  CHECK(eastl::string(c.joy().getPovHatName(0)) == "D-pad");
  CHECK(eastl::string(c.joy().getButtonName(other.buttons.ofs)) == "C3:A");
}

TEST_CASE("a pad that shares controls keeps its own hat")
{
  FakeDriver xinput;
  FakePad first("x360"), second("x360");
  Composite c;
  xinput.pads = {&first, &second};
  c.add(xinput, true);
  first.state.povValues[0] = 9000;
  second.state.povValues[0] = 18000;
  c.poll();
  CHECK(c.drv->getRealDeviceDesc(1).buttons.ofs == c.drv->getRealDeviceDesc(0).buttons.ofs);
  CHECK(c.state().povValues[0] == 9000);
  CHECK(c.state().povValues[1] == 18000);
}

TEST_CASE("the vendor of the pad used last is reported, a pad that does not drive changes nothing")
{
  DualSenseAndXbox p;
  CHECK(p.c.drv->getVendorId() == GAMEPAD_VENDOR_SONY);
  p.xbox.state.buttons.set(0);
  p.c.poll();
  CHECK(p.c.drv->getVendorId() == GAMEPAD_VENDOR_MICROSOFT);

  p.dualSense.state.buttons.set(0);
  p.c.poll();
  CHECK(p.c.drv->getVendorId() == GAMEPAD_VENDOR_SONY);

  p.xbox.state.buttons.reset();
  p.c.poll();
  CHECK(p.c.drv->getVendorId() == GAMEPAD_VENDOR_SONY);
}

TEST_CASE("a pad connected later takes the gyro mode the game set")
{
  FakeDriver sony;
  FakePad dualSense("ds", true), later("ds", true);
  Composite c;
  c.add(sony, true);
  c.joy().reportGyroSensorsAngDelta(true);

  sony.pads = {&dualSense};
  c.refresh();
  CHECK_FALSE(dualSense.tiltCorrection);
  CHECK(dualSense.angDelta);

  c.joy().enableSensorsTiltCorrection(true);
  sony.pads = {&dualSense, &later};
  c.refresh();
  CHECK(later.tiltCorrection);
  CHECK(later.angDelta);
}

TEST_CASE("each driver keeps the absolute stick dead zone the game set")
{
  FakeDriver sony, xinput;
  FakePad dualSense("ds4"), xbox("x360");
  Composite c;
  dualSense.dzBase = 0.1f;
  xbox.dzBase = 0.24f;
  sony.pads = {&dualSense};
  xinput.pads = {&xbox};
  c.add(sony, true);
  c.add(xinput, true);

  c.drv->setStickDeadZoneScale(0, true, 1.5f);
  CHECK_THAT(dualSense.getStickDeadZoneAbs(0), WithinAbs(0.15f, 1e-6f));
  CHECK_THAT(xbox.getStickDeadZoneAbs(0), WithinAbs(0.15f, 1e-6f));
}

TEST_CASE("rumble reaches the target pad, or every pad without a target")
{
  FakeDriver xinput;
  FakePad first("x360"), second("x360");
  Composite c;
  xinput.pads = {&first, &second};
  c.add(xinput, true);

  c.drv->setRumbleTargetRealDevIdx(1);
  c.joy().doRumble(1, 1);
  CHECK(first.rumbles == 0);
  CHECK(second.rumbles == 1);

  c.drv->setRumbleTargetRealDevIdx(-1);
  c.joy().doRumble(1, 1);
  CHECK(first.rumbles == 1);
  CHECK(second.rumbles == 2);
}

TEST_CASE("the trigger effect state comes from the rumble target, or else from the first pad that has one")
{
  FakeDriver xinput;
  FakePad first("x360"), second("x360");
  Composite c;
  xinput.pads = {&first, &second};
  c.add(xinput, true);
  first.hasTriggerEffect = second.hasTriggerEffect = true;
  first.triggerEffect.state[TRIGGER_EFFECT_INDEX_R2] = TRIGGER_EFFECT_STATE_WEAPON_PULLING;
  second.triggerEffect.state[TRIGGER_EFFECT_INDEX_R2] = TRIGGER_EFFECT_STATE_WEAPON_FIRED;

  TriggerEffectStateInfo out;
  REQUIRE(c.joy().getTriggerEffectState(out));
  CHECK(out.state[TRIGGER_EFFECT_INDEX_R2] == TRIGGER_EFFECT_STATE_WEAPON_PULLING);

  c.drv->setRumbleTargetRealDevIdx(1);
  REQUIRE(c.joy().getTriggerEffectState(out));
  CHECK(out.state[TRIGGER_EFFECT_INDEX_R2] == TRIGGER_EFFECT_STATE_WEAPON_FIRED);

  // out of range: no state, out untouched
  c.drv->setRumbleTargetRealDevIdx(2);
  out.state[TRIGGER_EFFECT_INDEX_R2] = TRIGGER_EFFECT_STATE_VIBRATION_ACTIVE;
  CHECK_FALSE(c.joy().getTriggerEffectState(out));
  CHECK(out.state[TRIGGER_EFFECT_INDEX_R2] == TRIGGER_EFFECT_STATE_VIBRATION_ACTIVE);

  c.drv->setRumbleTargetRealDevIdx(0);
  first.hasTriggerEffect = false;
  CHECK_FALSE(c.joy().getTriggerEffectState(out));

  c.drv->setRumbleTargetRealDevIdx(-1);
  REQUIRE(c.joy().getTriggerEffectState(out));
  CHECK(out.state[TRIGGER_EFFECT_INDEX_R2] == TRIGGER_EFFECT_STATE_WEAPON_FIRED);
}

TEST_CASE("with the gamepad layout group, the trigger effect state comes from the pad that drives")
{
  DualSenseAndXbox p;
  p.dualSense.hasTriggerEffect = true;
  p.dualSense.triggerEffect.state[TRIGGER_EFFECT_INDEX_R2] = TRIGGER_EFFECT_STATE_WEAPON_PULLING;
  TriggerEffectStateInfo out;

  p.xbox.state.buttons.set(1);
  p.c.poll();
  CHECK_FALSE(p.c.joy().getTriggerEffectState(out));
  p.c.drv->setRumbleTargetRealDevIdx(0);
  CHECK_FALSE(p.c.joy().getTriggerEffectState(out));
  p.c.drv->setRumbleTargetRealDevIdx(-1);

  p.dualSense.state.buttons.set(0);
  p.c.poll();
  REQUIRE(p.c.joy().getTriggerEffectState(out));
  CHECK(out.state[TRIGGER_EFFECT_INDEX_R2] == TRIGGER_EFFECT_STATE_WEAPON_PULLING);
}

TEST_CASE("the touch pad frames come from the pad pressed last, a pad that does not drive drops its frames")
{
  DualSenseAndXbox p;
  p.dualSense.touchpad = true;
  JoystickTouchpadFrame frames[IGenJoystick::TOUCHPAD_FRAME_CAPACITY];
  p.xbox.state.buttons.set(0);
  p.c.poll();
  p.dualSense.touchpadFrames = 2;
  CHECK(p.c.joy().fetchTouchpadFrames(make_span(frames)) == -1);
  CHECK(p.dualSense.touchpadFrames == 0);

  p.dualSense.state.buttons.set(0);
  p.c.poll();
  p.dualSense.touchpadFrames = 3;
  CHECK(p.c.joy().fetchTouchpadFrames(make_span(frames)) == 3);
  CHECK(p.dualSense.touchpadFrames == 0);
}
