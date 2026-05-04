// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "include_dinput.h"

#include <EASTL/vector.h>

#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_critsec.h>


namespace HumanInput
{

class Di8JoystickDevice;

struct Di8Device
{
  Di8JoystickDevice *joy = nullptr;
  DIDEVICEINSTANCE instance = {0};
  IDirectInputDevice8 *device = nullptr;
  DIDEVCAPS caps = {.dwSize = sizeof(DIDEVCAPS)};

  // instance guids are guaranteed to be the same on the same machine, but not on other machines
  bool operator==(const DIDEVICEINSTANCE *other) { return !memcmp(&this->instance.guidInstance, &other->guidInstance, sizeof(GUID)); }
}; // struct Di8Device


using VidPidList = eastl::vector<DWORD>;
using Di8Devices = eastl::vector<Di8Device>;


struct DeviceEnumerator : public DaThread
{
  DeviceEnumerator(int timeout_ms, bool exclude_xinput, bool remap_as_x360) :
    DaThread("Di8Enumerator", DEFAULT_STACK_SZ, 0, WORKER_THREADS_AFFINITY_MASK),
    timeoutMs{timeout_ms},
    excludeXinputDevices{exclude_xinput},
    remapAsX360{remap_as_x360}
  {}

  void execute() override;

  bool foundChanges() const { return !isThreadRunnning() && ::interlocked_acquire_load(modifiedDevices) != 0; }
  const Di8Devices fetch()
  {
    G_ASSERTF_RETURN(!isThreadRunnning(), {}, "[HID][DI8] cannot fetch devices while populating the list");
    Di8Devices result;
    WinAutoLock l(cs);
    lastKnownDevicesCount = interlocked_acquire_load(modifiedDevices);
    eastl::swap(result, found);
    ::interlocked_release_store(modifiedDevices, 0);
    return result;
  }


  int timeoutMs = 0;
  int startedAtMs = 0;

  VidPidList blacklist;
  bool excludeXinputDevices = false;
  bool remapAsX360 = false;

  WinCritSec cs;
  Di8Devices found;
  int lastKnownDevicesCount = 0;
  volatile int modifiedDevices = 0;
}; // struct DeviceEnumrator

} // namespace HumanInput
