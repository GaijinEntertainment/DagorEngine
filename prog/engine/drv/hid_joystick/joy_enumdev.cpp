// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "include_dinput.h"
#include "joy_enumdev.h"
#include "joy_classdrv.h"
#include "joy_device.h"
#include <drv/hid/dag_hiDInput.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiJoyFF.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <generic/dag_sort.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_watchdog.h>
#include "../hid_commonCode/dsPadIds.h"

using namespace HumanInput;


static BOOL CALLBACK add_joystick_inputs(const DIDEVICEOBJECTINSTANCE *pdidoi, VOID *pContext);
static BOOL CALLBACK set_axis_limits(const DIDEVICEOBJECTINSTANCE *pdidoi, VOID *pContext);
static BOOL CALLBACK detect_hid_usage(const DIDEVICEOBJECTINSTANCE *pdidoi, VOID *pContext);

static const DWORD COOP_LEVEL = DISCL_FOREGROUND | DISCL_EXCLUSIVE;


#define DEL_RET_NULL_ON_FAIL(ptr, f, profiler_tag)                              \
  do                                                                            \
  {                                                                             \
    TIME_PROFILE(profiler_tag);                                                 \
    if (HRESULT hr = f; FAILED(hr))                                             \
    {                                                                           \
      HumanInput::printHResult(__FILE__, __LINE__, "[HID][DI8][ENUM] " #f, hr); \
      delete ptr;                                                               \
      return nullptr;                                                           \
    }                                                                           \
  } while (0);


#define RETURN_ON_FAIL(f, profiler_tag, ret)                                    \
  do                                                                            \
  {                                                                             \
    TIME_PROFILE(profiler_tag);                                                 \
    if (HRESULT hr = f; FAILED(hr))                                             \
    {                                                                           \
      HumanInput::printHResult(__FILE__, __LINE__, "[HID][DI8][ENUM] " #f, hr); \
      return ret;                                                               \
    }                                                                           \
  } while (0);

#define RET_CONT_ON_FAIL(f, profiler_tag) RETURN_ON_FAIL(f, profiler_tag, DIENUM_CONTINUE)
#define RET_STOP_ON_FAIL(f, profiler_tag) RETURN_ON_FAIL(f, profiler_tag, DIENUM_STOP)


static Di8JoystickDevice *create_new_joystick(const Di8Device &d, bool remap_as_x360)
{
  TIME_PROFILE(HID_DI8_create_new_device);
  bool needsPoll = d.caps.dwFlags & DIDC_POLLEDDEVICE;
  unsigned int *guid = (unsigned int *)&d.instance.guidProduct;

  Di8JoystickDevice *j = new (inimem) Di8JoystickDevice(d.device, needsPoll, remap_as_x360, true /*quiet*/, guid);

  j->setDeviceName(d.instance.tszInstanceName);
  j->setInstanceGuid((const unsigned *)&d.instance.guidInstance);
  // can't j->setId() yet, can only be done when we transfer data to the destination array

  DEL_RET_NULL_ON_FAIL(j, d.device->EnumObjects(detect_hid_usage, j, DIDFT_COLLECTION), HID_DI8_EnumObjects_usage);
  const bool doRemapAsX360 = remap_as_x360 && j->isGamepad();

  auto cb = doRemapAsX360 ? set_axis_limits : add_joystick_inputs;
  DEL_RET_NULL_ON_FAIL(j, d.device->EnumObjects(cb, j, DIDFT_ALL), HID_DI8_EnumObjects_setup);

  if (doRemapAsX360)
    j->applyRemapX360(d.instance.tszProductName);
  else
    j->applyRemapHats();

  return j;
}


// An XInput device has "IG_" in its HID path (ex. "\\?\hid#vid_045e&pid_02ff&ig_00#..."). Ask the device itself: a snapshot taken
// before EnumDevices can miss a pad that is still being plugged in.
static bool is_xinput_device(IDirectInputDevice8 *device)
{
  DIPROPGUIDANDPATH prop = {};
  prop.diph.dwSize = sizeof(prop);
  prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  prop.diph.dwHow = DIPH_DEVICE;
  if (FAILED(device->GetProperty(DIPROP_GUIDANDPATH, &prop.diph)))
    return false;
  CharUpperW(prop.wszPath);
  return wcsstr(prop.wszPath, L"IG_") != nullptr;
}


static bool setup_device(Di8Device &d)
{
  RETURN_ON_FAIL(d.device->SetDataFormat(&c_dfDIJoystick2), HID_DI8_SetDataFormat, false);
  RETURN_ON_FAIL(d.device->SetCooperativeLevel((HWND)win32_get_main_wnd(), COOP_LEVEL), HID_DI8_SetCooperativeLevel, false);
  RETURN_ON_FAIL(d.device->GetCapabilities(&d.caps), HID_DI8_GetCapabilities, false);
  return true;
}


static BOOL CALLBACK add_di8_device(const DIDEVICEINSTANCE *inst, VOID *user_data)
{
  TIME_PROFILE(HID_DI8_add_or_remove_di8_device);
  debug("[HID][DI8][ENUM] creating vid:pid=%04x:%04x", LOWORD(inst->guidProduct.Data1), HIWORD(inst->guidProduct.Data1));

  DeviceEnumerator *self = reinterpret_cast<DeviceEnumerator *>(user_data);
  if (self->isThreadTerminating())
  {
    debug("[HID][DI8][ENUM] device enumeration termination requested. Aborting.");
    return DIENUM_STOP;
  }

  int alreadySpent = get_time_msec() - self->startedAtMs;
  if (alreadySpent > self->timeoutMs)
  {
    logerr("[HID][DI8][ENUM] spent %dms (more than %dms) in device enumeration, aborting. Devices may be inconsistent", alreadySpent,
      self->timeoutMs);
    return DIENUM_STOP;
  }

  if (self->excludeDsPads && dspad::is_dspad(LOWORD(inst->guidProduct.Data1), HIWORD(inst->guidProduct.Data1)))
  {
    debug("[HID][DI8][ENUM] skip device vid:pid=%04x:%04x: an excluded dsPad", LOWORD(inst->guidProduct.Data1),
      HIWORD(inst->guidProduct.Data1));
    return DIENUM_CONTINUE;
  }

  Di8Device d{.instance = *inst};
  RET_CONT_ON_FAIL(dinput8->CreateDevice(d.instance.guidInstance, &d.device, nullptr), HID_DI8_CreateDevice);
  if (self->excludeXinputDevices && is_xinput_device(d.device))
  {
    debug("[HID][DI8][ENUM] skip device vid:pid=%04x:%04x: an XInput device", LOWORD(inst->guidProduct.Data1),
      HIWORD(inst->guidProduct.Data1));
    d.device->Release();
    return DIENUM_CONTINUE;
  }
  if (!setup_device(d))
  {
    d.device->Release();
    return DIENUM_CONTINUE;
  }

  d.joy = create_new_joystick(d, self->remapAsX360);
  if (d.joy)
    self->found.push_back(d);
  return self->isThreadTerminating() ? DIENUM_STOP : DIENUM_CONTINUE;
}


void DeviceEnumerator::releaseFound()
{
  WinAutoLock l(cs);
  for (Di8Device &d : found)
    delete d.joy;
  found.clear();
}


void DeviceEnumerator::execute()
{
  TIME_PROFILE(HID_DI8_DeviceEnumerator_execute);
  debug("[HID][DI8][ENUM] starting");
  auto shouldAbort = [this]() -> bool { return this->isThreadTerminating(); };

  WinAutoLock l(cs);
  releaseFound();
  this->startedAtMs = get_time_msec();
  dinput8->EnumDevices(DI8DEVCLASS_GAMECTRL, add_di8_device, this, DIEDFL_ATTACHEDONLY);

  if (shouldAbort())
  {
    debug("[HID][DI8][ENUM] aborted during or after enumeration, will not report devices");
    releaseFound();
    return;
  }

  // TODO: actually count lost and added devices, for now we rely on destroyDevices() after enumeration
  ::interlocked_release_store(modifiedDevices, found.empty() ? lastKnownDevicesCount : found.size());
  debug("[HID][DI8][ENUM] finished: found %d joysticks (previously %d)", found.size(), lastKnownDevicesCount);
}


static FastNameMapEx devNames;

static int cmp_dev_name(HumanInput::Di8JoystickDevice *const *a, HumanInput::Di8JoystickDevice *const *b)
{
  if (int d = strcmp(a[0]->getDeviceID(), b[0]->getDeviceID()))
    return d;
  return (a[0] > b[0]) ? 1 : ((a[0] < b[0]) ? -1 : 0);
}


void Di8JoystickClassDriver::refreshDeviceList()
{
  TIME_PROFILE(HID_DI8_refreshDeviceList);
  bool was_enabled = enabled;
  if (deviceConfigChanged)
  {
    destroyDevices();
    deviceConfigChanged = false;

    enable(false);

    TIME_PROFILE(HID_DI8_add_cur_devices);
    Di8Devices currentlyAttached = deviceEnumerator.fetch();
    device.reserve(currentlyAttached.size());
    for (const auto &d : currentlyAttached)
    {
      d.joy->setId(device.size(), devNames.addNameId(d.instance.tszInstanceName) + 256);
      debug("[HID][DI8] adding joystick #%d - [%s] '%s' (%d axes, %d buttons, %d hats)", device.size(), d.joy->getDeviceID(),
        d.joy->getName(), d.joy->getAxisCount(), d.joy->getButtonCount(), d.joy->getPovHatCount());
      for (int s = 0; s < 2; s++)
        d.joy->setStickDeadZoneScale(s, stickDeadZoneScale[s]);
      device.push_back(d.joy);
    }
    if (stg_joy.sortDevices)
      sort(device, &cmp_dev_name);
  }

  if (secDrv && secDrv->isDeviceConfigChanged())
  {
    // secDrv deletes its devices on refresh, and defJoy can be one of them
    if (defJoy && find_value_idx(device, defJoy) < 0)
      setDefaultJoystick(nullptr);
    secDrv->refreshDeviceList();
    secDrv->setDefaultJoystick(defJoy);
  }

  // update global settings
  if (getDeviceCount() > 0)
  {
    stg_joy.present = true;
    enable(was_enabled);
  }
  else
  {
    stg_joy.present = false;
    stg_joy.enabled = false;
  }

  useDefClient(defClient);
  acquireDevices();

  prevUpdateRefTime = ::ref_time_ticks();
  for (int i = 0; i < device.size(); i++)
    device[i]->updateState(0, device[i] == defJoy); // Might be loosing input here...
}


// Some devices incorrectly report themselves as gamepads but don't match the axis/button convention
static constexpr const char *DEFINITELY_JOYSTICKS[] = {
  "10F5:7012" // VelocityOne Rudder
};


static BOOL CALLBACK detect_hid_usage(const DIDEVICEOBJECTINSTANCE *pdidoi, VOID *pContext)
{
  TIME_PROFILE(HID_DI8_detect_hid_usage);
  Di8JoystickDevice &joy = *reinterpret_cast<Di8JoystickDevice *>(pContext);

  static constexpr DWORD GENERIC_DESKTOP_PAGE = 0x01;
  static constexpr uint16_t USAGE_GAMEPAD = 0x05;
  bool isCollection = (pdidoi->dwType & DIDFT_COLLECTION) == DIDFT_COLLECTION;
  if (isCollection && pdidoi->wUsagePage == GENERIC_DESKTOP_PAGE)
  {
    const char *vidpid = joy.getDeviceID();
    bool isJoystick = false;
    for (int i = 0, total = countof(DEFINITELY_JOYSTICKS); !isJoystick && i < total; ++i)
      isJoystick |= !strcmp(vidpid, DEFINITELY_JOYSTICKS[i]);

    bool isGamepad = !isJoystick && pdidoi->wUsage == USAGE_GAMEPAD;
    debug("[HID][DI8][ENUM][%s] '%s' is %sgamepad, usage=0x%02X", vidpid, joy.getName(), isGamepad ? "a " : "not a ", pdidoi->wUsage);
    joy.markAsGamepad(isGamepad);
    return DIENUM_STOP;
  }

  return DIENUM_CONTINUE;
}


static BOOL CALLBACK add_joystick_inputs(const DIDEVICEOBJECTINSTANCE *pdidoi, VOID *pContext)
{
  TIME_PROFILE(HID_DI8_enumObjectsCallback);
  Di8JoystickDevice &joy = *reinterpret_cast<Di8JoystickDevice *>(pContext);

  if (pdidoi->dwType & DIDFT_AXIS)
  {
    // debug(" * axis %s usagePage %04X usage %04X", pdidoi->tszName, pdidoi->wUsagePage, pdidoi->wUsage);
    // try out GUIDs first
    if (pdidoi->guidType == GUID_Slider)
    {
      if (pdidoi->dwOfs == DIJOFS_SLIDER(0))
        joy.addAxis(JOY_SLIDER0, DIJOFS_SLIDER(0), pdidoi->tszName);
      else if (pdidoi->dwOfs == DIJOFS_SLIDER(1))
        joy.addAxis(JOY_SLIDER1, DIJOFS_SLIDER(1), pdidoi->tszName);
      else
      {
        joy.addAxis(JOY_SLIDER0, 0, pdidoi->tszName);
        debug("[HID][DI8][ENUM] incorrect slider offset: %d != %d,%d", pdidoi->dwOfs, DIJOFS_SLIDER(0), DIJOFS_SLIDER(1));
      }
    }
    else if (pdidoi->guidType == GUID_XAxis)
      joy.addAxis(JOY_AXIS_X, DIJOFS_X, pdidoi->tszName);
    else if (pdidoi->guidType == GUID_YAxis)
      joy.addAxis(JOY_AXIS_Y, DIJOFS_Y, pdidoi->tszName);
    else if (pdidoi->guidType == GUID_ZAxis)
      joy.addAxis(JOY_AXIS_Z, DIJOFS_Z, pdidoi->tszName);

    else if (pdidoi->guidType == GUID_RxAxis)
      joy.addAxis(JOY_AXIS_RX, DIJOFS_RX, pdidoi->tszName);
    else if (pdidoi->guidType == GUID_RyAxis)
      joy.addAxis(JOY_AXIS_RY, DIJOFS_RY, pdidoi->tszName);
    else if (pdidoi->guidType == GUID_RzAxis)
      joy.addAxis(JOY_AXIS_RZ, DIJOFS_RZ, pdidoi->tszName);

    else
    {
      // if GUID not found try than on offset (which are too often incorrect)
      if (pdidoi->dwOfs == DIJOFS_X)
        joy.addAxis(JOY_AXIS_X, DIJOFS_X, pdidoi->tszName);
      else if (pdidoi->dwOfs == DIJOFS_Y)
        joy.addAxis(JOY_AXIS_Y, DIJOFS_Y, pdidoi->tszName);
      else if (pdidoi->dwOfs == DIJOFS_Z)
        joy.addAxis(JOY_AXIS_Z, DIJOFS_Z, pdidoi->tszName);

      else if (pdidoi->dwOfs == DIJOFS_RX)
        joy.addAxis(JOY_AXIS_RX, DIJOFS_RX, pdidoi->tszName);
      else if (pdidoi->dwOfs == DIJOFS_RY)
        joy.addAxis(JOY_AXIS_RY, DIJOFS_RY, pdidoi->tszName);
      else if (pdidoi->dwOfs == DIJOFS_RZ)
        joy.addAxis(JOY_AXIS_RZ, DIJOFS_RZ, pdidoi->tszName);
      else if (pdidoi->dwOfs == DIJOFS_SLIDER(0))
        joy.addAxis(JOY_SLIDER0, DIJOFS_SLIDER(0), pdidoi->tszName);
      else if (pdidoi->dwOfs == DIJOFS_SLIDER(1))
        joy.addAxis(JOY_SLIDER1, DIJOFS_SLIDER(1), pdidoi->tszName);
      else
      {
        debug("[HID][DI8][ENUM][%s] unknown axis: type=%p:%p:%p:%p <%s>", joy.getDeviceID(), ((unsigned *)&pdidoi->guidType)[0],
          ((unsigned *)&pdidoi->guidType)[1], ((unsigned *)&pdidoi->guidType)[2], ((unsigned *)&pdidoi->guidType)[3], pdidoi->tszName);
        return DIENUM_CONTINUE;
      }
    }

    DIPROPRANGE diprg;
    diprg.diph.dwSize = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprg.diph.dwHow = DIPH_BYID;
    diprg.diph.dwObj = pdidoi->dwType; // Specify the enumerated axis
    diprg.lMin = JOY_XINPUT_MIN_AXIS_VAL;
    diprg.lMax = JOY_XINPUT_MAX_AXIS_VAL;

    // Set the range for the axis
    RET_STOP_ON_FAIL(joy.getDev()->SetProperty(DIPROP_RANGE, &diprg.diph), HID_DI8_SetProperty);
  }
  else if (pdidoi->dwType & DIDFT_BUTTON)
  {
    // debug("[HID][DI8] button %s usagePage %04X usage %04X", pdidoi->tszName, pdidoi->wUsagePage, pdidoi->wUsage);
    joy.addButton(pdidoi->tszName);
  }
  else if (pdidoi->dwType & DIDFT_POV)
  {
    // debug("[HID][DI8][ENUM][%s] POV %s usagePage %04X usage %04X", joy.getDeviceID(), pdidoi->tszName, pdidoi->wUsagePage,
    //   pdidoi->wUsage);
    joy.addPovHat(pdidoi->tszName);
  }
  else if ((pdidoi->dwType & DIDFT_NODATA) == 0)
  {
    debug("[HID][DI8][ENUM][%s] UNKNOWN %s usagePage 0x%04X usage 0x%04X dwType 0x%08X", joy.getDeviceID(), pdidoi->tszName,
      pdidoi->wUsagePage, pdidoi->wUsage, pdidoi->dwType);
  }
  // else
  //   debug("[HID][DI8][ENUM] unknown obj: %ph <%s> guid=%p:%p:%p:%p dwFlags=%ph",
  //     pdidoi->dwType, pdidoi->tszName, pdidoi->guidType, pdidoi->dwFlags);

  return DIENUM_CONTINUE;
}


static BOOL CALLBACK set_axis_limits(const DIDEVICEOBJECTINSTANCE *pdidoi, VOID *pContext)
{
  TIME_PROFILE(HID_DI8_set_axis_limits);
  Di8JoystickDevice &joy = *reinterpret_cast<Di8JoystickDevice *>(pContext);
  if (pdidoi->dwType & DIDFT_AXIS)
  {
    DIPROPRANGE diprg;
    diprg.diph.dwSize = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprg.diph.dwHow = DIPH_BYID;
    diprg.diph.dwObj = pdidoi->dwType; // Specify the enumerated axis
    diprg.lMin = JOY_XINPUT_MIN_AXIS_VAL;
    diprg.lMax = JOY_XINPUT_MAX_AXIS_VAL;

    // Set the range for the axis
    RET_STOP_ON_FAIL(joy.getDev()->SetProperty(DIPROP_RANGE, &diprg.diph), HID_DI8_SetProperty);
  }

  return DIENUM_CONTINUE;
}
