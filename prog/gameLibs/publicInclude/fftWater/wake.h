//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_carray.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_bounds2.h>

class BaseTexture;
typedef BaseTexture Texture;
// wake
namespace fft_water
{

class Wake
{
public:
  Wake();
  ~Wake();

  void close();
  void setLevel(float water_level);

  void simulateDt(const Point2 &origin, float dt);
  bool init(int res);
  void calculateKernel(int half_kernel);
  void reset() { cleared_flag = false; }
  bool isOn() const { return wakeOn; }
  bool isEmpty() const { return !wakeOn || wakeAutoOff; }
  void switchOff()
  {
    cleared_flag = wakeOn ? false : cleared_flag;
    wakeOn = false;
    setNullTex();
  }
  void switchOn() { wakeOn = true; }
  void setAutoOffTimeThreshold(float time) { autoOffTimeThreshold = time; }
  float getAutoOffTimeThreshold() const { return autoOffTimeThreshold; }
  void changeBox()
  {
    regionBox[0] = Point2(lastPos.x - regionSize, lastPos.y - regionSize);
    regionBox[1] = Point2(lastPos.x + regionSize, lastPos.y + regionSize);
  }
  void moveRegion(const Point2 &new_pos) // just mark to move
  {
    const Point2 ofs = new_pos - lastPos;
    lastPos = new_pos;
    changeBox();
    movedFlag = false;
    moveOfs += ofs;
  }
  int getResolution() const { return resolution; }
  void setRegionSize(float v)
  {
    cleared_flag = regionSize == v ? cleared_flag : false;
    regionSize = v;
    changeBox();
  }
  float getRegionSize() const { return regionSize; }
  void setMainObstacle(const Point3 &pos, const Point2 &dir, const Point2 &dirSize, float speed);
  void addObstacle(const Point3 &pos, float str);
  void addImpulse(const Point3 &pos, float str);
  int maxImpulsesCount() const { return impulses.capacity(); }
  int maxObstaclesCount() const { return obstacles.capacity(); }
  bool hasFreeImpulses() const { return impulses.size() < impulses.capacity(); }
  bool hasFreeObstacles() const { return obstacles.size() < obstacles.capacity(); }
  Texture *getGradientTex() const { return wakeGradients.getTex2D(); }
  const Point2 &getMainDir() const { return mainDir; }

  const Point4 &getWorldToWakeTransform() const { return worldToWake; }

protected:
  void renderMove(const Point2 &ofs);
  void setNullTex();
  carray<UniqueTex, 3> height; // ping pong
  UniqueTex verticalDerivative;
  UniqueTex wakeGradients;
  int resolution;
  int currentHeight;
  Point2 moveOfs;
  Tab<Point4> obstacles, impulses;
  PostFxRenderer makeDerivatives, makeHeightmap, copyHeightmap, calculateGradients; //
  float waterLevel;
  float alpha;
  bool cleared_flag, movedFlag;
  Point2 lastPos, lastNotEmptyPos;
  float regionSize;
  BBox2 regionBox;

  Point3 mainPos;
  float mainSpeed;
  Point2 mainDir, dirSize;
  float autoOffTimeThreshold, autoOffTimer;
  bool wakeOn, wakeAutoOff;
  Point4 worldToWake;
};

}; // namespace fft_water
