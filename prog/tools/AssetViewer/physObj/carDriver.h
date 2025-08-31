// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vehiclePhys/physCar.h>
#include <EditorCore/ec_input.h>


class TestDriver : public ICarDriver, public ICarController
{
public:
  // ICarDriver
  ICarController *getController() override { return this; }

  // ICarController
  real getSteeringAngle() override { return steer / 180 * PI; }
  real getGasFactor() override { return gas; }
  real getBrakeFactor() override { return brake; }
  float getHandbrakeState() override { return handbrk ? 1.0 : -1.0; }

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

  void update(IPhysCar *car, real dt) override
  {
    const float wheel_ret_v = 180;
    const float wheel_v = 20;
    const float wheel_max = maxSteer;
    const float gas_v = maxGas / 0.5;
    const float brake_v = maxBrake / 0.3;

    left = ec_is_key_down(ImGuiKey_Keypad4) || ec_is_key_down(ImGuiKey_LeftArrow) || ec_is_key_down(ImGuiKey_A);

    right = ec_is_key_down(ImGuiKey_Keypad6) || ec_is_key_down(ImGuiKey_RightArrow) || ec_is_key_down(ImGuiKey_D);

    up = ec_is_key_down(ImGuiKey_Keypad8) || ec_is_key_down(ImGuiKey_UpArrow) || ec_is_key_down(ImGuiKey_W);

    down = ec_is_key_down(ImGuiKey_Keypad5) || ec_is_key_down(ImGuiKey_DownArrow) || ec_is_key_down(ImGuiKey_S);

    handbrk = ec_is_key_down(ImGuiKey_LeftCtrl);

    rear = ec_is_key_down(ImGuiKey_R);


    if (ec_is_key_down(ImGuiKey_Backspace))
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
