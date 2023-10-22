#include "joy_device.h"
#include <humanInput/dag_hiGlobals.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_mathBase.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <perfmon/dag_autofuncprof.h>

using namespace HumanInput;

#define VIRTUAL_THUMBKEY_PRESS_THRESHOLD 0.5f

HidJoystickDevice::HidJoystickDevice(IOHIDDeviceRef dev) : buttons(inimem), axes(inimem), povHats(midmem)
{
  connected = false;
  device = dev;
  CFRetain(device);

  client = NULL;
  memset(&state, 0, sizeof(state));

  char product[256];
  if (CFStringRef cfs = (CFStringRef)IOHIDDeviceGetProperty(dev, CFSTR(kIOHIDProductKey)))
    CFStringGetCString(cfs, product, sizeof(product), kCFStringEncodingUTF8);
  else
    strcpy(product, "???");
  for (int l = strlen(product) - 1; l >= 0; l--)
    if (strchr(" \t\n\r", product[l]))
      product[l] = 0;
    else
      break;
  SNPRINTF(devID, sizeof(devID), "%04X:%04X", (int)IOHIDDevice_GetVendorID(dev), (int)IOHIDDevice_GetProductID(dev));

  debug_ctx("added joystick: dev=%p <%s> %04lX:%04lX", device, product, IOHIDDevice_GetVendorID(dev), IOHIDDevice_GetProductID(dev));
  setDeviceName(product);

  // IOHIDDeviceOpen(dev, kIOHIDOptionsTypeSeizeDevice);
  if (CFArrayRef elements = IOHIDDeviceCopyMatchingElements(dev, NULL, kIOHIDOptionsTypeNone))
  {
    uintptr_t add_stor_idx = 0;
    for (CFIndex idx = 0, cnt = CFArrayGetCount(elements); idx < cnt; idx++)
    {
      IOHIDElementRef elem = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, idx);
      if (!elem)
        continue;

      // is this an input element?
      IOHIDElementType type = IOHIDElementGetType(elem);
      switch (type)
      {
        case kIOHIDElementTypeInput_Misc:
        case kIOHIDElementTypeInput_Button:
        case kIOHIDElementTypeInput_Axis: break;
        default: continue;
      }

      uint32_t eu = IOHIDElementGetUsage(elem), epu = IOHIDElementGetUsagePage(elem);
      debug("%d/%d %p  %d %X.%X", idx, cnt, elem, elem ? IOHIDElementGetType(elem) : -1, epu, eu);
      if (epu == kHIDPage_Button && eu != 0xFFFFFFFFU)
      {
        ButtonData &b = buttons.push_back();
        b.elem = elem;
        b.name = String(0, "Button%d", eu);
        debug("add button: %s", b.name);
        CFRetain(elem);
      }
      else if (type != kIOHIDElementTypeInput_Button && epu == kHIDPage_GenericDesktop && eu == kHIDUsage_GD_Hatswitch)
      {
        PovHatData &p = povHats.push_back();
        p.elem = elem;
        p.name = String(0, "POV%d", povHats.size());
        p.axisName[0] = String(0, "POV%d_H", povHats.size());
        p.axisName[1] = String(0, "POV%d_V", povHats.size());
        debug("add pov: %s", p.name);
        CFRetain(elem);
      }
      else if (type != kIOHIDElementTypeInput_Button)
      {
        if (IOHIDElementGetReportCount(elem) > 1)
          continue;
        int *stor = NULL;
        const char *aname = NULL;
        bool signed_val = false;
        if (epu == kHIDPage_GenericDesktop)
          switch (eu)
          {
            case kHIDUsage_GD_X:
              stor = &state.x;
              aname = "X axis";
              signed_val = true;
              break;
            case kHIDUsage_GD_Y:
              stor = &state.y;
              aname = "Y axis";
              signed_val = true;
              break;
            case kHIDUsage_GD_Z:
              stor = &state.z;
              aname = "Z axis";
              signed_val = true;
              break;
            case kHIDUsage_GD_Rx:
              stor = &state.rx;
              aname = "Rx axis";
              signed_val = true;
              break;
            case kHIDUsage_GD_Ry:
              stor = &state.ry;
              aname = "Ry axis";
              signed_val = true;
              break;
            case kHIDUsage_GD_Rz:
              stor = &state.rz;
              aname = "Rz axis";
              signed_val = true;
              break;
            case kHIDUsage_GD_Slider:
              stor = &state.slider[0];
              aname = "Slider";
              break;
          }
        else if (epu == kHIDPage_Simulation)
          switch (eu)
          {
            case kHIDUsage_Sim_Aileron:
              stor = &state.x;
              aname = "Ailerons";
              signed_val = true;
              break;
            case kHIDUsage_Sim_Elevator:
              stor = &state.y;
              aname = "Elevators";
              signed_val = true;
              break;
            case kHIDUsage_Sim_Rudder:
              stor = &state.z;
              aname = "Rudder";
              signed_val = true;
              break;
            case kHIDUsage_Sim_Throttle:
              stor = &state.slider[0];
              aname = "Throttle";
              break;
          }
        else if (epu == kHIDPage_Game)
          switch (eu)
          {
            case kHIDUsage_Game_TurnRightOrLeft:
              stor = &state.rx;
              aname = "Rx axis";
              signed_val = true;
              break;
            case kHIDUsage_Game_PitchUpOrDown:
              stor = &state.ry;
              aname = "Ry axis";
              signed_val = true;
              break;
            case kHIDUsage_Game_RollRightOrLeft:
              stor = &state.rz;
              aname = "Rz axis";
              signed_val = true;
              break;
            case kHIDUsage_Game_MoveRightOrLeft:
              stor = &state.x;
              aname = "X axis";
              signed_val = true;
              break;
            case kHIDUsage_Game_MoveForwardOrBackward:
              stor = &state.y;
              aname = "Y axis";
              signed_val = true;
              break;
            case kHIDUsage_Game_MoveUpOrDown:
              stor = &state.z;
              aname = "Z axis";
              signed_val = true;
              break;
          }
        if (!stor && epu != kHIDPage_Undefined && epu < kHIDPage_VendorDefinedStart && IOHIDElementGetLogicalMax(elem) > 1)
          ; // add as generic axis
        else if (!stor)
        {
          logerr("skip %X:%X", epu, eu);
          continue;
        }

        // prevent use of crossed storage/names
        if (stor)
          for (int i = 0; i < axes.size(); i++)
            if (axes[i].valStorage == stor)
              stor = NULL;
        if (name)
          for (int i = 0; i < axes.size(); i++)
            if (strcmp(axes[i].name, name) == 0)
              name = NULL;

        // register new axis
        AxisData &a = axes.push_back();
        a.elem = elem;
        a.dv = 1.0 / (MAX_AXIS_VAL - MIN_AXIS_VAL);
        a.v0 = -a.dv * MIN_AXIS_VAL;
        a.maxLogVal = IOHIDElementGetPhysicalMax(elem);
        a.signedVal = signed_val ? 32000 : 0;

        a.v0 = 0;
        a.dv = 1;
        if (stor)
          a.valStorage = stor;
        else
        {
          a.valStorage = (int *)add_stor_idx;
          add_stor_idx++;
        }
        if (aname)
          a.name = aname;
        else
          a.name = String(0, "Axis%d", axes.size());
        debug("add axis: %s", a.name);
        CFRetain(elem);
      }
    }

    if (add_stor_idx)
    {
      clear_and_resize(axesValStor, add_stor_idx);
      for (int i = 0; i < axes.size(); i++)
        if ((uintptr_t)axes[i].valStorage < add_stor_idx)
          axes[i].valStorage = &axesValStor[(uintptr_t)axes[i].valStorage];
    }

    CFRelease(elements);
    connected = true;
  }
}

HidJoystickDevice::~HidJoystickDevice()
{
  setClient(NULL);

  for (int i = 0; i < buttons.size(); i++)
    CFRetain(buttons[i].elem);
  for (int i = 0; i < axes.size(); i++)
    CFRetain(axes[i].elem);
  for (int i = 0; i < povHats.size(); i++)
    CFRetain(povHats[i].elem);

  // IOHIDDeviceClose(device, kIOHIDOptionsTypeSeizeDevice);
  CFRelease(device);
  device = NULL;
}


void HidJoystickDevice::setDeviceName(const char *nm)
{
  name = nm;
  debug_ctx("joy device name: <%s>", name);
}

bool HidJoystickDevice::updateState(int dt_msec, bool def)
{
  bool changed = false;

  static int lastKeyPressedTime = 0;
  int ctime = ::get_time_msec();

  if (state.buttons.hasAny() && ctime > lastKeyPressedTime + stg_joy.keyRepeatDelay)
  {
    lastKeyPressedTime = ctime;
    state.repeat = true;
    if (def)
      raw_state_joy.repeat = true;
  }
  else
  {
    state.repeat = false;
    if (def)
      raw_state_joy.repeat = false;
  }

  // record event
  static constexpr int DATA_SZ = 14 * sizeof(int);
  char prev_state[DATA_SZ];
  memcpy(prev_state, &state, DATA_SZ);

  for (int i = 0; i < axes.size(); i++)
  {
    IOHIDValueRef valueRef;
    if (IOHIDDeviceGetValue(device, axes[i].elem, &valueRef) == kIOReturnSuccess)
    {
      int v = IOHIDValueGetScaledValue(valueRef, kIOHIDValueScaleTypePhysical);
      if (axes[i].signedVal)
        v = v * MAX_AXIS_VAL * 2 / axes[i].maxLogVal - MAX_AXIS_VAL;
      else
        v = v * MAX_AXIS_VAL / axes[i].maxLogVal;
      if (*axes[i].valStorage != v)
        changed = true;
      *axes[i].valStorage = v;
    }
  }

  state.buttonsPrev = state.buttons;
  state.buttons.reset();
  for (unsigned i = 0; i < buttons.size(); i++)
  {
    IOHIDValueRef valueRef = NULL;
    if (IOHIDDeviceGetValue(device, buttons[i].elem, &valueRef) == kIOReturnSuccess)
      if (valueRef && IOHIDValueGetIntegerValue(valueRef))
        state.buttons.set(i);
  }

  for (int i = 0; i < povHats.size(); i++)
  {
    state.povValuesPrev[i] = state.povValues[i];

    IOHIDValueRef valueRef;
    if (IOHIDDeviceGetValue(device, povHats[i].elem, &valueRef) != kIOReturnSuccess)
      continue;
    unsigned v = IOHIDValueGetScaledValue(valueRef, kIOHIDValueScaleTypePhysical);
    if (v == 360)
      state.povValues[i] = -1;
    else
      state.povValues[i] = int(v) * 100;
    changed |= (state.povValuesPrev[i] != state.povValues[i]);
  }

  if (state.getKeysPressed().hasAny()) // Little trick to avoid new static bool isFirstTimePressed
    lastKeyPressedTime = ctime + (stg_joy.keyFirstRepeatDelay - stg_joy.keyRepeatDelay);

  if (!state.buttons.cmpEq(state.buttonsPrev))
  {
    changed = true;
    for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
      if (state.buttons.getIter(i, inc))
        if (!state.buttonsPrev.get(i))
          state.bDownTms[i] = 0;
  }

  for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
    if (state.buttonsPrev.getIter(i, inc))
      state.bDownTms[i] += dt_msec;

  if (!changed)
    changed = memcmp(prev_state, &state, DATA_SZ) != 0;

  if (def)
  {
    if (changed)
      raw_state_joy = state;
    else
      memcpy(raw_state_joy.bDownTms, state.bDownTms, sizeof(raw_state_joy.bDownTms));
  }

  if (client && changed)
    client->stateChanged(this, ordId);

  return changed && !state.buttons.cmpEq(state.buttonsPrev);
}

void HidJoystickDevice::setClient(IGenJoystickClient *cli)
{
  if (cli == client)
    return;

  if (client)
    client->detached(this);
  client = cli;
  if (client)
    client->attached(this);
}


IGenJoystickClient *HidJoystickDevice::getClient() const { return client; }

int HidJoystickDevice::getButtonCount() const { return buttons.size(); }
const char *HidJoystickDevice::getButtonName(int idx) const
{
  if (idx >= 0 && idx < buttons.size())
    return buttons[idx].name;
  return NULL;
}

int HidJoystickDevice::getAxisCount() const { return axes.size() + 2 * povHats.size(); }
const char *HidJoystickDevice::getAxisName(int idx) const
{
  const int ac = axes.size();
  if (idx >= 0 && idx < ac)
    return axes[idx].name;
  if (idx >= ac && idx < ac + 2 * povHats.size())
    return povHats[(idx - ac) / 2].axisName[(idx - ac) % 2];
  return NULL;
}


int HidJoystickDevice::getPovHatCount() const { return povHats.size(); }
const char *HidJoystickDevice::getPovHatName(int idx) const
{
  if (idx >= 0 && idx < povHats.size())
    return povHats[idx].name;
  return NULL;
}

int HidJoystickDevice::getButtonId(const char *name) const
{
  for (int i = 0; i < buttons.size(); i++)
    if (strcmp(buttons[i].name, name) == 0)
      return i;
  return -1;
}
int HidJoystickDevice::getAxisId(const char *name) const
{
  for (int i = 0; i < axes.size(); i++)
    if (strcmp(axes[i].name, name) == 0)
      return i;
  for (int i = 0; i < povHats.size(); i++)
    for (int j = 0; j < 2; j++)
      if (strcmp(povHats[i].axisName[j], name) == 0)
        return axes.size() + i * 2 + j;
  return -1;
}
int HidJoystickDevice::getPovHatId(const char *name) const
{
  for (int i = 0; i < povHats.size(); i++)
    if (strcmp(povHats[i].name, name) == 0)
      return i;
  return -1;
}

void HidJoystickDevice::setAxisLimits(int axis_id, float min_val, float max_val)
{
  if (axis_id >= 0 && axis_id < axes.size())
  {
    axes[axis_id].dv = (max_val - min_val) / (MAX_AXIS_VAL - MIN_AXIS_VAL);
    axes[axis_id].v0 = min_val - axes[axis_id].dv * MIN_AXIS_VAL;
  }
}


bool HidJoystickDevice::getButtonState(int btn_id) const { return btn_id >= 0 ? state.buttons.get(btn_id) : false; }
float HidJoystickDevice::getAxisPos(int axis_id) const
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
    return axes[axis_id].v0 + axes[axis_id].dv * axes[axis_id].valStorage[0];
  if (axis_id >= ac && axis_id < ac + 2 * povHats.size())
    return getVirtualPOVAxis((axis_id - ac) / 2, (axis_id - ac) % 2);
  return 0;
}
int HidJoystickDevice::getAxisPosRaw(int axis_id) const
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
    return axes[axis_id].valStorage[0];
  if (axis_id >= ac && axis_id < ac + 2 * povHats.size())
    return getVirtualPOVAxis((axis_id - ac) / 2, (axis_id - ac) % 2);
  return 0;
}

int HidJoystickDevice::getPovHatAngle(int hat_id) const
{
  if (hat_id >= 0 && hat_id < povHats.size())
    return state.povValues[hat_id];
  return -1;
}

int HidJoystickDevice::getVirtualPOVAxis(int hat_id, int axis_id) const
{
  if ((axis_id != 0) && (axis_id != 1))
    return 0;
  int value = getPovHatAngle(hat_id);
  if (value >= 0)
  {
    float angle = -(float(value) / 36000.f) * TWOPI + HALFPI;
    float axisVal = (axis_id == 1) ? sin(angle) : cos(angle);
    return int(axisVal * 32000);
  }
  return 0;
}
