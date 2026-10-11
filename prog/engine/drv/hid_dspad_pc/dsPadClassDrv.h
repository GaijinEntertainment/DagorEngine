// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoystick.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <generic/dag_tab.h>
#include <EASTL/unique_ptr.h>
#include "dsPadHid.h"

namespace HumanInput
{
class SonyPadDevice;

// Enumerates Sony pads over raw HID, reads them on its own thread and publishes them as "ds4" devices.
// The IO thread creates a device and marks it lost; the main thread publishes and destroys it in refreshDeviceList().
class SonyPadClassDriver final : public IGenJoystickClassDrv, public IWndProcComponent
{
public:
  SonyPadClassDriver();
  ~SonyPadClassDriver();

  bool init();

  // IGenHidClassDrv
  void enable(bool en) override;
  void acquireDevices() override {}
  void unacquireDevices() override {}
  void destroy() override;
  void refreshDeviceList() override;
  int getDeviceCount() const override { return devices.size(); }
  void updateDevices() override;

  // IGenJoystickClassDrv
  IGenJoystick *getDevice(int idx) const override;
  IGenJoystick *getDeviceByUserId(unsigned short user_id) const override;
  bool isDeviceConfigChanged() const override { return interlocked_acquire_load(configChanged) != 0; }
  void useDefClient(IGenJoystickClient *cli) override;
  IGenJoystick *getDefaultJoystick() override { return defJoy; }
  void setDefaultJoystick(IGenJoystick *ref) override;
  void enableAutoDefaultJoystick(bool enable) override { enableAutoDef = enable; }
  // composite only, nothing chains a secondary driver
  IGenJoystickClassDrv *setSecondaryClassDrv(IGenJoystickClassDrv *) override { return nullptr; }

  bool isStickDeadZoneSetupSupported() const override { return true; }
  float getStickDeadZoneScale(int stick_idx, bool main_dev) const override;
  void setStickDeadZoneScale(int stick_idx, bool main_dev, float scale) override;
  float getStickDeadZoneAbs(int stick_idx, bool main_dev, IGenJoystick *for_joy) const override;
  void enableGyroscope(bool enable, bool main_dev) override;
  int getVendorId() const override { return GAMEPAD_VENDOR_SONY; }

  // IWndProcComponent
  RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result) override;

private:
  class IoThread;

  struct Slot
  {
    eastl::unique_ptr<sonypad::HidDevice> hid; // IO thread only
    eastl::unique_ptr<SonyPadDevice> dev;
    int64_t lastReportTicks = 0; // IO thread only
    char twinPath[512] = {};     // IO thread only: the skipped bluetooth twin
  };

  void ioLoop();
  void scanDevices(bool retry_skipped, bool device_change);
  bool openPad(const sonypad::HidDeviceInfo &info);
  void loseSlot(Slot &slot);
  void requestRescan();

  Tab<Slot> slots;                         // guarded by cs
  Tab<sonypad::HidDeviceInfo> skippedPads; // IO thread only
  Tab<SonyPadDevice *> devices;            // main thread; refreshDeviceList rebuilds it under cs
  WinCritSec cs;
  eastl::unique_ptr<IoThread> ioThread;
  sonypad::HidWaitSet waitSet;
  volatile int rescanRequested = 1;
  volatile int configChanged = 0;
  bool scanLogged = false;

  IGenJoystickClient *defClient = nullptr;
  IGenJoystick *defJoy = nullptr;
  int64_t prevUpdateRefTime = 0;
  bool enabled = false;
  bool enableAutoDef = true;
  bool gyroEnabled = true;
  float stickDeadZoneScale[2] = {1, 1};
  int nextOrdId = 0;
};

} // namespace HumanInput
