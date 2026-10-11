// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gameinput.h"
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/gdk/plm.h>
#include <debug/dag_debug.h>


namespace gameinput
{


static IGameInput *game_input = nullptr;
static GameInputCallbackToken device_connection_cb_token = {0};
static bool initialized = false;
static int init_refcount = 0;


struct DevicesListWrapper
{
  DevicesList list = {};
  WinCritSec cs;
  volatile uint32_t generation = 0;
};


static DevicesListWrapper gamepads_list;
static DevicesListWrapper keyboards_list;
static DevicesListWrapper mouses_list;
static DevicesListWrapper flightsticks_list;


struct TrackedKind
{
  GameInputKind kind;
  DevicesListWrapper *list;
};

static constexpr TrackedKind tracked_kinds[] = {
  {GameInputKindGamepad, &gamepads_list},
  {GameInputKindKeyboard, &keyboards_list},
  {GameInputKindMouse, &mouses_list},
  {GameInputKindFlightStick, &flightsticks_list},
};

static constexpr GameInputKind make_tracked_kinds_mask()
{
  unsigned mask = GameInputKindUnknown;
  for (const TrackedKind &tracked : tracked_kinds)
    mask |= tracked.kind;
  return GameInputKind(mask);
}

static constexpr GameInputKind TRACKED_KINDS_MASK = make_tracked_kinds_mask();


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
  for (const TrackedKind &tracked : tracked_kinds)
    if (kind & tracked.kind)
      return tracked.list;

  logwarn("Unsupported GameInputKind: 0x%x", kind);
  dump_input_kind(kind);
  return nullptr;
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

  bool connected = status & GameInputDeviceConnected;
  bool supported = false;

  for (const TrackedKind &tracked : tracked_kinds)
  {
    if (!(kind & tracked.kind))
      continue;

    supported = true;
    if (update_devices_state(tracked.list, dev, connected))
    {
      debug("Devices list 0x%x updated", kind);
      interlocked_increment(tracked.list->generation);
    }
  }

  if (!supported)
    logwarn("Unsupported device kind");
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

    // GameInputCreate fails while the title is suspended
    gdk::wait_while_suspended();
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
    result = game_input->RegisterDeviceCallback(nullptr, TRACKED_KINDS_MASK, GameInputDeviceConnected, GameInputAsyncEnumeration,
      nullptr, device_connection_callback, &device_connection_cb_token);
    initialized &= SUCCEEDED(result);
  }
}


static void clear_devices_list(DevicesListWrapper &dlw)
{
  WinAutoLock lock(dlw.cs);
  dlw.list.fill(nullptr);
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


DevicesList get_devices(GameInputKind kind)
{
  DevicesListWrapper *dlw = select_list_by_kind(kind);
  if (!dlw)
    return {};

  WinAutoLock lock(dlw->cs);
  return dlw->list;
}


bool has_input_device_of_kind(GameInputKind kind)
{
  for (const IGameInputDevice *device : gameinput::get_devices(kind))
  {
    if (device)
      return true;
  }
  return false;
}


} // namespace gameinput
