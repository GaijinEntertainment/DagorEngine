#pragma once

#include <vehiclePhys/physCar.h>
#include <winGuiWrapper/wgw_input.h>


class TestDriver : public ICarDriver, public ICarController
{
public:
  // ICarDriver
  virtual bool isSubOf(unsigned int class_id) { return false; }
  virtual ICarController *getController() { return this; }
  virtual void onDamage(IPhysCar *by_who, real damage) {}

  // ICarController
  virtual real getSteeringAngle() { return steer / 180 * PI; }
  virtual real getGasFactor() { return gas; }
  virtual real getBrakeFactor() { return brake; }
  virtual float getHandbrakeState() { return handbrk ? 1.0 : -1.0; }
  virtual void disable(bool set) {}
  virtual bool isAutoReverseEnabled() { return true; }
  virtual bool getManualReverse() { return false; }

public:
  unsigned left : 1, right : 1, up : 1, down : 1, rear : 1, handbrk : 1;
  float steer, gas, brake;
  float maxSteer, maxGas, maxBrake;
  bool flagBackPressed;

  TestDriver() { resetDriver(); }

  void resetDriver()
  {
    left = 0;
    right = 0;
    up = 0;
    down = 0;
    rear = 0;
    handbrk = 0;
    steer = 0.0;
    gas = 0.0;
    brake = 0.0;
    maxSteer = 25.0;
    maxGas = 1.0;
    maxBrake = 0.5;
    flagBackPressed = false;
  }

  void disable()
  {
    steer = 0;
    gas = 0;
    brake = 0;
  }

  virtual void update(IPhysCar *car, real dt)
  {
    const float wheel_ret_v = 180;
    const float wheel_v = 20;
    const float wheel_max = maxSteer;
    const float gas_v = maxGas / 0.5;
    const float brake_v = maxBrake / 0.3;

    left = wingw::is_key_pressed(wingw::V_NUMPAD4) || wingw::is_key_pressed(wingw::V_LEFT) || wingw::is_key_pressed('A');

    right = wingw::is_key_pressed(wingw::V_NUMPAD6) || wingw::is_key_pressed(wingw::V_RIGHT) || wingw::is_key_pressed('D');

    up = wingw::is_key_pressed(wingw::V_NUMPAD8) || wingw::is_key_pressed(wingw::V_UP) || wingw::is_key_pressed('W');

    down = wingw::is_key_pressed(wingw::V_NUMPAD5) || wingw::is_key_pressed(wingw::V_DOWN) || wingw::is_key_pressed('S');

    handbrk = wingw::is_key_pressed(wingw::V_LCONTROL);

    rear = wingw::is_key_pressed('R');


    if (wingw::is_key_pressed(wingw::V_BACK))
    {
      if (!flagBackPressed)
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


    float st = float(right) - float(left);
    if (left || right)
    {
      if (steer * st < 0)
        steer += wheel_ret_v * st * dt;
      else
        steer += wheel_v * st * dt;
      if (steer > wheel_max)
        steer = wheel_max;
      else if (steer < -wheel_max)
        steer = -wheel_max;
    }
    else if (fabs(steer) < wheel_ret_v * dt)
      steer = 0;
    else
      steer += steer > 0 ? -wheel_ret_v * dt : wheel_ret_v * dt;

    if (up)
    {
      gas += gas_v * dt;
      if (gas > maxGas)
        gas = maxGas;
    }
    else
    {
      gas -= gas_v * 8 * dt;
      if (gas < 0)
        gas = 0;
    }

    if (down)
    {
      brake += brake_v * dt;
      if (brake > maxBrake)
        brake = maxBrake;
    }
    else
    {
      brake -= brake_v * 4 * dt;
      if (brake < 0)
        brake = 0;
    }
    if (up && !down)
      brake = 0;
    else if (!up && down)
      gas = 0;
  }
};
