// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>

#include <drv/hid/dag_hiJoystick.h>
#include <forceFeedback/forceFeedback.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_inpDevClsDrv.h>
#include <math.h>

using namespace HumanInput;
using force_feedback::rumble::EventParams;

namespace
{
struct Motors
{
  float lo = 0, hi = 0;
};

struct FakePad final : IGenJoystick
{
  JoystickRawState state;
  Motors strongest;
  int rumbleCalls = 0;

  void doRumble(float lo_freq, float hi_freq) override
  {
    strongest.lo = max(strongest.lo, fabsf(lo_freq));
    strongest.hi = max(strongest.hi, fabsf(hi_freq));
    rumbleCalls++;
  }

  const char *getName() const override { return "fake"; }
  const char *getDeviceID() const override { return "fake"; }
  bool getDeviceDesc(DataBlock &) const override { return false; }
  unsigned short getUserId() const override { return 0; }
  const JoystickRawState &getRawState() const override { return state; }
  void setClient(IGenJoystickClient *) override {}
  IGenJoystickClient *getClient() const override { return nullptr; }
  int getButtonCount() const override { return 0; }
  const char *getButtonName(int) const override { return nullptr; }
  int getAxisCount() const override { return 0; }
  const char *getAxisName(int) const override { return nullptr; }
  int getPovHatCount() const override { return 0; }
  const char *getPovHatName(int) const override { return nullptr; }
  int getButtonId(const char *) const override { return -1; }
  int getAxisId(const char *) const override { return -1; }
  int getPovHatId(const char *) const override { return -1; }
  void setAxisLimits(int, float, float) override {}
  bool getButtonState(int) const override { return false; }
  float getAxisPos(int) const override { return 0; }
  int getAxisPosRaw(int) const override { return 0; }
  int getPovHatAngle(int) const override { return -1; }
  bool isConnected() override { return true; }
};

FakePad pad;

struct FakeJoystickDriver final : IGenJoystickClassDrv
{
  void enable(bool) override {}
  void acquireDevices() override {}
  void unacquireDevices() override {}
  void destroy() override {}
  void refreshDeviceList() override {}
  int getDeviceCount() const override { return 1; }
  void updateDevices() override {}
  IGenJoystick *getDevice(int) const override { return &pad; }
  IGenJoystick *getDeviceByUserId(unsigned short) const override { return &pad; }
  bool isDeviceConfigChanged() const override { return false; }
  void useDefClient(IGenJoystickClient *) override {}
  IGenJoystick *getDefaultJoystick() override { return &pad; }
  void setDefaultJoystick(IGenJoystick *) override {}
  void enableAutoDefaultJoystick(bool) override {}
  IGenJoystickClassDrv *setSecondaryClassDrv(IGenJoystickClassDrv *) override { return nullptr; }
};

FakeJoystickDriver driver;

Motors play(float lo_freq, float hi_freq)
{
  {
    WinAutoLock lock(global_cls_drv_update_cs);
    pad.strongest = {};
    pad.rumbleCalls = 0;
  }
  force_feedback::rumble::add_event({EventParams::LO, lo_freq, 50});
  force_feedback::rumble::add_event({EventParams::HI, hi_freq, 50});
  // both events land within 3 thread ticks
  for (int waitedMsec = 0; waitedMsec < 2000; waitedMsec += 5)
  {
    {
      WinAutoLock lock(global_cls_drv_update_cs);
      if (pad.rumbleCalls >= 3)
        break;
    }
    sleep_msec(5);
  }
  sleep_msec(100);
  WinAutoLock lock(global_cls_drv_update_cs);
  CHECK(pad.rumbleCalls >= 3);
  return pad.strongest;
}
} // namespace

TEST_CASE("the rumble scale multiplies both motors of every event")
{
  global_cls_drv_joy = &driver;

  Motors m = play(1.f, 0.5f);
  CHECK(m.lo == 1.f);
  CHECK(m.hi == 0.5f);

  const float scaleLoHi[][3] = {{0.5f, 0.5f, 0.25f}, {0.f, 0.f, 0.f}, {2.f, 1.f, 0.5f}, {-1.f, 0.f, 0.f}};
  for (const auto &[scale, lo, hi] : scaleLoHi)
  {
    INFO("scale " << scale);
    force_feedback::rumble::set_scale(scale);
    m = play(1.f, 0.5f);
    CHECK(m.lo == lo);
    CHECK(m.hi == hi);
  }

  force_feedback::rumble::shutdown();
  global_cls_drv_joy = nullptr;
}
