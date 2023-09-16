//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_carray.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_integer.h>
#include <generic/dag_tab.h>

#define PRE_SIMULATION_TIME 30.0f // 30 sec first iteration
#define PRE_SIMULATION_STEP 10
#define MAX_BEAUFORT_SCALE  12


struct WindSpawner
{
  Point3 position;
  Point3 direction;
  float width;
  float lastWaveSpawnTime;
  float randNextSpawnTime;
};

struct WindWave
{
  Point3 position;
  Point3 direction;
  float radius;
  float speed;
  float strength;
  float maxStrength;
  float lifeTime;
  float lifeTimeSpan;
  bool alive;

  WindWave() : alive(false), lifeTimeSpan(0.0f), lifeTime(0.0f), maxStrength(0.0f), strength(0.0f), speed(0.0f), radius(0.0f) {}
};


class DynamicWind
{

public:
  struct WindDesc
  {
    Point2 angleSpread;
    Point2 windRadiusLim;
    Point2 windSpeedLim;
    IPoint2 spawnerAmountLim;
    float internalCascadeBoxSize;
    float windSpawnTimeMult;
    unsigned int startingPoolSize;
  };

private:
  WindDesc desc;

  int beaufortScale;
  Point2 spawnTimeLim;
  Point3 direction;
  bool isFirstIteration;

  void spawnWave(int index);
  // simulate at the start for a better looking wind
  void firstSimulation(const Point3 &pos);

  void reset();
  int findWavesIndex();

  bool isEnabled;

public:
  DynamicWind();
  ~DynamicWind();
  Tab<WindWave> waves;
  Tab<WindSpawner> spawner;

  void update(const Point3 &pos, float dt);
  void init(int beaufortScale, const Point3 &direction);
  void loadWindDesc(WindDesc &_desc);
  void enableDynamicWind(bool enabled) { isEnabled = enabled; };
};
