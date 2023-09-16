//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "valueShaker.h"

#include <math/dag_Point3.h>

extern float globalCameraShakeLimit;

class TMatrix;

class CameraShaker
{
private:
  float currentShake;
  float wishShake;
  float wishShakeHiFreq;
  float wishSmoothShakeHiFreq;
  float shakeMultiplier;
  ValueShaker shaker;
  ValueShaker hiFreqShaker;
  bool useRandomMult;
  Point3 calcShakeFreqValue(const float wish_shake_freq);

public:
  Point3 shakerValue;
  Point3 shakerSmoothHiFreqValue;
  Point3 shakerHiFreqValue;
  Point3 shakerPosInfluence;
  Point3 shakerDirInfluence;
  float shakeZeroTau;
  float shakeHiFreqZeroTau;
  float shakeHiFreqSmoothZeroTau;
  float shakeFadeCoeff = 0.25f;
  float shakeSmoothFadeCoeff = 0.03f;

  CameraShaker();

  void reset();
  void setShake(float wishShake);
  void setShakeHiFreq(float wishShake);
  void setShakeMultiplier(float mult);
  void setSmoothShakeHiFreq(float wishShake);
  void setRandomMult(bool enabled);
  void shakeMatrix(TMatrix &tm);
  void update(float dt, float eps = 0.01f);
};
