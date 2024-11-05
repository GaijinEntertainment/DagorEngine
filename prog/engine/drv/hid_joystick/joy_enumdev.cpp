// Copyright (C) Gaijin Games KFT.  All rights reserved.

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif
#include "include_dinput.h"
#include "joy_classdrv.h"
#include "joy_device.h"
#include <drv/hid/dag_hiDInput.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiJoyFF.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <generic/dag_sort.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_watchdog.h>

using namespace HumanInput;

#define DEBUG_DUMP_EFFECTS (0)

static BOOL CALLBACK enumJoysticksCallback(const DIDEVICEINSTANCE *pdidInstance, VOID *pContext);
static BOOL CALLBACK enumObjectsCallback(const DIDEVICEOBJECTINSTANCE *pdidoi, VOID *pContext);
static BOOL CALLBACK enumObjSetLim(const DIDEVICEOBJECTINSTANCE *pdidoi, VOID *pContext);
static BOOL isXInputDevice(const GUID *pGuidProductFromDirectInput);
static bool exclude_xinput_dev = false, remap_360 = false;
static bool enum_probe_only = false;

static FastNameMapEx devNames;

static int cmp_dev_name(HumanInput::Di8JoystickDevice *const *a, HumanInput::Di8JoystickDevice *const *b)
{
  if (int d = strcmp(a[0]->getDeviceID(), b[0]->getDeviceID()))
    return d;
  return (a[0] > b[0]) ? 1 : ((a[0] < b[0]) ? -1 : 0);
}

void Di8JoystickClassDriver::checkDeviceList()
{
  static int last_t0 = 0;

  if (!maybeDeviceConfigChanged || (last_t0 && get_time_msec() < last_t0 + 500))
    return;
  maybeDeviceConfigChanged = 0;

  Tab<Di8JoystickDevice *> dev;
  HRESULT hr;
  exclude_xinput_dev = excludeXinputDev;
  remap_360 = remapAsX360;
  enum_probe_only = true;
  hr = dinput8->EnumDevices(DI8DEVCLASS_GAMECTRL, enumJoysticksCallback, &dev, DIEDFL_ATTACHEDONLY);
  exclude_xinput_dev = remap_360 = false;
  enum_probe_only = false;
  last_t0 = get_time_msec();
  if (FAILED(hr))
  {
    HumanInput::printHResult(__FILE__, __LINE__, "EnumDevices", hr);
    return;
  }

  bool changed = dev.size() != device.size();
  if (!changed)
  {
    for (int i = 0; i < dev.size(); i++)
    {
      bool found = false;
      for (int j = 0; j < device.size(); j++)
        if (dev[i]->isEqual(device[j]))
        {
          found = true;
          break;
        }
      if (!found)
      {
        changed = true;
        break;
      }
    }
  }
  if (!changed)
  {
    for (int i = 0; i < device.size(); i++)
    {
      bool found = false;
      for (int j = 0; j < dev.size(); j++)
        if (device[i]->isEqual(dev[j]))
        {
          found = true;
          break;
        }
      if (!found)
      {
        changed = true;
        break;
      }
    }
  }

  for (int i = 0; i < dev.size(); i++)
    delete dev[i];

  if (changed)
    deviceConfigChanged = true;
}

bool Di8JoystickClassDriver::tryRefreshDeviceList()
{
  if (deviceCheckerListRunning())
  {
    logwarn("Di8JoystickClassDriver::tryRefreshDeviceList is locked by AsyncDeviceListChecker job");
    return false;
  }
  destroyDevices();
  deviceConfigChanged = false;

  bool was_enabled = enabled;
  enable(false);

  exclude_xinput_dev = excludeXinputDev;
  remap_360 = remapAsX360;
  int start = get_time_msec();
  HRESULT hr = [&] {
    ScopeSetWatchdogTimeout _wd(60000);
    return dinput8->EnumDevices(DI8DEVCLASS_GAMECTRL, enumJoysticksCallback, &device, DIEDFL_ATTACHEDONLY);
  }();
  // if EnumDevices takes more than 30s than don't add joystick to update list
  if (int dt = get_time_msec() - start; dt > 30000)
  {
    logwarn("IDirectInput8A::EnumDevices took %dms . The Joysticks will be ignored!", dt);
    return false;
  }
  exclude_xinput_dev = remap_360 = false;
  if (FAILED(hr))
  {
    HumanInput::printHResult(__FILE__, __LINE__, "EnumDevices", hr);
    return false;
  }
  if (stg_joy.sortDevices)
    sort(device, &cmp_dev_name);

  if (secDrv)
  {
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
    device[i]->updateState(0, device[i] == defJoy);

  return true;
}

#if DEBUG_DUMP_EFFECTS

static DWORD dbgTypeFlags[] = {DIEFT_CONSTANTFORCE, DIEFT_RAMPFORCE, DIEFT_PERIODIC, DIEFT_CONDITION, DIEFT_CUSTOMFORCE,
  DIEFT_HARDWARE, DIEFT_FFATTACK, DIEFT_FFFADE, DIEFT_SATURATION, DIEFT_POSNEGCOEFFICIENTS, DIEFT_POSNEGSATURATION, DIEFT_DEADBAND,
  DIEFT_STARTDELAY};
static const char *dbgTypeFlagsStr[] = {"DIEFT_CONSTANTFORCE", "DIEFT_RAMPFORCE", "DIEFT_PERIODIC", "DIEFT_CONDITION",
  "DIEFT_CUSTOMFORCE", "DIEFT_HARDWARE", "DIEFT_FFATTACK", "DIEFT_FFFADE", "DIEFT_SATURATION", "DIEFT_POSNEGCOEFFICIENTS",
  "DIEFT_POSNEGSATURATION", "DIEFT_DEADBAND", "DIEFT_STARTDELAY"};

static DWORD dbgParamFlags[] = {DIEP_DURATION, DIEP_SAMPLEPERIOD, DIEP_GAIN, DIEP_TRIGGERBUTTON, DIEP_TRIGGERREPEATINTERVAL, DIEP_AXES,
  DIEP_DIRECTION, DIEP_ENVELOPE, DIEP_TYPESPECIFICPARAMS};
static const char *dbgParamFlagsStr[] = {"DIEP_DURATION", "DIEP_SAMPLEPERIOD", "DIEP_GAIN", "DIEP_TRIGGERBUTTON",
  "DIEP_TRIGGERREPEATINTERVAL", "DIEP_AXES", "DIEP_DIRECTION", "DIEP_ENVELOPE", "DIEP_TYPESPECIFICPARAMS"};

static BOOL CALLBACK enumEffectsCallback(LPCDIEFFECTINFO eff, LPVOID dev)
{
  debug("Found effect '%s'", eff->tszName);
  for (int i = 0; i < countof(dbgTypeFlags); i++)
    if (eff->dwEffType & dbgTypeFlags[i])
      debug(" - Type: %s", dbgTypeFlagsStr[i]);

  for (int i = 0; i < countof(dbgParamFlags); i++)
    if (eff->dwStaticParams & dbgParamFlags[i])
      debug(" - [%c] Param: %s", (eff->dwDynamicParams & dbgParamFlags[i]) ? 'D' : 'S', dbgParamFlagsStr[i]);

  return TRUE;
};

#endif // DEBUG_DUMP_EFFECTS

static BOOL CALLBACK enumJoysticksCallback(const DIDEVICEINSTANCE *pdidInstance, VOID *pContext)
{
  Tab<Di8JoystickDevice *> &device = *reinterpret_cast<Tab<Di8JoystickDevice *> *>(pContext);
  IDirectInputDevice8 *lpJoystick = NULL;
  HRESULT hr;

  if (exclude_xinput_dev)
    if (isXInputDevice(&pdidInstance->guidProduct))
      return DIENUM_CONTINUE;

  hr = dinput8->CreateDevice(pdidInstance->guidInstance, &lpJoystick, NULL);
  if (FAILED(hr))
  {
    HumanInput::printHResult(__FILE__, __LINE__, "CreateDevice", hr);
    return DIENUM_CONTINUE;
  }

  hr = lpJoystick->SetDataFormat(&c_dfDIJoystick2);
  if (FAILED(hr))
  {
    HumanInput::printHResult(__FILE__, __LINE__, "SetDataFormat", hr);
    return DIENUM_CONTINUE;
  }

  hr = lpJoystick->SetCooperativeLevel((HWND)win32_get_main_wnd(), DISCL_FOREGROUND | DISCL_EXCLUSIVE);
  if (FAILED(hr))
  {
    HumanInput::printHResult(__FILE__, __LINE__, "SetCooperativeLevel", hr);
    return DIENUM_CONTINUE;
  }

  DIDEVCAPS joyCaps;
  memset(&joyCaps, 0, sizeof(joyCaps));
  joyCaps.dwSize = sizeof(joyCaps);

  hr = lpJoystick->GetCapabilities(&joyCaps);
  if (FAILED(hr))
  {
    HumanInput::printHResult(__FILE__, __LINE__, "GetCapabilities", hr);
    return DIENUM_CONTINUE;
  }

  Di8JoystickDevice *joyDev = new (inimem) Di8JoystickDevice(lpJoystick, joyCaps.dwFlags & DIDC_POLLEDDEVICE, remap_360,
    enum_probe_only, (unsigned *)&pdidInstance->guidProduct);

  if (enum_probe_only)
  {
    device.push_back(joyDev);
    return DIENUM_CONTINUE;
  }

  joyDev->setDeviceName(pdidInstance->tszInstanceName);
  joyDev->setId(device.size(), devNames.addNameId(pdidInstance->tszInstanceName) + 256);

  hr = lpJoystick->EnumObjects(remap_360 ? enumObjSetLim : enumObjectsCallback, joyDev, DIDFT_ALL);
  if (FAILED(hr))
  {
    HumanInput::printHResult(__FILE__, __LINE__, "EnumObjects", hr);

    delete joyDev;
    return DIENUM_CONTINUE;
  };

#if DEBUG_DUMP_EFFECTS
  hr = lpJoystick->EnumEffects(enumEffectsCallback, joyDev, DIEFT_ALL);
  if (FAILED(hr))
  {
    HumanInput::printHResult(__FILE__, __LINE__, "EnumEffects", hr);
    // ignore error - it's not fatal to have no effects at all
  }
#endif

  if (remap_360)
    joyDev->applyRemapX360(pdidInstance->tszProductName);
  else
    joyDev->applyRemapHats();

  device.push_back(joyDev);

  return DIENUM_CONTINUE;
}

static BOOL CALLBACK enumObjectsCallback(const DIDEVICEOBJECTINSTANCE *pdidoi, VOID *pContext)
{
  Di8JoystickDevice &joy = *reinterpret_cast<Di8JoystickDevice *>(pContext);

  if (pdidoi->dwType & DIDFT_AXIS)
  {
    debug(" * axis %s usagePage %04X usage %04X", pdidoi->tszName, pdidoi->wUsagePage, pdidoi->wUsage);
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
        DEBUG_CTX("incorrect slider offset: %d != %d,%d", pdidoi->dwOfs, DIJOFS_SLIDER(0), DIJOFS_SLIDER(1));
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
        DEBUG_CTX("unknown axis: type=%p:%p:%p:%p <%s>", ((unsigned *)&pdidoi->guidType)[0], ((unsigned *)&pdidoi->guidType)[1],
          ((unsigned *)&pdidoi->guidType)[2], ((unsigned *)&pdidoi->guidType)[3], pdidoi->tszName);
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
    HRESULT hr = joy.getDev()->SetProperty(DIPROP_RANGE, &diprg.diph);
    if (FAILED(hr))
    {
      HumanInput::printHResult(__FILE__, __LINE__, "SetProperty", hr);
      return DIENUM_STOP;
    }
  }
  else if (pdidoi->dwType & DIDFT_BUTTON)
  {
    debug(" * button %s usagePage %04X usage %04X", pdidoi->tszName, pdidoi->wUsagePage, pdidoi->wUsage);
    joy.addButton(pdidoi->tszName);
  }
  else if (pdidoi->dwType & DIDFT_POV)
  {
    debug(" * POV %s usagePage %04X usage %04X", pdidoi->tszName, pdidoi->wUsagePage, pdidoi->wUsage);
    joy.addPovHat(pdidoi->tszName);
  }
  else
  {
    debug(" * UNKNOWN %s usagePage %04X usage %04X", pdidoi->tszName, pdidoi->wUsagePage, pdidoi->wUsage);
  }
  // else
  //   DEBUG_CTX("unknown obj: %ph <%s> guid=%p:%p:%p:%p dwFlags=%ph",
  //     pdidoi->dwType, pdidoi->tszName, pdidoi->guidType, pdidoi->dwFlags);

  return DIENUM_CONTINUE;
}

static BOOL CALLBACK enumObjSetLim(const DIDEVICEOBJECTINSTANCE *pdidoi, VOID *pContext)
{
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
    HRESULT hr = joy.getDev()->SetProperty(DIPROP_RANGE, &diprg.diph);
    if (FAILED(hr))
    {
      HumanInput::printHResult(__FILE__, __LINE__, "SetProperty", hr);
      return DIENUM_STOP;
    }
  }

  return DIENUM_CONTINUE;
}


#include <wbemidl.h>
#include <oleauto.h>
#include <stdio.h>
#pragma comment(lib, "Oleaut32.lib")

//-----------------------------------------------------------------------------
// Enum each PNP device using WMI and check each device ID to see if it contains
// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it's an XInput device
// Unfortunately this information can not be found by just using DirectInput
//-----------------------------------------------------------------------------
static BOOL isXInputDevice(const GUID *pGuidProductFromDirectInput)
{
  IWbemLocator *pIWbemLocator = NULL;
  IEnumWbemClassObject *pEnumDevices = NULL;
  IWbemClassObject *pDevices[20] = {0};
  IWbemServices *pIWbemServices = NULL;
  BSTR bstrNamespace = NULL;
  BSTR bstrDeviceID = NULL;
  BSTR bstrClassName = NULL;
  DWORD uReturned = 0;
  bool bIsXinputDevice = false;
  UINT iDevice = 0;
  VARIANT var;
  HRESULT hr;

  // CoInit if needed
  hr = CoInitialize(NULL);
  bool bCleanupCOM = SUCCEEDED(hr);

  // Create WMI
  hr = CoCreateInstance(__uuidof(WbemLocator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IWbemLocator), (LPVOID *)&pIWbemLocator);
  if (FAILED(hr) || pIWbemLocator == NULL)
    goto LCleanup;

  bstrNamespace = SysAllocString(L"\\\\.\\root\\cimv2");
  if (bstrNamespace == NULL)
    goto LCleanup;
  bstrClassName = SysAllocString(L"Win32_PNPEntity");
  if (bstrClassName == NULL)
    goto LCleanup;
  bstrDeviceID = SysAllocString(L"DeviceID");
  if (bstrDeviceID == NULL)
    goto LCleanup;

  // Connect to WMI
  hr = pIWbemLocator->ConnectServer(bstrNamespace, NULL, NULL, 0, 0, NULL, NULL, &pIWbemServices);
  if (FAILED(hr) || pIWbemServices == NULL)
    goto LCleanup;

  // Switch security level to IMPERSONATE.
  CoSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
    NULL, EOAC_NONE);

  hr = pIWbemServices->CreateInstanceEnum(bstrClassName, 0, NULL, &pEnumDevices);
  if (FAILED(hr) || pEnumDevices == NULL)
    goto LCleanup;

  // Loop over all devices
  for (;;)
  {
    // Get 20 at a time
    hr = pEnumDevices->Next(10000, 20, pDevices, &uReturned);
    if (FAILED(hr))
      goto LCleanup;
    if (uReturned == 0)
      break;

    for (iDevice = 0; iDevice < uReturned; iDevice++)
    {
      // For each device, get its device ID
      hr = pDevices[iDevice]->Get(bstrDeviceID, 0L, &var, NULL, NULL);
      if (SUCCEEDED(hr) && var.vt == VT_BSTR && var.bstrVal != NULL)
      {
        // Check if the device ID contains "IG_".  If it does, then it's an XInput device
        // This information can not be found from DirectInput
        if (wcsstr(var.bstrVal, L"IG_"))
        {
          // If it does, then get the VID/PID from var.bstrVal
          DWORD dwPid = 0, dwVid = 0;
          WCHAR *strVid = wcsstr(var.bstrVal, L"VID_");
          if (strVid && swscanf(strVid, L"VID_%4X", &dwVid) != 1)
            dwVid = 0;
          WCHAR *strPid = wcsstr(var.bstrVal, L"PID_");
          if (strPid && swscanf(strPid, L"PID_%4X", &dwPid) != 1)
            dwPid = 0;

          // Compare the VID/PID to the DInput device
          DWORD dwVidPid = MAKELONG(dwVid, dwPid);
          if (dwVidPid == pGuidProductFromDirectInput->Data1)
          {
            bIsXinputDevice = true;
            goto LCleanup;
          }
        }
      }
      if (pDevices[iDevice])
      {
        pDevices[iDevice]->Release();
        pDevices[iDevice] = NULL;
      }
    }
  }

LCleanup:
  if (bstrNamespace)
    SysFreeString(bstrNamespace);
  if (bstrDeviceID)
    SysFreeString(bstrDeviceID);
  if (bstrClassName)
    SysFreeString(bstrClassName);
  for (iDevice = 0; iDevice < 20; iDevice++)
    if (pDevices[iDevice])
      pDevices[iDevice]->Release();
  if (pEnumDevices)
    pEnumDevices->Release();
  if (pIWbemLocator)
    pIWbemLocator->Release();
  if (pIWbemServices)
    pIWbemServices->Release();

  if (bCleanupCOM)
    CoUninitialize();

  return bIsXinputDevice;
}
