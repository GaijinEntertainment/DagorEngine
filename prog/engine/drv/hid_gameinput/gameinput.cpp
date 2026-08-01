// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gameinput.h"
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <debug/dag_debug.h>


namespace gameinput
{


static IGameInput *game_input = nullptr;
static GameInputCallbackToken device_connection_cb_token = {0};
static bool initialized = false;
static int init_refcount = 0;


struct DevicesListWrapper
{
  DevicesList list;
  WinCritSec cs;
  volatile uint32_t generation = 0;

  DevicesListWrapper() { list.resize(MAX_DEVICES_PER_TYPE, nullptr); }
};


static DevicesListWrapper gamepads_list;
static DevicesListWrapper keyboards_list;
static DevicesListWrapper mouses_list;
static DevicesListWrapper flightsticks_list;


static void dump_input_kind(GameInputKind kind)
{
#define DUMP(X) \
  if (kind & X) \
  debug("- %s", #X)

  DUMP(GameInputKindUnknown);
  DUMP(GameInputKindRawDeviceReport);
  DUMP(GameInputKindController);
  DUMP(GameInputKindKeyboard);
  DUMP(GameInputKindMouse);
  DUMP(GameInputKindTouch);
  DUMP(GameInputKindMotion);
  DUMP(GameInputKindArcadeStick);
  DUMP(GameInputKindFlightStick);
  DUMP(GameInputKindGamepad);
  DUMP(GameInputKindRacingWheel);
  DUMP(GameInputKindUiNavigation);

#undef DUMP
}


static DevicesListWrapper *select_list_by_kind(GameInputKind kind)
{
  if (kind & GameInputKindGamepad)
    return &gamepads_list;
  if (kind & GameInputKindKeyboard)
    return &keyboards_list;
  if (kind & GameInputKindMouse)
    return &mouses_list;
  if (kind & GameInputKindFlightStick)
    return &flightsticks_list;

  logwarn("Unsupported GameInputKind: 0x%x", kind);
  dump_input_kind(kind);
  return nullptr;
}


static eastl::vector<DevicesListWrapper *> select_all_supported_lists(GameInputKind kind)
{
  eastl::vector<DevicesListWrapper *> result;
  if (kind & GameInputKindGamepad)
    result.push_back(&gamepads_list);
  if (kind & GameInputKindKeyboard)
    result.push_back(&keyboards_list);
  if (kind & GameInputKindMouse)
    result.push_back(&mouses_list);
  if (kind & GameInputKindFlightStick)
    result.push_back(&flightsticks_list);
  return result;
}


unsigned get_devices_config_generation(GameInputKind kind)
{
  DevicesListWrapper *dlw = select_list_by_kind(kind);
  return dlw ? interlocked_acquire_load(dlw->generation) : 0;
}


static bool update_devices_state(DevicesListWrapper *dlw, IGameInputDevice *device, bool connected)
{
  WinAutoLock lock(dlw->cs);
  if (connected)
  {
    size_t emptySlot = MAX_DEVICES_PER_TYPE;
    for (size_t i = 0; i < MAX_DEVICES_PER_TYPE; ++i)
    {
      if (!dlw->list[i] && emptySlot >= MAX_DEVICES_PER_TYPE)
        emptySlot = i;
      if (dlw->list[i] == device)
      {
        debug("%s found existing device %p in list %p at %zu", __FUNCTION__, device, dlw, i);
        return true;
      }
    }

    if (emptySlot < MAX_DEVICES_PER_TYPE)
    {
      dlw->list[emptySlot] = device;
      debug("%s added device %p to list %p at %zu", __FUNCTION__, device, dlw, emptySlot);
      return true;
    }
  }
  else
  {
    for (size_t i = 0; i < MAX_DEVICES_PER_TYPE; ++i)
    {
      if (dlw->list[i] == device)
      {
        dlw->list[i] = nullptr;
        debug("%s removed device %p from list %p at %zu", __FUNCTION__, device, dlw, i);
        return true;
      }
    }
  }
  return false;
}


static void __cdecl device_connection_callback(GameInputCallbackToken, void *, IGameInputDevice *dev, uint64_t,
  GameInputDeviceStatus status, GameInputDeviceStatus)
{
  const GameInputDeviceInfo *deviceInfo = dev->GetDeviceInfo();
  if (!deviceInfo)
    return;

  GameInputKind kind = deviceInfo->supportedInput;
  uint16_t vid = deviceInfo->vendorId;
  uint16_t pid = deviceInfo->productId;
  debug("Device (%X:%X) %p kind: 0x%x", vid, pid, dev, kind);
  dump_input_kind(kind);
  eastl::vector<DevicesListWrapper *> dlws = select_all_supported_lists(kind);
  if (dlws.empty())
  {
    logwarn("Unsupported device kind");
    return;
  }

  bool connected = status & GameInputDeviceConnected;

  for (DevicesListWrapper *dlw : dlws)
  {
    if (update_devices_state(dlw, dev, connected))
    {
      debug("Devices list 0x%x updated", kind);
      interlocked_increment(dlw->generation);
    }
  }
}


void init()
{
  if (init_refcount++ > 0)
    return;

  static constexpr size_t MAX_RETRIES = 20;
  static constexpr uint32_t SLEEP_MS = 10;
  size_t iter = 0;
  HRESULT result = S_OK;
  do
  {
    if (iter > MAX_RETRIES)
      DAG_FATAL("Failed to initialize input. Something went wrong.");

    result = GameInputCreate(&game_input);
    if (FAILED(result))
    {
      game_input = nullptr;
      logwarn("GameInputCreate failed: 0x%08X", result);
      sleep_msec(SLEEP_MS);
      ++iter;
    }
    else
    {
      debug("GameInput initialized");
      initialized = true;
    }
  } while (FAILED(result));
  // Above is a workaround for rare cases when input fails to initialize.
  // https://forums.gdklive.com/questions/111320/gameinputcreate-failed-with-error-code-0x87e50004.html
  // TODO: replace with G_VERIFY(SUCCEEDED(GameInputCreate(&game_input))); when MS fixes bug.

  if (initialized)
  {
    constexpr GameInputKind trackedKinds =
      GameInputKindGamepad | GameInputKindKeyboard | GameInputKindMouse | GameInputKindFlightStick;
    result = game_input->RegisterDeviceCallback(nullptr, trackedKinds, GameInputDeviceConnected, GameInputAsyncEnumeration, nullptr,
      device_connection_callback, &device_connection_cb_token);
    initialized &= SUCCEEDED(result);
  }
}


static void clear_devices_list(DevicesListWrapper &dlw)
{
  WinAutoLock lock(dlw.cs);
  eastl::fill(dlw.list.begin(), dlw.list.end(), nullptr);
}


void shutdown()
{
  G_ASSERT(init_refcount > 0);
  if (init_refcount == 0 || --init_refcount > 0)
    return;

  if (game_input)
  {
    static constexpr uint64_t UNREGISTER_TIMEOUT_USEC = 1000000;
    if (device_connection_cb_token)
      game_input->UnregisterCallback(device_connection_cb_token, UNREGISTER_TIMEOUT_USEC);
    game_input->Release();
    game_input = nullptr;
  }
  device_connection_cb_token = {0};

  clear_devices_list(gamepads_list);
  clear_devices_list(keyboards_list);
  clear_devices_list(mouses_list);
  clear_devices_list(flightsticks_list);

  initialized = false;
  debug("GameInput shut down");
}


void ReadingDeleter::operator()(IGameInputReading *reading)
{
  if (reading)
    reading->Release();
}


Reading get_current_reading(GameInputKind kind, IGameInputDevice *device)
{
  G_ASSERT(game_input);

  Reading result;
  IGameInputReading *reading = nullptr;
  if (SUCCEEDED(game_input->GetCurrentReading(kind, device, &reading)))
    result.reset(reading);

  return result;
}


void get_devices(GameInputKind kind, DevicesList &devices)
{
  devices.clear();
  DevicesListWrapper *dlw = select_list_by_kind(kind);
  if (dlw)
  {
    WinAutoLock lock(dlw->cs);
    devices.resize(MAX_DEVICES_PER_TYPE, nullptr);
    eastl::copy(dlw->list.begin(), dlw->list.end(), devices.begin());
  }
}


bool has_input_device_of_kind(GameInputKind kind)
{
  gameinput::DevicesList devices;
  gameinput::get_devices(kind, devices);
  for (const IGameInputDevice *device : devices)
  {
    if (device)
      return true;
  }
  return false;
}


} // namespace gameinput
