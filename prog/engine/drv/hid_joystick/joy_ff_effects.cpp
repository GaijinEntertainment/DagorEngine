// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "include_dinput.h"
#include "joy_ff_effects.h"
#include <drv/hid/dag_hiDInput.h>
#include <math/dag_mathBase.h>
#include <debug/dag_debug.h>
using namespace HumanInput;

// helper inline routines
__forceinline int scaleClamped(real p, real minp, real maxp, real scale)
{
  if (p <= minp)
    return minp * scale;
  else if (p >= maxp)
    return maxp * scale;
  return p * scale;
}

static DWORD hardcoded_axis[3] = {DIJOFS_X, DIJOFS_Y, DIJOFS_Z};

// common effect object
Di8GenJoyEffect::Di8GenJoyEffect(Di8JoystickDevice *dev)
{
  device = dev;
  eff = NULL;
  subType = 0;
  memset(&state, 0, sizeof(state));
}
void Di8GenJoyEffect::fxDestroy(IGenJoyFfEffect *p)
{
  if (device && eff)
  {
    device->delFx(p);
    eff->Unload();
    eff->Release();
  }
  device = NULL;
  eff = NULL;
  memset(&state, 0, sizeof(state));
}

void Di8GenJoyEffect::fxDownload()
{
  HRESULT hr;
  if (eff)
  {
    hr = eff->Download();
    if (FAILED(hr))
      HumanInput::printHResult(__FILE__, __LINE__, "eff->Download", hr);
  }
}
bool Di8GenJoyEffect::fxStart(int count, bool solo)
{
  HRESULT hr;
  if (eff)
  {
    hr = eff->Start(count > 0 ? count : INFINITE, solo ? DIES_SOLO : 0);
    if (FAILED(hr))
    {
      HumanInput::printHResult(__FILE__, __LINE__, "eff->Start", hr);
      return false;
    }
    state.isStarted = count == 0; //== for now works only for continous play
    state.count = count;
    state.solo = solo;
    return true;
  }
  return false;
}
bool Di8GenJoyEffect::fxIsPlaying()
{
  if (eff)
  {
    HRESULT hr;
    DWORD dw;
    hr = eff->GetEffectStatus(&dw);
    if (FAILED(hr))
    {
      HumanInput::printHResult(__FILE__, __LINE__, "eff->GetEffectStatus", hr);
      return false;
    }
    return dw & (DIEGES_PLAYING | DIEGES_EMULATED);
  }
  return false;
}
void Di8GenJoyEffect::fxStop()
{
  HRESULT hr;
  if (eff)
  {
    hr = eff->Stop();
    if (FAILED(hr))
      HumanInput::printHResult(__FILE__, __LINE__, "eff->Stop", hr);
    state.isStarted = false;
  }
}
void Di8GenJoyEffect::fxRestart()
{
  if (state.isStarted)
    fxStart(state.count, state.solo);
}
void Di8GenJoyEffect::fxSetParameters(DIEFFECT *diEffect, int flags)
{
  HRESULT hr;
  hr = eff->SetParameters(diEffect, flags);

  if (FAILED(hr))
  {
    if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
    {
      device->acquire();
      hr = eff->SetParameters(diEffect, flags);
    }
    G_ASSERT(hr != E_INVALIDARG);
    HumanInput::printHResult(__FILE__, __LINE__, "SetParameters", hr);
    return;
  }
}
void Di8GenJoyEffect::fxSetDirPolar(float ang)
{
  if (eff)
  {
    DIEFFECT diEffect;
    LONG dir[2] = {ang * 180 / PI * DI_DEGREES, 0};

    memset(&diEffect, 0, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.dwFlags = DIEFF_POLAR;
    diEffect.rglDirection = &dir[0];
    diEffect.cAxes = axisNum;
    fxSetParameters(&diEffect, DIEP_DIRECTION);
  }
}
void Di8GenJoyEffect::fxSetDirSpherical(float ang_x, float ang_y, float ang_z)
{
  if (eff)
  {
    DIEFFECT diEffect;
    LONG dir[3] = {ang_x * 180 / PI * DI_DEGREES, ang_y * 180 / PI * DI_DEGREES, ang_z * 180 / PI * DI_DEGREES};

    memset(&diEffect, 0, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.dwFlags = DIEFF_SPHERICAL;
    diEffect.rglDirection = &dir[0];
    diEffect.cAxes = axisNum;
    fxSetParameters(&diEffect, DIEP_DIRECTION);
  }
}
void Di8GenJoyEffect::fxSetDirCartesian(float x, float y, float z)
{
  if (eff)
  {
    DIEFFECT diEffect;
    LONG dir[3] = {x * 10000, y * 10000, z * 10000};

    memset(&diEffect, 0, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.dwFlags = DIEFF_CARTESIAN;
    diEffect.rglDirection = dir;
    diEffect.cAxes = axisNum;
    fxSetParameters(&diEffect, DIEP_DIRECTION);
  }
}


//
// Condition force feedback effect
//
DiJoyConditionFx::DiJoyConditionFx(Di8JoystickDevice *dev) : Di8GenJoyEffect(dev)
{
  axisNum = 1;
  memset(data, 0, sizeof(data));
  data[0].lDeadBand = 10000;
}
bool DiJoyConditionFx::create(int condfx_type, JoyCookedCreateParams &cp)
{
  HRESULT hr;
  DIEFFECT diEffect;
  GUID guid;

  switch (condfx_type)
  {
    case JFF_COND_Friction: guid = GUID_Friction; break;
    case JFF_COND_Damper: guid = GUID_Damper; break;
    case JFF_COND_Inertia: guid = GUID_Inertia; break;
    case JFF_COND_Spring: guid = GUID_Spring; break;
    default: return false;
  }
  G_ASSERT(cp.axisNum > 0 && cp.axisNum <= 3);
  subType = condfx_type;
  axisNum = cp.axisNum;

  memset(&diEffect, 0, sizeof(diEffect));
  diEffect.dwSize = sizeof(DIEFFECT);
  diEffect.dwFlags = cp.type | DIEFF_OBJECTOFFSETS;
  diEffect.dwDuration = cp.dur;
  diEffect.dwSamplePeriod = 0;
  diEffect.dwGain = DI_FFNOMINALMAX;         // No scaling
  diEffect.dwTriggerButton = DIEB_NOTRIGGER; // Not a button response
  diEffect.dwTriggerRepeatInterval = 0;      // Not applicable
  diEffect.cAxes = cp.axisNum;
  diEffect.rgdwAxes = &hardcoded_axis[0];
  diEffect.rglDirection = &cp.dir[0];
  diEffect.lpEnvelope = NULL;
  diEffect.cbTypeSpecificParams = sizeof(data[0]) * axisNum;
  diEffect.lpvTypeSpecificParams = data;

  if (device->isJoyFxEnabled())
  {
    hr = device->getDev()->CreateEffect(guid, &diEffect, &eff, NULL);
    if (!eff || FAILED(hr))
    {
      HumanInput::printHResult(__FILE__, __LINE__, "CreateEffect", hr);
      return false;
    }
  }

  return true;
}
void DiJoyConditionFx::apply()
{
  if (eff)
  {
    DIEFFECT diEffect;

    memset(&diEffect, 0, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.cbTypeSpecificParams = sizeof(data[0]) * axisNum;
    diEffect.lpvTypeSpecificParams = data;

    fxSetParameters(&diEffect, DIEP_TYPESPECIFICPARAMS);
  }
}
void DiJoyConditionFx::setOffsetEx(int axis_idx, float p)
{
  G_ASSERT(axis_idx >= 0 && axis_idx < axisNum);
  data[axis_idx].lOffset = scaleClamped(p, -1, 1, 10000);
}
void DiJoyConditionFx::setCoefEx(int axis_idx, bool neg, float p)
{
  G_ASSERT(axis_idx >= 0 && axis_idx < axisNum);
  if (neg)
  {
    data[axis_idx].lNegativeCoefficient = scaleClamped(p, -1, 1, 10000);
  }
  else
  {
    data[axis_idx].lPositiveCoefficient = scaleClamped(p, -1, 1, 10000);
  }
}
void DiJoyConditionFx::setSaturationEx(int axis_idx, bool neg, float maxp)
{
  G_ASSERT(axis_idx >= 0 && axis_idx < axisNum);
  if (neg)
    data[axis_idx].dwNegativeSaturation = scaleClamped(maxp, 0, 1, 10000);
  else
    data[axis_idx].dwPositiveSaturation = scaleClamped(maxp, 0, 1, 10000);
}
void DiJoyConditionFx::setDeadzoneEx(int axis_idx, float dp)
{
  G_ASSERT(axis_idx >= 0 && axis_idx < axisNum);
  data[axis_idx].lDeadBand = scaleClamped(dp, 0, 1, 10000);
}

//
// Constant force feedback effect
//
DiJoyConstantForceFx::DiJoyConstantForceFx(Di8JoystickDevice *dev) : Di8GenJoyEffect(dev) { data.lMagnitude = 0; }

bool DiJoyConstantForceFx::create(JoyCookedCreateParams &cp, float force)
{
  HRESULT hr;
  DIEFFECT diEffect;

  setForce(force);

  memset(&diEffect, 0, sizeof(diEffect));
  diEffect.dwSize = sizeof(DIEFFECT);
  diEffect.dwFlags = cp.type | DIEFF_OBJECTOFFSETS;
  diEffect.dwDuration = cp.dur;
  diEffect.dwSamplePeriod = 0;
  diEffect.dwGain = DI_FFNOMINALMAX;         // No scaling
  diEffect.dwTriggerButton = DIEB_NOTRIGGER; // Not a button response
  diEffect.dwTriggerRepeatInterval = 0;      // Not applicable
  diEffect.cAxes = cp.axisNum;
  diEffect.rgdwAxes = &hardcoded_axis[0];
  diEffect.rglDirection = &cp.dir[0];
  diEffect.lpEnvelope = NULL;
  diEffect.cbTypeSpecificParams = sizeof(data);
  diEffect.lpvTypeSpecificParams = &data;

  hr = device->getDev()->CreateEffect(GUID_ConstantForce, &diEffect, &eff, NULL);
  if (!eff || FAILED(hr))
  {
    HumanInput::printHResult(__FILE__, __LINE__, "CreateEffect", hr);
    return false;
  }

  return true;
}
void DiJoyConstantForceFx::apply()
{
  if (eff)
  {
    DIEFFECT diEffect;

    memset(&diEffect, 0, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.cbTypeSpecificParams = sizeof(data);
    diEffect.lpvTypeSpecificParams = &data;

    fxSetParameters(&diEffect, DIEP_TYPESPECIFICPARAMS);
  }
}
void DiJoyConstantForceFx::setForce(float p) { data.lMagnitude = scaleClamped(p, -1, 1, 10000); }


//
// Periodic force feedback effect
//
DiJoyPeriodicFx::DiJoyPeriodicFx(Di8JoystickDevice *dev) : Di8GenJoyEffect(dev)
{
  memset(&data, 0, sizeof(data));
  data.dwPeriod = 1000;
  axisNum = 1;
}

bool DiJoyPeriodicFx::create(int periodfx_type, JoyCookedCreateParams &cp)
{
  HRESULT hr;
  DIEFFECT diEffect;
  GUID guid;

  switch (periodfx_type)
  {
    case JFF_PERIOD_Square: guid = GUID_Square; break;
    case JFF_PERIOD_Sine: guid = GUID_Sine; break;
    case JFF_PERIOD_Triangle: guid = GUID_Triangle; break;
    case JFF_PERIOD_SawtoothUp: guid = GUID_SawtoothUp; break;
    case JFF_PERIOD_SawtoothDown: guid = GUID_SawtoothDown; break;
    default: return false;
  }
  subType = periodfx_type;
  axisNum = cp.axisNum;

  memset(&diEffect, 0, sizeof(diEffect));
  diEffect.dwSize = sizeof(DIEFFECT);
  diEffect.dwFlags = cp.type | DIEFF_OBJECTOFFSETS;
  diEffect.dwDuration = cp.dur;
  diEffect.dwSamplePeriod = 0;
  diEffect.dwGain = DI_FFNOMINALMAX;         // No scaling
  diEffect.dwTriggerButton = DIEB_NOTRIGGER; // Not a button response
  diEffect.dwTriggerRepeatInterval = 0;      // Not applicable
  diEffect.cAxes = cp.axisNum;
  diEffect.rgdwAxes = &hardcoded_axis[0];
  diEffect.rglDirection = &cp.dir[0];
  diEffect.lpEnvelope = NULL;
  diEffect.cbTypeSpecificParams = sizeof(data);
  diEffect.lpvTypeSpecificParams = &data;

  hr = device->getDev()->CreateEffect(guid, &diEffect, &eff, NULL);
  if (!eff || FAILED(hr))
  {
    HumanInput::printHResult(__FILE__, __LINE__, "CreateEffect", hr);
    return false;
  }

  return true;
}

void DiJoyPeriodicFx::apply()
{
  if (eff)
  {
    DIEFFECT diEffect;

    memset(&diEffect, 0, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.cbTypeSpecificParams = sizeof(data);
    diEffect.lpvTypeSpecificParams = &data;

    fxSetParameters(&diEffect, DIEP_TYPESPECIFICPARAMS);
  }
}
void DiJoyPeriodicFx::setAmplitude(float dp) { data.dwMagnitude = scaleClamped(dp, 0, 1, 10000); }
void DiJoyPeriodicFx::setBaseline(float p0) { data.lOffset = scaleClamped(p0, -1, 1, 10000); }
void DiJoyPeriodicFx::setPeriod(float t)
{
  if (t <= 0)
    data.dwPeriod = 1;
  else
    data.dwPeriod = t * 1000000;
}
void DiJoyPeriodicFx::setPhase(float ang) { data.dwPhase = ang * 180 / PI * DI_DEGREES; }


//
// Ramp force feedback effect
//
DiJoyRampForceFx::DiJoyRampForceFx(Di8JoystickDevice *dev) : Di8GenJoyEffect(dev)
{
  data.lStart = 0;
  data.lEnd = 0;
}

bool DiJoyRampForceFx::create(JoyCookedCreateParams &cp, float start_f, float end_f)
{
  HRESULT hr;
  DIEFFECT diEffect;

  setStartForce(start_f);
  setEndForce(end_f);

  memset(&diEffect, 0, sizeof(diEffect));
  diEffect.dwSize = sizeof(DIEFFECT);
  diEffect.dwFlags = cp.type | DIEFF_OBJECTOFFSETS;
  diEffect.dwDuration = cp.dur;
  diEffect.dwSamplePeriod = cp.dur;
  diEffect.dwGain = DI_FFNOMINALMAX;         // No scaling
  diEffect.dwTriggerButton = DIEB_NOTRIGGER; // Not a button response
  diEffect.dwTriggerRepeatInterval = 0;      // Not applicable
  diEffect.cAxes = cp.axisNum;
  diEffect.rgdwAxes = &hardcoded_axis[0];
  diEffect.rglDirection = &cp.dir[0];
  diEffect.lpEnvelope = NULL;
  diEffect.cbTypeSpecificParams = sizeof(data);
  diEffect.lpvTypeSpecificParams = &data;

  hr = device->getDev()->CreateEffect(GUID_RampForce, &diEffect, &eff, NULL);
  if (!eff || FAILED(hr))
  {
    HumanInput::printHResult(__FILE__, __LINE__, "CreateEffect", hr);
    return false;
  }
  return true;
}

void DiJoyRampForceFx::apply()
{
  if (eff)
  {
    DIEFFECT diEffect;

    memset(&diEffect, 0, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.cbTypeSpecificParams = sizeof(data);
    diEffect.lpvTypeSpecificParams = &data;

    fxSetParameters(&diEffect, DIEP_TYPESPECIFICPARAMS);
  }
}
void DiJoyRampForceFx::setStartForce(float p) { data.lStart = scaleClamped(p, -1, 1, 10000); }
void DiJoyRampForceFx::setEndForce(float p) { data.lEnd = scaleClamped(p, -1, 1, 10000); }
