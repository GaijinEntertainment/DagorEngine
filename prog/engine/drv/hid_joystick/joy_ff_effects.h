// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/hid/dag_hiJoyFF.h>
#include <drv/hid/dag_hiDeclDInput.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include "joy_device.h"

namespace HumanInput
{
// processed coord system data
struct JoyCookedCreateParams
{
  int type;
  int axisNum;
  unsigned long axisId[3];
  long dir[3];
  long dur;
};

// generic direct input effect container
class Di8GenJoyEffect
{
protected:
  IDirectInputEffect *eff;
  Di8JoystickDevice *device;
  int subType;
  struct
  {
    int count;
    bool solo, isStarted;
  } state;
  int axisNum;

public:
  Di8GenJoyEffect(Di8JoystickDevice *dev);

  void fxDestroy(IGenJoyFfEffect *p);

  void fxDownload();
  bool fxStart(int count, bool solo);
  bool fxIsPlaying();
  void fxStop();
  void fxRestart();

  void fxSetDirPolar(float ang);
  void fxSetDirSpherical(float ang_x, float ang_y, float ang_z);
  void fxSetDirCartesian(float x, float y, float z);

  void fxSetParameters(DIEFFECT *diEffect, int flags);
};

// Condition force feedback effect
class DiJoyConditionFx : public IJoyFfCondition, public Di8GenJoyEffect
{
  DICONDITION data[3];

public:
  DiJoyConditionFx(Di8JoystickDevice *dev);

  bool create(int condfx_type, JoyCookedCreateParams &cp);

  // IGenJoyFfEffect interface implementation
  virtual void destroy()
  {
    fxDestroy(this);
    delete this;
  }
  virtual void download() { fxDownload(); }
  virtual bool start(int count, bool solo) { return fxStart(count, solo); }
  virtual bool isPlaying() { return fxIsPlaying(); }
  virtual void stop() { fxStop(); }
  virtual void restore()
  {
    fxDownload();
    apply();
    fxRestart();
  }
  virtual void setDirPolar(float ang) { fxSetDirPolar(ang); }
  virtual void setDirSpherical(float ang_x, float ang_y, float ang_z) { fxSetDirSpherical(ang_x, ang_y, ang_z); }
  virtual void setDirCartesian(float x, float y, float z) { fxSetDirCartesian(x, y, z); }

  virtual void apply();

  // IJoyFfCondition interface implementation
  virtual int getType() { return subType; }
  virtual void setOffsetEx(int axis_idx, float p);
  virtual void setCoefEx(int axis_idx, bool neg, float p);
  virtual void setSaturationEx(int axis_idx, bool neg, float maxp);
  virtual void setDeadzoneEx(int axis_idx, float dp);
};


// Constant force feedback effect
class DiJoyConstantForceFx : public IJoyFfConstForce, public Di8GenJoyEffect
{
  DICONSTANTFORCE data;

public:
  DiJoyConstantForceFx(Di8JoystickDevice *dev);

  bool create(JoyCookedCreateParams &cp, float force);

  // IGenJoyFfEffect interface implementation
  virtual void destroy()
  {
    fxDestroy(this);
    delete this;
  }
  virtual void download() { fxDownload(); }
  virtual bool start(int count, bool solo) { return fxStart(count, solo); }
  virtual bool isPlaying() { return fxIsPlaying(); }
  virtual void stop() { fxStop(); }
  virtual void restore()
  {
    fxDownload();
    apply();
    fxRestart();
  }
  virtual void setDirPolar(float ang) { fxSetDirPolar(ang); }
  virtual void setDirSpherical(float ang_x, float ang_y, float ang_z) { fxSetDirSpherical(ang_x, ang_y, ang_z); }
  virtual void setDirCartesian(float x, float y, float z) { fxSetDirCartesian(x, y, z); }

  virtual void apply();

  // IJoyFfConstForce interface implementation
  virtual void setForce(float p);
};


// Periodic force feedback effect
class DiJoyPeriodicFx : public IJoyFfPeriodic, public Di8GenJoyEffect
{
  DIPERIODIC data;

public:
  DiJoyPeriodicFx(Di8JoystickDevice *dev);

  bool create(int periodfx_type, JoyCookedCreateParams &cp);

  // IGenJoyFfEffect interface implementation
  virtual void destroy()
  {
    fxDestroy(this);
    delete this;
  }
  virtual void download() { fxDownload(); }
  virtual bool start(int count, bool solo) { return fxStart(count, solo); }
  virtual bool isPlaying() { return fxIsPlaying(); }
  virtual void stop() { fxStop(); }
  virtual void restore()
  {
    fxDownload();
    apply();
    fxRestart();
  }
  virtual void setDirPolar(float ang) { fxSetDirPolar(ang); }
  virtual void setDirSpherical(float ang_x, float ang_y, float ang_z) { fxSetDirSpherical(ang_x, ang_y, ang_z); }
  virtual void setDirCartesian(float x, float y, float z) { fxSetDirCartesian(x, y, z); }

  virtual void apply();

  // IJoyFfPeriodic interface implementation
  virtual int getType() { return subType; }

  virtual void setAmplitude(float dp);
  virtual void setBaseline(float p0);
  virtual void setPeriod(float t);
  virtual void setPhase(float ang);
};


// Ramp force feedback effect
class DiJoyRampForceFx : public IJoyFfRampForce, public Di8GenJoyEffect
{
  DIRAMPFORCE data;

public:
  DiJoyRampForceFx(Di8JoystickDevice *dev);

  bool create(JoyCookedCreateParams &cp, float start_f, float end_f);

  // IGenJoyFfEffect interface implementation
  virtual void destroy()
  {
    fxDestroy(this);
    delete this;
  }
  virtual void download() { fxDownload(); }
  virtual bool start(int count, bool solo) { return fxStart(count, solo); }
  virtual bool isPlaying() { return fxIsPlaying(); }
  virtual void stop() { fxStop(); }
  virtual void restore()
  {
    fxDownload();
    apply();
    fxRestart();
  }
  virtual void setDirPolar(float ang) { fxSetDirPolar(ang); }
  virtual void setDirSpherical(float ang_x, float ang_y, float ang_z) { fxSetDirSpherical(ang_x, ang_y, ang_z); }
  virtual void setDirCartesian(float x, float y, float z) { fxSetDirCartesian(x, y, z); }

  virtual void apply();

  // IJoyFfRampForce interface implementation
  virtual void setStartForce(float p);
  virtual void setEndForce(float p);
};
} // namespace HumanInput
