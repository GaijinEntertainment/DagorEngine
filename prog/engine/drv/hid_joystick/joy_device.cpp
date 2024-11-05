// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "include_dinput.h"
#include "joy_device.h"
#include "joy_ff_effects.h"
#include <drv/hid/dag_hiDInput.h>
#include <drv/hid/dag_hiGlobals.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_mathBase.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <perfmon/dag_autofuncprof.h>
#include <stdio.h>

using namespace HumanInput;

#define VIRTUAL_THUMBKEY_PRESS_THRESHOLD 0.5f
static void decodeDataPS3(const DIJOYSTATE2 &is, JoystickRawState &os);
static void setupButtonNamesPS3(dag::Span<String> btn);
static void setupButtonNamesPS4(dag::Span<String> btn);
static void decodeDataPS4(const DIJOYSTATE2 &is, JoystickRawState &os);
static void decodeDataDef(const DIJOYSTATE2 &is, JoystickRawState &os);
static void setupButtonNamesDef(dag::Span<String> btn);

Di8JoystickDevice::Di8JoystickDevice(IDirectInputDevice8 *dev, bool must_poll, bool remap_360, bool quiet, unsigned *guid) :
  buttons(inimem), axes(inimem), povHats(midmem), fxList(midmem), remapAsX360(remap_360)
{
  if (guid[1] == 0 && guid[2] == 0x49500000 && guid[3] == 0x44495644)
    SNPRINTF(devID, sizeof(devID), "%04X:%04X", guid[0] & 0xFFFF, guid[0] >> 16);
  else
    SNPRINTF(devID, sizeof(devID), "%08X:%08X%08X%08X", guid[0], guid[1], guid[2], guid[3]);

  device = dev;
  client = NULL;
  mustPoll = must_poll;
  memset(&state, 0, sizeof(state));
  decodeData = NULL;
  if (!quiet)
    DEBUG_CTX("added joystick: dev=%p mustPoll=%d", device, mustPoll);
}

static void safe_release_device(IDirectInputDevice8 *dev)
{
#ifndef SEH_DISABLED
  __try
  {
    dev->Release();
  }
  __except (EXCEPTION_CONTINUE_EXECUTION)
  {
    __noop;
  }
#else
  dev->Release();
#endif
}

Di8JoystickDevice::~Di8JoystickDevice()
{
  setClient(NULL);

  //== Attention: all fx are destroyed together with device
  for (int i = fxList.size() - 1; i >= 0; i--)
    if (fxList[i])
      fxList[i]->destroy();
  clear_and_shrink(fxList);

  if (device)
  {
    // Workaround again faulty joystick driver for Speed-Link Vibration Joystick
    // http://crash-stats.so.gaijinent.com/report/index/5a503e4d-5442-4b4a-b439-ab5482120411
    safe_release_device(device);
  }
  device = NULL;
}

bool Di8JoystickDevice::isEqual(Di8JoystickDevice *dev)
{
  if (!device || !dev->device)
    return false;

  DIDEVICEINSTANCE i1, i2;
  memset(&i1, 0, sizeof(i1));
  i1.dwSize = sizeof(i1);
  memset(&i2, 0, sizeof(i2));
  i2.dwSize = sizeof(i2);
  if (device->GetDeviceInfo(&i1) != DI_OK)
    return false;
  if (dev->device->GetDeviceInfo(&i2) != DI_OK)
    return false;
  return memcmp(&i1, &i2, sizeof(i1)) == 0;
}


void Di8JoystickDevice::setDeviceName(const char *nm)
{
  name = nm;
  for (int l = i_strlen(name) - 1; l >= 0; l--)
    if (strchr(" \t\n\r", name[l]))
      name[l] = 0;
    else
      break;
  name.resize(strlen(name) + 1);
  DEBUG_CTX("joy device name: <%s>", (char *)name);
}


void Di8JoystickDevice::addAxis(AxisType axis_type, int di_type, const char *axis_name)
{
  int l = append_items(axes, 1);
  axes[l].type = axis_type;
  axes[l].diType = di_type;
  axes[l].name = axis_name;
  axes[l].dv = 1.0 / (JOY_XINPUT_MAX_AXIS_VAL - JOY_XINPUT_MIN_AXIS_VAL);
  axes[l].v0 = -axes[l].dv * JOY_XINPUT_MIN_AXIS_VAL;
  switch (axis_type)
  {
    case JOY_AXIS_X: axes[l].valStorage = &state.x; break;
    case JOY_AXIS_Y: axes[l].valStorage = &state.y; break;
    case JOY_AXIS_Z: axes[l].valStorage = &state.z; break;
    case JOY_AXIS_RX: axes[l].valStorage = &state.rx; break;
    case JOY_AXIS_RY: axes[l].valStorage = &state.ry; break;
    case JOY_AXIS_RZ: axes[l].valStorage = &state.rz; break;

    case JOY_SLIDER0:
    case JOY_SLIDER1:
      if (di_type == 0)
      {
        int *p = &state.slider[0];
        for (int i = 0; i < (int)axes.size() - 1; i++)
          if (axes[i].valStorage == p)
          {
            p++;
            i = -1;
          }
        int s_idx = p - &state.slider[0];
        if (s_idx < 2)
        {
          axes[l].diType = DIJOFS_SLIDER(s_idx);
          axis_type = axes[l].type = (AxisType)(JOY_SLIDER0 + s_idx);
        }
        else
        {
          axes.pop_back();
          logerr("cannot map axis: %d, <%s>", (int)axis_type, axis_name);
          return;
        }
      }
      axes[l].valStorage = &state.slider[axis_type - JOY_SLIDER0];
      break;

    default: axes[l].valStorage = NULL;
  }

  DEBUG_CTX("add axis: %d, <%s>", (int)axis_type, axis_name);
}


void Di8JoystickDevice::addButton(const char *button_name)
{
  if (remapAsX360)
    return;
  if (buttons.size() >= MAX_BUTTONS)
  {
    DEBUG_CTX("buttons maximum (%d) is already reached; button <%s> skipped", MAX_BUTTONS, button_name);
    return;
  }
  buttons.push_back(String(button_name));
  DEBUG_CTX("add button: <%s>", button_name);
}


void Di8JoystickDevice::addPovHat(const char *hat_name)
{
  if (remapAsX360)
    return;
  if (povHats.size() >= JoystickRawState::MAX_POV_HATS)
  {
    LOGERR_CTX("maximum joystick pov hats count (%d) reached, hat <%s> skipped", JoystickRawState::MAX_POV_HATS, hat_name);
    return;
  }
  PovHatData &phd = povHats.push_back();
  phd.name = hat_name;
  for (int i = 0; i < 2; i++)
    phd.axisName[i].printf(i_strlen(hat_name) + 4, "%s%d", hat_name, i ? "_V" : "_H");
}


void Di8JoystickDevice::addFx(IGenJoyFfEffect *fx)
{
  if (isJoyFxEnabled())
  {
    for (int i = fxList.size() - 1; i >= 0; i--)
      if (!fxList[i])
      {
        fxList[i] = fx;
        return;
      }
    fxList.push_back(fx);
  }
}


void Di8JoystickDevice::delFx(IGenJoyFfEffect *fx)
{
  for (int i = fxList.size() - 1; i >= 0; i--)
    if (fxList[i] == fx)
    {
      fxList[i] = NULL;
      return;
    }
}


bool Di8JoystickDevice::processAxes(JoyCookedCreateParams &dest, const JoyFfCreateParams &src)
{
  int i;
  if (src.axisNum < 1 || src.axisNum > 3)
  {
    DEBUG_CTX("invalid axis num: %d", src.axisNum);
    return false;
  }

  switch (src.coordType)
  {
    case JFF_COORD_TYPE_Cartesian:
      dest.type = DIEFF_CARTESIAN;
      for (i = 0; i < src.axisNum; i++)
        dest.dir[i] = src.dir[i] * 10000;
      break;
    case JFF_COORD_TYPE_Polar:
      dest.type = DIEFF_POLAR;
      dest.dir[0] = src.dir[0] * 180 / PI * DI_DEGREES;
      dest.dir[1] = 0;
      break;
    case JFF_COORD_TYPE_Sperical:
      dest.type = DIEFF_SPHERICAL;
      for (i = 0; i < src.axisNum; i++)
        dest.dir[i] = src.dir[i] * 180 / PI * DI_DEGREES;
      break;
    default: DEBUG_CTX("invalid coord type: %d", src.coordType); return false;
  }
  dest.axisNum = src.axisNum;

  for (i = 0; i < src.axisNum; i++)
    if (src.axisId[i] >= axes.size())
    {
      DEBUG_CTX("invalid axisId[%d]=%d", i, src.axisId[i]);
      return false;
    }
    else
      dest.axisId[i] = axes[src.axisId[i]].diType;

  if (src.dur > 0)
    dest.dur = src.dur * 1000000;
  else
    dest.dur = INFINITE;

  return true;
}

void Di8JoystickDevice::applyRemapX360(const char *product_name)
{
  axes.resize(6);
  axes[0].valStorage = &state.x;
  axes[0].name = "L.Thumb.h";
  axes[1].valStorage = &state.y;
  axes[1].name = "L.Thumb.v";
  axes[2].valStorage = &state.rx;
  axes[2].name = "R.Thumb.h";
  axes[3].valStorage = &state.ry;
  axes[3].name = "R.Thumb.v";
  axes[4].valStorage = &state.slider[0];
  axes[4].name = "L.Trigger";
  axes[5].valStorage = &state.slider[1];
  axes[5].name = "R.Trigger";
  for (int i = 0; i < axes.size(); i++)
  {
    axes[i].dv = 1.0f;
    axes[i].v0 = 0.0f;
  }

  buttons.resize(26);
  buttons[0] = "D.Up";
  buttons[1] = "D.Down";
  buttons[2] = "D.Left";
  buttons[3] = "D.Right";
  buttons[6] = "LS";
  buttons[7] = "RS";
  buttons[18] = "LS.Right";
  buttons[19] = "LS.Left";
  buttons[20] = "LS.Up";
  buttons[21] = "LS.Down";
  buttons[22] = "RS.Right";
  buttons[23] = "RS.Left";
  buttons[24] = "RS.Up";
  buttons[25] = "RS.Down";

  String productName(product_name);
  strlwr(productName);
  String deviceId(getDeviceID());
  strlwr(deviceId);
  if (strstr(productName, "playstation"))
  {
    setupButtonNamesPS3(make_span(buttons));
    decodeData = &decodeDataPS3;
  }
  else if (strstr(deviceId, "054c:05c4") || strstr(deviceId, "054c:09cc")) // DualShock 4 || DualShock 4 (gen2)
  {
    setupButtonNamesPS4(make_span(buttons));
    decodeData = &decodeDataPS4;
  }
  else
  {
    setupButtonNamesDef(make_span(buttons));
    decodeData = &decodeDataDef; // default decoder
  }
}

void Di8JoystickDevice::applyRemapHats()
{
  for (int i = 0; i < povHats.size(); i++)
    for (int j = 0; j < 4; j++)
    {
      buttons.push_back(String(128, "PovHat%d_%d", i, j));
    }
}

void Di8JoystickDevice::acquire()
{
  if (device)
  {
    AutoFuncProfT<AFP_MSEC, 100> _prof("[DINPUT] acquire() %d msec\n");

    HRESULT hr;
    hr = device->Acquire();
    if (hr == S_FALSE)
      return;
    if (FAILED(hr))
    {
      // HumanInput::printHResult(__FILE__, __LINE__, "Acquire", hr);
      return;
    }

    // Dront: this is a dirty hack for users that have significant slowdown because of this log
    static int timesReacquired = 0;
    const int max_times_to_show_this_debug = 50;

    if ((++timesReacquired < max_times_to_show_this_debug) || !(timesReacquired % max_times_to_show_this_debug))
      DEBUG_CTX("reacqired device %p, restoring effects", this);
    for (int i = fxList.size() - 1; i >= 0; i--)
      if (fxList[i])
        fxList[i]->restore();
  }
}


void Di8JoystickDevice::unacquire()
{
  if (device)
    device->Unacquire();
}


void Di8JoystickDevice::poll()
{
  if (device && mustPoll)
  {
    HRESULT hr = device->Poll();
    if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
    {
      Di8JoystickDevice::acquire();
      device->Poll();
    }
  }
}


bool Di8JoystickDevice::updateState(int dt_msec, bool def)
{
  DIJOYSTATE2 js;
  HRESULT hr;
  bool changed = false;

  hr = device->GetDeviceState(sizeof(DIJOYSTATE2), &js);
  if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
  {
    Di8JoystickDevice::acquire();
    hr = device->GetDeviceState(sizeof(DIJOYSTATE2), &js);
  }
  if (SUCCEEDED(hr))
  {
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
    // DEBUG_CTX("event recv");

    static constexpr int DATA_SZ = 14 * sizeof(int);
    char prev_state[DATA_SZ];
    memcpy(prev_state, &state, DATA_SZ);

    if (!remapAsX360)
    {
      state.x = js.lX;
      state.y = js.lY;
      state.z = js.lZ;
      state.rx = js.lRx;
      state.ry = js.lRy;
      state.rz = js.lRz;
      state.slider[0] = js.rglSlider[0];
      state.slider[1] = js.rglSlider[1];
      state.buttonsPrev = state.buttons;

      state.buttons.reset();
      for (unsigned i = 0; i < countof(js.rgbButtons) && i < MAX_BUTTONS; i++)
        if (js.rgbButtons[i] & 0x80)
          state.buttons.set(i);

      for (int i = 0, bit = buttons.size() - 4 * povHats.size(); i < countof(js.rgdwPOV) && i < JoystickRawState::MAX_POV_HATS;
           i++, bit += 4)
      {
        unsigned v = js.rgdwPOV[i];
        if ((v & 0xFFFF) != 0xFFFF)
        {
          if ((v <= 4500) || (v >= 31500))
            state.buttons.set(bit);
          if ((v >= 4500) && (v <= 13500))
            state.buttons.set(bit + 1);
          if ((v >= 13500) && (v <= 22500))
            state.buttons.set(bit + 2);
          if ((v >= 22500) && (v <= 31500))
            state.buttons.set(bit + 3);
        }
      }

      for (int i = 0; i < countof(js.rgdwPOV) && i < JoystickRawState::MAX_POV_HATS; i++)
      {
        state.povValuesPrev[i] = state.povValues[i];
        unsigned v = js.rgdwPOV[i];
        if ((v & 0xFFFF) == 0xFFFF)
          state.povValues[i] = -1;
        else
          state.povValues[i] = int(v);
        changed |= (state.povValuesPrev[i] != state.povValues[i]);
      }
    }
    else
    {
      state.buttonsPrev = state.buttons;
      decodeData(js, state);

      unsigned v = js.rgdwPOV[0];
      if ((v & 0xFFFF) != 0xFFFF)
      {
        if (v <= 4500 || v >= 31500)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_D_UP);
        if (v >= 4500 && v <= 13500)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_D_RIGHT);
        if (v >= 13500 && v <= 22500)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_D_DOWN);
        if (v >= 22500 && v <= 31500)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_D_LEFT);
      }

      // Virtual keys
      if (!stg_joy.disableVirtualControls)
      {
        if (state.x > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_RIGHT);
        else if (state.x < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_LEFT);

        if (state.y > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_UP);
        else if (state.y < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_L_THUMB_DOWN);

        if (state.rx > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_THUMB_RIGHT);
        else if (state.rx < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_THUMB_LEFT);

        if (state.ry > MAXSHORT * VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_THUMB_UP);
        else if (state.ry < MAXSHORT * -VIRTUAL_THUMBKEY_PRESS_THRESHOLD)
          state.buttons.orWord0(JOY_XINPUT_REAL_MASK_R_THUMB_DOWN);
      }
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
    // DEBUG_CTX("%d, %d, %d  %d, %d, %d, %d, %d",
    //   state.x, state.y, state.z, state.rx, state.ry, state.rz, state.slider[0], state.slider[1]);
  }

  return changed && !state.buttons.cmpEq(state.buttonsPrev);
}

void Di8JoystickDevice::setClient(IGenJoystickClient *cli)
{
  if (cli == client)
    return;

  if (client)
    client->detached(this);
  client = cli;
  if (client)
    client->attached(this);
}


IGenJoystickClient *Di8JoystickDevice::getClient() const { return client; }


int Di8JoystickDevice::getButtonCount() const { return buttons.size(); }


const char *Di8JoystickDevice::getButtonName(int idx) const
{
  if (idx >= 0 && idx < buttons.size())
    return buttons[idx];
  return NULL;
}


int Di8JoystickDevice::getAxisCount() const { return axes.size() + 2 * povHats.size(); }


const char *Di8JoystickDevice::getAxisName(int idx) const
{
  const int ac = axes.size();
  if (idx >= 0 && idx < ac)
    return axes[idx].name;
  if (idx >= ac && idx < ac + 2 * povHats.size())
    return povHats[(idx - ac) / 2].axisName[(idx - ac) % 2];
  return NULL;
}


int Di8JoystickDevice::getPovHatCount() const { return povHats.size(); }

const char *Di8JoystickDevice::getPovHatName(int idx) const
{
  if (idx >= 0 && idx < povHats.size())
    return povHats[idx].name;
  return NULL;
}

int Di8JoystickDevice::getButtonId(const char *button_name) const
{
  for (int i = 0; i < buttons.size(); i++)
    if (strcmp(buttons[i], button_name) == 0)
      return i;
  return -1;
}


int Di8JoystickDevice::getAxisId(const char *axis_name) const
{
  for (int i = 0; i < axes.size(); i++)
    if (strcmp(axes[i].name, axis_name) == 0)
      return i;
  for (int i = 0; i < povHats.size(); i++)
    for (int j = 0; j < 2; j++)
      if (strcmp(povHats[i].axisName[j], axis_name) == 0)
        return axes.size() + i * 2 + j;
  return -1;
}


int Di8JoystickDevice::getPovHatId(const char *hat_name) const
{
  for (int i = 0; i < povHats.size(); i++)
    if (strcmp(povHats[i].name, hat_name) == 0)
      return i;
  return -1;
}

void Di8JoystickDevice::setAxisLimits(int axis_id, float min_val, float max_val)
{
  if (axis_id >= 0 && axis_id < axes.size())
  {
    axes[axis_id].dv = (max_val - min_val) / (JOY_XINPUT_MAX_AXIS_VAL - JOY_XINPUT_MIN_AXIS_VAL);
    axes[axis_id].v0 = min_val - axes[axis_id].dv * JOY_XINPUT_MIN_AXIS_VAL;
  }
}


bool Di8JoystickDevice::getButtonState(int btn_id) const { return btn_id >= 0 ? state.buttons.get(btn_id) : false; }


float Di8JoystickDevice::getAxisPos(int axis_id) const
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
  {
    // DEBUG_CTX("axis %d: %d -> %.5f",
    //   axis_id, axes[axis_id].valStorage[0], axes[axis_id].v0 + axes[axis_id].dv * axes[axis_id].valStorage[0]);
    return axes[axis_id].v0 + axes[axis_id].dv * axes[axis_id].valStorage[0];
  }
  if (axis_id >= ac && axis_id < ac + 2 * povHats.size())
    return getVirtualPOVAxis((axis_id - ac) / 2, (axis_id - ac) % 2);
  return 0;
}
int Di8JoystickDevice::getAxisPosRaw(int axis_id) const
{
  const int ac = axes.size();
  if (axis_id >= 0 && axis_id < ac)
    return axes[axis_id].valStorage[0];
  if (axis_id >= ac && axis_id < ac + 2 * povHats.size())
    return getVirtualPOVAxis((axis_id - ac) / 2, (axis_id - ac) % 2);
  return 0;
}

int Di8JoystickDevice::getPovHatAngle(int hat_id) const
{
  if (hat_id >= 0 && hat_id < povHats.size())
  {
    return state.povValues[hat_id];
  }
  return -1;
}

int Di8JoystickDevice::getVirtualPOVAxis(int hat_id, int axis_id) const
{
  if ((axis_id != 0) && (axis_id != 1))
    return 0;
  int value = getPovHatAngle(hat_id);
  if (value >= 0)
  {
    float angle = -(float(value) / 36000.f) * TWOPI + HALFPI;
    float axisVal = (axis_id == 1) ? sin(angle) : cos(angle);
    return int(axisVal * JOY_XINPUT_MAX_AXIS_VAL);
  }
  return 0;
}

//
// effects creation
//
IJoyFfCondition *Di8JoystickDevice::createConditionFx(int condfx_type, const JoyFfCreateParams &cp)
{
  JoyCookedCreateParams ccp;
  if (!processAxes(ccp, cp) || !isJoyFxEnabled())
    return NULL;

  DiJoyConditionFx *fx = new (midmem) DiJoyConditionFx(this);
  if (!fx->create(condfx_type, ccp))
  {
    delete fx;
    return NULL;
  }
  addFx(fx);
  return fx;
}

IJoyFfConstForce *Di8JoystickDevice::createConstForceFx(const JoyFfCreateParams &cp, float force)
{
  JoyCookedCreateParams ccp;
  if (!processAxes(ccp, cp) || !isJoyFxEnabled())
    return NULL;

  DiJoyConstantForceFx *fx = new (midmem) DiJoyConstantForceFx(this);
  if (!fx->create(ccp, force))
  {
    delete fx;
    return NULL;
  }
  addFx(fx);
  return fx;
}

IJoyFfPeriodic *Di8JoystickDevice::createPeriodicFx(int period_type, const JoyFfCreateParams &cp)
{
  JoyCookedCreateParams ccp;
  if (!processAxes(ccp, cp) || !isJoyFxEnabled())
    return NULL;

  DiJoyPeriodicFx *fx = new (midmem) DiJoyPeriodicFx(this);
  if (!fx->create(period_type, ccp))
  {
    delete fx;
    return NULL;
  }
  addFx(fx);
  return fx;
}

IJoyFfRampForce *Di8JoystickDevice::createRampForceFx(const JoyFfCreateParams &cp, float start_f, float end_f)
{
  JoyCookedCreateParams ccp;
  if (!processAxes(ccp, cp) || !isJoyFxEnabled())
    return NULL;

  DiJoyRampForceFx *fx = new (midmem) DiJoyRampForceFx(this);
  if (!fx->create(ccp, start_f, end_f))
  {
    delete fx;
    return NULL;
  }
  addFx(fx);
  return fx;
}

bool Di8JoystickDevice::isConnected()
{
  DIJOYSTATE2 js;
  HRESULT hr;

  hr = device->GetDeviceState(sizeof(DIJOYSTATE2), &js);
  if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
  {
    Di8JoystickDevice::acquire();
    hr = device->GetDeviceState(sizeof(DIJOYSTATE2), &js);
  }

  // In a windowed (or fullscreen) mode if we are switching to the desktop or changing
  // an application focus, the device is not acquired anymore but still is connected.
  // To avoid pointless reconnection process (or possible ui hints about device lost)
  // we should ignore this
  return SUCCEEDED(hr) || hr == DIERR_NOTACQUIRED;
}

#define DEADCLAMP(value, threshold)                        \
  if (((value) < (threshold)) && ((value) > -(threshold))) \
  value = 0

static void decodeDataPS3(const DIJOYSTATE2 &is, JoystickRawState &os)
{
  os.x = is.lX;
  os.y = -is.lY;
  os.z = 0;
  os.rx = is.lZ;
  os.ry = -is.lRz;
  os.rz = 0;
  os.slider[0] = (is.rgbButtons[4] & 0x80) ? 32000 : 0;
  os.slider[1] = (is.rgbButtons[5] & 0x80) ? 32000 : 0;
  DEADCLAMP(os.x, 2500);
  DEADCLAMP(os.y, 2500);
  DEADCLAMP(os.rx, 2500);
  DEADCLAMP(os.ry, 2500);

  static unsigned btn_remap[12] = {JOY_XINPUT_REAL_MASK_Y, JOY_XINPUT_REAL_MASK_B, JOY_XINPUT_REAL_MASK_A, JOY_XINPUT_REAL_MASK_X,
    JOY_XINPUT_REAL_MASK_L_TRIGGER, JOY_XINPUT_REAL_MASK_R_TRIGGER, JOY_XINPUT_REAL_MASK_L_SHOULDER, JOY_XINPUT_REAL_MASK_R_SHOULDER,
    JOY_XINPUT_REAL_MASK_START, JOY_XINPUT_REAL_MASK_BACK, JOY_XINPUT_REAL_MASK_L_THUMB, JOY_XINPUT_REAL_MASK_R_THUMB};

  os.buttons.reset();
  for (int i = 0; i < 12; i++)
    if (is.rgbButtons[i] & 0x80)
      os.buttons.orWord0(btn_remap[i]);
}
static void setupButtonNamesPS3(dag::Span<String> btn)
{
  static struct
  {
    int btn;
    const char *name;
  } btn_remap[10] = {{JOY_XINPUT_REAL_BTN_A, "X"}, {JOY_XINPUT_REAL_BTN_B, "O"}, {JOY_XINPUT_REAL_BTN_X, "Quad"},
    {JOY_XINPUT_REAL_BTN_Y, "Tri"}, {JOY_XINPUT_REAL_BTN_L_SHOULDER, "L1"}, {JOY_XINPUT_REAL_BTN_R_SHOULDER, "R1"},
    {JOY_XINPUT_REAL_BTN_L_TRIGGER, "L2"}, {JOY_XINPUT_REAL_BTN_R_TRIGGER, "R2"}, {JOY_XINPUT_REAL_BTN_BACK, "Select"},
    {JOY_XINPUT_REAL_BTN_START, "Start"}};
  for (int i = 0; i < 10; i++)
    btn[btn_remap[i].btn] = btn_remap[i].name;
}

static void setupButtonNamesPS4(dag::Span<String> btn)
{
  static struct
  {
    int btn;
    const char *name;
  } btn_remap[10] = {{JOY_XINPUT_REAL_BTN_A, "X"}, {JOY_XINPUT_REAL_BTN_B, "O"}, {JOY_XINPUT_REAL_BTN_X, "Quad"},
    {JOY_XINPUT_REAL_BTN_Y, "Tri"}, {JOY_XINPUT_REAL_BTN_L_SHOULDER, "L1"}, {JOY_XINPUT_REAL_BTN_R_SHOULDER, "R1"},
    {JOY_XINPUT_REAL_BTN_L_TRIGGER, "L2"}, {JOY_XINPUT_REAL_BTN_R_TRIGGER, "R2"}, {JOY_XINPUT_REAL_BTN_BACK, "Share"},
    {JOY_XINPUT_REAL_BTN_START, "Options"}};
  for (int i = 0; i < 10; i++)
    btn[btn_remap[i].btn] = btn_remap[i].name;
}
static void decodeDataPS4(const DIJOYSTATE2 &is, JoystickRawState &os)
{
  os.x = is.lX;
  os.y = -is.lY;
  os.z = 0;
  os.rx = is.lZ;
  os.ry = -is.lRz;
  os.rz = 0;

  os.slider[0] = (is.lRx + 32000) / 2;
  os.slider[1] = (is.lRy + 32000) / 2;
  DEADCLAMP(os.x, 2500);
  DEADCLAMP(os.y, 2500);
  DEADCLAMP(os.rx, 2500);
  DEADCLAMP(os.ry, 2500);

  static unsigned btn_remap[12] = {JOY_XINPUT_REAL_MASK_Y, JOY_XINPUT_REAL_MASK_A, JOY_XINPUT_REAL_MASK_B, JOY_XINPUT_REAL_MASK_X,
    JOY_XINPUT_REAL_MASK_L_SHOULDER, JOY_XINPUT_REAL_MASK_R_SHOULDER, JOY_XINPUT_REAL_MASK_L_TRIGGER, JOY_XINPUT_REAL_MASK_R_TRIGGER,
    JOY_XINPUT_REAL_MASK_BACK, JOY_XINPUT_REAL_MASK_START, JOY_XINPUT_REAL_MASK_L_THUMB, JOY_XINPUT_REAL_MASK_R_THUMB};

  os.buttons.reset();
  for (int i = 0; i < 12; i++)
    if (is.rgbButtons[i] & 0x80)
      os.buttons.orWord0(btn_remap[i]);
}

static void decodeDataDef(const DIJOYSTATE2 &is, JoystickRawState &os)
{
  os.x = is.lX;
  os.y = -is.lY;
  os.z = 0;
  os.rx = is.lZ;
  os.ry = -is.lRz;
  os.rz = 0;
  os.slider[0] = is.rglSlider[0];
  os.slider[1] = is.rglSlider[1];
  DEADCLAMP(os.x, 2500);
  DEADCLAMP(os.y, 2500);
  DEADCLAMP(os.rx, 2500);
  DEADCLAMP(os.ry, 2500);

  static unsigned btn_remap[12] = {JOY_XINPUT_REAL_MASK_X, JOY_XINPUT_REAL_MASK_A, JOY_XINPUT_REAL_MASK_B, JOY_XINPUT_REAL_MASK_Y,
    JOY_XINPUT_REAL_MASK_L_SHOULDER, JOY_XINPUT_REAL_MASK_R_SHOULDER, JOY_XINPUT_REAL_MASK_L_TRIGGER, JOY_XINPUT_REAL_MASK_R_TRIGGER,
    JOY_XINPUT_REAL_MASK_BACK, JOY_XINPUT_REAL_MASK_START, JOY_XINPUT_REAL_MASK_L_THUMB, JOY_XINPUT_REAL_MASK_R_THUMB};

  os.buttons.reset();
  for (int i = 0; i < 12; i++)
    if (is.rgbButtons[i] & 0x80)
      os.buttons.orWord0(btn_remap[i]);
}
static void setupButtonNamesDef(dag::Span<String> btn)
{
  static int btn_remap[12] = {
    JOY_XINPUT_REAL_BTN_X,
    JOY_XINPUT_REAL_BTN_A,
    JOY_XINPUT_REAL_BTN_B,
    JOY_XINPUT_REAL_BTN_Y,
    JOY_XINPUT_REAL_BTN_L_SHOULDER,
    JOY_XINPUT_REAL_BTN_R_SHOULDER,
    JOY_XINPUT_REAL_BTN_L_TRIGGER,
    JOY_XINPUT_REAL_BTN_R_TRIGGER,
    JOY_XINPUT_REAL_BTN_BACK,
    JOY_XINPUT_REAL_BTN_START,
    -1,
    -1,
  };
  for (int i = 0; i < 12; i++)
    if (btn_remap[i] >= 0)
      btn[btn_remap[i]].printf(8, "B%d", i + 1);
}

#undef DEADCLAMP

int Di8JoystickDevice::getPairedButtonForAxis(int axis_idx) const
{
  if (!remapAsX360)
    return -1;
  if (axis_idx == JOY_XINPUT_REAL_AXIS_L_TRIGGER)
    return JOY_XINPUT_REAL_BTN_L_TRIGGER;
  if (axis_idx == JOY_XINPUT_REAL_AXIS_R_TRIGGER)
    return JOY_XINPUT_REAL_BTN_R_TRIGGER;
  return -1;
}
int Di8JoystickDevice::getPairedAxisForButton(int btn_idx) const
{
  if (!remapAsX360)
    return -1;
  if (btn_idx == JOY_XINPUT_REAL_BTN_L_TRIGGER)
    return JOY_XINPUT_REAL_AXIS_L_TRIGGER;
  if (btn_idx == JOY_XINPUT_REAL_BTN_R_TRIGGER)
    return JOY_XINPUT_REAL_AXIS_R_TRIGGER;
  return -1;
}
int Di8JoystickDevice::getJointAxisForAxis(int axis_idx) const
{
  if (!remapAsX360)
    return -1;
  switch (axis_idx)
  {
    case JOY_XINPUT_REAL_AXIS_L_THUMB_H: return JOY_XINPUT_REAL_AXIS_L_THUMB_V;
    case JOY_XINPUT_REAL_AXIS_L_THUMB_V: return JOY_XINPUT_REAL_AXIS_L_THUMB_H;
    case JOY_XINPUT_REAL_AXIS_R_THUMB_H: return JOY_XINPUT_REAL_AXIS_R_THUMB_V;
    case JOY_XINPUT_REAL_AXIS_R_THUMB_V: return JOY_XINPUT_REAL_AXIS_R_THUMB_H;
  }
  return -1;
}
