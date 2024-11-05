// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// #include "camera.h"

#include <generic/dag_tab.h>

#include <math/dag_maFilter.h>
#include <math/dag_noise.h>
#include <math/dag_math3d.h>


class IPhysCar;

class CameraController
{
public:
  CameraController(IPhysCar *_car);
  virtual ~CameraController();

  void setParamsPreset(int n_preset);

  virtual void reset();
  virtual void update(float dt);

  virtual void getITm(TMatrix &tm) { tm = itm; }
  virtual void lookBack(bool set);
  virtual void setLookTurn(const Point2 &turn_k);
  virtual bool zoomable() { return false; }

protected:
  void camera_noise_init()
  {
    perlin_noise::init_noise(0x123456);
    shakePhase = 0;
  }

  /// Update camera position and speed
  void move(Point3 &vDest, float dt);

  Point3 getVirtualPos(const Point3 &vOffs, const Point3 &target_pt, const Point3 &car_dir, const Point3 &vel);

  virtual float getFov();

  void testChaseCameraValid(float dt, const TMatrix &cartm, const Point3 &cvel, const Point3 &wvel);
  void setShake(float shakeFactor, float vertAmp, float gammaAmp, float rotApm, float noiseFreq);
  Point3 getCarTargetPt(const TMatrix *tm);

  void updateValues(float dt);
  void updateTilt(float dt, const Point3 &dest, const Point3 &vel);

  static void calctm(const Point3 &pos, const Point3 &targ, float cam_ang, TMatrix &out_tm);

protected:
  IPhysCar *car;

  /// Collide with world and dynamic objects
  bool bClip;

  ///
  Point3 vExpSlowdown;
  /// Spring force coefficients
  Point3 vSpringEffect;

  float velDirWeight, minUsableVel, maxUsableVel;

  struct ShakeTbl
  {
    float shakeFactor;
    float vertAmp;
    float gammaAmp;
    float rotAmp;
    float noiseFreq;
  };
  Tab<ShakeTbl> shakeTbl;

  float shakeFactor;
  float idleInertiaFactor;

  bool chaseValid, afterSwitch;
  Point3 lastGoodPos, lastPosSph;
  Point3 sphVel, sphVelMax, sphAccMax;

  Point3 maxWVel;
  float maxCosTeta;
  float maxVel2ForRotY;
  float testTimeLeft, testTimeMax;
  MeanAverageFilter smWVelX, smWVelY;

  TMatrix itm;
  float fov, nitroFov, fovFactor;

  Point3 prevPos, prevPrevPos;
  float current_tilt;

  float shakePhase;

  Point3 vOffsTargetFullScreen, vOffsTargetSplitScreen;       //< Position of camera target, relative to vehicle
  Point3 vOffsCameraFullScreen, vOffsCameraSplitScreen;       //< Position of camera itself, relative to vehicle
  Point3 vFovOffsCameraFullScreen, vFovOffsCameraSplitScreen; //< Position of camera itself, relative to vehicle

  Point2 tgtLookTurnK, curLookTurnK;

  int back;
};
