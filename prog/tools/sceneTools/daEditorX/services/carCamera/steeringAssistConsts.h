#pragma once

struct SteeringAssistConsts
{
  float resetDirInAirTimeout;
  float bigSteerPwrThres;
  float bigSteerStartDriftTimeout;
  float bigSteerDriftMul;
  float handBrakeDriftMul;
  float driftScaleDecay;
  float driftScaleActiveRetRate;
  float driftScaleIdleRetRate;
  float driftAlignWt;
  float maxDriftScale;
  float velRotateRate;
  float velRotateRetRate;
  float minVelForSteer;
  float straightDirWt, straightDirInertia;
  float straightVelWt, straightVelInertia;
  float cornerDirWt, cornerDirInertia;
  float cornerVelWt, cornerVelInertia;
  float driftAlignThresDeg;
  float cornerAlignThresDeg;
  float enterDriftStateScaleThres;

  void setDefaults()
  {
    resetDirInAirTimeout = 0.2;
    bigSteerPwrThres = 0.8;
    bigSteerStartDriftTimeout = 1.4;
    bigSteerDriftMul = 3.125;
    handBrakeDriftMul = 4.0 / 0.1;
    driftScaleDecay = 1.25;
    driftScaleActiveRetRate = 1.0;
    driftScaleIdleRetRate = 4.0;
    driftAlignWt = 0.7;
    maxDriftScale = 3.0;
    velRotateRate = 3.0;
    velRotateRetRate = 1.0;
    minVelForSteer = 2.0;
    straightDirWt = 0.8;
    straightDirInertia = 0.07;
    straightVelWt = 0.2;
    straightVelInertia = 0.06;
    cornerDirWt = 0.02;
    cornerDirInertia = 0.09;
    cornerVelWt = 0.06;
    cornerVelInertia = 0.08;
    driftAlignThresDeg = 1.0;
    cornerAlignThresDeg = 10.0;
    enterDriftStateScaleThres = 0.2;
  }
};
