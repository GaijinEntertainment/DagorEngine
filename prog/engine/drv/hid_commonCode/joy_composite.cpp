// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiComposite.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiJoyFF.h>
#include <drv/hid/dag_hiCreate.h>
#include <startup/dag_inpDevClsDrv.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <string.h>
#include <stdlib.h>

#include <math/random/dag_random.h>
#include <math/dag_mathBase.h>
#include <stdio.h>

#if _TARGET_PC_WIN
#include <supp/_platform.h>
#include <xinput.h>
#endif
#include <util/dag_localization.h>
#include <util/dag_string.h>
#include "composite_loc.h"
#include <osApiWrappers/dag_miscApi.h>

HumanInput::CompositeJoystickClassDriver *global_cls_composite_drv_joy = NULL;
#if _TARGET_PC
static HumanInput::OnShowIME cb_show_ime_steam = NULL;
#endif

using namespace HumanInput;

#if _TARGET_XBOX
static inline bool should_use_native_names(HumanInput::Device &) { return true; }
#else
static inline bool should_use_native_names(HumanInput::Device &device) { return device.isXInput; }
#endif

void HumanInput::enableJoyFx(bool isEnabled)
{

  if (global_cls_drv_joy)
    for (int i = 0; i < global_cls_drv_joy->getDeviceCount(); ++i)
    {
      HumanInput::IGenJoystick *joy = ::global_cls_drv_joy->getDevice(i);
      joy->enableJoyFx(isEnabled);
    }
}

class HumanInput::CompositeJoystickDevice : public IGenJoystick
{
public:
  struct DeviceTmp
  {
    HumanInput::Device dev;
    int stBtn, stAxis, stPov;
  };
  enum
  {
    MAX_NAMED_BTN = JoystickRawState::MAX_BUTTONS,
    MAX_NAMED_AXIS = 64,
    MAX_NAMED_POV = JoystickRawState::MAX_POV_HATS,
  };

  CompositeJoystickDevice() : devices(midmem), client(NULL)
  {
    btnNum = axesNum = povsNum = 0;
    mem_set_0(btnNames);
    constexpr size_t name_buffer_size = 16;
    char name[name_buffer_size];
    for (int i = 0; i < btnNames.size(); i++)
    {
      SNPRINTF(name, name_buffer_size, "Button%d", i + 1);
      btnNames[i] = str_dup(name, strmem);
    }
    mem_set_0(axisNames);
    for (int i = 0; i < axisNames.size(); i++)
    {
      SNPRINTF(name, name_buffer_size, "Axis%d", i + 1);
      axisNames[i] = str_dup(name, strmem);
    }
    mem_set_0(povNames);
    for (int i = 0; i < povNames.size(); i++)
    {
      SNPRINTF(name, name_buffer_size, "POV%d", i + 1);
      povNames[i] = str_dup(name, strmem);
    }
    memset(&state, 0, sizeof(state));
    rumbleDevIdx = -1;
  }

  ~CompositeJoystickDevice()
  {
    for (int i = 0; i < btnNames.size(); i++)
      strmem->free(btnNames[i]);
    for (int i = 0; i < axisNames.size(); i++)
      strmem->free(axisNames[i]);
    for (int i = 0; i < povNames.size(); i++)
      strmem->free(povNames[i]);
  }

  void updateState(int dt_msec)
  {
    enum
    {
      DATA_SZ = offsetof(JoystickRawState, repeat)
    };
    char prevState[DATA_SZ];
    memcpy(prevState, &state, DATA_SZ);
    bool changed = false;

    state.buttonsPrev = state.buttons;
    for (int j = 0; j < JoystickRawState::MAX_POV_HATS; j++)
    {
      state.povValuesPrev[j] = state.povValues[j];
      state.povValues[j] = -1;
    }

    state.buttons.reset();
    state.x = state.y = state.z = 0;
    state.rx = state.ry = state.rz = 0;
    state.slider[0] = state.slider[1] = 0;
    state.sensorX = state.sensorY = state.sensorZ = state.sensorG = 0.f;
    state.rawSensX = state.rawSensY = state.rawSensZ = state.rawSensG = 0;

    for (int i = 0; i < devices.size(); i++)
    {
      int mapToBtnOfs = devices[i].ofsButtons;
      int mapToPovOfs = devices[i].ofsPovs;

      if (devices[i].dev)
      {
        const JoystickRawState &st = devices[i].dev->getRawState();
        for (int j = 0, inc = 0, k = mapToBtnOfs; j < devices[i].numButtons; j += inc, k += inc)
          if (k < st.buttons.SZ)
          {
            if (st.buttons.getIter(j, inc))
              state.buttons.set(k);
          }
          else
            break;

        for (int j = 0; (j < devices[i].dev->getPovHatCount()) && (j + mapToPovOfs < JoystickRawState::MAX_POV_HATS); j++)
          state.povValues[j + mapToPovOfs] = st.povValues[j];

        state.x += st.x;
        state.y += st.y;
        state.z += st.z;
        state.rx += st.rx;
        state.ry += st.ry;
        state.rz += st.rz;
        state.slider[0] += st.slider[0];
        state.slider[1] += st.slider[1];
#define _UPDATE_SENSOR(SENSOR)  \
  if (float_nonzero(st.SENSOR)) \
    state.SENSOR += st.SENSOR;
        _UPDATE_SENSOR(sensorX)
        _UPDATE_SENSOR(sensorY)
        _UPDATE_SENSOR(sensorZ)
        _UPDATE_SENSOR(sensorG)
#undef _UPDATE_SENSOR
        state.rawSensX += st.rawSensX;
        state.rawSensY += st.rawSensY;
        state.rawSensZ += st.rawSensZ;
        state.rawSensG += st.rawSensG;
      }
    }

    if (!state.buttons.cmpEq(state.buttonsPrev))
    {
      changed = true;
      for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
        if (state.buttons.getIter(i, inc))
          if (!state.buttonsPrev.get(i))
            state.bDownTms[i] = 0;
    }

    if (!changed)
      changed = memcmp(prevState, &state, DATA_SZ) != 0;

    for (int i = 0, inc = 0; i < state.MAX_BUTTONS; i += inc)
      if (state.buttonsPrev.getIter(i, inc))
        state.bDownTms[i] += dt_msec;

    if (changed)
      raw_state_joy = state;
    else
      memcpy(raw_state_joy.bDownTms, state.bDownTms, sizeof(raw_state_joy.bDownTms));

    if (client && changed)
      client->stateChanged(this, 0);
  }

  virtual const char *getName() const { return "CompositeJoystick"; }
  virtual const char *getDeviceID() const { return "composite"; }
  virtual bool getDeviceDesc(DataBlock &) const { return false; }
  virtual unsigned short getUserId() const { return 0; }
  virtual const JoystickRawState &getRawState() const { return state; }
  virtual void setClient(IGenJoystickClient *cli)
  {
    if (cli == client)
      return;
    if (client)
      client->detached(this);
    client = cli;
    if (client)
      client->attached(this);
  }

  virtual IGenJoystickClient *getClient() const { return client; }

  virtual bool isStickDeadZoneSetupSupported() const { return true; }
  virtual float getStickDeadZoneScale(int /*stick_idx*/) const { return 0; }
  virtual void setStickDeadZoneScale(int /*stick_idx*/, float /*scale*/) {}
  virtual float getStickDeadZoneAbs(int /*stick_idx*/) const { return 0; }

  int getPairedButtonForAxis(int axis_id) const override
  {
    for (int i = 0; i < devices.size(); i++)
      if (axis_id >= devices[i].ofsAxes && axis_id < devices[i].ofsAxes + devices[i].numAxes)
        if (devices[i].dev)
        {
          int idx = devices[i].dev->getPairedButtonForAxis(axis_id - devices[i].ofsAxes);
          return idx < 0 ? -1 : idx + devices[i].ofsButtons;
        }
    return -1;
  }
  int getPairedAxisForButton(int btn_id) const override
  {
    for (int i = 0; i < devices.size(); i++)
      if (btn_id >= devices[i].ofsButtons && btn_id < devices[i].ofsButtons + devices[i].numButtons)
        if (devices[i].dev)
        {
          int idx = devices[i].dev->getPairedAxisForButton(btn_id - devices[i].ofsButtons);
          return idx < 0 ? -1 : idx + devices[i].ofsAxes;
        }
    return -1;
  }

  int getJointAxisForAxis(int axis_id) const override
  {
    for (int i = 0; i < devices.size(); i++)
      if (axis_id >= devices[i].ofsAxes && axis_id < devices[i].ofsAxes + devices[i].numAxes)
        if (devices[i].dev)
        {
          int idx = devices[i].dev->getJointAxisForAxis(axis_id - devices[i].ofsAxes);
          return idx < 0 ? -1 : idx + devices[i].ofsAxes;
        }
    return -1;
  }

#define FIND_NATIVE_NAME(IDX, FIELD, GET_NAME)                                               \
  if (IDX < 0)                                                                               \
    return NULL;                                                                             \
  for (int i = 0; i < devices.size(); i++)                                                   \
    if (IDX >= devices[i].ofs##FIELD && IDX < devices[i].ofs##FIELD + devices[i].num##FIELD) \
      return devices[i].dev ? devices[i].dev->GET_NAME(IDX - devices[i].ofs##FIELD) : NULL;  \
  return NULL

  const char *getNativeAxisName(int axis_idx) const { FIND_NATIVE_NAME(axis_idx, Axes, getAxisName); }
  const char *getNativeButtonName(int btn_idx) const { FIND_NATIVE_NAME(btn_idx, Buttons, getButtonName); }
#undef FIND_NATIVE_NAME

  virtual int getButtonCount() const { return btnNum; }
  virtual const char *getButtonName(int idx) const
  {
    if (idx >= 0 && idx < btnNames.size())
      return btnNames[idx];
    return "?";
  }

  virtual int getAxisCount() const { return axesNum; }
  virtual const char *getAxisName(int idx) const
  {
    if (idx >= 0 && idx < axisNames.size())
      return axisNames[idx];
    return "?";
  }

  virtual int getPovHatCount() const { return povsNum; }
  virtual const char *getPovHatName(int idx) const
  {
    if (idx >= 0 && idx < povNames.size())
      return povNames[idx];
    return "?";
  }

  virtual int getButtonId(const char *name) const
  {
    if (!name)
      return -1;

    const int numButtons = btnNames.size();
    for (int i = 0; i < numButtons; i++)
      if (!stricmp(btnNames[i], name))
        return i;
    return -1;
  }

  virtual int getAxisId(const char *name) const
  {
    if (!name)
      return -1;
    const int numAxes = axisNames.size();
    for (int i = 0; i < numAxes; i++)
      if (!stricmp(axisNames[i], name))
        return i;
    return -1;
  }

  virtual int getPovHatId(const char *name) const
  {
    if (!name)
      return -1;
    const int numPovs = povNames.size();
    for (int i = 0; i < numPovs; i++)
      if (!stricmp(povNames[i], name))
        return i;
    return -1;
  }

  virtual void setAxisLimits(int axis_id, float min_val, float max_val)
  {
    for (int i = 0; i < devices.size(); i++)
      if (axis_id >= devices[i].ofsAxes && axis_id < devices[i].ofsAxes + devices[i].numAxes)
        if (devices[i].dev)
          devices[i].dev->setAxisLimits(axis_id - devices[i].ofsAxes, min_val, max_val);
  }

  virtual bool getButtonState(int btn_id) const
  {
    bool btn = false;
    for (int i = 0; i < devices.size(); i++)
      if (btn_id >= devices[i].ofsButtons && btn_id < devices[i].ofsButtons + devices[i].numButtons)
        if (devices[i].dev)
          btn |= devices[i].dev->getButtonState(btn_id - devices[i].ofsButtons);
    return btn;
  }

  virtual float getAxisPos(int axis_id) const
  {
    float val = 0.f;
    for (int i = 0; i < devices.size(); i++)
      if (axis_id >= devices[i].ofsAxes && axis_id < devices[i].ofsAxes + devices[i].numAxes)
        if (devices[i].dev)
          val += devices[i].dev->getAxisPos(axis_id - devices[i].ofsAxes);
    return val;
  }

  virtual int getAxisPosRaw(int axis_id) const
  {
    int val = 0;
    for (int i = 0; i < devices.size(); i++)
      if (axis_id >= devices[i].ofsAxes && axis_id < devices[i].ofsAxes + devices[i].numAxes)
        if (devices[i].dev)
          val += devices[i].dev->getAxisPosRaw(axis_id - devices[i].ofsAxes);
    return val;
  }

  bool isAxisDigital(int axis_id) const
  {
    if (axis_id < 0)
      return false;
    for (int i = 0; i < devices.size(); i++)
      if (axis_id >= devices[i].ofsAxes && axis_id < devices[i].ofsAxes + devices[i].numAxes)
        return (axis_id >= devices[i].ofsAxes + devices[i].numAxes - devices[i].numPovs * 2);
    return false;
  }

  virtual int getPovHatAngle(int axis_id) const
  {
    G_ASSERT(axis_id >= 0 && axis_id < JoystickRawState::MAX_POV_HATS);
    if (axis_id >= 0 && axis_id < JoystickRawState::MAX_POV_HATS)
      return state.povValues[axis_id];
    return -1;
  }

  IGenJoystick *remapAxes(JoyFfCreateParams &cp)
  {
    IGenJoystick *dev = NULL;
    for (int a = 0; a < cp.axisNum; a++)
    {
      int &axis_id = cp.axisId[a];
      // debug_("axis[%d]=%d", a, axis_id);
      for (int i = 0; i < devices.size(); i++)
        if (axis_id >= devices[i].ofsAxes && axis_id < devices[i].ofsAxes + devices[i].numAxes)
        {
          dev = devices[i].dev;
          axis_id -= devices[i].ofsAxes;
          break;
        }
      // debug(" -> %d, dev=%p", axis_id, dev);
    }
    return dev;
  }
  virtual IJoyFfCondition *createConditionFx(int condfx_type, const JoyFfCreateParams &_cp)
  {
#if _TARGET_PC_WIN
    if (!isJoyFxEnabled())
      return NULL;
    JoyFfCreateParams cp = _cp;
    IGenJoystick *dev = remapAxes(cp);
    return dev ? dev->createConditionFx(condfx_type, cp) : NULL;
#else
    G_UNREFERENCED(_cp);
    G_UNREFERENCED(condfx_type);
    return NULL;
#endif
  }
  virtual IJoyFfConstForce *createConstForceFx(const JoyFfCreateParams &_cp, float force)
  {
#if _TARGET_PC_WIN
    if (!isJoyFxEnabled())
      return NULL;
    JoyFfCreateParams cp = _cp;
    IGenJoystick *dev = remapAxes(cp);
    return dev ? dev->createConstForceFx(cp, force) : NULL;
#else
    G_UNREFERENCED(_cp);
    G_UNREFERENCED(force);
    return NULL;
#endif
  }
  virtual IJoyFfPeriodic *createPeriodicFx(int period_type, const JoyFfCreateParams &_cp)
  {
#if _TARGET_PC_WIN
    if (!isJoyFxEnabled())
      return NULL;
    JoyFfCreateParams cp = _cp;
    IGenJoystick *dev = remapAxes(cp);
    return dev ? dev->createPeriodicFx(period_type, cp) : NULL;
#else
    G_UNREFERENCED(_cp);
    G_UNREFERENCED(period_type);
    return NULL;
#endif
  }
  virtual IJoyFfRampForce *createRampForceFx(const JoyFfCreateParams &_cp, float start_f, float end_f)
  {
#if _TARGET_PC_WIN
    if (!isJoyFxEnabled())
      return NULL;
    JoyFfCreateParams cp = _cp;
    IGenJoystick *dev = remapAxes(cp);
    return dev ? dev->createRampForceFx(cp, start_f, end_f) : NULL;
#else
    G_UNREFERENCED(_cp);
    G_UNREFERENCED(start_f);
    G_UNREFERENCED(end_f);
    return NULL;
#endif
  }

  virtual void doRumble(float lowFreq, float highFreq)
  {
    if (rumbleDevIdx >= 0)
    {
      if (rumbleDevIdx < devices.size() && devices[rumbleDevIdx].dev)
        devices[rumbleDevIdx].dev->doRumble(lowFreq, highFreq);
      return;
    }
    for (int i = 0; i < devices.size(); i++)
      if (devices[i].dev)
        devices[i].dev->doRumble(lowFreq, highFreq);
  }
  virtual void setLight(bool set, unsigned char r, unsigned char g, unsigned char b)
  {
    if (rumbleDevIdx >= 0)
    {
      if (rumbleDevIdx < devices.size() && devices[rumbleDevIdx].dev)
        devices[rumbleDevIdx].dev->setLight(set, r, g, b);
      return;
    }
    for (int i = 0; i < devices.size(); i++)
      if (devices[i].dev)
        devices[i].dev->setLight(set, r, g, b);
  }

  void resetSensorsOrientation() override
  {
    for (auto &d : devices)
      if (d.dev)
        d.dev->resetSensorsOrientation();
  }

  void enableSensorsTiltCorrection(bool enable) override
  {
    for (auto &d : devices)
      if (d.dev)
        d.dev->enableSensorsTiltCorrection(enable);
  }

  void reportGyroSensorsAngDelta(bool report_ang_delta) override
  {
    for (auto &d : devices)
      if (d.dev)
        d.dev->reportGyroSensorsAngDelta(report_ang_delta);
  }

  virtual bool isConnected()
  {
    for (int i = 0; i < devices.size(); ++i)
    {
      if (devices[i].dev == NULL)
        continue;
      if (devices[i].dev->isConnected())
        return true;
    }
    return false;
  }

  void clearDevices()
  {
    for (int i = 0; i < devices.size(); i++)
      devices[i].dev = NULL;
    btnNum = axesNum = povsNum = 0;
    firstX360Index = -1;
  }

  void resetDevices() { devices.clear(); }

  void addDevice(IGenJoystick *dev, bool is_xinput)
  {
    if (!dev)
      return;

    int index = -1;
    {
      const char *devID = dev->getDeviceID();
      for (int i = 0; i < devices.size(); i++)
        if (!devices[i].dev && (stricmp(devices[i].devID, devID) == 0 || (devices[i].isXInput && is_xinput)))
        {
          index = i;
          break;
        }
    }

    if (index < 0 || index >= devices.size())
      index = append_items(devices, 1);

    // required for XR-112
    if (firstX360Index < 0 && stricmp(dev->getDeviceID(), "x360") == 0)
      firstX360Index = index;
    if (index == firstX360Index || stricmp(dev->getDeviceID(), "x360") != 0)
    {
      // append controls
      devices[index].ofsButtons = btnNum;
      devices[index].ofsPovs = povsNum;
      devices[index].ofsAxes = axesNum;
      btnNum += dev->getButtonCount();
      povsNum += dev->getPovHatCount();
      axesNum += dev->getAxisCount();
    }
    else
    {
      // refer to existing x360
      devices[index].ofsButtons = devices[firstX360Index].ofsButtons;
      devices[index].ofsPovs = devices[firstX360Index].ofsPovs;
      devices[index].ofsAxes = devices[firstX360Index].ofsAxes;
    }

    devices[index].dev = dev;
    devices[index].name = dev->getName();
    devices[index].devID = dev->getDeviceID();

    devices[index].isXInput = is_xinput;
    devices[index].numButtons = dev->getButtonCount();
    devices[index].numPovs = dev->getPovHatCount();
    devices[index].numAxes = dev->getAxisCount();

    /*
    debug("composite.reg dev=%p (%s, %s) as %d, ofs=(%d,%d,%d) num=(%d,%d,%d)",
      dev, devices[index].name, devices[index].devID, index,
      devices[index].ofsButtons, devices[index].ofsAxes, devices[index].ofsPovs,
      devices[index].numButtons, devices[index].numAxes, devices[index].numPovs);
    */
  }
  void setupControlNames(int index)
  {
    IGenJoystick *dev = devices[index].dev;
    if (!dev)
      return;
    bool is_xinput = devices[index].isXInput;

    String loc_dev_name_stor;
    const char *dev_name = devices[index].name;
    if (localize_composite_joy_dev(dev_name, loc_dev_name_stor, global_cls_composite_drv_joy->getLocClient()))
      dev_name = loc_dev_name_stor;

    if (index != firstX360Index && dev && stricmp(dev->getDeviceID(), "x360") == 0)
      return; // required for XR-112

    bool is_first_xinput = is_xinput;
    for (int i = 0; i < index; i++)
      if (devices[index].isXInput)
        is_first_xinput = false;

    String locName, name, fullName;
    int b_base = devices[index].ofsButtons;
    int a_base = devices[index].ofsAxes;
    int p_base = devices[index].ofsPovs;
    for (int j = 0; j < devices[index].numButtons; j++)
      if (b_base + j < btnNames.size())
      {
        if (should_use_native_names(devices[index]))
          name = dev->getButtonName(j);
        else
          name.printf(0, "Button%d", j + 1);

        if (localize_composite_joy_elem(name, locName, global_cls_composite_drv_joy->getLocClient(),
              ICompositeLocalization::LOC_TYPE_BUTTON))
          name = locName;

        if (is_first_xinput || devices.size() == 1)
          fullName = name;
        else
          fullName.printf(0, "C%d:%s", index + 1, name.str());

        strmem->free(btnNames[b_base + j]);
        btnNames[b_base + j] = str_dup(fullName.str(), strmem);
      }
    for (int j = 0; j < devices[index].numAxes; j++)
      if (a_base + j < axisNames.size())
      {
        if (should_use_native_names(devices[index]))
          name = dev->getAxisName(j);
        else
          name.printf(0, "Axis %d", j + 1);

        if (localize_composite_joy_elem(name, locName, global_cls_composite_drv_joy->getLocClient(),
              ICompositeLocalization::LOC_TYPE_AXIS))
          name = locName;

        if (is_first_xinput || devices.size() == 1)
          fullName = name;
        else
          fullName.printf(0, "C%d:%s:%s", index + 1, dev_name, name.str());

        strmem->free(axisNames[a_base + j]);
        axisNames[a_base + j] = str_dup(fullName.str(), strmem);
      }
    for (int j = 0; j < devices[index].numPovs; j++)
      if (p_base + j < povNames.size())
      {
        if (should_use_native_names(devices[index]))
          name = dev->getPovHatName(j);
        else
          name.printf(0, "POV%d", j + 1);

        if (localize_composite_joy_elem(name, locName, global_cls_composite_drv_joy->getLocClient(),
              ICompositeLocalization::LOC_TYPE_POV))
          name = locName;

        if (is_first_xinput || devices.size() == 1)
          fullName = name;
        else
          fullName.printf(0, "C%d:%s", index + 1, name.str());

        strmem->free(povNames[p_base + j]);
        povNames[p_base + j] = str_dup(fullName.str(), strmem);
      }
  }

  int getNumDevices() const { return devices.size(); }
  const Device &getDevice(int index) const { return devices[index]; }
  void setDevice(int index, const Device &dev) { devices[index] = dev; }

protected:
  Tab<Device> devices;
  JoystickRawState state;
  IGenJoystickClient *client;
  carray<char *, MAX_NAMED_BTN> btnNames;
  carray<char *, MAX_NAMED_AXIS> axisNames;
  carray<char *, MAX_NAMED_POV> povNames;
  int btnNum, axesNum, povsNum;
  int firstX360Index = -1;

public:
  int rumbleDevIdx;
};

CompositeJoystickClassDriver *CompositeJoystickClassDriver::create() { return new CompositeJoystickClassDriver(); }

CompositeJoystickClassDriver::CompositeJoystickClassDriver() :
  compositeDevice(NULL),
  enabled(false),
  defClient(NULL),
  deviceConfigChanged(false),
  ffClient(NULL),
  locClient(NULL),
  haveXInput(false)
{
  prevUpdateRefTime = ::ref_time_ticks();
  memset(&classDrvs, 0, sizeof(classDrvs));
  compositeDevice = new CompositeJoystickDevice();
}

CompositeJoystickClassDriver::~CompositeJoystickClassDriver() { delete compositeDevice; }

bool CompositeJoystickClassDriver::init() { return true; }

void CompositeJoystickClassDriver::addClassDrv(IGenJoystickClassDrv *drv, bool is_xinput)
{
  if (!drv)
    return;
  for (int i = 0; i < MAX_CLASSDRV; i++)
    if (classDrvs[i].drv == drv)
      return;

  for (int i = 0; i < MAX_CLASSDRV; i++)
  {
    if (!classDrvs[i].drv)
    {
      deviceConfigChanged = true;
      classDrvs[i].drv = drv;
      classDrvs[i].isXInput = is_xinput;
      refreshCompositeDevicesList();
      return;
    }
  }

  debug("[ERROR] CompositeJoystickClassDriver: Too many class drivers.");
}

void CompositeJoystickClassDriver::destroyDevices()
{
  unacquireDevices();
  if (ffClient)
    ffClient->fxShutdown();
}

void CompositeJoystickClassDriver::enable(bool en)
{
  enabled = en;
  stg_joy.enabled = en;
  prevUpdateRefTime = ::ref_time_ticks();
}

void CompositeJoystickClassDriver::acquireDevices()
{
  for (int classDrvNo = 0; classDrvNo < MAX_CLASSDRV; classDrvNo++)
  {
    IGenJoystickClassDrv *classDrv = classDrvs[classDrvNo].drv;
    if (classDrv)
      classDrv->acquireDevices();
  }

  refreshCompositeDevicesList();
}

void CompositeJoystickClassDriver::unacquireDevices()
{
  for (int classDrvNo = 0; classDrvNo < MAX_CLASSDRV; classDrvNo++)
  {
    IGenJoystickClassDrv *classDrv = classDrvs[classDrvNo].drv;
    if (classDrv)
      classDrv->unacquireDevices();
  }
  refreshCompositeDevicesList();
}

void CompositeJoystickClassDriver::destroy()
{
  destroyDevices();
  stg_joy.present = false;
  for (int classDrvNo = 0; classDrvNo < MAX_CLASSDRV; classDrvNo++)
  {
    if (classDrvs[classDrvNo].drv)
    {
      classDrvs[classDrvNo].drv->destroy();
      classDrvs[classDrvNo].drv = NULL;
    }
  }
  delete this;
}

void CompositeJoystickClassDriver::updateDevices()
{
  int dt = get_time_usec(prevUpdateRefTime) / 1000;
  prevUpdateRefTime = ::ref_time_ticks();

  bool isMainThread = is_main_thread();
  deviceConfigChanged = false;
  for (int classDrvNo = 0; classDrvNo < MAX_CLASSDRV; classDrvNo++)
  {
    IGenJoystickClassDrv *classDrv = classDrvs[classDrvNo].drv;
    if (classDrv)
    {
      if (updateDevicesInMainThread || !isMainThread)
        classDrv->updateDevices();
      if (classDrv->isDeviceConfigChanged() && isMainThread)
      {
        if (ffClient)
          ffClient->fxShutdown();
        classDrv->refreshDeviceList();
        if (classDrv->getDeviceCount())
          classDrv->enable(true); // TEMP
        for (int s = 0; s < 2; s++)
        {
          classDrv->setStickDeadZoneScale(s, true, stickDzScale[s + 0]);
          classDrv->setStickDeadZoneScale(s, false, stickDzScale[s + 2]);
        }
        deviceConfigChanged = true;
      }
    }
  }

  if (deviceConfigChanged && isMainThread)
    refreshCompositeDevicesList();

  compositeDevice->updateState(dt);
}

bool CompositeJoystickClassDriver::isDeviceConfigChanged() const { return deviceConfigChanged; }

IGenJoystick *CompositeJoystickClassDriver::getDevice(int idx) const
{
  if (idx == 0)
    return compositeDevice;
  return NULL;
}
void CompositeJoystickClassDriver::useDefClient(IGenJoystickClient *cli)
{
  G_ASSERT(compositeDevice);
  defClient = cli;
  if (compositeDevice)
    compositeDevice->setClient(defClient);
}

IGenJoystick *CompositeJoystickClassDriver::getDeviceByUserId(unsigned short userId) const
{
  G_UNREFERENCED(userId);
  G_ASSERT(userId == 0);
  return compositeDevice;
}

IGenJoystickClassDrv *CompositeJoystickClassDriver::setSecondaryClassDrv(IGenJoystickClassDrv *drv)
{
  G_UNREFERENCED(drv);
  G_ASSERT(0);
  return NULL;
}

IGenJoystick *CompositeJoystickClassDriver::getDefaultJoystick() { return compositeDevice; }

int CompositeJoystickClassDriver::getRealDeviceCount(bool logical) const
{
  if (logical)
    return compositeDevice->getNumDevices();

  int ret = 0;
  for (int classDrvNo = 0; classDrvNo < MAX_CLASSDRV; classDrvNo++)
  {
    IGenJoystickClassDrv *classDrv = classDrvs[classDrvNo].drv;
    if (classDrv)
      ret += classDrv->getDeviceCount();
  }
  return ret;
}
const char *CompositeJoystickClassDriver::getRealDeviceName(int real_dev_idx) const
{
  G_ASSERT(real_dev_idx >= 0 && real_dev_idx < compositeDevice->getNumDevices());
  return compositeDevice->getDevice(real_dev_idx).name;
}
const char *CompositeJoystickClassDriver::getRealDeviceID(int real_dev_idx) const
{
  G_ASSERT(real_dev_idx >= 0 && real_dev_idx < compositeDevice->getNumDevices());
  return compositeDevice->getDevice(real_dev_idx).devID;
}
bool CompositeJoystickClassDriver::getRealDeviceDesc(int real_dev_idx, DataBlock &desc) const
{
  G_ASSERT(real_dev_idx >= 0 && real_dev_idx < compositeDevice->getNumDevices());
  if (!compositeDevice->getDevice(real_dev_idx).dev)
  {
    desc.clearData();
    desc.setBool("disconnected", true);
  }
  else if (!compositeDevice->getDevice(real_dev_idx).dev->getDeviceDesc(desc))
    desc.clearData();
  desc.setStr("name", compositeDevice->getDevice(real_dev_idx).name);
  desc.setStr("devId", compositeDevice->getDevice(real_dev_idx).devID);

  int a_base = compositeDevice->getDevice(real_dev_idx).ofsAxes;
  int b_base = compositeDevice->getDevice(real_dev_idx).ofsButtons;
  desc.setInt("btnOfs", b_base);
  desc.setInt("btnCnt", compositeDevice->getDevice(real_dev_idx).numButtons);
  desc.setInt("axesOfs", a_base);
  desc.setInt("axesCnt", compositeDevice->getDevice(real_dev_idx).numAxes);
  return true;
}
const char *CompositeJoystickClassDriver::getNativeAxisName(int axis_idx) const
{
  return compositeDevice ? compositeDevice->getNativeAxisName(axis_idx) : NULL;
}
const char *CompositeJoystickClassDriver::getNativeButtonName(int btn_idx) const
{
  return compositeDevice ? compositeDevice->getNativeButtonName(btn_idx) : NULL;
}

void CompositeJoystickClassDriver::setRumbleTargetRealDevIdx(int real_dev_idx)
{
  if (compositeDevice)
    compositeDevice->rumbleDevIdx = real_dev_idx;
}

#define REMAP_ABS2REL(ID, FIELD)                                          \
  for (int i = 0, ie = compositeDevice->getNumDevices(); i < ie; i++)     \
  {                                                                       \
    int ofs = compositeDevice->getDevice(i).ofs##FIELD;                   \
    if (ID >= ofs && ID < ofs + compositeDevice->getDevice(i).num##FIELD) \
    {                                                                     \
      out_real_dev_idx = i;                                               \
      return ID - ofs;                                                    \
    }                                                                     \
  }                                                                       \
  out_real_dev_idx = -1;                                                  \
  return -1

#define REMAP_REL2ABS(ID, FIELD)                                            \
  if (real_dev_idx < 0 || real_dev_idx >= compositeDevice->getNumDevices()) \
    return -1;                                                              \
  return ID + compositeDevice->getDevice(real_dev_idx).ofs##FIELD

int CompositeJoystickClassDriver::remapAbsButtonId(int abs_btn_id, int &out_real_dev_idx) const { REMAP_ABS2REL(abs_btn_id, Buttons); }
int CompositeJoystickClassDriver::remapRelButtonId(int rel_btn_id, int real_dev_idx) const { REMAP_REL2ABS(rel_btn_id, Buttons); }
int CompositeJoystickClassDriver::remapAbsAxisId(int abs_axis_id, int &out_real_dev_idx) const { REMAP_ABS2REL(abs_axis_id, Axes); }
int CompositeJoystickClassDriver::remapRelAxisId(int rel_axis_id, int real_dev_idx) const { REMAP_REL2ABS(rel_axis_id, Axes); }
int CompositeJoystickClassDriver::remapAbsPovHatId(int abs_pov_id, int &out_real_dev_idx) const { REMAP_ABS2REL(abs_pov_id, Povs); }
int CompositeJoystickClassDriver::remapRelPovHatId(int rel_pov_id, int real_dev_idx) const { REMAP_REL2ABS(rel_pov_id, Povs); }
#undef REMAP_ABS2REL
#undef REMAP_REL2ABS

void CompositeJoystickClassDriver::setLocClient(ICompositeLocalization *cli, bool refresh)
{
  locClient = cli;
  if (refresh)
    refreshCompositeDevicesList();
}

static bool check_main_gamepad(HumanInput::IGenJoystick *dev, bool main_gamepad)
{
  if (strcmp(dev->getDeviceID(), "x360") == 0 || strcmp(dev->getDeviceID(), "ds4") == 0)
    return main_gamepad;
  return !main_gamepad;
}
float CompositeJoystickClassDriver::getStickDeadZoneScale(int stick_idx, bool main_gamepad) const
{
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, 0);
  return stickDzScale[(main_gamepad ? 0 : 2) + stick_idx];
}
void CompositeJoystickClassDriver::setStickDeadZoneScale(int stick_idx, bool main_gamepad, float scale)
{
  G_ASSERT_RETURN(stick_idx >= 0 && stick_idx < 2, );
  stickDzScale[(main_gamepad ? 0 : 2) + stick_idx] = scale;
  if (compositeDevice)
    for (int classDrvNo = 0; classDrvNo < MAX_CLASSDRV; classDrvNo++)
      if (IGenJoystickClassDrv *classDrv = classDrvs[classDrvNo].drv)
        if (classDrv->isStickDeadZoneSetupSupported())
          for (int devNo = 0; devNo < classDrv->getDeviceCount(); devNo++)
          {
            HumanInput::IGenJoystick *dev = classDrv->getDevice(devNo);
            if (check_main_gamepad(dev, main_gamepad))
              dev->setStickDeadZoneScale(stick_idx, scale);
          }
}
float CompositeJoystickClassDriver::getStickDeadZoneAbs(int stick_idx, bool main_gamepad, IGenJoystick *for_joy) const
{
  if (!compositeDevice)
    return 0;
  for (int classDrvNo = 0; classDrvNo < MAX_CLASSDRV; classDrvNo++)
    if (IGenJoystickClassDrv *classDrv = classDrvs[classDrvNo].drv)
      if (classDrv->isStickDeadZoneSetupSupported())
        for (int devNo = 0; devNo < classDrv->getDeviceCount(); devNo++)
        {
          HumanInput::IGenJoystick *dev = classDrv->getDevice(devNo);
          if (for_joy && dev != for_joy)
            continue;
          if (check_main_gamepad(dev, main_gamepad))
            return dev->getStickDeadZoneAbs(stick_idx);
        }
  for (int classDrvNo = 0; classDrvNo < MAX_CLASSDRV; classDrvNo++)
    if (IGenJoystickClassDrv *classDrv = classDrvs[classDrvNo].drv)
      if (classDrv->isStickDeadZoneSetupSupported())
        if (!classDrv->getDeviceCount())
          return classDrv->getStickDeadZoneAbs(stick_idx, main_gamepad, nullptr);
  return 0;
}

void CompositeJoystickClassDriver::enableGyroscope(bool enable, bool main_dev)
{
  for (int i = 0; i < MAX_CLASSDRV; i++)
    if (classDrvs[i].drv)
      classDrvs[i].drv->enableGyroscope(enable, main_dev);
}

void CompositeJoystickClassDriver::refreshCompositeDevicesList()
{
  bool isXInput = false;

  if (compositeDevice)
  {
    compositeDevice->clearDevices();
    for (int classDrvNo = 0; classDrvNo < MAX_CLASSDRV; classDrvNo++)
    {
      IGenJoystickClassDrv *classDrv = classDrvs[classDrvNo].drv;
      if (classDrv)
      {
        for (int devNo = 0; devNo < classDrv->getDeviceCount(); devNo++)
        {
          HumanInput::IGenJoystick *dev = classDrv->getDevice(devNo);
          compositeDevice->addDevice(dev, classDrvs[classDrvNo].isXInput);
        }
        isXInput |= (classDrvs[classDrvNo].isXInput && classDrv->getDeviceCount());
      }
    }
    for (int i = 0; i < compositeDevice->getNumDevices(); i++)
      compositeDevice->setupControlNames(i);
  }

  haveXInput = isXInput;
  if (ffClient)
    ffClient->fxReinit();
}

void CompositeJoystickClassDriver::resetDevicesList()
{
  if (compositeDevice)
  {
    compositeDevice->resetDevices();
    refreshCompositeDevicesList();
  }
}

bool CompositeJoystickClassDriver::isAxisDigital(int axis_id) const
{
  if (compositeDevice)
    return compositeDevice->isAxisDigital(axis_id);
  return false;
}

#if _TARGET_PC
bool HumanInput::isImeAvailable() { return cb_show_ime_steam != NULL; }
bool HumanInput::showScreenKeyboard_IME(const DataBlock &init_params, OnFinishIME on_finish_cb, void *userdata)
{
  if (cb_show_ime_steam)
    return (*cb_show_ime_steam)(init_params, on_finish_cb, userdata);
  return false;
}

namespace HumanInput
{
void reg_cb_show_ime_steam(OnShowIME cb) { cb_show_ime_steam = cb; }
} // namespace HumanInput
#endif
