// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "joystickParams.h"

#include <workCycle/dag_workCycle.h>
#include <drv/hid/dag_hiJoystick.h>
#include <startup/dag_inpDevClsDrv.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <math/dag_mathBase.h>

#if DAGOR_DBGLEVEL > 0
#include <perfMon/dag_graphStat.h>
#endif


//-----------------------------------------------------------------------------


namespace joystick
{

const char *JoystickParams::axis_names[] = {"steer", "throttle", "brake", "camx", "camy"};

void AxisParams::applyParams(HumanInput::IGenJoystick *joy)
{
  if (axisId < 0)
    return;
  joy->setAxisLimits(axisId, rangeMin, rangeMax);
}

float AxisParams::applyDeadzone(float val)
{
  bool neg = (val < 0);
  if (neg)
    val = -val;

  if (val < innerDeadzone)
    return 0.0f;

  float res = min((val - innerDeadzone) / (1.0f - innerDeadzone - outerDeadzone), 1.0f);
  return neg ? -res : res;
}


JoystickParams::JoystickParams() : singleBTCenter(0), nonlinearity(8.0f) {}

void JoystickParams::applyParams(HumanInput::IGenJoystick *joy)
{
  G_ASSERT(joy);
  steer().applyParams(joy);
  throttle().applyParams(joy);
  if (!isSingleBTAxis())
    brake().applyParams(joy);
  camx().applyParams(joy);
  camy().applyParams(joy);
}


void JoystickParams::bindAxis(int axis_type, int axis_id)
{
  if (axis_type < 0 || axis_type >= NUM_AXES)
  {
    G_ASSERT(0);
    return;
  }
  axis[axis_type].axisId = axis_id;

  if (axis_type == AXIS_THROTTLE || axis_type == AXIS_BRAKE)
  {
    AxisParams &th = throttle();
    AxisParams &br = brake();
    if (isSingleBTAxis())
    {
      th.rangeMin = -1;
      th.rangeMax = 1;
    }
    else
    {
      th.rangeMin = 0;
      th.rangeMax = 1;
      br.rangeMin = 0;
      br.rangeMax = 1;
    }
  }
}

void JoystickParams::getGasBrake(HumanInput::IGenJoystick *joy, float &gas_f, float &brake_f)
{
  G_ASSERT(joy);

  if (isSingleBTAxis())
  {
    joystick::AxisParams &ax = throttle();

    float bt_f = joy->getAxisPos(ax.axisId);
    if (ax.inverse)
      bt_f = -bt_f;

    if (fabsf(bt_f - singleBTCenter) < ax.innerDeadzone)
    {
      brake_f = 0;
      gas_f = 0;
    }
    else if (bt_f < singleBTCenter)
    {
      gas_f = 0;
      brake_f = (singleBTCenter - bt_f - ax.innerDeadzone) / (1.0f - ax.innerDeadzone - ax.outerDeadzone);
      brake_f = clamp(brake_f, 0.0f, 1.0f);
    }
    else
    {
      gas_f = (bt_f - singleBTCenter - ax.innerDeadzone) / (1.0f - ax.innerDeadzone - ax.outerDeadzone);
      gas_f = clamp(gas_f, 0.0f, 1.0f);
      brake_f = 0;
    }
  }
  else
  {
    joystick::AxisParams &th = throttle();
    joystick::AxisParams &br = brake();

    gas_f = joy->getAxisPos(th.axisId);
    brake_f = joy->getAxisPos(br.axisId);
    if (th.inverse)
      gas_f = 1.0 - gas_f;
    if (br.inverse)
      brake_f = 1.0 - brake_f;

    if (brake_f < br.innerDeadzone)
      brake_f = 0;
    else
      brake_f = min((brake_f - br.innerDeadzone) / (1.0f - br.innerDeadzone - br.outerDeadzone), 1.0f);

    if (gas_f < th.innerDeadzone)
      gas_f = 0;
    else
      gas_f = min((gas_f - th.innerDeadzone) / (1.0f - th.innerDeadzone - th.outerDeadzone), 1.0f);
  }
}

#define NONLIN_CURVE_PT     0.7f
#define NONLIN_CURVE_PT_INV (1.0f - NONLIN_CURVE_PT)

float JoystickParams::applyCurve(float x)
{
  if (x == 0.0f)
    return 0.0f;

  bool neg = (x < 0);
  if (neg)
    x = -x;

  float res;
  if (x <= NONLIN_CURVE_PT)
    res = x / nonlinearity;
  else
  {
    float y = NONLIN_CURVE_PT / nonlinearity;
    res = (x - NONLIN_CURVE_PT) / NONLIN_CURVE_PT_INV * (1 - y) + y;
  }

  return neg ? -res : res;
}

float JoystickParams::getSteeringValRaw(HumanInput::IGenJoystick *joy)
{
  G_ASSERT(joy);
  return steer().applyDeadzone(joy->getAxisPos(steer().axisId));
}

void JoystickParams::setupXbox360Joystick(HumanInput::IGenJoystick *joy)
{
  G_ASSERT(joy);

  float innerDeadzone = 0.1f;
  float outerDeadzone = 0.1f;

  AxisParams &st = steer();
  st.innerDeadzone = innerDeadzone;
  st.outerDeadzone = outerDeadzone;
  // st.axisId = joy->getAxisId("Left Thumb Hor");
  st.axisId = joy->getAxisId("L.Thumb.h");

  AxisParams &th = throttle();
  th.innerDeadzone = innerDeadzone;
  th.outerDeadzone = outerDeadzone;
  th.rangeMin = 0.0f;
  th.rangeMax = 1.0f;
  // th.axisId = joy->getAxisId("Right Trigger");
  th.axisId = joy->getAxisId("R.Trigger");

  AxisParams &br = brake();
  br.innerDeadzone = innerDeadzone;
  br.outerDeadzone = outerDeadzone;
  br.rangeMin = 0.0f;
  br.rangeMax = 1.0f;
  // br.axisId = joy->getAxisId("Left Trigger");
  br.axisId = joy->getAxisId("L.Trigger");

  camx().innerDeadzone = 0.1;
  camx().outerDeadzone = 0.1;
  // camx().axisId = joy->getAxisId("Right Thumb Hor");
  camx().axisId = joy->getAxisId("R.Thumb.h");

  camy().innerDeadzone = 0.1f;
  camy().outerDeadzone = 0.1f;
  // camy().axisId = joy->getAxisId("Right Thumb Vert");
  camy().axisId = joy->getAxisId("R.Thumb.v");

  for (int i = 0; i < sizeof(axis) / sizeof(axis[0]); ++i)
  {
    AxisParams &a = axis[i];
    a.rangeMin = -1;
    a.rangeMax = 1;
    if (a.axisId >= 0)
      joy->setAxisLimits(a.axisId, a.rangeMin, a.rangeMax);
    else
      LOGERR_CTX("XBox360 joystick axis %s not found on gamepad", axis_names[i]);
  }
}

void get_joystick_params(HumanInput::IGenJoystick *joy, JoystickParams &jp) { jp.setupXbox360Joystick(joy); }

} // namespace joystick
