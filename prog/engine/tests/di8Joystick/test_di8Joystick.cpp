// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>

#include "../../drv/hid_joystick/include_dinput.h"
#include "../../drv/hid_joystick/joy_enumdev.h"
#include "../../drv/hid_joystick/joy_device.h"
#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiDInput.h>
#include <drv/hid/dag_hiJoystick.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <stdio.h>
#include <string.h>

using namespace HumanInput;

namespace
{
constexpr int ENUM_TIMEOUT_MS = 30000;
constexpr WORD USAGE_PAGE_GENERIC_DESKTOP = 0x01;
constexpr WORD USAGE_GAMEPAD = 0x05;

template <typename F>
bool wait_until(F cond)
{
  for (int startMs = get_time_msec(); !cond(); sleep_msec(1))
    if (get_time_msec() - startMs > 5000)
      return false;
  return true;
}

struct FakeDInput;

struct FakePad final : IDirectInputDevice8
{
  explicit FakePad(FakeDInput &o) : owner(o) {}
  FakeDInput &owner;

  STDMETHODIMP QueryInterface(REFIID, LPVOID *) override { return E_NOINTERFACE; }
  STDMETHODIMP_(ULONG) AddRef() override { return 1; }
  STDMETHODIMP_(ULONG) Release() override;
  STDMETHODIMP GetCapabilities(LPDIDEVCAPS caps) override
  {
    caps->dwFlags = 0;
    return DI_OK;
  }
  STDMETHODIMP EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACK cb, LPVOID ref, DWORD flags) override
  {
    if (flags == DIDFT_COLLECTION)
    {
      DIDEVICEOBJECTINSTANCE gamepad = {sizeof(gamepad)};
      gamepad.dwType = DIDFT_COLLECTION;
      gamepad.wUsagePage = USAGE_PAGE_GENERIC_DESKTOP;
      gamepad.wUsage = USAGE_GAMEPAD;
      cb(&gamepad, ref);
    }
    return DI_OK;
  }
  STDMETHODIMP GetProperty(REFGUID, LPDIPROPHEADER) override { return E_NOTIMPL; }
  STDMETHODIMP SetProperty(REFGUID, LPCDIPROPHEADER) override { return DI_OK; }
  STDMETHODIMP Acquire() override { return DI_OK; }
  STDMETHODIMP Unacquire() override { return DI_OK; }
  STDMETHODIMP GetDeviceState(DWORD size, LPVOID data) override
  {
    memset(data, 0, size);
    memset(static_cast<DIJOYSTATE2 *>(data)->rgdwPOV, 0xFF, sizeof(DIJOYSTATE2::rgdwPOV));
    return DI_OK;
  }
  STDMETHODIMP GetDeviceData(DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP SetDataFormat(LPCDIDATAFORMAT) override { return DI_OK; }
  STDMETHODIMP SetEventNotification(HANDLE) override { return E_NOTIMPL; }
  STDMETHODIMP SetCooperativeLevel(HWND, DWORD) override { return DI_OK; }
  STDMETHODIMP GetObjectInfo(LPDIDEVICEOBJECTINSTANCE, DWORD, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP GetDeviceInfo(LPDIDEVICEINSTANCE) override { return E_NOTIMPL; }
  STDMETHODIMP RunControlPanel(HWND, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP Initialize(HINSTANCE, DWORD, REFGUID) override { return E_NOTIMPL; }
  STDMETHODIMP CreateEffect(REFGUID, LPCDIEFFECT, LPDIRECTINPUTEFFECT *, LPUNKNOWN) override { return E_NOTIMPL; }
  STDMETHODIMP EnumEffects(LPDIENUMEFFECTSCALLBACK, LPVOID, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP GetEffectInfo(LPDIEFFECTINFO, REFGUID) override { return E_NOTIMPL; }
  STDMETHODIMP GetForceFeedbackState(LPDWORD) override { return E_NOTIMPL; }
  STDMETHODIMP SendForceFeedbackCommand(DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK, LPVOID, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP Escape(LPDIEFFESCAPE) override { return E_NOTIMPL; }
  STDMETHODIMP Poll() override { return DI_OK; }
  STDMETHODIMP SendDeviceData(DWORD, LPCDIDEVICEOBJECTDATA, LPDWORD, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP EnumEffectsInFile(LPCTSTR, LPDIENUMEFFECTSINFILECALLBACK, LPVOID, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP WriteEffectToFile(LPCTSTR, DWORD, LPDIFILEEFFECT, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP BuildActionMap(LPDIACTIONFORMAT, LPCTSTR, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP SetActionMap(LPDIACTIONFORMAT, LPCTSTR, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP GetImageInfo(LPDIDEVICEIMAGEINFOHEADER) override { return E_NOTIMPL; }
};

struct FakeDInput final : IDirectInput8
{
  explicit FakeDInput(int pad_count)
  {
    for (int i = 0; i < pad_count; i++)
    {
      DIDEVICEINSTANCE &inst = pads.push_back();
      inst = {sizeof(inst)};
      snprintf(inst.tszInstanceName, sizeof(inst.tszInstanceName), "Pad %d", i + 1);
      strcpy(inst.tszProductName, inst.tszInstanceName);
    }
    dinput8 = this;
  }
  ~FakeDInput() { dinput8 = nullptr; }

  eastl::vector<DIDEVICEINSTANCE> pads;
  eastl::function<void(int)> beforeCreate;
  DeviceEnumerator *enumerating = nullptr;
  eastl::vector<eastl::unique_ptr<FakePad>> made;
  volatile int releases = 0;

  STDMETHODIMP QueryInterface(REFIID, LPVOID *) override { return E_NOINTERFACE; }
  STDMETHODIMP_(ULONG) AddRef() override { return 1; }
  STDMETHODIMP_(ULONG) Release() override { return 1; }
  STDMETHODIMP CreateDevice(REFGUID, LPDIRECTINPUTDEVICE8 *dev, LPUNKNOWN) override
  {
    if (beforeCreate)
      beforeCreate(int(made.size()));
    *dev = made.emplace_back(eastl::make_unique<FakePad>(*this)).get();
    return DI_OK;
  }
  STDMETHODIMP EnumDevices(DWORD, LPDIENUMDEVICESCALLBACK cb, LPVOID ref, DWORD) override
  {
    enumerating = static_cast<DeviceEnumerator *>(ref);
    for (const DIDEVICEINSTANCE &inst : pads)
      if (cb(&inst, ref) == DIENUM_STOP)
        break;
    return DI_OK;
  }
  STDMETHODIMP GetDeviceStatus(REFGUID) override { return E_NOTIMPL; }
  STDMETHODIMP RunControlPanel(HWND, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP Initialize(HINSTANCE, DWORD) override { return E_NOTIMPL; }
  STDMETHODIMP FindDevice(REFGUID, LPCTSTR, LPGUID) override { return E_NOTIMPL; }
  STDMETHODIMP EnumDevicesBySemantics(LPCTSTR, LPDIACTIONFORMAT, LPDIENUMDEVICESBYSEMANTICSCB, LPVOID, DWORD) override
  {
    return E_NOTIMPL;
  }
  STDMETHODIMP ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK, LPDICONFIGUREDEVICESPARAMS, DWORD, LPVOID) override { return E_NOTIMPL; }
};

STDMETHODIMP_(ULONG) FakePad::Release()
{
  interlocked_increment(owner.releases);
  return 0;
}

eastl::unique_ptr<DeviceEnumerator> make_enumerator()
{
  return eastl::make_unique<DeviceEnumerator>(ENUM_TIMEOUT_MS, false, true, false);
}

void run_to_end(DeviceEnumerator &enumerator)
{
  enumerator.start();
  REQUIRE(wait_until([&] { return !enumerator.isThreadRunnning(); }));
}

IGenJoystickClassDrv *create_driver_with_changes()
{
  IGenJoystickClassDrv *drv = createJoystickClassDriver(false, true, false);
  REQUIRE(drv);
  REQUIRE(wait_until([&] {
    drv->updateDevices();
    return drv->isDeviceConfigChanged();
  }));
  return drv;
}
} // namespace

TEST_CASE("a result nobody fetched is released with the enumerator")
{
  FakeDInput di(2);
  auto enumerator = make_enumerator();
  run_to_end(*enumerator);
  CHECK(enumerator->foundChanges());
  CHECK(di.made.size() == 2u);
  CHECK(di.releases == 0);

  enumerator.reset();
  CHECK(di.releases == 2);
}

TEST_CASE("fetch hands the joysticks over to the caller")
{
  FakeDInput di(2);
  auto enumerator = make_enumerator();
  run_to_end(*enumerator);
  Di8Devices fetched = enumerator->fetch();
  enumerator.reset();
  REQUIRE(fetched.size() == 2);
  CHECK(di.releases == 0);

  for (Di8Device &d : fetched)
    delete d.joy;
  CHECK(di.releases == 2);
}

TEST_CASE("a new enumeration releases the result nobody fetched")
{
  FakeDInput di(2);
  auto enumerator = make_enumerator();
  run_to_end(*enumerator);
  enumerator->terminate(false);
  run_to_end(*enumerator);
  CHECK(di.made.size() == 4u);
  CHECK(di.releases == 2);

  enumerator.reset();
  CHECK(di.releases == 4);
}

TEST_CASE("an aborted enumeration releases the joysticks it made")
{
  FakeDInput di(2);
  auto enumerator = make_enumerator();
  volatile int secondCreating = 0;
  di.beforeCreate = [&](int pad_idx) {
    if (pad_idx == 0)
      return;
    interlocked_release_store(secondCreating, 1);
    wait_until([&] { return enumerator->isThreadTerminating(); });
  };
  enumerator->start();
  REQUIRE(wait_until([&] { return interlocked_acquire_load(secondCreating) != 0; }));
  enumerator->terminate(true, 5000);
  CHECK(di.made.size() == 2u);
  CHECK(di.releases == 2);
}

TEST_CASE("the class driver reports the pads it found")
{
  FakeDInput di(2);
  IGenJoystickClassDrv *drv = create_driver_with_changes();
  drv->refreshDeviceList();
  REQUIRE(drv->getDeviceCount() == 2);
  CHECK(eastl::string(drv->getDevice(0)->getName()) == "Pad 1");
  CHECK(eastl::string(drv->getDevice(1)->getName()) == "Pad 2");
  CHECK(di.releases == 0);

  drv->destroy();
  CHECK(di.releases == 2);
}

TEST_CASE("the class driver releases the pads it did not take yet when destroyed")
{
  FakeDInput di(2);
  IGenJoystickClassDrv *drv = create_driver_with_changes();
  drv->destroy();
  CHECK(di.made.size() == 2u);
  CHECK(di.releases == 2);
}

TEST_CASE("the class driver stops a running enumeration and releases what it made")
{
  FakeDInput di(2);
  volatile int secondCreating = 0;
  di.beforeCreate = [&](int pad_idx) {
    if (pad_idx == 0)
      return;
    interlocked_release_store(secondCreating, 1);
    wait_until([&] { return di.enumerating->isThreadTerminating(); });
  };
  IGenJoystickClassDrv *drv = createJoystickClassDriver(false, true, false);
  REQUIRE(drv);
  drv->updateDevices();
  REQUIRE(wait_until([&] { return interlocked_acquire_load(secondCreating) != 0; }));
  drv->destroy();
  CHECK(di.made.size() == 2u);
  CHECK(di.releases == 2);
}
