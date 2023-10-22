#pragma once

#include <vehiclePhys/physCar.h>

#include "steeringAssistConsts.h"
#include "joystickParams.h"
#include "joystickDrv/joy_drv.h"

class SteeringHelper;
class HandBrakeHelper;

class TestDriver : public ICarDriver, public ICarController, public IJoyCallback
{
public:
  TestDriver();
  virtual ~TestDriver();

  void resetDriver(IPhysCar *_car);
  void disable();
  virtual void update(IPhysCar *_car, real dt);

  // ICarDriver
  virtual bool isSubOf(unsigned int class_id) { return false; }
  virtual ICarController *getController() { return this; }
  virtual void onDamage(IPhysCar *by_who, real damage) {}

  // ICarController
  virtual real getSteeringAngle(); // { return steer; }
  virtual real getGasFactor();
  virtual real getBrakeFactor();
  virtual float getHandbrakeState() { return handbrakeValue; }
  virtual void disable(bool set) {}
  virtual bool isAutoReverseEnabled() { return true; }
  virtual bool getManualReverse() { return false; }

  // IJoyCallback
  virtual void updateJoyState();

  virtual bool getAccelState() { return accelEnabled; }

private:
  void updateDrivingControls(float dt);
  void getKbdInput();
  void actKeyboardSteering(float dt);
  void actKeyboardGasBrake(float dt);
  void actJoystickSteering(float dt);
  void actJoystickGasBrake();
  void autoReverse();
  bool reverseGasBrake();
  void updateAccel(float dt);
  void readControlSetup(HumanInput::IGenJoystick *joy_);

  unsigned left : 1, right : 1, up : 1, down : 1, rear : 1, handbrk : 1, accel : 1;
  float steer, steeringAngleJoystick, steeringJoyAxisPos;
  float gas, brake, handbrakeValue;
  float maxSteer, maxGas, maxBrake;
  bool flagBackPressed;
  float tPress;
  int press;

  float inAirTime, accelFlyTime;
  float bigSteerTime;
  float driftScale;
  bool driveStraightPrev, driftStatePrev, accelEnabled;

  SteeringHelper *steeringHelper;
  HandBrakeHelper *hbHelper;
  SteeringAssistConsts steeringAssistConsts;
  IPhysCar *car;

  joystick::JoystickParams joyControl;
  HumanInput::IGenJoystick *joy;
};
