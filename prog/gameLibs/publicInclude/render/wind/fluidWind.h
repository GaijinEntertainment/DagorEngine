//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/utility.h>
#include <EASTL/string.h>
#include <EASTL/map.h>
#include <EASTL/string_map.h>
#include <EASTL/unique_ptr.h>

#include <3d/dag_resPtr.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <math/dag_Point3.h>

class ComputeShaderElement;

class FluidWind
{
public:
  struct Desc
  {
    Point3 worldSize;
    int dimX, dimY, dimZ;
  };

  static const int INVALID_MOTOR_ID = -1;

  struct PhaseFade
  {
    bool enabled = false;
    float maxFadeTime = 0.5f;
  };
  struct PhaseSin
  {
    bool enabled = false;
    int numWaves = 1;
  };

  struct PhaseAttack
  {
    bool enabled = false;
    float maxAttackTime = 0.0f;
  };
  enum ShapeType
  {
    SHAPE_SPHERE,
    SHAPE_CYLINDER,
    SHAPE_CONE
  };
  enum MotorType
  {
    MOTOR_OMNI,
    MOTOR_VORTEX,
    MOTOR_DIRECTIONAL,
    MOTOR_TYPE_END
  };
  struct MotorBase
  {
    Point3 center = Point3(0, 0, 0);
    float radius = 0;
    float duration = 0;
    float strength = 0;
    bool shake = false;
    PhaseAttack phaseAttack;
    PhaseFade phaseFade;
    PhaseSin phaseSin;
  };
  struct Motor : MotorBase
  {
    Point3 direction;

    MotorType type;
    bool stopped;
    float time;
  };
  struct OmniMotor : MotorBase
  {};
  struct VortexMotor : MotorBase
  {};
  struct DirectionalMotor : MotorBase
  {
    Point3 direction;
  };

  FluidWind(const Desc &in_desc);
  ~FluidWind();

  void update(float dt, const Point3 &origin);
  void renderDebug();
  bool isInSleep() const;

  int pushOmni(const OmniMotor &omni);
  int pushVortex(const VortexMotor &vortex);
  int pushDirectional(const DirectionalMotor &directional);
  void stopMotor(int id);
  void stopMotors();
  void setMotorCenter(int id, const Point3 &pos);
  dag::ConstSpan<Motor> getMotors() const;

private:
  void init();
  void close();
  int pushMotor(MotorType type, const MotorBase &base);
  Point3 toLocalPoint(const Point3 &point) const;
  Point3 toLocalVector(const Point3 &vector) const;
  bool isContained(const Point3 &center, float radius) const;

  float totalSimTime = 0.0f;
  Point3 lastOrigin = Point3(0, 0, 0);
  float sleepTime = 0.0f;
  float fixedTimeStep = 1.0f;

  int fluidWindTexVarId;
  int fluidMotorCenterRadiusVarId;
  int fluidMotorDirectionDurationVarId;
  int fluidMotorStrengthVarId;
  int fluidMotorTimeVarId;
  int fluidWindOriginDeltaVarId;
  int fluidWindOriginVarId;
  int fluidWindStatusVarId;
  int fluidWindTimeStepVarId;

  Desc desc;
  Tab<Motor> motors;

  eastl::unique_ptr<ComputeShaderElement> clearData;
  eastl::unique_ptr<ComputeShaderElement> advect;
  carray<eastl::unique_ptr<ComputeShaderElement>, MOTOR_TYPE_END> drawMotors;
  eastl::unique_ptr<ComputeShaderElement> divergence;
  eastl::unique_ptr<ComputeShaderElement> jaccobi3d;
  eastl::unique_ptr<ComputeShaderElement> project3d;
  eastl::unique_ptr<ComputeShaderElement> render;

  UniqueTex speedTex[2];
  UniqueTex pressureTex[2];
  UniqueTex divergenceTex;
  UniqueTex outputTex;
  UniqueTex *speedTexCur[2] = {&speedTex[0], &speedTex[1]};
  UniqueTex *pressureTexCur[2] = {&pressureTex[0], &pressureTex[1]};
};
