// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define USE_BULLET_PHYSICS 1
#include "carDriver.h"
#undef USE_BULLET_PHYSICS

#include "steering.h"
#include "gamepadButtons.h"

#include <math/dag_mathBase.h>
#include <util/dag_globDef.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <libTools/util/strUtil.h>

#include <drv/hid/dag_hiJoystick.h>

#include <de3_interface.h>
#include <EditorCore/ec_input.h>
#include <EditorCore/ec_wndGlobal.h>

#include <imgui/imgui.h>

//--------------------------------------------

float joystick_steering_viscosity = 0.07f;

const float velAdd = 100 / 3.6;
const float nitroFactor = 1.0;

extern String get_car_common_fn();

TestDriver::TestDriver() : steeringHelper(NULL), hbHelper(NULL), car(NULL), joy(NULL)
{
  startup_joystick(this);
  resetDriver(NULL);
}


TestDriver::~TestDriver()
{
  disable();
  shutdown_joystick();
  car = NULL;
  del_it(steeringHelper);
  del_it(hbHelper);
}


void TestDriver::resetDriver(IPhysCar *_car)
{
  left = 0;
  right = 0;
  up = 0;
  down = 0;
  rear = 0;
  handbrk = 0;
  accel = 0;
  steer = 0.0;
  steeringAngleJoystick = 0.0;
  steeringJoyAxisPos = 0.0;
  gas = 0.0;
  brake = 0.0;
  maxSteer = 25.0;
  maxGas = 1.0;
  maxBrake = 0.5;
  flagBackPressed = false;

  steeringAssistConsts.setDefaults();
  tPress = 0;
  press = 0;
  handbrakeValue = -1;
  inAirTime = 0;
  bigSteerTime = 0;
  driftScale = 0;
  driveStraightPrev = false;
  driftStatePrev = false;
  accelEnabled = false;
  accelFlyTime = 0.0;

  car = _car;
  if (car)
  {
    del_it(steeringHelper);
    del_it(hbHelper);

    DataBlock blk(get_car_common_fn());
    G_ASSERT(blk.isValid());
    DataBlock *steerBlk = blk.getBlockByName("Steer");
    G_ASSERT(steerBlk);
    const DataBlock *b = steerBlk->getBlockByName(car->getSteerPreset());
    if (!b)
      b = steerBlk->getBlockByNameEx("default");
    steeringHelper = new SteeringHelper(b);

    DataBlock *hbBlk = blk.getBlockByName("HandBrake");
    G_ASSERT(hbBlk);
    hbHelper = new HandBrakeHelper(hbBlk);
  }

  updateJoyState();
}


void TestDriver::disable()
{
  steer = 0;
  steeringAngleJoystick = 0.0;
  steeringJoyAxisPos = 0.0;
  gas = 0;
  brake = 0;
  if (joy)
    joy->doRumble(0, 0);
}


void TestDriver::updateJoyState() { readControlSetup(get_joystick()); }


void TestDriver::update(IPhysCar *_car, real dt)
{
  if (!car || car != _car)
    return;

  getKbdInput();
  update_joysticks();
  updateDrivingControls(dt);
}


void TestDriver::getKbdInput()
{
  left = ec_is_key_down(ImGuiKey_Keypad4) || ec_is_key_down(ImGuiKey_LeftArrow) || ec_is_key_down(ImGuiKey_A);

  right = ec_is_key_down(ImGuiKey_Keypad6) || ec_is_key_down(ImGuiKey_RightArrow) || ec_is_key_down(ImGuiKey_D);

  up = ec_is_key_down(ImGuiKey_Keypad8) || ec_is_key_down(ImGuiKey_UpArrow) || ec_is_key_down(ImGuiKey_W);

  down = ec_is_key_down(ImGuiKey_Keypad5) || ec_is_key_down(ImGuiKey_DownArrow) || ec_is_key_down(ImGuiKey_S);

  handbrk = ec_is_key_down(ImGuiKey_LeftCtrl);

  rear = ec_is_key_down(ImGuiKey_R);

  accel = ec_is_shift_key_down();

  if (ec_is_key_down(ImGuiKey_Backspace))
  {
    if (!flagBackPressed && car)
    {
      TMatrix tm;
      tm.identity();
      tm.setcol(3, car->getPos() + Point3(0, 4, 0));
      car->setTm(tm);
      car->postReset();
      flagBackPressed = true;
    }
  }
  else
    flagBackPressed = false;
}


void TestDriver::updateDrivingControls(float dt)
{
  actKeyboardSteering(dt);
  if (joy)
    actJoystickSteering(dt);

  actKeyboardGasBrake(dt);
  if (joy)
    actJoystickGasBrake();

  autoReverse();

  if (joy)
    handbrk |= joy->getButtonState(JOY_BTN_X);

  if (joy)
    accel |= joy->getButtonState(JOY_BTN_A);
  updateAccel(dt);

  if (hbHelper)
    handbrakeValue = hbHelper->update(dt, handbrk);

  if (car->isAssistEnabled(car->CA_STEER))
  {
#define S steeringAssistConsts

    bool rear_gear = car->getMachinery()->getGearStep() < 0;
    bool in_air = (car->wheelsOnGround() < 3);
    float raw_steer = joy ? fabs(joyControl.getSteeringValRaw(joy)) : 0;
    bool no_steer = !left && !right && raw_steer < 0.01;
    float steer_power = fabs(getSteeringAngle() / car->getMaxWheelAngle());


    if (in_air)
    {
      inAirTime += dt;
      no_steer = true;
      driftScale = 0;
    }
    else
    {
      if (inAirTime > S.resetDirInAirTimeout && !rear_gear)
        driveStraightPrev = car->resetTargetCtrlDir(true, 0);
      inAirTime = 0;
    }

    if (!no_steer)
    {
      driveStraightPrev = false;
      if (steer_power > S.bigSteerPwrThres)
        bigSteerTime += dt;
      else
        bigSteerTime = 0;
      if (bigSteerTime > S.bigSteerStartDriftTimeout)
        driftScale += S.bigSteerDriftMul * dt * (raw_steer + right + left);
      if (handbrk)
        driftScale += S.handBrakeDriftMul * steer_power * dt;
    }
    else
    {
      if (!driveStraightPrev && !rear_gear)
        driveStraightPrev = car->resetTargetCtrlDir(false, driftStatePrev ? S.driftAlignThresDeg : S.cornerAlignThresDeg);
      driftScale -= S.driftScaleDecay * dt;
    }

    if (in_air)
    {
      car->changeTargetCtrlDirWt(0.0, 0.99 * dt / 0.016);
      car->changeTargetCtrlVelWt(0.0, 0.99 * dt / 0.016);
    }
    else if (driveStraightPrev)
    {
      car->changeTargetCtrlDirWt(S.straightDirWt, S.straightDirInertia * dt / 0.016);
      car->changeTargetCtrlVelWt(S.straightVelWt, S.straightVelInertia * dt / 0.016);
      driftScale = 0;
      driftStatePrev = false;
    }
    else if (rear_gear)
    {
      car->changeTargetCtrlDirWt(0.0, 0.99 * dt / 0.016);
      car->changeTargetCtrlVelWt(0.0, 0.99 * dt / 0.016);
    }
    else
    {
      float _steer = getSteeringAngle();
      if (car->getVel().lengthSq() < sqr(S.minVelForSteer))
        _steer = 0;

      driveStraightPrev = false;
      car->changeTargetCtrlDirWt(S.cornerDirWt, S.cornerDirInertia * dt / 0.016);
      car->changeTargetCtrlVelWt(S.cornerVelWt, S.cornerVelInertia * dt / 0.016);
      car->changeTargetCtrlDir(_steer * dt * S.velRotateRate, no_steer ? dt * S.velRotateRetRate : 0);

      if (driftScale > S.maxDriftScale)
        driftScale = S.maxDriftScale;
      else if (driftScale < 0)
        driftScale = 0;
      car->changeTargetCtrlDirDeviation(_steer * dt * driftScale,
        dt * (no_steer ? S.driftScaleIdleRetRate : S.driftScaleActiveRetRate), S.driftAlignWt);
      if (driftScale > S.enterDriftStateScaleThres)
        driftStatePrev = true;
    }
#undef S
  }
}


void TestDriver::actKeyboardSteering(float dt)
{
  float maxAng = maxSteer / 180 * PI;

  // steer
  if (steeringHelper)
    steer = steeringHelper->update(car->getCurWheelAngle(), car->getVel(), left, right, dt);

  steer = clamp(steer, -maxAng, maxAng);
}


void TestDriver::actKeyboardGasBrake(float dt)
{
  const float fullGasTime = 0.5f;
  const float fullBrakeTime = 0.2f;

  const float gasAccel = 1.0f / fullGasTime;
  const float brakeAccel = 1.0f / fullBrakeTime;

  if (up)
    gas = min(gas + gasAccel * dt, 1.0f);
  else
    gas = 0;

  if (down)
    brake = min(brake + brakeAccel * dt, 1.0f);
  else
    brake = 0;
}


void TestDriver::actJoystickSteering(float dt)
{
  float maxAng = car->getMaxWheelAngle();

  float rawAngle = joyControl.getSteeringValRaw(joy);
  if (joystick_steering_viscosity < 0.001f)
    steeringJoyAxisPos = rawAngle;
  else
    steeringJoyAxisPos = ::approach(steeringJoyAxisPos, rawAngle, dt, joystick_steering_viscosity);

  steeringAngleJoystick = clamp(joyControl.applyCurve(steeringJoyAxisPos), -1.0f, 1.0f) * maxAng;
}


void TestDriver::actJoystickGasBrake()
{
  real gas_f, brake_f;
  joyControl.getGasBrake(joy, gas_f, brake_f);

  gas = min(gas + gas_f, 1.0f);
  brake = min(brake + brake_f, 1.0f);
}


void TestDriver::autoReverse()
{
  ICarMachinery *machinery = car->getMachinery();

  if (!machinery->automaticGearBox())
    return;

  if ((getBrakeFactor() > 0.9f) && (lengthSq(car->getVel()) < sqr(10.0f / 3.6f)) &&
      !car->wheelsSlide(10.0f) /*&& (!rc->carPlayer->braked)*/)
  {
    float rpm = machinery->getEngineIdleRPM();
    if (rpm < lerp(machinery->getEngineIdleRPM(), machinery->getEngineMaxRPM(), 0.05f))
    {
      if (machinery->getGearStep() < 0)
        machinery->setGearStep(1);
      else
        machinery->setGearStep(-1);
    }
  }
}


real TestDriver::getSteeringAngle()
{
  if (fabsf(steeringAngleJoystick) > 0.001f)
    return steeringAngleJoystick;
  else
    return steer;
}


real TestDriver::getGasFactor()
{
  if (!car)
    return 0;
  return reverseGasBrake() ? brake : gas;
}


real TestDriver::getBrakeFactor()
{
  if (!car)
    return 1;
  return reverseGasBrake() ? gas : brake;
}


bool TestDriver::reverseGasBrake()
{
  if (!car)
    return false;

  ICarMachinery *machinery = car->getMachinery();
  return machinery->automaticGearBox() && (machinery->getGearStep() < 0);
}


void TestDriver::updateAccel(float dt)
{
  if (gas >= 1.0 && accel && !accelEnabled)
  {
    accelEnabled = true;
    car->getMachinery()->setNitro(nitroFactor);
  }
  else if ((!accel || gas < 1.0) && accelEnabled)
  {
    accelEnabled = false;
    car->getMachinery()->setNitro(0.0);
  }

  if (!car->wheelsOnGround())
    accelFlyTime += dt;
  else
    accelFlyTime = 0.0;

  if (accelFlyTime > 1.0)
    accelEnabled = false;

  if (accelEnabled)
  {
    float vel = velAdd;
    Point3 pos = car->getMassCenter();
    if ((vel > 0.0) && (accelFlyTime < 0.1))
    {
      Point3 dir = car->getDir();
      float impulse = car->getMass() * dt * vel;
      car->addImpulse(pos + dir * 3.0f, dir * impulse, car);
    }
  }
}


void TestDriver::readControlSetup(HumanInput::IGenJoystick *joy_)
{
  joy = joy_;
  if (joy)
  {
    joystick::get_joystick_params(joy, joyControl);
    joyControl.applyParams(joy);
    DAEDITOR3.conWarning("Joystick is attached");
  }
  else
    DAEDITOR3.conWarning("No joystick found");
}
