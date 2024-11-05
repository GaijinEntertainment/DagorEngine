//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// forward declarations for external classes
class TMatrix;


class FlyModeBlackBox
{
public:
  virtual ~FlyModeBlackBox() {}

  // resets fly mode settings to defaults
  virtual void resetSettings() = 0;

  // resets dynamic (move to (0,0,0) and hold)
  virtual void resetDynamic() = 0;

  // integrate current position (uses control-keys and ang_dx/ang_dy)
  virtual void integrate(float dt, float ang_dx, float ang_dy) = 0;

  // returns current inverse view matrix
  virtual void getInvViewMatrix(TMatrix &out_itm) = 0;

  // returns current view matrix
  virtual void getViewMatrix(TMatrix &out_tm) = 0;

  // set current view matrix
  virtual void setViewMatrix(const TMatrix &in_itm) = 0;

public:
  // fly mode settings
  float angSpdScaleH;
  float angSpdScaleV;
  float angSpdBank;
  float moveSpd;
  float turboSpd;
  float moveInertia;
  float stopInertia;
  float angStopInertiaH, angStopInertiaV;
  float angInertiaH, angInertiaV;

  // control-keys bits
  struct
  {
    unsigned int fwd : 1, back : 1, left : 1, right : 1, worldUp : 1, worldDown : 1, cameraUp : 1, cameraDown : 1, turbo : 1,
      bankLeft : 1, bankRight : 1;
  } keys;
};


FlyModeBlackBox *create_flymode_bb();
