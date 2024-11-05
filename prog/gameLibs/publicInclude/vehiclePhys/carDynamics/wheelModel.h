//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "tireForces.h"

class CarDifferential;

class CarWheelState
{
public:
  CarDifferential *diff; // Attach to this diff
  int diffSide;          // Which output from the differential

  // Dynamics
  float torqueBrakingTC;  // Total braking torque (incl. rr)
  float torqueFeedbackTC; // Feedback (road reaction)

  // Rotation
  float rotationV; // Rotation speed and acceleration

  TireForceModel2 pacejka;

public:
  CarWheelState()
  {
    // Reset all vars that may be loaded by load()
    // Other variables should be reset in resetData()
    diff = 0;
    diffSide = 0;
    torqueFeedbackTC = 0;
    torqueBrakingTC = 0;
    rotationV = 0;

    mass = 50;
    setRad(0.4);
  }

  void setRad(float r)
  {
    radius = r;
    recalcInertia();
  }

  void setMass(float m)
  {
    mass = m;
    recalcInertia();
  }

  // Differential support
  void setDiff(CarDifferential *d, int side)
  {
    diff = d;
    diffSide = side;
  }

  // Dynamic attributes
  float getRotV() { return rotationV; }
  float getRad() { return radius; }
  float getInertia() { return inertia; }

  // Feedback torque is the road reaction torque
  float getFeedbackTorque() { return torqueFeedbackTC; }
  float getBrakingTorque() { return torqueBrakingTC; }

private:
  void recalcInertia() { inertia = 0.5f * radius * radius * mass; }

private:
  float radius;
  float mass;
  float inertia;
};
