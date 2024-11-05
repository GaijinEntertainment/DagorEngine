// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <util/dag_globDef.h>


class DataBlock;
namespace HumanInput
{
class IGenJoystick;
}


namespace joystick
{

class AxisParams
{
public:
  AxisParams()
  {
    axisId = -1;
    rangeMin = -1.0f;
    rangeMax = 1.0f;
    inverse = false;
    innerDeadzone = 0.05f;
    outerDeadzone = 0.05f;
  }

  int axisId;

  float innerDeadzone, outerDeadzone;
  float rangeMin, rangeMax;
  bool inverse;

  void applyParams(HumanInput::IGenJoystick *joy);
  float applyDeadzone(float val);
};

class JoystickParams
{
public:
  enum
  {
    AXIS_STEER = 0,
    AXIS_THROTTLE,
    AXIS_BRAKE,
    AXIS_CAMX,
    AXIS_CAMY,
    NUM_AXES
  };

  JoystickParams();

  void operator=(const JoystickParams &) { G_ASSERT(0); } // SqPlus workaround

  float singleBTCenter;
  float nonlinearity;
  String name;

  AxisParams axis[NUM_AXES];
  AxisParams &steer() { return axis[AXIS_STEER]; }
  AxisParams &throttle() { return axis[AXIS_THROTTLE]; }
  AxisParams &brake() { return axis[AXIS_BRAKE]; }
  AxisParams &camx() { return axis[AXIS_CAMX]; }
  AxisParams &camy() { return axis[AXIS_CAMY]; }

  AxisParams *getAxis(int axis_type) { return &axis[axis_type]; }
  void bindAxis(int axis_type, int axis_id);

  void getGasBrake(HumanInput::IGenJoystick *joy, float &gas_f, float &brake_f);
  float getSteeringValRaw(HumanInput::IGenJoystick *joy);

  float applyCurve(float x);

  void applyParams(HumanInput::IGenJoystick *joy);
  bool isSingleBTAxis() { return throttle().axisId >= 0 && (throttle().axisId == brake().axisId); }

  void setupXbox360Joystick(HumanInput::IGenJoystick *joy);

  static const char *axis_names[];
};

void get_joystick_params(HumanInput::IGenJoystick *joy, JoystickParams &jp);

} // namespace joystick
